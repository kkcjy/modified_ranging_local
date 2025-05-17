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

// set localAddress and randOffTime(ms)
void localInit(uint16_t address) {
    localHost = (Local_Host_t*)malloc(sizeof(Local_Host_t));
    localHost->localAddress = address;
    localHost->baseTime = 0;

    srand((unsigned int)(get_current_milliseconds()));

    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        localHost->location.x = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.y = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.z = rand() % (FLIGHT_AREA_BOUND + 1);
    #endif

    #ifdef DRONE_MOVE_ENABLE
        localHost->velocity.x = rand() % (2 * MAX_DRONE_VELOCITY + 1) - MAX_DRONE_VELOCITY;
        localHost->velocity.y = rand() % (2 * MAX_DRONE_VELOCITY + 1) - MAX_DRONE_VELOCITY;
        localHost->velocity.z = rand() % (2 * MAX_DRONE_VELOCITY + 1) - MAX_DRONE_VELOCITY;
    #endif

    #ifdef RANDOM_DIFF_TIME_ENABLE
    localHost->randOffTime = rand() % (MAX_RANDOM_TIME_OFF + 1);
    #endif
}

// return current time(ms)
uint64_t getCurrentTime() {
    #ifdef RANDOM_DIFF_TIME_ENABLE
        return (get_current_milliseconds() - localHost->baseTime) + localHost->randOffTime;
    #else
        return (get_current_milliseconds() - localHost->baseTime);
    #endif
}

Coordinate_Tuple_t getCurrentLocation() {
    return localHost->location;
}

void modifyLocation(Time_t time_delay) {
    localHost->location.x += localHost->velocity.x * time_delay;
    localHost->location.y += localHost->velocity.y * time_delay;
    localHost->location.z += localHost->velocity.z * time_delay;
}

void local_sleep(uint64_t milliseconds) {
    struct timespec req, rem;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (milliseconds % 1000) * 1000000; 
    
    while (nanosleep(&req, &rem) == -1) {
        req = rem;
    }
}