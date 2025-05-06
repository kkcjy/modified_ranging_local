#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

#define         table_index_t               int8_t
#define         Time_t                      uint32_t

#define         UWB_COMMUNICATION_SEND_POSITION_ENABLE
#define         WARM_UP_WAIT_ENABLE

// UWB
#define         UWB_MAX_TIMESTAMP           1099511627776

// LOCAL_HOST_H
#define         MAX_RANDOM_TIME_OFF         10

// QUEUE_TASK_LOCK_H
#define         QUEUE_TASK_LENGTH           3

// TABLE_LINK_LIST_H
#define         TABLE_BUFFER_SIZE           10                  // max size of RangingTable_t.sendBuffer and RangingTable_t.receiveBuffer
#define         FREE_QUEUE_SIZE             TABLE_BUFFER_SIZE   // max size of freeQueue

// RANGING_BUFFER_H
#define         RANGING_BUFFER_SIZE         6                   // max size of RangingTable_t.validBuffer
#define         CONVERGENCE_THRESHOLD       0.7    
#define         VELOCITY                    0.4691763978616

// MODIFIED_RANGING_H
#define         MESSAGE_BODY_RX_SIZE        1                   // length of Rx in bodyUnits(message)
#define         MESSAGE_HEAD_TX_SIZE        1                   // length of Tx in header(message)
#define         MESSAGE_BODY_UNIT_SIZE      1                   // max size of bodyUnits(message)
#define         TX_BUFFER_POOL_SIZE         TABLE_BUFFER_SIZE   // max size of localSendBuffer(local)
#define         TABLE_SET_NEIGHBOR_NUM      10                  // number of neighbor recorded(local)
#define         RANGING_PERIOD_MIN          50
#define         RANGING_PERIOD_MAX          1000
#define         RANGING_PERIOD              200     
#define         RANGING_PERIOD_RAND_RANGE   50  
#define         M2T(X)                      ((unsigned int)(X))
#define         WARM_UP_TIME                10000               // time for device to warm up
#define         DISCARD_MESSAGE_NUM         25                  // number of message thrown during device warming up
#define         MAX_INITIAL_CALCULATION     6                   // number of times initializeRecordBuffer called(no more than RANGING_BUFFER_SIZE)

#endif