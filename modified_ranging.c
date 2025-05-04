#include <string.h>
#include "modified_ranging.h"
#include "nullVal.h"

static RangingTableSet_t* rangingTableSet;      // local rangingTableSet

void initRangingTableSet() {
    rangingTableSet = (RangingTableSet_t*)malloc(sizeof(RangingTableSet_t));
    rangingTableSet->count = 0;
    // rangingTableSet->mutex = xSemaphoreCreateMutex();
    rangingTableSet->topLocalSendBuffer = NULL_INDEX;
    for (table_index_t i = 0; i < TABLE_BUFFER_SIZE; i++) {
        rangingTableSet->localSendBuffer[i].timestamp.full = NULL_TIMESTAMP;
        rangingTableSet->localSendBuffer[i].seqNumber = NULL_SEQ;
        #ifdef UKF_RELATIVE_POSITION_ENABLE
        rangingTableSet->sendBuffer[i].ukfBufferId = NULL_INDEX;
        #endif
    }
    for (int i = 0; i < TABLE_SET_NEIGHBOR_NUM; i++) {
        initRangingTable(&rangingTableSet->neighborReceiveBuffer[i]);
    }
    DEBUG_PRINT("[initRangingTableSet]: RangingTableSet is ready\n");
}

// find the index of RangingTable by address
table_index_t findRangingTable(uint16_t address){
    for (table_index_t i = 0; i < rangingTableSet->count; i++) {
        if(rangingTableSet->neighborReceiveBuffer[i].address == address) {
            return i;
        }
    }
    return NULL_INDEX;
}

// activate a RangingTable for the new neighbor
table_index_t registerRangingTable(uint16_t address) {
    table_index_t index = findRangingTable(address);
    if(index != NULL_INDEX) {
        DEBUG_PRINT("[registerRangingTable]: RangingTable already exists\n");
        return index;
    }
    // push back
    if(rangingTableSet->count < TABLE_SET_NEIGHBOR_NUM) {
        enableRangingTable(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->count], address);
        index = rangingTableSet->count++;
    }
    else {
        DEBUG_PRINT("[registerRangingTable]: RangingTableSet is full, cannot register new table\n");
        return NULL_INDEX;
    }
    return index;
}

void unregisterRangingTable(uint16_t address) {
    table_index_t index = findRangingTable(address);
    if(index == NULL_INDEX) {
        DEBUG_PRINT("[unregisterRangingTable]: RangingTable does not exist\n");
        return;
    }
    // pop back
    for(int i = index; i < rangingTableSet->count - 1; i++) {
        memcpy(&rangingTableSet->neighborReceiveBuffer[i], &rangingTableSet->neighborReceiveBuffer[i + 1], sizeof(RangingTable_t));
    }
    disableRangingTable(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->count - 1]);
    rangingTableSet->count--;
}

// find the index of LocalSendBufferNode by seq
table_index_t findLocalSendBufferNode(uint16_t seq) {
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (rangingTableSet->localSendBuffer[index].seqNumber != NULL_SEQ && count < rangingTableSet->count) {
        if(rangingTableSet->localSendBuffer[index].seqNumber == seq) {
            return index;
        }
        index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        count++;
    }
    return NULL_INDEX;
}