#include "defs.h"
#include "ranging_table.h"

void initRangingTable(RangingTable_t *table) {
    table->state = UNUSED;
    table->address = NULL_ADDR;
    initRangingList(&table->sendList);
    initRangingList(&table->receiveList);
    initRangingBuffer(&table->validBuffer);
}

void enableRangingTable(RangingTable_t *table, uint16_t address) {
    table->state = USING;
    table->address = address;
}

void disableRangingTable(RangingTable_t *table) {
    table->state = UNUSED;
    table->address = NULL_ADDR;
    initRangingList(&table->sendList);
    initRangingList(&table->receiveList);
    initRangingBuffer(&table->validBuffer);
}

/* judge whether the data of communication with neighborA(tableA) is older than neighborB(tableB)
    when it comes to the role of sender, the most recent role assignment should be receiver
            Rx         Tx                 Rx

        sx                 Rx         Tx
*/
bool compareTablePriority(RangingTable_t *tableA, RangingTable_t *tableB) {
    RangingBufferNode_t nodeA = tableA->validBuffer.receiveBuffer[tableA->validBuffer.topReceiveBuffer];
    RangingBufferNode_t nodeB = tableB->validBuffer.receiveBuffer[tableB->validBuffer.topReceiveBuffer];
    return nodeA.receiveRxTimestamp.full < nodeB.receiveRxTimestamp.full;
}

void printRangingTable(RangingTable_t *table) {
    DEBUG_PRINT("State: %s\n", (table->state == UNUSED) ? "NOT_USING" : "USING");
    DEBUG_PRINT("Address: %d\n", table->address);
    if (table->state == USING) {
        DEBUG_PRINT("[SendList]:\n");
        table_index_t index = table->sendList.topRangingList;
        if(index == NULL_INDEX) {
            return;
        }
        int count = 0;
        while(count < RANGING_LIST_SIZE) {
            DEBUG_PRINT("localSeq: %d,remoteSeq: %d,Tx: %lld,Rx: %lld,Tf: %lld\n", 
                table->sendList.rangingList[index].localSeq, table->sendList.rangingList[index].remoteSeq, 
                table->sendList.rangingList[index].TxTimestamp.full, table->sendList.rangingList[index].RxTimestamp.full, 
                table->sendList.rangingList[index].Tf);
            index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        }
        DEBUG_PRINT("[ReceiveList]:\n");
        index = table->receiveList.topRangingList;
        while(count < RANGING_LIST_SIZE) {
            DEBUG_PRINT("localSeq: %d,remoteSeq: %d,Tx: %lld,Rx: %lld,Tf: %lld\n", 
                table->receiveList.rangingList[index].localSeq, table->receiveList.rangingList[index].remoteSeq, 
                table->receiveList.rangingList[index].TxTimestamp.full, table->receiveList.rangingList[index].RxTimestamp.full, 
                table->receiveList.rangingList[index].Tf);
            index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        }
    }
    printRangingBuffer(&table->validBuffer);
}