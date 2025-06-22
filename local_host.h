#ifndef LOCAL_HOST_H
#define LOCAL_HOST_H

#include "modified_ranging_defconfig.h"
#include "ranging_struct.h"
#include "debug.h"

typedef struct {
    uint16_t localAddress;              // local address
    uint64_t baseTime;                  // local base time(ms)  —— for a same world time
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t location;        // local location
    #endif
    uint64_t randOffTime;               // rand time(ms)        —— for diff between drones
    #if defined(RANDOM_MOVE_ENABLE) || defined(OPPOSITE_MOVE_ENABLE) || defined(SEPARATE_MOVE_ENABLE)
    Velocity_Tuple_t velocity;          // local vilocity
    #endif
} Local_Host_t;

extern Local_Host_t *localHost;


uint64_t get_current_milliseconds();
uint16_t string_to_hash(const char *str);
void localInit(uint16_t address);
uint64_t getCurrentTime();
Coordinate_Tuple_t getCurrentLocation();
void modifyLocation(Time_t time_delay);
void local_sleep(uint64_t milliseconds);

#endif