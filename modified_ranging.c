#include <string.h>
#include <stdlib.h>
#include "modified_ranging.h"
#include "nullVal.h"
#include "time_backup.h"

static uint16_t localAddress;                   // local address
static RangingTableSet_t* rangingTableSet;      // local rangingTableSet
static uint16_t localSendSeqNumber = 1;         // sequence number of locally sent messages
#ifdef WARM_UP_WAIT_ENABLE
    static int discardCount = 0;                // wait for device warming up and discard message
#endif

void initRangingTableSet() {
    rangingTableSet = (RangingTableSet_t*)malloc(sizeof(RangingTableSet_t));
    rangingTableSet->counter = 0;
    // rangingTableSet->mutex = xSemaphoreCreateMutex();
    rangingTableSet->topLocalSendBuffer = NULL_INDEX;
    for (table_index_t i = 0; i < TABLE_BUFFER_SIZE; i++) {
        rangingTableSet->localSendBuffer[i].timestamp.full = NULL_TIMESTAMP;
        rangingTableSet->localSendBuffer[i].seqNumber = NULL_SEQ;
    }
    for (int i = 0; i < TABLE_SET_NEIGHBOR_NUM; i++) {
        initRangingTable(&rangingTableSet->neighborReceiveBuffer[i]);
    }
    DEBUG_PRINT("[initRangingTableSet]: RangingTableSet is ready\n");
}

// find the index of RangingTable by address
table_index_t findRangingTable(uint16_t address) {
    for (table_index_t i = 0; i < rangingTableSet->counter; i++) {
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
    if(rangingTableSet->counter < TABLE_SET_NEIGHBOR_NUM) {
        enableRangingTable(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->counter], address);
        index = rangingTableSet->counter++;
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
    for(int i = index; i < rangingTableSet->counter - 1; i++) {
        memcpy(&rangingTableSet->neighborReceiveBuffer[i], &rangingTableSet->neighborReceiveBuffer[i + 1], sizeof(RangingTable_t));
    }
    disableRangingTable(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->counter - 1]);
    rangingTableSet->counter--;
}

// find the index of LocalSendBufferNode by seq
table_index_t findLocalSendBufferNode(uint16_t seq) {
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (rangingTableSet->localSendBuffer[index].seqNumber != NULL_SEQ && count < rangingTableSet->counter) {
        if(rangingTableSet->localSendBuffer[index].seqNumber == seq) {
            return index;
        }
        index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        count++;
    }
    return NULL_INDEX;
}

static void printRangingMessage(Ranging_Message_t* rangingMessage) {
    DEBUG_PRINT("====================START DEBUG_PRINT RANGINGMESSAGE====================\n");
    DEBUG_PRINT("[Header]:\n");
    DEBUG_PRINT("srcAddress: %u\n", rangingMessage->header.srcAddress);
    DEBUG_PRINT("msgSequence: %u\n", rangingMessage->header.msgSequence);
    DEBUG_PRINT("msgLength: %u\n", rangingMessage->header.msgLength);
    DEBUG_PRINT("TxTimestamps:\n");
    for(int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++) {
        DEBUG_PRINT("\tseqNumber: %u, timestamp: %llu",
        rangingMessage->header.TxTimestamps[i].seqNumber, rangingMessage->header.TxTimestamps[i].timestamp.full);
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(", coordinate:(%u,%u,%u)\n",rangingMessage->header.TxCoodinates[i].x, rangingMessage->header.TxCoodinates[i].y, rangingMessage->header.TxCoodinates[i].z);
        #else
            DEBUG_PRINT("\n");
        #endif
    }
    DEBUG_PRINT("[BodyUnits]:\n");
    for(int i = 0; i < MESSAGE_BODY_UNIT_SIZE; i++){
        DEBUG_PRINT("dest: %u\n", rangingMessage->bodyUnits[i].dest);
        DEBUG_PRINT("RxTimestamps:\n");
        for(int j = 0; j < MESSAGE_BODY_RX_SIZE; j++){
            DEBUG_PRINT("\tseqNumber: %u, timestamp: %llu",
            rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber, rangingMessage->bodyUnits[i].RxTimestamps[j].timestamp.full);
            #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                DEBUG_PRINT(", coordinate:(%u,%u,%u)\n",rangingMessage->bodyUnits[i].RxCoodinates[j].x, rangingMessage->bodyUnits[i].RxCoodinates[j].y, rangingMessage->bodyUnits[i].RxCoodinates[j].z);
            #else
                DEBUG_PRINT("\n");
            #endif
        }
    }
    DEBUG_PRINT("====================END DEBUG_PRINT RANGINGMESSAGE====================\n");
}

