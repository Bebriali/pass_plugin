#include <cstdio>
#include <cstdint>
#include <fstream>
#include <filesystem>

/** class-wrapper above file
 */
class Logger {
private:
    std::ofstream file;
public:
    Logger() {
        std::filesystem::create_directories("log");
        file.open("log/values.log", std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            fprintf(stderr, "[Logger Error] Could not open log/values.log!\n");
        }
    }

    bool is_open() const {
        return file.is_open();
    }

    template<typename T>
    void write(uintptr_t id, T val) {
        if (file.is_open()) {
            // Печатаем ID как hex для удобства сопоставления с дампом
            file << "0x" << std::hex << id << std::dec << " " << val << "\n";
        }
    }

    void write_mem(uintptr_t id, uint64_t val, uintptr_t addr) {
        if (file.is_open()) {
            file << "0x" << std::hex << id << std::dec << " " 
                    << val << " 0x" << std::hex << addr << std::dec << "\n";
        }
    }

    ~Logger() = default;
};

static Logger GLogger;

extern "C" {
    void log_val(void* id, uint32_t val) noexcept {
        if (GLogger.is_open())
            GLogger.write(reinterpret_cast<std::uintptr_t>(id), val);
    }

    void log_long_val(void* id, uint64_t val) noexcept {
        if (GLogger.is_open())
            GLogger.write(reinterpret_cast<std::uintptr_t>(id), val);
    }

    void log_addr(void* id, void* addr) noexcept {
        if (GLogger.is_open())
            GLogger.write(reinterpret_cast<std::uintptr_t>(id), reinterpret_cast<std::uintptr_t>(addr));
    }

    void log_mem(void* id, uint64_t val, void* addr) noexcept {
        if (GLogger.is_open())
            GLogger.write_mem(
                reinterpret_cast<std::uintptr_t>(id),
                val,
                reinterpret_cast<std::uintptr_t>(addr)
            );
    }
}
