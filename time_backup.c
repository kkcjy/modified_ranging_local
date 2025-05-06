#include "time_backup.h"

// get current time(ms)
uint64_t get_current_milliseconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((uint64_t)ts.tv_sec * 1000000 + (uint64_t)ts.tv_nsec / 1000) / 1000;
}

// sleep specific time(ms)
void precise_msleep(uint64_t milliseconds) {
    struct timespec req = {
        .tv_sec = milliseconds / 1000,              
        .tv_nsec = (milliseconds % 1000) * 1000000   
    };
    nanosleep(&req, NULL);
}

// set initTime and randOffTime(ms)
void iniTimeSetting() {
    initTime = get_current_milliseconds(); 

    srand((unsigned int)(initTime));
    randOffTime = rand() % (MAX_RANDOM_TIME_OFF + 1);
}

// return current time(ms)
uint64_t getCurrentTime() {
    return (get_current_milliseconds() - initTime) + randOffTime;
}