#ifndef LOCAL_HOST_H
#define LOCAL_HOST_H

#include "defs.h"

typedef struct {
    uint16_t localAddress;
    uint64_t initTime;              // local init time(ms)
    uint64_t randOffTime;           // rand time(ms) for diff between vehicles
} __attribute__((packed)) Local_Host_t;


uint64_t get_current_milliseconds();
void LocalInit(Local_Host_t localHost, uint16_t address);
uint64_t getCurrentTime(Local_Host_t localHost);
void local_sleep(uint64_t milliseconds);

#endif