#ifndef RANGING_BUFFER_H
#define RANGING_BUFFER_H

#include "defs.h"
#include "debug.h"
#include "base_struct.h"
#include "ranging_list.h"
#include "nullVal.h"

// for function calculateTof
typedef enum {
    FIRST_CALCULATE,
    SECOND_CALCULATE_UNQUALIFIED,
    SECOND_CALCULATE_ABNORMAL
} CALCULATE_FLAG;

/* a pair of communications
              SENDEDR               |               RECEIVER
                 T1                 |                 T1
    receiveRxTimestamp   <---   receiveTxTimestamp    |       sendTxTimestamp   --->   sendRxTimestamp
                                    |
                 T2                 |                 T2
       sendTxTimestamp   --->   sendRxTimestamp       |    receiveRxTimestamp   <---   receiveTxTimestamp
      [TxSeq]          [RxSeq]      |      [RxSeq]          [TxSeq]
*/
typedef struct {
    dwTime_t sendTxTimestamp;
    dwTime_t sendRxTimestamp;
    dwTime_t receiveTxTimestamp;
    dwTime_t receiveRxTimestamp;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t sendTxCoordinate; 
    Coordinate_Tuple_t sendRxCoordinate; 
    Coordinate_Tuple_t receiveTxCoordinate;
    Coordinate_Tuple_t receiveRxCoordinate;
    #endif
    int64_t sumTof;
    uint16_t TxSeq;
    uint16_t RxSeq;
} __attribute__((packed)) RangingBufferNode_t;

/* used for validBuffer
    sendBuffer
        local   <---    neighbor
        local   --->    neighbor
    receiveBuffer
        local   --->    neighbor
        local   <---    neighbor
*/
typedef struct {
    uint8_t sendLength;
    uint8_t receiveLength;
    table_index_t topSendBuffer;
    table_index_t topReceiveBuffer;
    RangingBufferNode_t sendBuffer[RANGING_BUFFER_SIZE];
    RangingBufferNode_t receiveBuffer[RANGING_BUFFER_SIZE];
} __attribute__((packed)) RangingBuffer_t;

int64_t getInitTofSum();
void initRangingBufferNode(RangingBufferNode_t *node);
void initRangingBuffer(RangingBuffer_t *buffer);
void addRangingBuffer(RangingBuffer_t *buffer, RangingBufferNode_t *node, StatusType status);
table_index_t searchRangingBuffer(RangingBuffer_t *buffer, uint16_t localSeq, StatusType status);
double calculateTof(RangingBuffer_t *buffer, RangingListNode_t* tableNode, uint16_t checkLocalSeq, StatusType status, CALCULATE_FLAG flag, float *Modified, float *Classic, float *True);
void initializeRecordBuffer(RangingList_t *listA, RangingList_t *listB, table_index_t firstIndex, RangingBuffer_t* rangingBuffer, StatusType status);
void printRangingBuffer(RangingBuffer_t *buffer);

#endif