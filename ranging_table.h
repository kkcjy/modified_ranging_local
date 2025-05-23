#ifndef RANGING_TABLE_H
#define RANGING_TABLE_H

#include "base_struct.h"
#include "debug.h"
#include "table_linked_list.h"
#include "ranging_buffer.h"

typedef struct {
    TableState state; 
    uint16_t address;
    TableLinkedList_t sendBuffer; 
    TableLinkedList_t receiveBuffer; 
    RangingBuffer_t validBuffer;
} __attribute__((packed)) RangingTable_t;


void initRangingTable(RangingTable_t *table);
void enableRangingTable(RangingTable_t *table, uint16_t address);
void disableRangingTable(RangingTable_t *table);
bool compareTablePriority(RangingTable_t *tableA, RangingTable_t *tableB);
void printRangingTable(RangingTable_t *table);

# endif