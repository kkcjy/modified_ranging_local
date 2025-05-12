#ifndef NULL_VAL_H
#define NULL_VAL_H

#include "defs.h"


#define         NULL_ADDR               0
#define         NULL_INDEX              -1
#define         NULL_DONE_INDEX         -2
#define         NULL_SEQ                0
#define         NULL_TIMESTAMP          0
#define         NULL_TOF                0

static const dwTime_t nullTimeStamp = {.full = NULL_TIMESTAMP};
static const Coordinate_Tuple_t nullCoordinate = {.x = -1, .y = -1, .z = -1};

#endif