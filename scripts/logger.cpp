#include <stdio.h>

static FILE* global_log_f = NULL;

void init_log() {
    if (!global_log_f) {
        global_log_f = fopen("log/values.log", "w"); // BUG // FIXME[flops]: You forgot to fclose it
    }
}

// FIXME[flops]: logger.cpp isn't script btw, better move it to src/ or rename dir into runtime/

// FIXME[flops]: Names with `__` prefix are reserved by stdlib, pls rename those funcs
extern "C" void __log_val(int id, int val) {
    init_log();
    if (global_log_f) {
        fprintf(global_log_f, "%d %d\n", id, val);
        fflush(global_log_f);
    }
}

// TODO[flops]: You pass ID as int32 in pass, so better will be to refactor it:
//  extern "C" void ...(uint32_t id, uint64_t val) 
extern "C" void __log_long_val(int id, long long val) {
    init_log();
    if (global_log_f) {
        fprintf(global_log_f, "%d %lld\n", id, val);
        fflush(global_log_f);
    }
}

extern "C" void __log_addr(int id, void* addr) {
    init_log();
    if (global_log_f) {
        fprintf(global_log_f, "%d %llu\n", id, (unsigned long long) addr);
        fflush(global_log_f);
    }
}
