#pragma once
#ifndef TABLE_LINK_LIST_H
#define TABLE_LINK_LIST_H

#include "base_struct.h"

typedef struct {
    table_index_t freeIndex[FREE_QUEUE_SIZE];
    table_index_t head;
    table_index_t tail;
    table_index_t size;
} __attribute__((packed)) FreeQueue;

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
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
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
    FreeQueue freeQueue;                
    table_index_t head;                 // -128 ~ 127
    table_index_t tail;                 // -128 ~ 127
} __attribute__((packed)) TableLinkedList_t;

# endif