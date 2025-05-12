#ifndef RANGING_BUFFER_H
#define RANGING_BUFFER_H

#include "defs.h"
#include "debug.h"
#include "base_struct.h"
#include "table_linked_list.h"
#include "nullVal.h"

// for function calculateTof
typedef enum {
    FIRST_CALCULATE,
    SECOND_CALCULATE_UNQUALIFIED,
    SECOND_CALCULATE_ABNORMAL
} FLAG;

/* for a round-trip communication(RangingBufferNode_t)
send                  T1
                Tx   --->   Rx

receive               T2
                Rx   <---   Tx
*/
typedef struct {
    dwTime_t sendTx;    
    dwTime_t sendRx; 
    dwTime_t receiveTx;   
    dwTime_t receiveRx;
    int64_t T1;
    int64_t T2;
    int64_t sumTof;
    uint16_t localSeq;

    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t sendTxCoordinate; 
    Coordinate_Tuple_t sendRxCoordinate; 
    Coordinate_Tuple_t receiveTxCoordinate;
    Coordinate_Tuple_t receiveRxCoordinate;
    #endif
} __attribute__((packed)) RangingBufferNode_t;

/* store RANGING_BUFFER_SIZE round-trip communication
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
void initRangingBufferNode_t(RangingBufferNode_t *node);
void initRangingBuffer(RangingBuffer_t *buffer);
void addRangingBuffer(RangingBuffer_t *buffer, RangingBufferNode_t *node, StatusType status);
table_index_t searchRangingBuffer(RangingBuffer_t *buffer, uint16_t localSeq, StatusType status);
double calculateTof(RangingBuffer_t *buffer, TableNode_t* tableNode, uint16_t checkLocalSeq, StatusType status, FLAG flag);
void initializeRecordBuffer(TableLinkedList_t *listA, TableLinkedList_t *listB, table_index_t firstIndex, RangingBuffer_t* rangingBuffer, StatusType status);
void printRangingBuffer(RangingBuffer_t *buffer);

#endif