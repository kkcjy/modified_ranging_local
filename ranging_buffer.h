#pragma once
#ifndef RANGING_BUFFER_H
#define RANGING_BUFFER_H

#include "base_struct.h"

/* for a round-trip communication(RangingBufferNode)
send                  T1
                Tx   --->   Rx

receive               T2
                Rx   <---   Tx
*/
typedef struct {
    dwTime_t sendTx;    
    dwTime_t sendRx; 
    dwTime_t receiveTx;   
    dwTime_t receiveRx;
    int64_t T1;
    int64_t T2;
    int64_t sumTof;
    uint16_t localSeq;
    uint16_t preLocalSeq;   // for searching

    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
        Coordinate16_Tuple_t sendTxCoordinate; 
        Coordinate16_Tuple_t sendRxCoordinate; 
        Coordinate16_Tuple_t receiveTxCoordinate;
        Coordinate16_Tuple_t receiveRxCoordinate;
    #endif
} __attribute__((packed)) RangingBufferNode; 

// store RANGING_BUFFER_SIZE round-trip communication
typedef struct {
    uint8_t sendLength;
    uint8_t receiveLength;
    table_index_t topSendBuffer;
    table_index_t topReceiveBuffer;
    RangingBufferNode sendBuffer[RANGING_BUFFER_SIZE];
    RangingBufferNode receiveBuffer[RANGING_BUFFER_SIZE];
}RangingBuffer; 

#endif