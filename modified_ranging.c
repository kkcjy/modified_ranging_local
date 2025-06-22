#include "modified_ranging.h"

Local_Host_t *localHost;                        // local host
RangingTableSet_t* rangingTableSet;             // local rangingTableSet
uint16_t localSendSeqNumber = 1;                // seqNumber of message local sent  
int RangingPeriod = RANGING_PERIOD;             // period of sending
#ifdef COMPENSATE_MODE
    int64_t lastD;     
    float compensateRate = 0.618;      
#endif

void initRangingTable(RangingTable_t *table) {
    table->state = UNUSED;
    table->address = NULL_ADDR;
    initRangingList(&table->sendList);
    initRangingList(&table->receiveList);
    initRangingBuffer(&table->validBuffer);
}

void initRangingTableSet() {
    rangingTableSet = (RangingTableSet_t*)malloc(sizeof(RangingTableSet_t));
    rangingTableSet->counter = 0;
    rangingTableSet->topLocalSendBuffer = 0;
    for (table_index_t i = 0; i < TX_BUFFER_POOL_SIZE; i++) {
        rangingTableSet->localSendBuffer[i].timestamp.full = NULL_TIMESTAMP;
        rangingTableSet->localSendBuffer[i].seqNumber = NULL_SEQ;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            rangingTableSet->localSendBuffer[i].TxCoordinate = nullCoordinate;
        #endif
    }
    for (int i = 0; i < TABLE_SET_NEIGHBOR_NUM; i++) {
        rangingTableSet->neighborPriorityQueue[i] = NULL_INDEX;
        initRangingTable(&rangingTableSet->neighborReceiveBuffer[i]);
    }
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
        rangingTableSet->neighborReceiveBuffer[rangingTableSet->counter].state = USING;
        rangingTableSet->neighborReceiveBuffer[rangingTableSet->counter].address = address;
        index = rangingTableSet->counter++;
    }
    else {
        DEBUG_PRINT("[registerRangingTable]: RangingTableSet is full, cannot register new table\n");
        return NULL_INDEX;
    }
    return index;
}

void addLocalSendBuffer(dwTime_t timestamp, Coordinate_Tuple_t TxCoordinate) {
    rangingTableSet->topLocalSendBuffer = (rangingTableSet->topLocalSendBuffer + 1) % TX_BUFFER_POOL_SIZE;
    // localSendSeqNumber has updated
    rangingTableSet->localSendBuffer[rangingTableSet->topLocalSendBuffer].seqNumber = localSendSeqNumber - 1;
    rangingTableSet->localSendBuffer[rangingTableSet->topLocalSendBuffer].timestamp = timestamp;
    rangingTableSet->localSendBuffer[rangingTableSet->topLocalSendBuffer].TxCoordinate = TxCoordinate;
}

// find the index of LocalSendBufferNode by seq
table_index_t findLocalSendBuffer(uint16_t seq) {
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (rangingTableSet->localSendBuffer[index].seqNumber != NULL_SEQ && count < RANGING_LIST_SIZE) {
        if(rangingTableSet->localSendBuffer[index].seqNumber == seq) {
            return index;
        }
        index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        count++;
    }
    return NULL_INDEX;
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

// update the priority queue(top - 0), push address to the tail
void updatePriority(int address) {
    for(int i = 0; i < rangingTableSet->counter; i++) {
        if(rangingTableSet->neighborReceiveBuffer[i].address == address) {
            // move the index of table to the tail of priority queue
            for(int j = i; j < rangingTableSet->counter - 1; j++) {
                uint8_t tmp = rangingTableSet->neighborPriorityQueue[j];
                rangingTableSet->neighborPriorityQueue[j] = rangingTableSet->neighborPriorityQueue[j + 1];
                rangingTableSet->neighborPriorityQueue[j + 1] = tmp;
            }
            rangingTableSet->neighborPriorityQueue[rangingTableSet->counter - 1] = i;
            return;
        }
        else {
            rangingTableSet->neighborPriorityQueue[i] = i;
        }
    }
}

void printRangingMessage(Ranging_Message_t* rangingMessage) {
    DEBUG_PRINT("[Header]:\n");
    DEBUG_PRINT("srcAddress: %u\n", rangingMessage->header.srcAddress);
    DEBUG_PRINT("msgSequence: %u\n", rangingMessage->header.msgSequence);
    DEBUG_PRINT("msgLength: %u\n", rangingMessage->header.msgLength);
    DEBUG_PRINT("TxTimestamps:\n");
    for(int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++) {
        DEBUG_PRINT("\tseqNumber: %u, timestamp: %llu",
        rangingMessage->header.TxTimestamps[i].seqNumber, rangingMessage->header.TxTimestamps[i].timestamp.full);
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
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
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                DEBUG_PRINT(", coordinate:(%u,%u,%u)\n",rangingMessage->bodyUnits[i].RxCoodinates[j].x, rangingMessage->bodyUnits[i].RxCoodinates[j].y, rangingMessage->bodyUnits[i].RxCoodinates[j].z);
            #else
                DEBUG_PRINT("\n");
            #endif
        }
    }
}

