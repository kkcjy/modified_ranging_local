#ifndef RANGING_STRUCT_H
#define RANGING_STRUCT_H

#include "modified_ranging_defconfig.h"
#include "dwTypes.h"
#include "debug.h"


// --------------------    BASE STRUCTURE   --------------------
typedef enum {
    SENDER,         
    RECEIVER     
} StatusType;

typedef enum {
    UNUSED,
    USING
} TableState;

typedef struct {
    dwTime_t timestamp;                      
    uint16_t seqNumber;                      
} __attribute__((packed)) Timestamp_Tuple_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t z;
} __attribute__((packed)) Coordinate_Tuple_t;

typedef struct {
    int x;
    int y;
    int z;
} __attribute__((packed)) Velocity_Tuple_t;


// --------------------     NULL VALUE      --------------------
#define         NULL_ADDR                   0
#define         NULL_INDEX                  -1
#define         NULL_DONE_INDEX             -2
#define         NULL_SEQ                    0
#define         NULL_TIMESTAMP              0
#define         NULL_TOF                    0
#define         NULL_DIS                    -1

static const dwTime_t nullTimeStamp = {.full = NULL_TIMESTAMP};
static const Coordinate_Tuple_t nullCoordinate = {.x = -1, .y = -1, .z = -1};


/* --------------------     RANGING LIST    --------------------
    a single communication
              SENDEDR               |               RECEIVER
           Tx   --->   Rx           |           Rx   <---   Tx
   [localSeq]          [remoteSeq]  |   [localSeq]          [remoteSeq]
*/
typedef struct {
    dwTime_t TxTimestamp;
    dwTime_t RxTimestamp;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoordinate;
    Coordinate_Tuple_t RxCoordinate;
    #endif
    uint16_t localSeq;
    uint16_t remoteSeq;
} __attribute__((packed)) RangingListNode_t;

// used for sendList and receivelist
typedef struct {
    RangingListNode_t rangingList[RANGING_LIST_SIZE];
    table_index_t topRangingList;               
} __attribute__((packed)) RangingList_t;

void initRangingListNode(RangingListNode_t *node);
void initRangingList(RangingList_t *list);
table_index_t addRangingList(RangingList_t *list, RangingListNode_t *node, table_index_t index, StatusType status);
table_index_t searchRangingList(RangingList_t *list, dwTime_t timeStamp, StatusType status);
table_index_t findLocalSeqIndex(RangingList_t *list, uint16_t remoteSeq);
void printRangingListNode(RangingListNode_t *node);
void printRangingList(RangingList_t *list);


/* --------------------    RANGING BUFFER   --------------------
    a pair of communications
              SENDEDR               |               RECEIVER
                 T1                 |                 T1
    receiveRx   <---   receiveTx    |       sendTx   --->   sendRx
                                    |
                 T2                 |                 T2
       sendTx   --->   sendRx       |    receiveRx   <---   receiveTx
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
    float sumTof;
    uint16_t TxSeq;
    uint16_t RxSeq;
} __attribute__((packed)) RangingBufferNode_t;

// used for validBuffer
typedef struct {
    uint8_t sendLength;
    uint8_t receiveLength;
    table_index_t topSendBuffer;
    table_index_t topReceiveBuffer;
    RangingBufferNode_t sendBuffer[RANGING_BUFFER_SIZE];
    RangingBufferNode_t receiveBuffer[RANGING_BUFFER_SIZE];
} __attribute__((packed)) RangingBuffer_t;

// for function calculateTof
typedef enum {
    FIRST_CALCULATE,
    SECOND_CALCULATE
} CALCULATE_FLAG;

int64_t getInitTofSum();
void initRangingBufferNode(RangingBufferNode_t *bufferNode);
void initRangingBuffer(RangingBuffer_t *buffer);
void addRangingBuffer(RangingBuffer_t *buffer, RangingBufferNode_t *bufferNode, StatusType status);
table_index_t searchRangingBuffer(RangingBuffer_t *buffer, uint16_t localSeq, StatusType status);
float calculateTof(RangingBuffer_t *buffer, RangingListNode_t* listNode, uint16_t checkLocalSeq, StatusType status, CALCULATE_FLAG flag, float *Modified, float *Classic, float *True);
void initializeCalculateTof(RangingList_t *listA, RangingList_t *listB, table_index_t IndexA, RangingBuffer_t* rangingBuffer, StatusType status);
void printRangingBuffer(RangingBuffer_t *buffer);


#endif