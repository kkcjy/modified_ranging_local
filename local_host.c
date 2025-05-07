#include "local_host.h"


// get current time(ms)
uint64_t get_current_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000) / 1000;
}

// set localAddress initTime and randOffTime(ms)
void LocalInit(Local_Host_t localHost, uint16_t address) {
    localHost.localAddress = address;

    localHost.initTime = get_current_milliseconds(); 

    srand((unsigned int)(localHost.initTime));
    localHost.randOffTime = rand() % (MAX_RANDOM_TIME_OFF + 1);
}

// return current time(ms)
uint64_t getCurrentTime(Local_Host_t localHost) {
    return (get_current_milliseconds() - localHost.initTime) + localHost.randOffTime;
}