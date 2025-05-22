#ifndef BASE_STRUCT_H
#define BASE_STRUCT_H

#include "defs.h"
#include "dwTypes.h"

// not used
typedef enum {
    CLASSIC_MODE,
    MODIFIED_MODE
} RangingMODE;

typedef enum {
    SENDER,         // sender in communication
    RECEIVER        // receiver in communication
} StatusType;

typedef enum {
    NULL_STATE,
    USING
} TableState;

// time + seq
typedef struct {
    dwTime_t timestamp;                      
    uint16_t seqNumber;                      
} __attribute__((packed)) Timestamp_Tuple_t; 

#endif