#include "local_host.h"


static _Atomic uint64_t last_time = 0;

// get current time(ms) 
uint64_t get_current_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t current_time = (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000;

    uint64_t expected = last_time;
    while (current_time <= expected) {
        current_time = expected + 1;
        if (atomic_compare_exchange_weak(&last_time, &expected, current_time)) {
            break;
        }
    }
    atomic_store(&last_time, current_time);
    return current_time;
}

// convert local address by hash
uint16_t string_to_hash(const char *str) {
    uint16_t hash = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = (hash << 5) + hash + str[i];  
    }
    return hash;
}

void localInit(uint16_t address) {
    localHost = (Local_Host_t*)malloc(sizeof(Local_Host_t));
    localHost->localAddress = address;
}

// return current time(ms)
uint64_t getCurrentTime() {
    return get_current_milliseconds();
}

void local_sleep(uint64_t milliseconds) {
    struct timespec req, rem;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000; 
    
    while (nanosleep(&req, &rem) == -1) {
        req = rem;
    }
}