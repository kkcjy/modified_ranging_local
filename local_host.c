#include "local_host.h"


// get current time(ms) - should not be called
uint64_t get_current_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000) / 1000;
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

    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
        localHost->location.x = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.y = rand() % (FLIGHT_AREA_BOUND + 1);
        localHost->location.z = rand() % (FLIGHT_AREA_BOUND + 1);
    #endif

    localHost->randOffTime = rand() % (MAX_RANDOM_TIME_OFF + 1);
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

void local_sleep(uint64_t milliseconds) {
    struct timespec req = {
        .tv_sec = milliseconds / 1000,              
        .tv_nsec = (milliseconds % 1000) * 1000000   
    };
    nanosleep(&req, NULL);
}