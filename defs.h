#ifndef DEFS_H
#define DEFS_H
#define _POSIX_C_SOURCE 200809L

#define         table_index_t               int8_t
#define         Time_t                      uint32_t

#define         UWB_COMMUNICATION_SEND_POSITION_ENABLE          // enable sending position
#define         DYNAMIC_RANGING_FREQUENCY_ENABLE                // enable dynamic ranging frequency
#define         SAFE_DISTANCE               1                   // (m)should enable DYNAMIC_RANGING_FREQUENCY_ENABLE
#define         SAFE_DISTANCE_ROUND_BORDER  2                   // should enable DYNAMIC_RANGING_FREQUENCY_ENABLE
// #define         CLASSIC_TOF_ENABLE                              // allow classicTof if ratioes not satisfy(used in calculateTof)
#define         WARM_UP_WAIT_ENABLE

// LOCAL_HOST_H
#define         MAX_RANDOM_TIME_OFF         10
#define         FLIGHT_AREA_BOUND           10                  // (m)

// QUEUE_TASK_LOCK_H
#define         QUEUE_TASK_LENGTH           3

// TABLE_LINK_LIST_H
#define         TABLE_BUFFER_SIZE           10                  // max size of RangingTable_t.sendBuffer and RangingTable_t.receiveBuffer
#define         FREE_QUEUE_SIZE             TABLE_BUFFER_SIZE   // max size of freeQueue

// RANGING_BUFFER_H
#define         RANGING_BUFFER_SIZE         6                   // max size of RangingTable_t.validBuffer
#define         CONVERGENCE_THRESHOLD       0.7    
#define         VELOCITY                    0.4691763978616     
#define         UWB_MAX_TIMESTAMP           1099511627776

// MODIFIED_RANGING_H
#define         MESSAGE_BODY_RX_SIZE        1                   // length of Rx in bodyUnits(message)
#define         MESSAGE_HEAD_TX_SIZE        1                   // length of Tx in header(message)
#define         MESSAGE_BODY_UNIT_SIZE      1                   // max size of bodyUnits(message)
#define         TX_BUFFER_POOL_SIZE         TABLE_BUFFER_SIZE   // max size of localSendBuffer(local)
#define         TABLE_SET_NEIGHBOR_NUM      10                  // number of neighbor recorded(local)
#define         RANGING_PERIOD_MIN          50                  // (ms)
#define         RANGING_PERIOD_MAX          1000                // (ms)
#define         RANGING_PERIOD              200                 // (ms)
#define         RANGING_PERIOD_LOW          100                 // (ms)
#define         RANGING_PERIOD_RAND_RANGE   50                  // (ms)
#define         M2T(X)                      ((unsigned int)(X))
#define         WARM_UP_TIME                10000               // time for device to warm up
#define         DISCARD_MESSAGE_NUM         25                  // number of message thrown during device warming up
#define         MAX_INITIAL_CALCULATION     6                   // number of times initializeRecordBuffer called(no more than RANGING_BUFFER_SIZE)


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h> 
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include <sys/socket.h>

#endif