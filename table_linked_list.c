#include "table_linked_list.h"


void initFreeQueue(FreeQueue_t *queue) {
    queue->room = FREE_QUEUE_SIZE;
    queue->tail = FREE_QUEUE_SIZE - 1;
    queue->head = 0;
    for (table_index_t i = 0; i < FREE_QUEUE_SIZE; i++) {
        queue->freeIndex[i] = i;
    }
}

// spare space is empty
bool isEmpty(FreeQueue_t *queue) {
    return queue->room == 0;
}

// spare space is full
bool isFull(FreeQueue_t *queue) {
    return queue->room == FREE_QUEUE_SIZE;
}

// get a node from freequeue
table_index_t pop(FreeQueue_t *queue) {
    if(isEmpty(queue)) {
        return NULL_INDEX;
    }
    table_index_t index = queue->freeIndex[queue->head];
    queue->freeIndex[queue->head] = NULL_INDEX;
    queue->head = (queue->head + 1) % FREE_QUEUE_SIZE;
    queue->room--;
    return index;
}

// push a node into freequeue
void push(FreeQueue_t *queue, table_index_t index) {
    if(isFull(queue)) {
        return;
    }
    queue->tail = (queue->tail + 1) % FREE_QUEUE_SIZE;
    queue->freeIndex[queue->tail] = index;
    queue->room++;
}

void initTableLinkedList(TableLinkedList_t *list) {
    for (uint8_t i = 0; i < TABLE_BUFFER_SIZE; i++) {
        list->tableBuffer[i].TxTimestamp.full = NULL_TIMESTAMP;
        list->tableBuffer[i].RxTimestamp.full = NULL_TIMESTAMP;
        list->tableBuffer[i].Tf = NULL_TOF;
        list->tableBuffer[i].localSeq = NULL_SEQ;
        list->tableBuffer[i].remoteSeq = NULL_SEQ;
        list->tableBuffer[i].pre = NULL_INDEX;
        list->tableBuffer[i].next = NULL_INDEX;
    }
    initFreeQueue(&list->freeQueue);
    list->head = NULL_INDEX;
    list->tail = NULL_INDEX;
}

// add record and fill in record
table_index_t addTableLinkedList(TableLinkedList_t *list, TableNode_t *node) {
    table_index_t index = list->head;

    // fill in record
    while (index != NULL_INDEX) {
        /* this node has been recorded and is not complete
            during the processing of Ranging_Message_With_Additional_Info_t
            get Rx for the first time and store it in the list(set Tx null)
            get Tx for the second time, find the corresponding position and store it
        */ 
        if (list->tableBuffer[index].localSeq == node->remoteSeq) {
            list->tableBuffer[index].TxTimestamp = node->TxTimestamp;
            list->tableBuffer[index].remoteSeq = node->remoteSeq;
            return index;
        }
        index = list->tableBuffer[index].next;
    }

    // freequeue is empty, remove the last record
    if (isEmpty(&list->freeQueue)) {
        deleteTail(list);
    }

    index = pop(&list->freeQueue);

    // add record
    list->tableBuffer[index].TxTimestamp = node->TxTimestamp;
    list->tableBuffer[index].RxTimestamp = node->RxTimestamp;
    list->tableBuffer[index].Tf = node->Tf;
    list->tableBuffer[index].localSeq = node->localSeq;
    list->tableBuffer[index].remoteSeq = node->remoteSeq;

    // update
    if (list->head == NULL_INDEX) {
        list->head = index;
        list->tail = index;
    }
    else {
        list->tableBuffer[list->head].pre = index;
        list->tableBuffer[index].next = list->head;
        list->head = index;
    }
    return index;
}

// delete the last record
void deleteTail(TableLinkedList_t *list) {
    if (list->tail == NULL_INDEX) {
        return;
    }
    
    table_index_t index = list->tail;
    list->tail = list->tableBuffer[index].pre;
    if (list->tail != NULL_INDEX) {
        list->tableBuffer[list->tail].next = NULL_INDEX;
    }

    // clean data
    list->tableBuffer[index].TxTimestamp.full = NULL_TIMESTAMP;
    list->tableBuffer[index].RxTimestamp.full = NULL_TIMESTAMP;
    list->tableBuffer[index].Tf = NULL_TOF;
    list->tableBuffer[index].localSeq = NULL_SEQ;
    list->tableBuffer[index].remoteSeq = NULL_SEQ;
    list->tableBuffer[index].next = NULL_INDEX;
    list->tableBuffer[index].pre = NULL_INDEX;
    push(&list->freeQueue, index);
}

// search for index of tableNode whose Rx/Tx time is closest and which has complete Tx and Rx
table_index_t searchTableLinkedList(TableLinkedList_t *list, dwTime_t timeStamp, StatusType status) {
    table_index_t index = list->head;
    table_index_t ans = NULL_INDEX;
    while (index != NULL_INDEX) {
        // Rx、Tx is full
        if(status == RECEIVER 
            && list->tableBuffer[index].TxTimestamp.full != NULL_TIMESTAMP 
            && list->tableBuffer[index].RxTimestamp.full != NULL_TIMESTAMP 
            && list->tableBuffer[index].TxTimestamp.full < timeStamp.full) {
                if (ans == NULL_INDEX || list->tableBuffer[index].TxTimestamp.full > list->tableBuffer[ans].TxTimestamp.full) {
                    ans = index;
            }
        }
        if(status == SENDER
            && list->tableBuffer[index].RxTimestamp.full != NULL_TIMESTAMP 
            && list->tableBuffer[index].TxTimestamp.full != NULL_TIMESTAMP 
            && list->tableBuffer[index].RxTimestamp.full < timeStamp.full) {
                if (ans == NULL_INDEX || list->tableBuffer[index].RxTimestamp.full > list->tableBuffer[ans].RxTimestamp.full) {
                    ans = index;
            }
        }
        index = list->tableBuffer[index].next;
    }
    return ans;
}

// find the index of specified remoteSeq record
table_index_t findRemoteSeqIndex(TableLinkedList_t *list, uint16_t remoteSeq){
    table_index_t index = list->head;
    while (index != NULL_INDEX) {
        // have processed
        if (list->tableBuffer[index].remoteSeq == remoteSeq) {
            return NULL_DONE_INDEX;
        }
        if (list->tableBuffer[index].localSeq == remoteSeq) {
            return index;
        }
        index = list->tableBuffer[index].next;
    }
    return NULL_INDEX;
}

void printTableNode(TableNode_t *node) {
    DEBUG_PRINT("TxTimestamp: %ld, RxTimestamp: %ld, Tf: %ld, localSeq: %d, remoteSeq: %d\n", 
        node->TxTimestamp.full, node->RxTimestamp.full, node->Tf, node->localSeq, node->remoteSeq);
}

void printTableLinkedList(TableLinkedList_t *list) {
    DEBUG_PRINT("--------------------START DEBUG_PRINT TABLELINKEDLIST--------------------\n");
    table_index_t index = list->head;
    while (index != NULL_INDEX)
    {
        DEBUG_PRINT("TxTimestamp: %ld, RxTimestamp: %ld, Tf: %ld, localSeq: %d, remoteSeq: %d\n", 
            list->tableBuffer[index].TxTimestamp.full, list->tableBuffer[index].RxTimestamp.full, list->tableBuffer[index].Tf, list->tableBuffer[index].localSeq, list->tableBuffer[index].remoteSeq);
        index = list->tableBuffer[index].next;
    }
    DEBUG_PRINT("--------------------END DEBUG_PRINT TABLELINKEDLIST--------------------\n");
}