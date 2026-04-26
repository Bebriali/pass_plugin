#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <fstream>
#include <filesystem>
#include <memory>

#include "../include/json.hpp"

/** class-wrapper above file
 */
class Logger {
private:
    nlohmann::json root;
    std::string Filename;
    static std::unique_ptr<Logger> Instance;
    static constexpr const char* DefaultLogFilename = "log/values.log";

    explicit Logger(std::string Filename) : Filename(std::move(Filename)) {
        root["values"] = nlohmann::json::array();
    }

public:
    template<typename T>
    void write(uintptr_t id, T val) {
        root["values"].push_back({
            {"id", "0x" + toHexStr(id)},
            {"val", val}
        });
    }

    void writeMem(uintptr_t id, uint64_t val, uintptr_t addr) {
        root["values"].push_back({
            {"id", "0x" + toHexStr(id)},
            {"val", val},
            {"addr", "0x" + toHexStr(addr)}
        });
    }

    static void init(const char* filename) noexcept {
        if (!Instance) {
            const char* fileToUse = filename && filename[0] ? filename : std::getenv("DEFUSE_IR_DUMP_PATH");
            if (!fileToUse || !*fileToUse) {
                fileToUse = DefaultLogFilename;
            }
            Instance = std::unique_ptr<Logger>(new Logger(std::string(fileToUse)));
        }
    }

    static Logger* get() noexcept {
        if (!Instance) {
            init(nullptr);
        }
        return Instance.get();
    }

    ~Logger() {
        nlohmann::json final_json;
        
        std::ifstream in(Filename);
        if (in.is_open()) {
            try {
                in >> final_json;
            } catch (...) {
                final_json = nlohmann::json::object();
            }
            in.close();
        }

        final_json["values"] = root["values"];

        std::ofstream out(Filename);
        if (!out.is_open()) {
            std::fprintf(stderr, "Logger: failed to open log file %s\n", Filename.c_str());
            return;
        }
        out << final_json.dump(4);
    }

private:
    std::string toHexStr(uintptr_t id) {
        char buf[32];
        std::sprintf(buf, "%lx", id);
        return std::string(buf);
    }
};

std::unique_ptr<Logger> Logger::Instance;

extern "C" {
    void initLogger(const char* filename) noexcept {
        Logger::init(filename);
    }

    void logLongVal(uint64_t id, uint64_t val) noexcept {
        Logger* logger = Logger::get();
        if (logger) {
            logger->write(id, val);
        }
    }

    void logMem(uint64_t id, uint64_t val, uint64_t addr) noexcept {
        Logger* logger = Logger::get();
        if (logger) {
            logger->writeMem(
                id,
                val,
                addr
            );
        }
    }
}
