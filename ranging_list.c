#include "ranging_list.h"


void initRangingListNode(RangingListNode_t *node) {
    node->TxTimestamp.full = NULL_TIMESTAMP;
    node->RxTimestamp.full = NULL_TIMESTAMP;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        node->TxCoordinate = nullCoordinate;
        node->RxCoordinate = nullCoordinate;
    #endif
    node->Tf = NULL_TOF;
    node->localSeq = NULL_SEQ;
    node->remoteSeq = NULL_SEQ;
}

void initRangingList(RangingList_t *list) {
    list->topRangingList = NULL_INDEX;
    for (int i = 0; i < RANGING_LIST_SIZE; i++) {
        initRangingListNode(&list->rangingList[i]);
    }
}

// add record and fill in record
table_index_t addRangingList(RangingList_t *list, RangingListNode_t *node, table_index_t index, StatusType status) {
    if(status == RECEIVER) {
        // RxNode with Rx info —— Rx + localSeq
        if(index == NULL_INDEX) {
            index = (list->topRangingList + 1) % RANGING_LIST_SIZE;
            list->rangingList[index] = *node;
            list->topRangingList = index;
        }

        // TxNode with Tx info —— Tx + remoteSeq
        else {
            list->rangingList[index].TxTimestamp = node->TxTimestamp;
            list->rangingList[index].TxCoordinate = node->TxCoordinate;
            list->rangingList[index].remoteSeq = node->remoteSeq;
        }
    }

    else if(status == SENDER) {
        // FullNode with Tx and Rx info —— Tx + Rx + localSeq + remoteSeq
        index = (list->topRangingList + 1) % RANGING_LIST_SIZE;
        list->rangingList[index].TxTimestamp = node->TxTimestamp;
        list->rangingList[index].RxTimestamp = node->RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            list->rangingList[index].TxCoordinate = node->TxCoordinate;
            list->rangingList[index].RxCoordinate = node->RxCoordinate;
        #endif
        list->rangingList[index].localSeq = node->localSeq;
        list->rangingList[index].remoteSeq = node->remoteSeq;
        list->topRangingList = index;
        return index;
    }
    return NULL_INDEX;
}

// search for index of tableNode whose Rx/Tx time is closest with full info of Tx and Rx
table_index_t searchRangingList(RangingList_t *list, dwTime_t timeStamp, StatusType status) {
    table_index_t index = list->topRangingList;
    if(index == NULL_INDEX) {
        return NULL_INDEX;
    }
    table_index_t ans = NULL_INDEX;
    int count = 0;
    while (count < RANGING_LIST_SIZE) {
        // Rx、Tx is full
        if(status == RECEIVER
            && list->rangingList[index].TxTimestamp.full != NULL_TIMESTAMP 
            && list->rangingList[index].RxTimestamp.full != NULL_TIMESTAMP 
            && list->rangingList[index].TxTimestamp.full < timeStamp.full) {
                if (ans == NULL_INDEX || list->rangingList[index].TxTimestamp.full > list->rangingList[ans].TxTimestamp.full) {
                    ans = index;
            }
        }
        if(status == SENDER
            && list->rangingList[index].RxTimestamp.full != NULL_TIMESTAMP 
            && list->rangingList[index].TxTimestamp.full != NULL_TIMESTAMP 
            && list->rangingList[index].RxTimestamp.full < timeStamp.full) {
                if (ans == NULL_INDEX || list->rangingList[index].RxTimestamp.full > list->rangingList[ans].RxTimestamp.full) {
                    ans = index;
            }
        }
        index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        count++;
    }
    return ans;
}

// find the index of matching localSeq by remoteSeq
table_index_t findLocalSeqIndex(RangingList_t *list, uint16_t remoteSeq){
    table_index_t index = list->topRangingList;
    if(index == NULL_INDEX) {
        return NULL_INDEX;
    }
    int count = 0;
    while(count < RANGING_LIST_SIZE) {
        if(list->rangingList[index].localSeq != NULL_SEQ && list->rangingList[index].localSeq == remoteSeq) {
            return index;
        }
        index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        count++;
    }
    return NULL_INDEX;
}

void printRangingListNode(RangingListNode_t *node) {
    DEBUG_PRINT("TxTimestamp: %ld, RxTimestamp: %ld, Tf: %ld, localSeq: %d, remoteSeq: %d\n", 
        node->TxTimestamp.full, node->RxTimestamp.full, node->Tf, node->localSeq, node->remoteSeq);
}

void printRangingList(RangingList_t *list) {
    DEBUG_PRINT("--------------------START DEBUG_PRINT TABLELINKEDLIST--------------------\n");
    table_index_t index = list->topRangingList;
    if(index == NULL_INDEX) {
        return;
    }
    int count = 0;
    while(count < RANGING_LIST_SIZE) {
        DEBUG_PRINT("TxTimestamp: %ld, RxTimestamp: %ld, Tf: %ld, localSeq: %d, remoteSeq: %d\n", 
            list->rangingList[index].TxTimestamp.full, list->rangingList[index].RxTimestamp.full, list->rangingList[index].Tf, list->rangingList[index].localSeq, list->rangingList[index].remoteSeq);
        index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
    }
    DEBUG_PRINT("--------------------END DEBUG_PRINT TABLELINKEDLIST--------------------\n");
}