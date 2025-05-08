#ifndef MODIFIED_RANGING_H
#define MODIFIED_RANGING_H

#include "defs.h"
#include "ranging_table.h"
#include "nullVal.h"
#include "local_host.h"
#include "base_struct.h"
#include "lock.h"

typedef struct {
    uint16_t dest;          // fill in the address of neighbor                                  
    Timestamp_Tuple_t RxTimestamps[MESSAGE_BODY_RX_SIZE]; 
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t RxCoodinates[MESSAGE_BODY_RX_SIZE];
    #endif
} __attribute__((packed)) Message_Body_Unit_t;

typedef struct {
    uint16_t srcAddress;    // address of the source of message                             
    uint16_t msgSequence;   // the msgSequence-th message sent from local                         
    Timestamp_Tuple_t TxTimestamps[MESSAGE_HEAD_TX_SIZE];
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoodinates[MESSAGE_HEAD_TX_SIZE];
    #endif
    uint16_t msgLength;                                      
} __attribute__((packed)) Message_Header_t;

/* Ranging_Message_t ———— message sent
    header: local address, local sequence, TxTimestamps, TxCoodinates, msgLength
    bodyUnits: neighbor address, RxTimestamps, RxCoodinates
*/
typedef struct {
    Message_Header_t header;
    Message_Body_Unit_t bodyUnits[MESSAGE_BODY_UNIT_SIZE];
} __attribute__((packed)) Ranging_Message_t;

/* Ranging_Message_With_Additional_Info_t  ———— message received
    rangingMessage, rxSeqNumber, rxTimestamp, rxCoordinate
*/
typedef struct {
    Ranging_Message_t rangingMessage;
    Timestamp_Tuple_t RxTimestamp;          // local timestamp when message is received
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t RxCoordinate;        // local cooedinate when message is received
    #endif
} __attribute__((packed)) Ranging_Message_With_Additional_Info_t;

typedef struct { 
    uint16_t seqNumber;  
    dwTime_t timestamp;
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
    Coordinate_Tuple_t TxCoordinate; 
    #endif
} __attribute__((packed)) LocalSendBufferNode_t;

typedef struct {
    int counter;                // number of neighbors                                   
    QueueTaskLock_t mutex;            
    table_index_t topLocalSendBuffer;
    LocalSendBufferNode_t localSendBuffer[TX_BUFFER_POOL_SIZE]; 
    RangingTable_t neighborReceiveBuffer[TABLE_SET_NEIGHBOR_NUM];      
} __attribute__((packed)) RangingTableSet_t;


void initRangingTableSet();
table_index_t registerRangingTable(uint16_t address);
void unregisterRangingTable(uint16_t address);
table_index_t findRangingTable(uint16_t address);
table_index_t findLocalSendBufferNode(uint16_t seq);
int setPriorityIndex();
void printRangingMessage(Ranging_Message_t* rangingMessage);
void printLocalSendBuffer();
void printRangingTableSet();
Time_t generateRangingMessage(Ranging_Message_t *rangingMessage);
bool processRangingMessage(Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo);

#endif