void printLocalSendBuffer() {
    DEBUG_PRINT("-----------------------------LOCALSENDBUFFER-----------------------------\n");
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (count < RANGING_LIST_SIZE) {
        DEBUG_PRINT("seq: %d,Tx: %lld", 
            rangingTableSet->localSendBuffer[index].seqNumber, rangingTableSet->localSendBuffer[index].timestamp.full);
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(", coordinate:(%u,%u,%u)\n",rangingTableSet->localSendBuffer[index].TxCoordinate.x, rangingTableSet->localSendBuffer[index].TxCoordinate.y, rangingTableSet->localSendBuffer[index].TxCoordinate.z);
        #else
            DEBUG_PRINT("\n");
        #endif
        index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        count++;
    }
    DEBUG_PRINT("-----------------------------LOCALSENDBUFFER-----------------------------\n");
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

// flag == 1 -> generate, flag == 0 -> process
void printRangingTableSet(StatusType type) {
    if(type == SENDER) {
        DEBUG_PRINT("\n==========================GENERATE_RANGINGTABLESET=======================\n");
    }
    else if(type == RECEIVER) {
        DEBUG_PRINT("\n=========================PROCESS_RANGINGTABLESET=========================\n");
    }
    printLocalSendBuffer();
    DEBUG_PRINT("\n--------------------------NEIGHBORRECEIVEBUFFER--------------------------\n");
    for (table_index_t i = 0; i < rangingTableSet->counter; i++) {
        printRangingTable(&rangingTableSet->neighborReceiveBuffer[i]);
    }
    DEBUG_PRINT("--------------------------NEIGHBORRECEIVEBUFFER--------------------------\n");
    if(type == SENDER) {
        DEBUG_PRINT("==========================GENERATE_RANGINGTABLESET=======================\n\n");
    }
    else if(type == RECEIVER) {
        DEBUG_PRINT("=========================PROCESS_RANGINGTABLESET=========================\n\n");
    }
}

Time_t generateRangingMessage(Ranging_Message_t *rangingMessage) {
    Time_t taskDelay = RANGING_PERIOD;

    int8_t bodyUnitCounter = 0;     // counter for valid bodyunits

    /* generate bodyUnits
        bodyUnit: dest(address of neighbor) + Rx(statuses during the last MESSAGE_BODY_RX_SIZE times of message reception)
    */
    while(bodyUnitCounter < MESSAGE_BODY_UNIT_SIZE && bodyUnitCounter < rangingTableSet->counter) {
        // push the message of rangingTable into bodyUnit
        RangingTable_t *rangingTable = &rangingTableSet->neighborReceiveBuffer[rangingTableSet->neighborPriorityQueue[bodyUnitCounter]];
        Message_Body_Unit_t *bodyUnit = &rangingMessage->bodyUnits[bodyUnitCounter];
        // dest
        bodyUnit->dest = rangingTable->address;

        // Rx
        table_index_t index = rangingTable->receiveList.topRangingList;
        int count = 0;
        while(count < MESSAGE_BODY_RX_SIZE && count < RANGING_LIST_SIZE) {
            // receiveBuffer --> bodyUnit->RxTimestamps
            bodyUnit->RxTimestamps[count].seqNumber = rangingTable->receiveList.rangingList[index].localSeq;
            bodyUnit->RxTimestamps[count].timestamp = rangingTable->receiveList.rangingList[index].RxTimestamp;
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                bodyUnit->RxCoodinates[count] = rangingTable->receiveList.rangingList[index].RxCoordinate;
            #endif
            index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
            count++;
        }
        bodyUnitCounter++;
    }

    /* generate header
        srcAddress(local address) + msgSequence(localseq) + Tx(statuses during the last MESSAGE_HEAD_TX_SIZE times of message sending) + msgLength
    */
    Message_Header_t *header = &rangingMessage->header;

    // srcAddress
    header->srcAddress = localHost->localAddress;

    // msgSequence
    header->msgSequence = localSendSeqNumber++;

    // Tx
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    for(int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++){
        // localSendBuffer --> header->TxTimestamps
        if(rangingTableSet->localSendBuffer[index].timestamp.full != NULL_TIMESTAMP){
            rangingMessage->header.TxTimestamps[i].seqNumber = rangingTableSet->localSendBuffer[index].seqNumber;
            rangingMessage->header.TxTimestamps[i].timestamp = rangingTableSet->localSendBuffer[index].timestamp;
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                rangingMessage->header.TxCoodinates[i] = rangingTableSet->localSendBuffer[index].TxCoordinate;
            #endif
            index = (index - 1 + RANGING_LIST_SIZE) % RANGING_LIST_SIZE;
        }
        // insufficient data, fill with null
        else{
            rangingMessage->header.TxTimestamps[i].seqNumber = NULL_SEQ;
            rangingMessage->header.TxTimestamps[i].timestamp.full = NULL_TIMESTAMP;
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                rangingMessage->header.TxCoodinates[i] = nullCoordinate;
            #endif
        }
    }

    // msgLength
    header->msgLength = sizeof(Message_Header_t) + sizeof(Message_Body_Unit_t) * bodyUnitCounter;
    
    // DEBUG_PRINT("\n*************************[generateRangingMessage]************************\n");
    // printRangingMessage(rangingMessage);
    // DEBUG_PRINT("*************************[generateRangingMessage]************************\n\n");

    return taskDelay;
}

// return the min Tof with neighbor
bool processRangingMessage(Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo) {
    float H_Modified = NULL_DIS;
    float H_Classic = NULL_DIS;
    float H_True = NULL_DIS;
    float B_Modified = NULL_DIS;
    float B_Classic = NULL_DIS;
    float B_True = NULL_DIS;
    float ModifiedD = NULL_DIS;
    float ClassicD = NULL_DIS;
    float TrueD = NULL_DIS;

    Ranging_Message_t *rangingMessage = &rangingMessageWithAdditionalInfo->rangingMessage;

    // DEBUG_PRINT("\n*************************[processRangingMessage]*************************\n");
    // printRangingMessage(rangingMessage);
    // DEBUG_PRINT("*************************[processRangingMessage]*************************\n\n");

    /* process additionalinfo
        RxTimestamp + RxCoordinate  
    */
    // node: Tx <-- null, Rx <-- AdditionalInfo
    RangingListNode_t RxNode;
    RxNode.TxTimestamp = nullTimeStamp;
    RxNode.RxTimestamp = rangingMessageWithAdditionalInfo->RxTimestamp;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        RxNode.TxCoordinate = nullCoordinate;
        RxNode.RxCoordinate = rangingMessageWithAdditionalInfo->RxCoordinate;
    #endif
    RxNode.Tf = NULL_TOF;
    RxNode.localSeq = rangingMessage->header.msgSequence;
    RxNode.remoteSeq = NULL_SEQ;

    /* process header
        srcAddress + msgSequence + Tx
    */
    uint16_t neighborAddress = rangingMessage->header.srcAddress;

    table_index_t neighborIndex = findRangingTable(neighborAddress);

    // handle new neighbor
    if(neighborIndex == NULL_INDEX) {
        neighborIndex = registerRangingTable(neighborAddress);
        if(neighborIndex == NULL_INDEX) {
            DEBUG_PRINT("Warning: Failed to register new neighbor.\n");
            return false;
        }
    }

    updatePriority(neighborAddress);

    RangingTable_t *neighborReceiveBuffer = &rangingTableSet->neighborReceiveBuffer[neighborIndex];

    // DEBUG_PRINT("[RxNode]: ");
    // printRangingListNode(&RxNode);

    // first time calling(RxNode with Rx info —— Rx + localSeq)
    addRangingList(&neighborReceiveBuffer->receiveList, &RxNode, NULL_INDEX, RECEIVER);

    // find corresponding Rx to calculate
    for (int i = MESSAGE_HEAD_TX_SIZE - 1; i >= 0; i--) {
        if(rangingMessage->header.TxTimestamps[i].seqNumber == NULL_SEQ) {
            continue;
        }

        table_index_t receiveListIndex = findLocalSeqIndex(&neighborReceiveBuffer->receiveList, rangingMessage->header.TxTimestamps[i].seqNumber);
        
        // not processed
        if (receiveListIndex != NULL_INDEX && neighborReceiveBuffer->receiveList.rangingList[receiveListIndex].remoteSeq == NULL_SEQ) {
            RangingListNode_t TxNode;
            TxNode.TxTimestamp = rangingMessage->header.TxTimestamps[i].timestamp;
            TxNode.RxTimestamp = nullTimeStamp;
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                TxNode.TxCoordinate = rangingMessage->header.TxCoodinates[i];
                TxNode.RxCoordinate = nullCoordinate;
            #endif
            TxNode.Tf = NULL_TOF;
            TxNode.localSeq = NULL_SEQ;
            TxNode.remoteSeq = rangingMessage->header.TxTimestamps[i].seqNumber;

            // DEBUG_PRINT("[TxNode]: ");
            // printRangingListNode(&TxNode);

            // second time calling(TxNode with Tx info —— Tx + remoteSeq)
            addRangingList(&neighborReceiveBuffer->receiveList, &TxNode, receiveListIndex, RECEIVER);

            if(neighborReceiveBuffer->validBuffer.sendLength < MAX_INITIAL_CALCULATION){
                initializeCalculateTof(&neighborReceiveBuffer->receiveList, &neighborReceiveBuffer->sendList, receiveListIndex, &neighborReceiveBuffer->validBuffer, RECEIVER);
                if(neighborReceiveBuffer->validBuffer.sendLength >= MAX_INITIAL_CALCULATION){
                    // finish calling initializeCalculateTof, update Tof
                    int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                    neighborReceiveBuffer->validBuffer.sendBuffer[i].sumTof = initTof * 2;

                    // DEBUG_PRINT("[initializeCalculateTof]: finish calling, initTof = %lld\n",initTof);
                }
            }
            else{
                // from old data to recent data, get the last valid D
                calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->receiveList.rangingList[receiveListIndex],
                    neighborReceiveBuffer->receiveList.rangingList[receiveListIndex].localSeq, RECEIVER, FIRST_CALCULATE, &H_Modified, &H_Classic, &H_True);
            }
        }
        // missed
        else{
            DEBUG_PRINT("Warning: Cannot find corresponding Rx timestamp for Tx[%d] timestamp while processing rangingMessage->header\n", rangingMessage->header.TxTimestamps[i].seqNumber);
        }
    }

    /* process bodyUnit
        dest + Rx
    */
    for (int i = 0; i < MESSAGE_BODY_UNIT_SIZE; i++) {
        if (rangingMessage->bodyUnits[i].dest == localHost->localAddress) {
            // find corresponding Rx to calculate
            for (int j = MESSAGE_BODY_RX_SIZE - 1; j >= 0 ; j--) {
                if(rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber == NULL_SEQ) {
                    continue;
                }

                table_index_t localSendBufferIndex = findLocalSendBuffer(rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber);
                
                if (localSendBufferIndex != NULL_INDEX) {
                    RangingListNode_t FullNode;
                    FullNode.TxTimestamp = rangingTableSet->localSendBuffer[localSendBufferIndex].timestamp;
                    FullNode.RxTimestamp = rangingMessage->bodyUnits[i].RxTimestamps[j].timestamp;
                    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                        FullNode.TxCoordinate = rangingTableSet->localSendBuffer[localSendBufferIndex].TxCoordinate;
                        FullNode.RxCoordinate = rangingMessage->bodyUnits[i].RxCoodinates[j];
                    #endif
                    FullNode.Tf = NULL_TOF;
                    FullNode.localSeq = rangingTableSet->localSendBuffer[localSendBufferIndex].seqNumber;
                    FullNode.remoteSeq = rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber;

                    // DEBUG_PRINT("[FullNode]: ");
                    // printRangingListNode(&FullNode);

                    // Tx - null Rx - full
                    table_index_t sendListIndex = addRangingList(&neighborReceiveBuffer->sendList, &FullNode, NULL_INDEX, SENDER);

                    if(neighborReceiveBuffer->validBuffer.receiveLength < MAX_INITIAL_CALCULATION){
                        initializeCalculateTof(&neighborReceiveBuffer->sendList, &neighborReceiveBuffer->receiveList, sendListIndex, &neighborReceiveBuffer->validBuffer, SENDER);
                        if(neighborReceiveBuffer->validBuffer.receiveLength == MAX_INITIAL_CALCULATION){
                            int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                            for (int i = 0; i < neighborReceiveBuffer->validBuffer.receiveLength; i++) {
                                neighborReceiveBuffer->validBuffer.receiveBuffer[i].sumTof = initTof * 2;
                            }
                            // DEBUG_PRINT("[initializeCalculateTof]: finish calling, initTof = %lld\n",initTof);
                        }
                    }
                    else {
                        calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->sendList.rangingList[sendListIndex],
                            neighborReceiveBuffer->sendList.rangingList[sendListIndex].localSeq, SENDER, FIRST_CALCULATE, &B_Modified, &B_Classic, &B_True);
                    }
                }
                // missed
                else {
                    DEBUG_PRINT("Warning: Cannot find corresponding Tx timestamp for Rx[%d] timestamp while processing rangingMessage->bodyUnit\n", rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber);
                    continue;
                }
            }
            break;
        }
    }

    if(H_Modified != NULL_DIS && B_Modified != NULL_DIS) {
        ModifiedD = (H_Modified + B_Modified) / 2;
        ClassicD = (H_Classic + B_Classic) / 2;
        TrueD = (H_True + B_True) / 2;
    }
    else if(H_Modified != NULL_DIS) {
        ModifiedD = H_Modified;
        ClassicD = H_Classic;
        TrueD = H_True;
    }
    else if(B_Modified != NULL_DIS) {
        ModifiedD = B_Modified;
        ClassicD = B_Classic;
        TrueD = B_True;
    }

    if(ModifiedD != NULL_DIS) {
        #ifdef COMPENSATE_MODE
            float compensateD = (ModifiedD - lastD) * compensateRate;
            lastD = ModifiedD;
            DEBUG_PRINT("[current_%d]: ModifiedD = %f, ClassicD = %f, TrueD = %f, time = %lld\n", localHost->localAddress, ModifiedD + compensateD, ClassicD, TrueD, rangingMessageWithAdditionalInfo->RxTimestamp.full);
        #else
            DEBUG_PRINT("[current_%d]: ModifiedD = %f, ClassicD = %f, TrueD = %f, time = %lld\n", localHost->localAddress, ModifiedD, ClassicD, TrueD, rangingMessageWithAdditionalInfo->RxTimestamp.full);
        #endif
   }
    else {
        DEBUG_PRINT("[current]: No valid Current distance calculated\n");
    }

    #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
        if(ModifiedD != NULL_DIS) {
            return ModifiedD < SAFE_DISTANCE;
        }
    #endif

    return false;
}