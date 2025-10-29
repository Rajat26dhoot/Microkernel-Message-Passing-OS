#define _POSIX_C_SOURCE 199309L
#include <time.h>
#include <string.h>
#include "kernel.h"

kernel_t kernel;

uint64_t get_time_ns() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

const char* state_to_string(proc_state_t state) {
    switch(state) {
        case PROC_READY: return "READY";
        case PROC_RUNNING: return "RUNNING";
        case PROC_BLOCKED: return "BLOCKED";
        case PROC_WAITING: return "WAITING";
        case PROC_TERMINATED: return "TERMINATED";
        default: return "UNKNOWN";
    }
}

const char* priority_to_string(priority_t priority) {
    switch(priority) {
        case PRIORITY_LOW: return "LOW";
        case PRIORITY_NORMAL: return "NORMAL";
        case PRIORITY_HIGH: return "HIGH";
        case PRIORITY_REALTIME: return "REALTIME";
        default: return "UNKNOWN";
    }
}
