#ifndef BASE_STRUCT_H
#define BASE_STRUCT_H

#include "defs.h"
#include "dwTypes.h"


// SENDER / RECEIVER
typedef enum {
    SENDER,         
    RECEIVER     
} StatusType;

// UNUSED / USING
typedef enum {
    UNUSED,
    USING
} TableState;

// timestamp + seqNumber
typedef struct {
    dwTime_t timestamp;                      
    uint16_t seqNumber;                      
} __attribute__((packed)) Timestamp_Tuple_t;

// x y z
typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} __attribute__((packed)) Coordinate_Tuple_t;

// x y z
typedef struct {
    int x;
    int y;
    int z;
} __attribute__((packed)) Velocity_Tuple_t;

#endif