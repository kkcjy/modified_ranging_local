#ifndef TABLE_LINK_LIST_H
#define TABLE_LINK_LIST_H

#include "defs.h"
#include "dwTypes.h"
#include "debug.h"
#include "base_struct.h"
#include "nullVal.h"

typedef struct {
    table_index_t freeIndex[FREE_QUEUE_SIZE];
    table_index_t head;
    table_index_t tail;
    table_index_t room;     // spare space
} __attribute__((packed)) FreeQueue_t;

/* for a message
    SENDER
                Tx   --->   Rx   
        localSeq              remoteSeq
    RECEIVER
                Rx   <---   Tx   
        localSeq              remoteSeq
*/
typedef struct{
    dwTime_t TxTimestamp;         
    dwTime_t RxTimestamp;            
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoordinate;    
    Coordinate_Tuple_t RxCoordinate;   
    #endif
    int64_t Tf;
    uint16_t localSeq;                 
    uint16_t remoteSeq;                 
    table_index_t pre;                  // point to prenode
    table_index_t next;                 // point to nextnode
} __attribute__((packed)) TableNode_t;

// used for sendbuffer and receivebuffer
typedef struct {
    TableNode_t tableBuffer[TABLE_BUFFER_SIZE];
    FreeQueue_t freeQueue;                
    table_index_t head;                 // -128 ~ 127
    table_index_t tail;                 // -128 ~ 127
} __attribute__((packed)) RangingList_t;


void initFreeQueue(FreeQueue_t *queue);
bool isEmpty(FreeQueue_t *queue);
bool isFull(FreeQueue_t *queue);
table_index_t pop(FreeQueue_t *queue);
void push(FreeQueue_t *queue, table_index_t index);
void initTableLinkedList(RangingList_t *list);
table_index_t addTableLinkedList(RangingList_t *list, TableNode_t *node);
void deleteTail(RangingList_t *list);
table_index_t searchTableLinkedList(RangingList_t *list, dwTime_t timeStamp, StatusType status);
table_index_t findRemoteSeqIndex(RangingList_t *list, uint16_t remoteSeq);
void printTableNode(TableNode_t *node);
void printTableLinkedList(RangingList_t *list);

# endif