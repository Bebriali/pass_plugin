#include <cstdio>
#include <cstdint>

/** class-wrapper above file
 */
class Logger {
public:
    FILE* file = nullptr;

    Logger() {
        file = fopen("log/values.log", "w");
    }

    ~Logger() {
        if (file) {
            fclose(file);
        }
    }
};

static Logger GLogger;

extern "C" {
    void log_val(uint32_t id, uint32_t val) {
        if (GLogger.file) {
            fprintf(GLogger.file, "%u %u\n", id, val);
        }
    }

    void log_long_val(uint32_t id, uint64_t val) {
        if (GLogger.file) {
            fprintf(GLogger.file, "%u %llu\n", id, (unsigned long long)val);
        }
    }

    void log_addr(uint32_t id, void* addr) {
        if (GLogger.file) {
            fprintf(GLogger.file, "%u %llu\n", id, (unsigned long long)addr);
        }
    }

}
