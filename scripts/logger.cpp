#include <stdio.h>

static FILE* global_log_f = NULL;

void init_log() {
    if (!global_log_f) {
        global_log_f = fopen("log/values.log", "w");
    }
}

extern "C" void __log_val(int id, int val) {
    init_log();
    if (global_log_f) {
        fprintf(global_log_f, "%d %d\n", id, val);
        fflush(global_log_f);
    }
}

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