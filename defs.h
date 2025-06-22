#ifndef DEFS_H
#define DEFS_H
#define _POSIX_C_SOURCE 200809L

#define         table_index_t               int8_t
#define         Time_t                      uint32_t

// ENABLE_MODE
#define         COMMUNICATION_SEND_POSITION_ENABLE              // enable drones to send position(Warning: open)
// #define         DYNAMIC_RANGING_FREQUENCY_ENABLE                // enable dynamic ranging frequency(RANGING_PERIOD_LOW/RANGING_PERIOD)
#define         SAFE_DISTANCE               2000                // distance < SAFE_DISTANCE -> RANGING_PERIOD_LOW(set DYNAMIC_RANGING_FREQUENCY_ENABLE)
#define         SAFE_DISTANCE_ROUND_BORDER  2                   // distance < SAFE_DISTANCE more than SAFE_DISTANCE_ROUND_BORDER -> RANGING_PERIOD(set DYNAMIC_RANGING_FREQUENCY_ENABLE)
#define         CLASSIC_TOF_ENABLE                              // allow classicTof if ratioes not satisfy
// #define         WARM_UP_WAIT_ENABLE                             // discard first DISCARD_MESSAGE_NUM message
// #define         RANDOM_DIFF_TIME_ENABLE                         // enable diff(0 ~ MAX_RANDOM_TIME_OFF) time between drones
// #define         PACKET_LOSS_ENABLE                              // simulate packet loss
#define         PACKET_LOSS_RATE            25                  // rate of packet loss(0~100)
/* Warning: enable only one of RANDOM_MOVE_ENABLE、OPPOSITE_MOVE_ENABLE */
// #define         RANDOM_MOVE_ENABLE                              // enable random move
#define         RANDOM_VELOCITY             3                   // vx/vy/vz < RANDOM_VELOCITY(m/s)
#define         OPPOSITE_MOVE_ENABLE                            // enable opposite flight
#define         OPPOSITE_VELOCITY           7                   // vx/vy/vz = OPPOSITE_VELOCITY(m/s) / 0(m/s)
#define         OPPOSITE_DISTANCE_BASE      5000                // (mm)low distance of drones in OPPOSITE mode           
#define         ALIGN_ENABLE                
#define         ALIGN_ROUNDS                50                  // initial rounds with stationary state
#define         COMPENSATE_MODE                                 // enable compensate for ranging

// LOCAL_HOST_H
#define         MAX_RANDOM_TIME_OFF         10                  // diff time between 
#define         FLIGHT_AREA_UPON_BASE       40000               // (mm)upon space of area for drones to fly
#define         FLIGHT_AREA_LOW_BASE        10000               // (mm)low space of area for drones to fly

// LOCK_H
#define         QUEUE_TASK_LENGTH           3

// RANGING_LIST_H
#define         RANGING_LIST_SIZE           10                  // max size of RangingTable_t.sendBuffer and RangingTable_t.receiveBuffer
#define         FREE_QUEUE_SIZE             RANGING_LIST_SIZE   // max size of freeQueue

// RANGING_BUFFER_H
#define         RANGING_BUFFER_SIZE         6                   // max size of RangingTable_t.validBuffer
#define         CONVERGENCE_THRESHOLD       1
#define         VELOCITY                    0.4691763978616     // (m/s)
#define         UWB_MAX_TIMESTAMP           1099511627776

// MODIFIED_RANGING_H
#define         MESSAGE_BODY_RX_SIZE        3                   // length of Rx in bodyUnits(message)
#define         MESSAGE_HEAD_TX_SIZE        3                   // length of Tx in header(message)
#define         MESSAGE_BODY_UNIT_SIZE      3                   // max size of bodyUnits(message)
#define         TX_BUFFER_POOL_SIZE         RANGING_LIST_SIZE   // max size of localSendBuffer(local)
#define         TABLE_SET_NEIGHBOR_NUM      10                  // number of neighbor recorded(local)
#define         RANGING_PERIOD_MIN          50                  // (ms)
#define         RANGING_PERIOD_MAX          1000                // (ms)
#define         RANGING_PERIOD              200                 // (ms)
#define         RANGING_PERIOD_LOW          100                 // (ms)
#define         RANGING_PERIOD_RAND_RANGE   50                  // (ms)
#define         WARM_UP_TIME                10000               // time for device to warm up
#define         DISCARD_MESSAGE_NUM         25                  // number of message thrown during device warming up
#define         MAX_INITIAL_CALCULATION     6                   // number of times initializeRecordBuffer called(Warning: no more than RANGING_BUFFER_SIZE)
#define         M2T(X)                      ((unsigned int)(X))


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdatomic.h>
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