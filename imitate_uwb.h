#ifndef IMITATE_UWB
#define IMITATE_UWB

#include "defs.h"

Local_Host_t localHost;                 // local host
RangingTableSet_t* rangingTableSet;     // local rangingTableSet
QueueTaskLock_t queueTaskLock;          // lock for task
#ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
    int safeRoundCounter = 0;           // counter for safe distance
#endif

#endif