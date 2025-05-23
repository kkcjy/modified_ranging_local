#include "modified_ranging.h"

Local_Host_t *localHost;                        // local host
RangingTableSet_t* rangingTableSet;             // local rangingTableSet
uint16_t localSendSeqNumber = 1;                // seqNumber of message local sent  
int rangingPeriod = RANGING_PERIOD;             // period of sending

void initRangingTableSet() {
    rangingTableSet = (RangingTableSet_t*)malloc(sizeof(RangingTableSet_t));
    rangingTableSet->counter = 0;
    rangingTableSet->topLocalSendBuffer = 0;
    for (table_index_t i = 0; i < TX_BUFFER_POOL_SIZE; i++) {
        rangingTableSet->localSendBuffer[i].timestamp.full = NULL_TIMESTAMP;
        rangingTableSet->localSendBuffer[i].seqNumber = NULL_SEQ;
    }
    for (int i = 0; i < TABLE_SET_NEIGHBOR_NUM; i++) {
        rangingTableSet->neighborIdxPriorityQueue[i] = NULL_INDEX;
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
        enableRangingTable(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->counter], address);
        index = rangingTableSet->counter++;
    }
    else {
        DEBUG_PRINT("[registerRangingTable]: RangingTableSet is full, cannot register new table\n");
        return NULL_INDEX;
    }
    // DEBUG_PRINT("[registerRangingTable]: register RangingTable successfully\n");
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
    // DEBUG_PRINT("[unregisterRangingTable]: unregister RangingTable successfully\n");
}

