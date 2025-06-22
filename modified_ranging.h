#ifndef MODIFIED_RANGING_H
#define MODIFIED_RANGING_H

#include "defs.h"
#include "ranging_buffer.h"
#include "nullVal.h"
#include "local_host.h"
#include "base_struct.h"


typedef struct {
    uint16_t srcAddress;    // address of the source of message                             
    uint16_t msgSequence;   // sequence of message sent                         
    Timestamp_Tuple_t TxTimestamps[MESSAGE_HEAD_TX_SIZE];
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoodinates[MESSAGE_HEAD_TX_SIZE];
    #endif
    uint16_t msgLength;                                      
} __attribute__((packed)) Message_Header_t;

typedef struct {
    uint16_t dest;          // destination = address of neighbor                                  
    Timestamp_Tuple_t RxTimestamps[MESSAGE_BODY_RX_SIZE]; 
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t RxCoodinates[MESSAGE_BODY_RX_SIZE];
    #endif
} __attribute__((packed)) Message_Body_Unit_t;

/* header + bodyUnits
    header:     address, sequence, TxTimestamps, TxCoodinates, msgLength
    bodyUnits:  neighborAddress, RxTimestamps, RxCoodinates
*/
typedef struct {
    Message_Header_t header;
    Message_Body_Unit_t bodyUnits[MESSAGE_BODY_UNIT_SIZE];
} __attribute__((packed)) Ranging_Message_t;

// rangingMessage + RxTimestamp + RxCoordinate
typedef struct {
    Ranging_Message_t rangingMessage;
    dwTime_t RxTimestamp;                   // local timestamp when message is received
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t RxCoordinate;        // local cooedinate when message is received
    #endif
} __attribute__((packed)) Ranging_Message_With_Additional_Info_t;

typedef struct {
    dwTime_t timestamp;
    uint16_t seqNumber;  
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoordinate; 
    #endif
} __attribute__((packed)) LocalSendBufferNode_t;

typedef struct {
    TableState state; 
    uint16_t address;
    RangingList_t sendList; 
    RangingList_t receiveList; 
    RangingBuffer_t validBuffer;
} __attribute__((packed)) RangingTable_t;

typedef struct {
    int counter;                                                // number of neighbors                                   
    table_index_t topLocalSendBuffer;
    LocalSendBufferNode_t localSendBuffer[TX_BUFFER_POOL_SIZE]; 
    uint8_t neighborPriorityQueue[TABLE_SET_NEIGHBOR_NUM];      // used for choosing neighbors to sent messages
    RangingTable_t neighborReceiveBuffer[TABLE_SET_NEIGHBOR_NUM];
} __attribute__((packed)) RangingTableSet_t;


void initRangingTable(RangingTable_t *table);
void initRangingTableSet();
table_index_t registerRangingTable(uint16_t address);
void addLocalSendBuffer(dwTime_t timestamp, Coordinate_Tuple_t TxCoordinate);
table_index_t findLocalSendBuffer(uint16_t seq);
table_index_t findRangingTable(uint16_t address);
void updatePriority(int address);
void printRangingMessage(Ranging_Message_t* rangingMessage);
void printLocalSendBuffer();
void printRangingTable(RangingTable_t *table);
void printRangingTableSet(StatusType type);
Time_t generateRangingMessage(Ranging_Message_t *rangingMessage);
bool processRangingMessage(Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo);

#endif