static void printLocalSendBuffer() {
    DEBUG_PRINT("--------------------START DEBUG_PRINT LOCALSENDBUFFER--------------------\n");
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (index != NULL_INDEX && count < TABLE_BUFFER_SIZE) {
        DEBUG_PRINT("seq: %d,Tx: %lld", 
            rangingTableSet->localSendBuffer[index].seqNumber, rangingTableSet->localSendBuffer[index].timestamp.full);
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(", coordinate:(%u,%u,%u)\n",rangingTableSet->localSendBuffer[index].TxCoordinate.x, rangingTableSet->localSendBuffer[index].TxCoordinate.y, rangingTableSet->localSendBuffer[index].TxCoordinate.z);
        #else
            DEBUG_PRINT("\n");
        #endif
        index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        count++;
    }
    DEBUG_PRINT("--------------------END DEBUG_PRINT LOCALSENDBUFFER--------------------\n");
}

static void printRangingTableSet() {
    DEBUG_PRINT("====================START DEBUG_PRINT RANGINGTABLESET====================\n");
    printSendBuffer();
    for (table_index_t i = 0; i < rangingTableSet->counter; i++) {
        printRangingTable(&rangingTableSet->neighborReceiveBuffer[i]);
    }
    DEBUG_PRINT("====================START DEBUG_PRINT RANGINGTABLESET====================\n");
}

static Time_t generateRangingMessage(Ranging_Message_t *rangingMessage) {
    Time_t curTime = getCurrentTime();
    Time_t taskDelay = M2T(RANGING_PERIOD + rand()%(RANGING_PERIOD_RAND_RANGE + 1) - RANGING_PERIOD_RAND_RANGE/2);

    int8_t bodyUnitCounter = 0;     // counter for valid bodyunits
    int neighborCounter = 0;        // counter for neighbors recorded in rangingTableSet

    /* generate bodyUnits
        bodyUnit: dest(address of neighbor) + Rx(statuses during the last MESSAGE_BODY_RX_SIZE times of message reception)
    */
    while(bodyUnitCounter < MESSAGE_BODY_UNIT_SIZE && neighborCounter < rangingTableSet->counter) {
        // push the message of rangingTable into bodyUnit
        RangingTable_t *rangingTable = &rangingTableSet->neighborReceiveBuffer[neighborCounter];
        if(rangingTable->state == USING){
            Message_Body_Unit_t *bodyUnit = &rangingMessage->bodyUnits[bodyUnitCounter];
            // dest
            bodyUnit->dest = rangingTable->address;

            // Rx
            table_index_t index = rangingTable->receiveBuffer.head;
            for(int i = 0; i < MESSAGE_BODY_RX_SIZE; i++){
                // receiveBuffer --> bodyUnit->RxTimestamps
                if(index != NULL_INDEX){
                    bodyUnit->RxTimestamps[i].seqNumber = rangingTable->receiveBuffer.tableBuffer[index].remoteSeq;
                    bodyUnit->RxTimestamps[i].timestamp = rangingTable->receiveBuffer.tableBuffer[index].RxTimestamp;
                    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                        bodyUnit->RxCoodinates[i].x = rangingTable->receiveBuffer.tableBuffer[index].RxCoordinate.x;
                        bodyUnit->RxCoodinates[i].y = rangingTable->receiveBuffer.tableBuffer[index].RxCoordinate.y;
                        bodyUnit->RxCoodinates[i].z = rangingTable->receiveBuffer.tableBuffer[index].RxCoordinate.z;
                    #endif
                    index = rangingTable->receiveBuffer.tableBuffer[index].next;
                }
                // insufficient data, fill with null
                else{
                    bodyUnit->RxTimestamps[i].seqNumber = NULL_SEQ;
                    bodyUnit->RxTimestamps[i].timestamp.full = NULL_TIMESTAMP;
                    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                        bodyUnit->RxCoodinates[i] = nullCoordinate;
                    #endif
                }
            }
            bodyUnitCounter++;
        }
        neighborCounter++;
    }

    /* generate header
        srcAddress(local address) + msgSequence(localseq) + Tx(statuses during the last MESSAGE_HEAD_TX_SIZE times of message sending) + msgLength
    */
    Message_Header_t *header = &rangingMessage->header;
    // srcAddress
    header->srcAddress = localAddress;

    // msgSequence
    header->msgSequence = localSendSeqNumber++;

    // Tx
    table_index_t index = rangingTableSet->localSendBuffer;
    for(int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++){
        // localSendBuffer --> header->TxTimestamps
        if(rangingTableSet->localSendBuffer[index].timestamp.full != NULL_TIMESTAMP){
            rangingMessage->header.TxTimestamps[i].seqNumber = rangingTableSet->localSendBuffer[index].seqNumber;
            rangingMessage->header.TxTimestamps[i].timestamp = rangingTableSet->localSendBuffer[index].timestamp;
            #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                rangingMessage->header.TxCoodinates[i].x = rangingTableSet->localSendBuffer[index].TxCoordinate.x;
                rangingMessage->header.TxCoodinates[i].y = rangingTableSet->localSendBuffer[index].TxCoordinate.y;
                rangingMessage->header.TxCoodinates[i].z = rangingTableSet->localSendBuffer[index].TxCoordinate.z;
            #endif
            index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        }
        // insufficient data, fill with null
        else{
            rangingMessage->header.TxTimestamps[i].seqNumber = NULL_SEQ;
            rangingMessage->header.TxTimestamps[i].timestamp.full = NULL_TIMESTAMP;
            #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                rangingMessage->header.TxCoodinates[i] = nullCoordinate;
            #endif
        }
    }

    // msgLength
    header->msgLength = sizeof(Message_Header_t) + sizeof(Message_Body_Unit_t) * bodyUnitCounter;
    
    printRangingMessage(rangingMessage);

    return taskDelay;
}

