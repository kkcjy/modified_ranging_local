#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW 4  
#endif
#define _POSIX_C_SOURCE 200809L
#include "local_host.h"


// get current actual time(ns)
#ifdef __GNUC__
    #define FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define FORCE_INLINE inline
#endif

FORCE_INLINE uint64_t read_tsc(void);
FORCE_INLINE uint64_t get_current_actual_time(void);

static _Atomic uint64_t last_time = 0;
static uint64_t tsc_freq = 0;
static uint64_t base_tsc = 0;
static uint64_t base_ns = 0;


static void set_max_realtime_priority() {
    struct sched_param p = { .sched_priority = sched_get_priority_max(SCHED_FIFO) };
    sched_setscheduler(0, SCHED_FIFO, &p);  
}

__attribute__((constructor, cold)) static void calibrate_tsc() {
    set_max_realtime_priority(); 

    struct timespec ts1, ts2;
    uint64_t tsc1, tsc2;

    uint64_t best_freq = 0;
    uint64_t min_deviation = UINT64_MAX;

    for (int i = 0; i < 5; i++) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
        tsc1 = read_tsc();
        
        for (volatile int j = 0; j < 100000; j++);  
        
        clock_gettime(CLOCK_MONOTONIC_RAW, &ts2);
        tsc2 = read_tsc();

        uint64_t delta_ns = ((uint64_t)(ts2.tv_sec - ts1.tv_sec) * 1000000000ULL) + 
                           (ts2.tv_nsec - ts1.tv_nsec);
        uint64_t delta_tsc = tsc2 - tsc1;

        if (delta_ns > 1000000) {  
            uint64_t freq = (delta_tsc * 1000000000ULL) / delta_ns;
            uint64_t deviation = (freq > tsc_freq) ? (freq - tsc_freq) : (tsc_freq - freq);

            if (deviation < min_deviation) {
                min_deviation = deviation;
                best_freq = freq;
            }
        }
    }

    tsc_freq = best_freq ? best_freq : 3000000000ULL;  
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts1);
    base_tsc = read_tsc();
    base_ns = ((uint64_t)ts1.tv_sec * 1000000000ULL) + ts1.tv_nsec;
    atomic_store(&last_time, base_ns);
}

FORCE_INLINE uint64_t read_tsc(void) {
    return __builtin_ia32_rdtsc();
}

FORCE_INLINE uint64_t get_current_actual_time(void) {
    uint64_t current_tsc = read_tsc();
    uint64_t delta_tsc = current_tsc - base_tsc;
    uint64_t current_ns = base_ns + (delta_tsc * 1000000000ULL) / tsc_freq;

    uint64_t expected = atomic_load_explicit(&last_time, memory_order_relaxed);
    if (current_ns <= expected) {
        current_ns = expected + 1;
    }
    atomic_store_explicit(&last_time, current_ns, memory_order_relaxed);

    return current_ns;
}

// convert local address by hash
uint16_t string_to_hash(const char *str) {
    uint16_t hash = 0;
    for (int i = 0; str[i] != '\0'; i++) {
        hash = (hash << 5) + hash + str[i];  
    }
    return hash;
}

// set localAddress and randOffTime(ns)
void localInit(uint16_t address) {
    localHost = (Local_Host_t*)malloc(sizeof(Local_Host_t));
    localHost->localAddress = address;
    localHost->baseTime = 0;

    srand((unsigned int)(get_current_actual_time()));

    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        localHost->location.x = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.y = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.z = rand() % (FLIGHT_AREA_BOUND + 1);
    #endif

    #ifdef RANDOM_DIFF_TIME_ENABLE
        localHost->randOffTime = rand() % (MAX_RANDOM_TIME_OFF + 1);
    #endif
}

// return current time(ns)
uint64_t getCurrentTime() {
    #ifdef RANDOM_DIFF_TIME_ENABLE
        return (get_current_actual_time() - localHost->baseTime) + localHost->randOffTime;
    #else
        return (get_current_actual_time() - localHost->baseTime);
    #endif
}

Coordinate_Tuple_t getCurrentLocation() {
    return localHost->location;
}

// (ms)
void local_sleep(uint64_t milliseconds) {
    struct timespec req, rem;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000; 
    
    while (nanosleep(&req, &rem) == -1) {
        req = rem;
    }
}