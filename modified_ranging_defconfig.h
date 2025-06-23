#ifndef DEFS_H
#define DEFS_H
#define _POSIX_C_SOURCE 200809L


#define         table_index_t               int8_t
#define         Time_t                      uint32_t

// ENABLE_MODE
#define         COMMUNICATION_SEND_POSITION_ENABLE              // enable drone to send position
#define         CLASSIC_TOF_ENABLE                              // enable classicTof if ratioes not satisfy
#define         COMPENSATE_ENABLE                               // enable compensate for ranging

// #define         DYNAMIC_RANGING_FREQUENCY_ENABLE                // enable dynamic ranging frequency
#define         SAFE_DISTANCE               2000                // distance < SAFE_DISTANCE -> RANGING_PERIOD_LOW
#define         SAFE_DISTANCE_ROUND_BORDER  2                   // distance > SAFE_DISTANCE more than SAFE_DISTANCE_ROUND_BORDER times -> RANGING_PERIOD

// #define         RANDOM_DIFF_TIME_ENABLE                         // enable diff(0 ~ MAX_RANDOM_TIME_OFF) time between drones
#define         MAX_RANDOM_TIME_OFF         10                  // upon of diff time between drones

// #define         PACKET_LOSS_ENABLE                              // enable packet loss
#define         PACKET_LOSS_RATE            25                  // percent rate of packet loss

/* Warning: enable only one of RANDOM_MOVE_ENABLE、OPPOSITE_MOVE_ENABLE */
// #define         RANDOM_MOVE_ENABLE                              // enable random move
#define         RANDOM_VELOCITY             3                   // vx/vy/vz < RANDOM_VELOCITY(m/s)
#define         OPPOSITE_MOVE_ENABLE                            // enable opposite flight
#define         OPPOSITE_VELOCITY           7                   // vx/vy/vz = OPPOSITE_VELOCITY(m/s) / 0(m/s)
#define         OPPOSITE_DISTANCE_BASE      5000                // (mm)low distance of drones in OPPOSITE mode           

#define         ALIGN_ENABLE                                    // enable drone to keep in stationary state for alignment
#define         ALIGN_ROUNDS                50                  // initial rounds with stationary state

// LOCAL_HOST_H
#define         FLIGHT_AREA_UPON_BASE       40000               // (mm)upon space of area for drones to fly
#define         FLIGHT_AREA_LOW_BASE        10000               // (mm)low space of area for drones to fly

// LOCK_H
#define         QUEUE_TASK_LENGTH           3                   // length of queueTask

// RANGING_STRUCT_H
#define         RANGING_LIST_SIZE           10                  // max size of rangingList
#define         RANGING_BUFFER_SIZE         6                   // max size of rangingBuffer
#define         CONVERGENCE_THRESHOLD       1
#define         VELOCITY                    0.4691763978616     // (m/s)
#define         UWB_MAX_TIMESTAMP           1099511627776

// MODIFIED_RANGING_H
#define         MESSAGE_BODY_RX_SIZE        3                   // length of Rx in bodyUnits(message)
#define         MESSAGE_HEAD_TX_SIZE        3                   // length of Tx in header(message)
#define         MESSAGE_BODY_UNIT_SIZE      3                   // max size of bodyUnits(message)
#define         TABLE_SET_NEIGHBOR_NUM      10                  // number of neighbor recorded(local)
#define         LOCAL_SEND_BUFFER_SIZE      RANGING_LIST_SIZE   // max size of localSendBuffer(local)
#define         RANGING_PERIOD              200                 // (ms)
#define         RANGING_PERIOD_LOW          100                 // (ms)
#define         RANGING_PERIOD_RAND_RANGE   50                  // (ms)
#define         MAX_INITIAL_CALCULATION     6                   // number of times initializeCalculateTof called(no more than RANGING_BUFFER_SIZE)
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