static void processRangingMessage(Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo) {
    #ifdef WARM_UP_WAIT_ENABLE
        if((discardCount * RANGING_PERIOD < WARM_UP_TIME) || discardCount < DISCARD_MESSAGE_NUM) {
            DEBUG_PRINT("[processRangingMessage]: Discarding message, discardCount:%d\n",discardCount);
            discardCount++;
            return;
        }
    #endif

    Ranging_Message_t *rangingMessage = &rangingMessageWithAdditionalInfo->rangingMessage;

    /* process additionalinfo
        RxTimestamp + RxCoordinate  
    */
    // node: Tx <-- null, Rx <-- AdditionalInfo
    TableNode_t firstNode;
    firstNode.TxTimestamp = nullTimeStamp;
    firstNode.RxTimestamp = rangingMessageWithAdditionalInfo->RxTimestamp.timestamp;
    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
        firstNode.TxCoordinate = nullCoordinate;
        firstNode.RxCoordinate.x = rangingMessageWithAdditionalInfo->RxCoordinate.x;
        firstNode.RxCoordinate.y = rangingMessageWithAdditionalInfo->RxCoordinate.y;
        firstNode.RxCoordinate.z = rangingMessageWithAdditionalInfo->RxCoordinate.z;
    #endif
    firstNode.Tf = NULL_TOF;
    firstNode.localSeq = rangingMessageWithAdditionalInfo->RxTimestamp.seqNumber;
    firstNode.remoteSeq = rangingMessage->header.msgSequence;
    
    /* process header
        srcAddress + msgSequence + Tx
    */
    uint16_t neighborAddress = rangingMessage->header.srcAddress;
    table_index_t neighborIndex = findRangingTable(neighborAddress);

    RangingTable_t *neighborReceiveBuffer = &rangingTableSet->neighborReceiveBuffer[neighborIndex];
    // first time calling: Tx - null Rx - full
    table_index_t newReceiveRecordIndex_AdditionalInfo = addTableLinkedList(&neighborReceiveBuffer->receiveBuffer, &firstNode);

    // handle new neighbor
    if(neighborIndex == NULL_INDEX) {
        neighborIndex = registerRangingTable(neighborAddress);
        if(neighborIndex == NULL_INDEX) {
            DEBUG_PRINT("Warning: Failed to register new neighbor.\n");
            return;
        }
    }

    // find corresponding Rx to calculate
    for (int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++) {
        table_index_t receiveBufferIndex = findRemoteSeqIndex(&neighborReceiveBuffer->receiveBuffer, rangingMessage->header.TxTimestamps[i].seqNumber);
        if (receiveBufferIndex != NULL_INDEX) {
            TableNode_t secondNode;
            secondNode.TxTimestamp = rangingMessage->header.TxTimestamps[i].timestamp;
            secondNode.RxTimestamp = nullTimeStamp;
            #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                secondNode.TxCoordinate.x = rangingMessage->header.TxCoodinates[i].x;
                secondNode.TxCoordinate.y = rangingMessage->header.TxCoodinates[i].y;
                secondNode.TxCoordinate.z = rangingMessage->header.TxCoodinates[i].z;
                secondNode.RxCoordinate = nullCoordinate;
            #endif
            secondNode.Tf = NULL_TOF;
            secondNode.localSeq = rangingMessageWithAdditionalInfo->RxTimestamp.seqNumber;
            secondNode.remoteSeq = rangingMessage->header.msgSequence;
            // second time calling: fill in Tx
            table_index_t newReceiveRecordIndex_Header = addTableLinkedList(&neighborReceiveBuffer->receiveBuffer, &secondNode);

            if(neighborReceiveBuffer->validBuffer.sendLength < MAX_INITIAL_CALCULATION){
                initializeRecordBuffer(&neighborReceiveBuffer->receiveBuffer, &neighborReceiveBuffer->sendBuffer, receiveBufferIndex, &neighborReceiveBuffer->validBuffer, RECEIVER);
                if(neighborReceiveBuffer->validBuffer.sendLength == MAX_INITIAL_CALCULATION){
                    // finish calling initializeRecordBuffer, update Tof
                    int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                    for (int i = 0; i < neighborReceiveBuffer->validBuffer.sendLength; i++) {
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].sumTof = initTof*2;
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].T1 = initTof;
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].T2 = initTof;
                    }
                    DEBUG_PRINT("[initializeRecordBuffer]: finish calling, initTof = %lld\n",initTof);
                }
            }
            else{
                double D = calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->receiveBuffer.tableBuffer[receiveBufferIndex],
                    neighborReceiveBuffer->receiveBuffer.tableBuffer[receiveBufferIndex].localSeq, RECEIVER, true);
                if(D == -1) {
                    DEBUG_PRINT("[calculateTof]: Failed to calculate TOF\n");
                }
            }
        }
        else{
            DEBUG_PRINT("Warning: Cannot find corresponding Rx timestamp for Tx timestamp while processing rangingMessage->header\n");
        }
    }

    /* process bodyUnit
        dest + Rx
    */
    for (int i = 0; i < MESSAGE_BODY_UNIT_SIZE; i++) {
        if (rangingMessage->bodyUnits[i].dest == localAddress) {
            // RxTimestamp with larger seqNumber come first when generating message
            for (int j = MESSAGE_BODY_RX_SIZE - 1; j >= 0 ; j--) {
                if (rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber != NULL_SEQ) {
                    table_index_t sendBufferIndex = findLocalSendBufferNode(rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber);
                    if (sendBufferIndex == NULL_INDEX) {
                        DEBUG_PRINT("Warning: Cannot find corresponding Tx timestamp for Rx timestamp while processing rangingMessage->bodyUnit\n");
                        continue;
                    }

                    TableNode_t node;
                    node.TxTimestamp = rangingTableSet->localSendBuffer[sendBufferIndex].timestamp;
                    node.RxTimestamp = rangingMessage->bodyUnits[i].RxTimestamps[j].timestamp;
                    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
                        node.TxCoordinate.x = rangingTableSet->localSendBuffer[sendBufferIndex].TxCoordinate.x;
                        node.TxCoordinate.y = rangingTableSet->localSendBuffer[sendBufferIndex].TxCoordinate.y;
                        node.TxCoordinate.z = rangingTableSet->localSendBuffer[sendBufferIndex].TxCoordinate.z;
                        node.RxCoordinate.x = rangingMessage->bodyUnits[i].RxCoodinates[j].x;
                        node.RxCoordinate.y = rangingMessage->bodyUnits[i].RxCoodinates[j].y;
                        node.RxCoordinate.z = rangingMessage->bodyUnits[i].RxCoodinates[j].z;
                    #endif
                    node.Tf = NULL_TOF;
                    node.localSeq = rangingTableSet->localSendBuffer[sendBufferIndex].seqNumber;
                    node.remoteSeq = rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber;
                    table_index_t newReceiveRecordIndex_BodyUnit = addTableLinkedList(&neighborReceiveBuffer->sendBuffer, &node);

                    if(neighborReceiveBuffer->validBuffer.receiveLength < MAX_INITIAL_CALCULATION){
                        initializeRecordBuffer(&neighborReceiveBuffer->sendBuffer, &neighborReceiveBuffer->receiveBuffer, newReceiveRecordIndex_BodyUnit, &neighborReceiveBuffer->validBuffer, SENDER);
                        if(neighborReceiveBuffer->validBuffer.receiveLength == MAX_INITIAL_CALCULATION){
                            int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                            for (int i = 0; i < neighborReceiveBuffer->validBuffer.receiveLength; i++) {
                                neighborReceiveBuffer->validBuffer.receiveBuffer[i].sumTof = initTof*2;
                                neighborReceiveBuffer->validBuffer.receiveBuffer[i].T1 = initTof;
                                neighborReceiveBuffer->validBuffer.receiveBuffer[i].T2 = initTof;
                            }
                            DEBUG_PRINT("initTof:%lld\n",initTof);
                        }
                    }
                    else {
                        double d = calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->sendBuffer.tableBuffer[newReceiveRecordIndex_BodyUnit],
                            neighborReceiveBuffer->sendBuffer.tableBuffer[newReceiveRecordIndex_BodyUnit].localSeq, SENDER, true);
                        if(d == -1){
                            DEBUG_PRINT("Warning: Failed to calculate TOF.\n");
                        }
                    }
                }
            }
            break;
        }
    }

    printRangingTableSet();
}

/* --------------------------------------------------------------------------------
Time_t generateRangingMessage(Ranging_Message_t *rangingMessage);
void processRangingMessage(Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo);

    uwbTask
*/

static void imitateUWBRangingTxTask() {
    
}