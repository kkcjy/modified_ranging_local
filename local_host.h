#ifndef LOCAL_HOST_H
#define LOCAL_HOST_H

#include "defs.h"
#include "debug.h"
#include "base_struct.h"

typedef struct {
    uint16_t localAddress;              // local address
} Local_Host_t;

extern Local_Host_t *localHost;


uint64_t get_current_milliseconds();
uint16_t string_to_hash(const char *str);
void localInit(uint16_t address);
uint64_t getCurrentTime();
void local_sleep(uint64_t milliseconds);

#endif