void addLocalSendBuffer(dwTime_t timestamp) {
    rangingTableSet->topLocalSendBuffer = (rangingTableSet->topLocalSendBuffer + 1) % TX_BUFFER_POOL_SIZE;
    // localSendSeqNumber has updated
    rangingTableSet->localSendBuffer[rangingTableSet->topLocalSendBuffer].seqNumber = localSendSeqNumber - 1;
    rangingTableSet->localSendBuffer[rangingTableSet->topLocalSendBuffer].timestamp = timestamp;
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

// find the index of LocalSendBufferNode by seq
table_index_t findLocalSendBufferNode(uint16_t seq) {
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (rangingTableSet->localSendBuffer[index].seqNumber != NULL_SEQ && count < TABLE_BUFFER_SIZE) {
        if(rangingTableSet->localSendBuffer[index].seqNumber == seq) {
            return index;
        }
        index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        count++;
    }
    return NULL_INDEX;
}

// get the index of neighbor(USING) in rangingTableSet with the oldest ranging record
int setPriorityIndex() {
    int topIndex = 0;
    for(int i = 0; i < rangingTableSet->counter; i++) {
        if(rangingTableSet->neighborReceiveBuffer[i].state == USING) {
            rangingTableSet->neighborIdxPriorityQueue[topIndex++] = i;
        }
    }

    for(int i = 1; i < topIndex; i++) {
        for(int j = i; j > 0 && compareTablePriority(&rangingTableSet->neighborReceiveBuffer[rangingTableSet->neighborIdxPriorityQueue[j]], 
            &rangingTableSet->neighborReceiveBuffer[rangingTableSet->neighborIdxPriorityQueue[j - 1]]); j--) {
                int temp = rangingTableSet->neighborIdxPriorityQueue[j];
                rangingTableSet->neighborIdxPriorityQueue[j] = rangingTableSet->neighborIdxPriorityQueue[j - 1];
                rangingTableSet->neighborIdxPriorityQueue[j - 1] = temp;
        }
    }
    return topIndex;
}

void printRangingMessage(Ranging_Message_t* rangingMessage) {
    DEBUG_PRINT("[Header]:\n");
    DEBUG_PRINT("srcAddress: %u\n", rangingMessage->header.srcAddress);
    DEBUG_PRINT("msgSequence: %u\n", rangingMessage->header.msgSequence);
    DEBUG_PRINT("msgLength: %u\n", rangingMessage->header.msgLength);
    DEBUG_PRINT("TxTimestamps:\n");
    for(int i = 0; i < MESSAGE_HEAD_TX_SIZE; i++) {
        DEBUG_PRINT("\tseqNumber: %u, timestamp: %llu\n",
        rangingMessage->header.TxTimestamps[i].seqNumber, rangingMessage->header.TxTimestamps[i].timestamp.full);
    }
    DEBUG_PRINT("[BodyUnits]:\n");
    for(int i = 0; i < MESSAGE_BODY_UNIT_SIZE; i++){
        DEBUG_PRINT("dest: %u\n", rangingMessage->bodyUnits[i].dest);
        DEBUG_PRINT("RxTimestamps:\n");
        for(int j = 0; j < MESSAGE_BODY_RX_SIZE; j++){
            DEBUG_PRINT("\tseqNumber: %u, timestamp: %llu\n",
            rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber, rangingMessage->bodyUnits[i].RxTimestamps[j].timestamp.full);
        }
    }
}

void printLocalSendBuffer() {
    DEBUG_PRINT("-----------------------------LOCALSENDBUFFER-----------------------------\n");
    table_index_t index = rangingTableSet->topLocalSendBuffer;
    int count = 0;
    while (count < TABLE_BUFFER_SIZE) {
        DEBUG_PRINT("seq: %d,Tx: %lld\n", 
            rangingTableSet->localSendBuffer[index].seqNumber, rangingTableSet->localSendBuffer[index].timestamp.full);

        index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        count++;
    }
    DEBUG_PRINT("-----------------------------LOCALSENDBUFFER-----------------------------\n");
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
    // (175 ~ 225)
    Time_t taskDelay = M2T(rangingPeriod + rand()%(RANGING_PERIOD_RAND_RANGE + 1) - RANGING_PERIOD_RAND_RANGE/2);

    int8_t bodyUnitCounter = 0;     // counter for valid bodyunits

    /* adjust
        select the neighbor with the oldest ranging record(modify compareTablePriority() to change the comparison rule)
    */
    int topIndex = setPriorityIndex();

    /* generate bodyUnits
        bodyUnit: dest(address of neighbor) + Rx(statuses during the last MESSAGE_BODY_RX_SIZE times of message reception)
    */
    while(bodyUnitCounter < MESSAGE_BODY_UNIT_SIZE && bodyUnitCounter < topIndex) {
        // push the message of rangingTable into bodyUnit
        RangingTable_t *rangingTable = &rangingTableSet->neighborReceiveBuffer[rangingTableSet->neighborIdxPriorityQueue[bodyUnitCounter]];
        Message_Body_Unit_t *bodyUnit = &rangingMessage->bodyUnits[bodyUnitCounter];
        // dest
        bodyUnit->dest = rangingTable->address;

        // Rx
        table_index_t index = rangingTable->receiveBuffer.head;
        for(int i = 0; i < MESSAGE_BODY_RX_SIZE; i++){
            // receiveBuffer --> bodyUnit->RxTimestamps
            if(index != NULL_INDEX) {
                bodyUnit->RxTimestamps[i].seqNumber = rangingTable->receiveBuffer.tableBuffer[index].localSeq;
                bodyUnit->RxTimestamps[i].timestamp = rangingTable->receiveBuffer.tableBuffer[index].RxTimestamp;
                index = rangingTable->receiveBuffer.tableBuffer[index].next;
            }
            // insufficient data, fill with null
            else{
                bodyUnit->RxTimestamps[i].seqNumber = NULL_SEQ;
                bodyUnit->RxTimestamps[i].timestamp.full = NULL_TIMESTAMP;
            }
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
            index = (index - 1 + TABLE_BUFFER_SIZE) % TABLE_BUFFER_SIZE;
        }
        // insufficient data, fill with null
        else{
            rangingMessage->header.TxTimestamps[i].seqNumber = NULL_SEQ;
            rangingMessage->header.TxTimestamps[i].timestamp.full = NULL_TIMESTAMP;
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

    Ranging_Message_t *rangingMessage = &rangingMessageWithAdditionalInfo->rangingMessage;

    // DEBUG_PRINT("\n*************************[processRangingMessage]*************************\n");
    // printRangingMessage(rangingMessage);
    // DEBUG_PRINT("*************************[processRangingMessage]*************************\n\n");

    /* process additionalinfo
        RxTimestamp + RxCoordinate  
    */
    // node: Tx <-- null, Rx <-- AdditionalInfo
    TableNode_t firstNode;
    firstNode.TxTimestamp = nullTimeStamp;
    firstNode.RxTimestamp = rangingMessageWithAdditionalInfo->RxTimestamp;
    firstNode.Tf = NULL_TOF;
    firstNode.localSeq = rangingMessage->header.msgSequence;
    firstNode.remoteSeq = NULL_SEQ;
    firstNode.pre = NULL_INDEX;
    firstNode.next = NULL_INDEX;

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

    RangingTable_t *neighborReceiveBuffer = &rangingTableSet->neighborReceiveBuffer[neighborIndex];

    // DEBUG_PRINT("[firstNode]: ");
    // printTableNode(&firstNode);

    // first time calling(localSeq): Tx - null Rx - full
    addTableLinkedList(&neighborReceiveBuffer->receiveBuffer, &firstNode);

    // find corresponding Rx to calculate
    for (int i = MESSAGE_HEAD_TX_SIZE - 1; i >= 0; i--) {
        if(rangingMessage->header.TxTimestamps[i].seqNumber == NULL_SEQ) {
            continue;
        }
        table_index_t receiveBufferIndex = findRemoteSeqIndex(&neighborReceiveBuffer->receiveBuffer, rangingMessage->header.TxTimestamps[i].seqNumber);
        if(receiveBufferIndex == NULL_DONE_INDEX) {
            // DEBUG_PRINT("Warning: have processed this node\n");
            continue;
        }
        // received
        if (receiveBufferIndex != NULL_INDEX) {
            TableNode_t secondNode;
            secondNode.TxTimestamp = rangingMessage->header.TxTimestamps[i].timestamp;
            secondNode.RxTimestamp = nullTimeStamp;
            secondNode.Tf = NULL_TOF;
            secondNode.localSeq = NULL_SEQ;
            secondNode.remoteSeq = rangingMessage->header.TxTimestamps[i].seqNumber;
            secondNode.pre = NULL_INDEX;
            secondNode.next = NULL_INDEX;

            // DEBUG_PRINT("[secondNode]: ");
            // printTableNode(&secondNode);

            // second time calling(remoteSeq): fill in Tx
            addTableLinkedList(&neighborReceiveBuffer->receiveBuffer, &secondNode);

            if(neighborReceiveBuffer->validBuffer.sendLength < MAX_INITIAL_CALCULATION){
                initializeRecordBuffer(&neighborReceiveBuffer->receiveBuffer, &neighborReceiveBuffer->sendBuffer, receiveBufferIndex, &neighborReceiveBuffer->validBuffer, RECEIVER);
                if(neighborReceiveBuffer->validBuffer.sendLength >= MAX_INITIAL_CALCULATION){
                    // finish calling initializeRecordBuffer, update Tof
                    int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                    for (int i = 0; i < neighborReceiveBuffer->validBuffer.sendLength; i++) {
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].sumTof = initTof*2;
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].T1 = initTof;
                        neighborReceiveBuffer->validBuffer.sendBuffer[i].T2 = initTof;
                    }
                    // DEBUG_PRINT("[initializeRecordBuffer]: finish calling, initTof = %lld\n",initTof);
                }
            }
            else{
                // from old data to recent data, get the last valid D
                calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->receiveBuffer.tableBuffer[receiveBufferIndex],
                    neighborReceiveBuffer->receiveBuffer.tableBuffer[receiveBufferIndex].localSeq, RECEIVER, FIRST_CALCULATE, &H_Modified, &H_Classic, &H_True);
            }
        }
        // missed
        else{
            DEBUG_PRINT("Warning: Cannot find corresponding Rx timestamp for Tx[%d] timestamp while processing rangingMessage->header\n", rangingMessage->header.TxTimestamps[i].seqNumber);
        }
    }

    // if(H_Modified != NULL_DIS) {
    //     printf("[Recent]: H_Modified = %f, H_Classic = %f, H_True = %f\n", H_Modified, H_Classic, H_True);
    // }
    // else {
    //     printf("[Recent]: No valid Modified H_Modified calculated\n");
    // }

    /* process bodyUnit
        dest + Rx
    */
    for (int i = 0; i < MESSAGE_BODY_UNIT_SIZE; i++) {
        if (rangingMessage->bodyUnits[i].dest == localHost->localAddress) {
            // RxTimestamp with larger seqNumber come first when generating message
            for (int j = MESSAGE_BODY_RX_SIZE - 1; j >= 0 ; j--) {
                if(rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber == NULL_SEQ) {
                    continue;
                }

                table_index_t sendBufferIndex = findLocalSendBufferNode(rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber);
                if (sendBufferIndex == NULL_INDEX) {
                    DEBUG_PRINT("Warning: Cannot find corresponding Tx timestamp for Rx[%d] timestamp while processing rangingMessage->bodyUnit\n", rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber);
                    continue;
                }

                if(findRemoteSeqIndex(&neighborReceiveBuffer->sendBuffer, rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber) == NULL_DONE_INDEX) {
                    // DEBUG_PRINT("Warning: have processed this node\n");
                    continue;
                }

                TableNode_t fullNode;
                fullNode.TxTimestamp = rangingTableSet->localSendBuffer[sendBufferIndex].timestamp;
                fullNode.RxTimestamp = rangingMessage->bodyUnits[i].RxTimestamps[j].timestamp;
                fullNode.Tf = NULL_TOF;
                fullNode.localSeq = rangingTableSet->localSendBuffer[sendBufferIndex].seqNumber;
                fullNode.remoteSeq = rangingMessage->bodyUnits[i].RxTimestamps[j].seqNumber;

                // DEBUG_PRINT("[fullNode]: ");
                // printTableNode(&fullNode);

                // Tx - null Rx - full
                table_index_t newReceiveRecordIndex_BodyUnit = addTableLinkedList(&neighborReceiveBuffer->sendBuffer, &fullNode);

                if(neighborReceiveBuffer->validBuffer.receiveLength < MAX_INITIAL_CALCULATION){
                    initializeRecordBuffer(&neighborReceiveBuffer->sendBuffer, &neighborReceiveBuffer->receiveBuffer, newReceiveRecordIndex_BodyUnit, &neighborReceiveBuffer->validBuffer, SENDER);
                    if(neighborReceiveBuffer->validBuffer.receiveLength == MAX_INITIAL_CALCULATION){
                        int64_t initTof = getInitTofSum() / MAX_INITIAL_CALCULATION;
                        for (int i = 0; i < neighborReceiveBuffer->validBuffer.receiveLength; i++) {
                            neighborReceiveBuffer->validBuffer.receiveBuffer[i].sumTof = initTof*2;
                            neighborReceiveBuffer->validBuffer.receiveBuffer[i].T1 = initTof;
                            neighborReceiveBuffer->validBuffer.receiveBuffer[i].T2 = initTof;
                        }
                        // DEBUG_PRINT("[initializeRecordBuffer]: finish calling, initTof = %lld\n",initTof);
                    }
                }
                else {
                    calculateTof(&neighborReceiveBuffer->validBuffer, &neighborReceiveBuffer->sendBuffer.tableBuffer[newReceiveRecordIndex_BodyUnit],
                        neighborReceiveBuffer->sendBuffer.tableBuffer[newReceiveRecordIndex_BodyUnit].localSeq, SENDER, FIRST_CALCULATE, &B_Modified, &B_Classic, &B_True);
                }
            }
            break;
        }
    }

    // if(B_Modified != NULL_DIS) {
    //     printf("[Recent]: B_Modified = %f, B_Classic = %f, B_True = %f\n", B_Modified, B_Classic, B_True);
    // }
    // else {
    //     printf("[Recent]: No valid Modified B_Modified calculated\n");
    // }

    if(H_Modified != NULL_DIS && B_Modified != NULL_DIS) {
        ModifiedD = (H_Modified + B_Modified) / 2;
        ClassicD = (H_Classic + B_Classic) / 2;
    }
    else if(H_Modified != NULL_DIS) {
        ModifiedD = H_Modified;
        ClassicD = H_Classic;
    }
    else if(B_Modified != NULL_DIS) {
        ModifiedD = B_Modified;
        ClassicD = B_Classic;
    }

    if(ModifiedD != NULL_DIS) {
        DEBUG_PRINT("[current-%d-%d]: ModifiedD = %f, ClassicD = %f, time = %lld\n", localHost->localAddress, neighborAddress, ModifiedD, ClassicD, rangingMessageWithAdditionalInfo->RxTimestamp.full);
    }
    else {
        DEBUG_PRINT("[current]: No valid Current distance calculated\n");
    }

    return false;
}