#ifndef TABLE_LINK_LIST_H
#define TABLE_LINK_LIST_H

#include "defs.h"
#include "dwTypes.h"
#include "debug.h"
#include "base_struct.h"
#include "nullVal.h"


/* a single communication
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
    int64_t Tf;
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

# endif