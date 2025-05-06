#include "ranging_table.h"
#include "nullVal.h"

void initRangingTable(RangingTable_t *table) {
    table->state = NULL_STATE;
    table->address = NULL_ADDR;
    initTableLinkedList(&table->sendBuffer);
    initTableLinkedList(&table->receiveBuffer);
    initRangingBuffer(&table->validBuffer);
}

void enableRangingTable(RangingTable_t *table, uint16_t address) {
    table->state = USING;
    table->address = address;
}

void disableRangingTable(RangingTable_t *table) {
    table->state = NULL_STATE;
    table->address = NULL_ADDR;
    initTableLinkedList(&table->sendBuffer);
    initTableLinkedList(&table->receiveBuffer);
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
    return nodeA.receiveRx.full < nodeB.receiveRx.full;
}

void printRangingTable(RangingTable_t *table) {
    DEBUG_PRINT("--------------------START DEBUG_PRINT NEIBORRECEIVEBUFFER--------------------\n");
    DEBUG_PRINT("State: %s\n", (table->state == NULL_STATE) ? "NOT_USING" : "USING");
    DEBUG_PRINT("Address: %d\n", table->address);
    if (table->state == USING) {
        DEBUG_PRINT("[SendBuffer]:\n");
        table_index_t index = table->sendBuffer.head;
        while (index != NULL_INDEX) {
            DEBUG_PRINT("localSeq: %d,remoteSeq: %d,Tx: %lld,Rx: %lld,Tf: %lld\n", 
                table->sendBuffer.tableBuffer[index].localSeq, table->sendBuffer.tableBuffer[index].remoteSeq, 
                table->sendBuffer.tableBuffer[index].TxTimestamp.full, table->sendBuffer.tableBuffer[index].RxTimestamp.full, 
                table->sendBuffer.tableBuffer[index].Tf);
            index = table->sendBuffer.tableBuffer[index].next;
        }
        DEBUG_PRINT("[ReceiveBuffer]:\n");
        index = table->receiveBuffer.head;
        while (index != NULL_INDEX) {
            DEBUG_PRINT("localSeq: %d,remoteSeq: %d,Tx: %lld,Rx: %lld,Tf: %lld\n", 
                table->receiveBuffer.tableBuffer[index].localSeq, table->receiveBuffer.tableBuffer[index].remoteSeq, 
                table->receiveBuffer.tableBuffer[index].TxTimestamp.full, table->receiveBuffer.tableBuffer[index].RxTimestamp.full, 
                table->receiveBuffer.tableBuffer[index].Tf);
            index = table->receiveBuffer.tableBuffer[index].next;
        }
    }
    printRangingBuffer(&table->validBuffer);
    DEBUG_PRINT("--------------------END DEBUG_PRINT NEIBORRECEIVEBUFFER--------------------\n");
}