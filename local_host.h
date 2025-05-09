#ifndef LOCAL_HOST_H
#define LOCAL_HOST_H

#include "defs.h"
#include "base_struct.h"

typedef struct {
    uint16_t localAddress;
    uint64_t initTime;                  // local init time(ms)
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t location;        // local location
    #endif
    uint64_t randOffTime;               // rand time(ms) for diff between vehicles
} __attribute__((packed)) Local_Host_t;


uint64_t get_current_milliseconds();
void localInit(Local_Host_t *localHost, uint16_t address);
uint64_t getCurrentTime(Local_Host_t *localHost);
Coordinate_Tuple_t getCurrentLocation(Local_Host_t *localHost);
void local_sleep(uint64_t milliseconds);

#endif