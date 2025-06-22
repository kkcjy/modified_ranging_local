#include "ranging_buffer.h"


static int64_t initTofSum;

int64_t getInitTofSum() {
    return initTofSum;
}

void initRangingBufferNode(RangingBufferNode_t *bufferNode) {
    bufferNode->sendTxTimestamp = nullTimeStamp;
    bufferNode->sendRxTimestamp = nullTimeStamp;
    bufferNode->receiveTxTimestamp = nullTimeStamp;
    bufferNode->receiveRxTimestamp = nullTimeStamp;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        bufferNode->sendTxCoordinate = nullCoordinate;
        bufferNode->sendRxCoordinate = nullCoordinate;
        bufferNode->receiveTxCoordinate = nullCoordinate;
        bufferNode->receiveRxCoordinate = nullCoordinate;
    #endif
    bufferNode->sumTof = NULL_TOF;
    bufferNode->TxSeq = NULL_SEQ;
    bufferNode->RxSeq = NULL_SEQ;
}

void initRangingBuffer(RangingBuffer_t *buffer) {
    buffer->topSendBuffer = NULL_INDEX;
    buffer->topReceiveBuffer = NULL_INDEX;
    buffer->receiveLength = 0;
    buffer->sendLength = 0;
    for (int i = 0; i < RANGING_BUFFER_SIZE; i++) {
        initRangingBufferNode(&buffer->sendBuffer[i]);
        initRangingBufferNode(&buffer->receiveBuffer[i]);
    }
    initTofSum = 0;
}

// add RangingBufferNode_t to RangingBuffer_t(send/receive) in validBuffer
void addRangingBuffer(RangingBuffer_t *buffer, RangingBufferNode_t *bufferNode, StatusType status) {
    if (status == SENDER) {
        buffer->topSendBuffer = (buffer->topSendBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->sendLength = buffer->sendLength < RANGING_BUFFER_SIZE ? buffer->sendLength + 1 : RANGING_BUFFER_SIZE;
        buffer->sendBuffer[buffer->topSendBuffer].sendTxTimestamp = bufferNode->sendTxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].sendRxTimestamp = bufferNode->sendRxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].receiveTxTimestamp = bufferNode->receiveTxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].receiveRxTimestamp = bufferNode->receiveRxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].sumTof = bufferNode->sumTof;
        buffer->sendBuffer[buffer->topSendBuffer].TxSeq = bufferNode->TxSeq;
        buffer->sendBuffer[buffer->topSendBuffer].RxSeq = bufferNode->RxSeq;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            buffer->sendBuffer[buffer->topSendBuffer].sendTxCoordinate = bufferNode->sendTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].sendRxCoordinate = bufferNode->sendRxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveTxCoordinate = bufferNode->receiveTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveRxCoordinate = bufferNode->receiveRxCoordinate;
        #endif
    }

    else if (status == RECEIVER) {
        buffer->topReceiveBuffer = (buffer->topReceiveBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->receiveLength = buffer->receiveLength < RANGING_BUFFER_SIZE ? buffer->receiveLength + 1 : RANGING_BUFFER_SIZE;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendTxTimestamp = bufferNode->sendTxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendRxTimestamp = bufferNode->sendRxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTxTimestamp = bufferNode->receiveTxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRxTimestamp = bufferNode->receiveRxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendTxCoordinate = bufferNode->sendTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendRxCoordinate = bufferNode->sendRxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTxCoordinate = bufferNode->receiveTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRxCoordinate = bufferNode->receiveRxCoordinate;
        #endif
        buffer->receiveBuffer[buffer->topReceiveBuffer].sumTof = bufferNode->sumTof;
        buffer->receiveBuffer[buffer->topReceiveBuffer].TxSeq = bufferNode->TxSeq;
        buffer->receiveBuffer[buffer->topReceiveBuffer].RxSeq = bufferNode->RxSeq;
    }
}

/* select index from RangingBuffer_t
    SENDER:     select the index closest to localSeq from receiveBuffer
    RECEIVER:   select the index closest to localSeq from sendBuffer
*/
table_index_t searchRangingBuffer(RangingBuffer_t *buffer, uint16_t seq, StatusType status) {
    table_index_t index = NULL_INDEX;
    if(status == SENDER) {
        table_index_t i = buffer->topReceiveBuffer;
        for (int j = 0; j < buffer->receiveLength; j++) {
            if(buffer->receiveBuffer[i].TxSeq < seq) {
                if(index == NULL_INDEX || buffer->receiveBuffer[i].TxSeq > buffer->receiveBuffer[index].TxSeq) {
                    index = i;
                }
            }
            i = (i - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
        }
    }
    else if(status == RECEIVER) {
        table_index_t i = buffer->topSendBuffer;
        for (int j = 0; j < buffer->sendLength; j++) {
            if(buffer->sendBuffer[i].RxSeq < seq) {
                if(index == NULL_INDEX || buffer->sendBuffer[i].RxSeq > buffer->sendBuffer[index].RxSeq) {
                    index = i;
                }
            }
            i = (i - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
        }
    }
    return index;
}

/* calculateTof and store valid calculation record
    FIRST_CALCULATE                 calculate Tof with the most recent valid record
    SECOND_CALCULATE_UNQUALIFIED    recalculate Tof with the next most recent valid record
    SECOND_CALCULATE_ABNORMAL       recalculate Tof with the next most recent valid record
*/
double calculateTof(RangingBuffer_t *buffer, RangingListNode_t* listNode, uint16_t checkLocalSeq, StatusType status, CALCULATE_FLAG flag, float *Modified, float *Classic, float *True) {
    dwTime_t Tx = listNode->TxTimestamp;
    dwTime_t Rx = listNode->RxTimestamp;
    uint16_t localSeq = listNode->localSeq;

    RangingBufferNode_t* bufferNode = NULL;
    
    table_index_t index = searchRangingBuffer(buffer, checkLocalSeq, status);
    if(index == NULL_INDEX){
        DEBUG_PRINT("Warning: Cannot find the record with localSeq:%d\n",checkLocalSeq);
        return -1;
    }
    if(status == SENDER){
        bufferNode = &buffer->receiveBuffer[index];
    }
    else if(status == RECEIVER){
        bufferNode = &buffer->sendBuffer[index];
    }

    int64_t Ra = 0, Rb = 0, Da = 0, Db = 0;
    /* SENDER
            sRx     <--Db-->    rTx         <--Rb-->        Rx

        sTx         <--Ra-->        rRx     <--Da-->    Tx
    */
    if(status == SENDER){
        Ra = (bufferNode->receiveRxTimestamp.full - bufferNode->sendTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (bufferNode->receiveTxTimestamp.full - bufferNode->sendRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - bufferNode->receiveTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - bufferNode->receiveRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
    */
    else if(status == RECEIVER){
        Ra = (bufferNode->sendRxTimestamp.full - bufferNode->receiveTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (bufferNode->sendTxTimestamp.full - bufferNode->receiveRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - bufferNode->sendTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - bufferNode->sendRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    
    int64_t T12 = bufferNode->sumTof;
    int64_t T23 = 0;
    float Ra_Da = (float)Ra/Da;
    float Rb_Db = (float)Rb/Db;
    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    // DEBUG_PRINT("[CalculateTof]: Ra_Da:%f,Rb_Db:%f\n", Ra_Da, Rb_Db);

    if(Ra_Da < CONVERGENCE_THRESHOLD || Rb_Db < CONVERGENCE_THRESHOLD) {
        if(Ra_Da < Rb_Db) {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / 2 - Ra * T12) / Da;
        }
        else {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / 2 - Rb * T12) / Db;
        }
    }

    // fail to satisfy the convergence requirements
    else {
        // DEBUG_PRINT("Warning: Ra/Da and Rb/Db are both greater than CONVERGENCE_THRESHOLD(%d), Ra_Da = %f, Rb_Db = %f\n",CONVERGENCE_THRESHOLD, Ra_Da, Rb_Db);
        #ifdef CLASSIC_TOF_ENABLE
            // calculate T23 with the latest data in classic method
            T23 = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (Ra + Db + Rb + Da);
        #else
            // calculate T23 with the data of the time before latest in modified method
            if(flag == FIRST_CALCULATE) {
                // DEBUG_PRINT("Warning: The latest record in rangingbuffer fails, and an attempt is made to recalculate the Tof using the next most recent valid record\n");
                if(status == SENDER) {
                    return calculateTof(buffer, listNode, bufferNode->TxSeq, status, SECOND_CALCULATE_UNQUALIFIED, Modified, Classic, True);
                }
                else if(status == RECEIVER) {
                    return calculateTof(buffer, listNode, bufferNode->RxSeq, status, SECOND_CALCULATE_UNQUALIFIED, Modified, Classic, True);
                }
            }
            else {
                return -1;
            }
        #endif
    }

    // abnormal result
    float Tof = T23 / 2;
    float D = Tof * VELOCITY;
    if(D < 0 || D > 1000){
        // DEBUG_PRINT("Warning: D = %f is out of range(0,1000)\n",D);
        if(flag == FIRST_CALCULATE){
            // DEBUG_PRINT("Warning: The latest record in rangingbuffer fails, and an attempt is made to recalculate the Tof using the next most recent valid record\n");
            if(status == SENDER) {
                return calculateTof(buffer, listNode, bufferNode->TxSeq, status, SECOND_CALCULATE_ABNORMAL, Modified, Classic, True);
            }
            else if(status == RECEIVER) {
                return calculateTof(buffer, listNode, bufferNode->RxSeq, status, SECOND_CALCULATE_ABNORMAL, Modified, Classic, True);
            }
        }
        else{
            return -1;
        }
    }

    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2 * sumAB);
    float classicD = classicTof * VELOCITY;

    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        float trueDx = (listNode->RxCoordinate.x - listNode->TxCoordinate.x);
        float trueDy = (listNode->RxCoordinate.y - listNode->TxCoordinate.y);
        float trueDz = (listNode->RxCoordinate.z - listNode->TxCoordinate.z);
        float trueD = sqrtf(trueDx*trueDx + trueDy*trueDy + trueDz*trueDz) / 1000;
        // DEBUG_PRINT("[CalculateTof%s%s]: modified_D = %f, classic_D = %f, true_D = %f\n", status == 0 ? "_SENDER" : "_RECEIVER", flag == FIRST_CALCULATE ? "_FIRST" : "_SECOND", D, classicD, trueD);
    #endif

    /* update ranging_buffer
        FIRST_CALCULATE                 update the most recent valid record
        SECOND_CALCULATE_UNQUALIFIED    update the most recent valid record
        SECOND_CALCULATE_ABNORMAL       update the next most recent valid record
    */
    RangingBufferNode_t newBufferNode;
    /* SENDER
            sRx     <--Db-->    rTx         <--Rb-->        Rx
                               
        sTx         <--Ra-->        rRx     <--Da-->    Tx
                                ------------------------------ stored
        
    */
    if(status == SENDER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        newBufferNode.sendTxTimestamp = Tx;
        newBufferNode.sendRxTimestamp = Rx;
        newBufferNode.TxSeq = listNode->localSeq;
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newBufferNode.receiveTxTimestamp = buffer->receiveBuffer[last].receiveTxTimestamp;
            newBufferNode.receiveRxTimestamp = buffer->receiveBuffer[last].receiveRxTimestamp;
            newBufferNode.RxSeq = buffer->receiveBuffer[last].RxSeq;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newBufferNode.receiveTxTimestamp = bufferNode->receiveTxTimestamp;
            newBufferNode.receiveRxTimestamp = bufferNode->receiveRxTimestamp;
            newBufferNode.RxSeq = bufferNode->RxSeq;
        }
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newBufferNode.sendTxCoordinate = listNode->TxCoordinate;
            newBufferNode.sendRxCoordinate = listNode->RxCoordinate;
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newBufferNode.receiveRxCoordinate = buffer->receiveBuffer[last].receiveRxCoordinate;
                newBufferNode.receiveTxCoordinate = buffer->receiveBuffer[last].receiveTxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newBufferNode.receiveRxCoordinate = bufferNode->receiveRxCoordinate;
                newBufferNode.receiveTxCoordinate = bufferNode->receiveTxCoordinate;
            }
        #endif
        newBufferNode.sumTof = T23;

        addRangingBuffer(buffer, &newBufferNode, status);
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
                                ------------------------------ stored
    */
    else if(status == RECEIVER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newBufferNode.sendTxTimestamp = buffer->sendBuffer[last].sendTxTimestamp;
            newBufferNode.sendRxTimestamp = buffer->sendBuffer[last].sendRxTimestamp;
            newBufferNode.TxSeq = buffer->sendBuffer[last].TxSeq;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newBufferNode.sendTxTimestamp = bufferNode->sendTxTimestamp;
            newBufferNode.sendRxTimestamp = bufferNode->sendRxTimestamp;
            newBufferNode.TxSeq = bufferNode->TxSeq;
        }
        newBufferNode.receiveTxTimestamp = Tx;
        newBufferNode.receiveRxTimestamp = Rx;
        newBufferNode.RxSeq = listNode->localSeq;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newBufferNode.sendTxCoordinate = buffer->sendBuffer[last].sendTxCoordinate;
                newBufferNode.sendRxCoordinate = buffer->sendBuffer[last].sendRxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newBufferNode.sendTxCoordinate = bufferNode->sendTxCoordinate;
                newBufferNode.sendRxCoordinate = bufferNode->sendRxCoordinate;
            }
            newBufferNode.receiveRxCoordinate = listNode->RxCoordinate;
            newBufferNode.receiveTxCoordinate = listNode->TxCoordinate;
        #endif
        newBufferNode.sumTof = T23;

        addRangingBuffer(buffer, &newBufferNode, status);
    }

    *Modified = D;
    *Classic = classicD;
    *True = trueD;

    return D;
}

void initializeCalculateTof(RangingList_t *listA, RangingList_t *listB, table_index_t indexA, RangingBuffer_t* rangingBuffer, StatusType status) {
    // fetch data successively from listA, listB, and listA 
    table_index_t indexA1 = indexA;
    if (indexA1 == NULL_INDEX || listA->rangingList[indexA1].TxTimestamp.full == NULL_TIMESTAMP || listA->rangingList[indexA1].RxTimestamp.full == NULL_TIMESTAMP || listA->rangingList[indexA1].Tf != NULL_TOF) {
        DEBUG_PRINT("Warning: The lastest record in listA is invalid or the record has owned Tof\n");
        return;
    }
    table_index_t indexB2 = searchRangingList(listB, listA->rangingList[indexA1].RxTimestamp, status);
    if (indexB2 == NULL_INDEX) {
        DEBUG_PRINT("No valid record in listB\n");
        return;
    }
    table_index_t indexA3 = searchRangingList(listA, listB->rangingList[indexB2].TxTimestamp, status ^ 1);
    if (indexA3 == NULL_INDEX) {
        DEBUG_PRINT("No valid record in listA\n");
        return;
    }

    /*
        A3.Tx          <--Ra-->          B2.Rx      <--Da-->      A1.Tx

                A3.Rx  <--Db-->  B2.Tx              <--Rb-->              A1.Rx
    */

    int64_t Ra = (listB->rangingList[indexB2].RxTimestamp.full - listA->rangingList[indexA3].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Db = (listB->rangingList[indexB2].TxTimestamp.full - listA->rangingList[indexA3].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Rb = (listA->rangingList[indexA1].RxTimestamp.full - listB->rangingList[indexB2].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Da = (listA->rangingList[indexA1].TxTimestamp.full - listB->rangingList[indexB2].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;

    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2 * sumAB);

    if(classicTof < 0){
        DEBUG_PRINT("[initializeCalculateTof]: Tof is less than 0\n");
        return;
    }
    DEBUG_PRINT("[initializeCalculateTof]: Tof = %lld\n",classicTof);

    RangingBufferNode_t SenderNode,ReceiverNode;

    // update ranging_buffer
    if(status == SENDER){
        /*
                    A3.Rx   B2.Tx                   A1.Rx

            A3.Tx                   B2.Rx   A1.Tx
        */  
        SenderNode.sendTxTimestamp = listA->rangingList[indexA3].TxTimestamp;
        SenderNode.sendRxTimestamp = listA->rangingList[indexA3].RxTimestamp;
        SenderNode.receiveTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        SenderNode.receiveRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            SenderNode.sendTxCoordinate = listA->rangingList[indexA3].TxCoordinate;
            SenderNode.sendRxCoordinate = listA->rangingList[indexA3].RxCoordinate;
            SenderNode.receiveTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            SenderNode.receiveRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
        #endif
        SenderNode.sumTof = classicTof * 2;
        SenderNode.TxSeq = listA->rangingList[indexA3].localSeq;
        SenderNode.RxSeq = listB->rangingList[indexB2].localSeq;
        addRangingBuffer(rangingBuffer, &SenderNode, RECEIVER);

        ReceiverNode.sendTxTimestamp = listA->rangingList[indexA1].TxTimestamp;
        ReceiverNode.sendRxTimestamp = listA->rangingList[indexA1].RxTimestamp;
        ReceiverNode.receiveTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        ReceiverNode.receiveRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            ReceiverNode.sendTxCoordinate = listA->rangingList[indexA1].TxCoordinate;
            ReceiverNode.sendRxCoordinate = listA->rangingList[indexA1].RxCoordinate;
            ReceiverNode.receiveTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            ReceiverNode.receiveRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
        #endif
        ReceiverNode.sumTof = classicTof * 2;
        ReceiverNode.TxSeq = listA->rangingList[indexA1].localSeq;
        ReceiverNode.RxSeq = listB->rangingList[indexB2].localSeq;
        addRangingBuffer(rangingBuffer, &ReceiverNode, SENDER);
    }
    else if(status == RECEIVER){
        /*
            A3.Tx                   B2.Rx   A1.Tx

                    A3.Rx   B2.Tx                   A1.Rx
        */
        SenderNode.sendTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        SenderNode.sendRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        SenderNode.receiveTxTimestamp = listA->rangingList[indexA3].TxTimestamp;
        SenderNode.receiveRxTimestamp = listA->rangingList[indexA3].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            SenderNode.sendTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            SenderNode.sendRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
            SenderNode.receiveTxCoordinate = listA->rangingList[indexA3].TxCoordinate;
            SenderNode.receiveRxCoordinate = listA->rangingList[indexA3].RxCoordinate;
        #endif
        SenderNode.sumTof = classicTof * 2;
        SenderNode.TxSeq = listB->rangingList[indexB2].localSeq;
        SenderNode.RxSeq = listA->rangingList[indexA3].localSeq;
        addRangingBuffer(rangingBuffer, &SenderNode, SENDER);

        ReceiverNode.sendTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        ReceiverNode.sendRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        ReceiverNode.receiveTxTimestamp = listA->rangingList[indexA1].TxTimestamp;
        ReceiverNode.receiveRxTimestamp = listA->rangingList[indexA1].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            ReceiverNode.sendTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            ReceiverNode.sendRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
            ReceiverNode.receiveTxCoordinate = listA->rangingList[indexA1].TxCoordinate;
            ReceiverNode.receiveRxCoordinate = listA->rangingList[indexA1].RxCoordinate;
        #endif
        ReceiverNode.sumTof = classicTof * 2;
        ReceiverNode.TxSeq = listB->rangingList[indexB2].localSeq;
        ReceiverNode.RxSeq = listA->rangingList[indexA1].localSeq;
        addRangingBuffer(rangingBuffer, &ReceiverNode, RECEIVER);
    }
    initTofSum += classicTof;
}

void printRangingBuffer(RangingBuffer_t *buffer) {
    DEBUG_PRINT("[ValidBuffer]:\n");
    int index = buffer->topSendBuffer;
    for(int bound = 0; bound < RANGING_BUFFER_SIZE; bound++){
        if(index == NULL_INDEX){
            break;
        }
        DEBUG_PRINT("SendBuffer[%d]: receiveTxTimestamp: %llu,receiveRxTimestamp: %llu,sendTxTimestamp: %llu,sendRxTimestamp: %llu,sumTof: %lld",
            index,buffer->sendBuffer[index].receiveTxTimestamp.full,buffer->sendBuffer[index].receiveRxTimestamp.full,
            buffer->sendBuffer[index].sendTxTimestamp.full,buffer->sendBuffer[index].sendRxTimestamp.full,
            buffer->sendBuffer[index].sumTof
        );
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(",receiveCoordinate: (%d,%d,%d),sendCoordinate: (%d,%d,%d)\n",
                buffer->sendBuffer[index].receiveRxCoordinate.x,buffer->sendBuffer[index].receiveRxCoordinate.y,buffer->sendBuffer[index].receiveRxCoordinate.z,
                buffer->sendBuffer[index].sendRxCoordinate.x,buffer->sendBuffer[index].sendRxCoordinate.y,buffer->sendBuffer[index].sendRxCoordinate.z);
        #else
                DEBUG_PRINT("\n");
        #endif
        index = (index - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
    }

    index = buffer->topReceiveBuffer;
    for (int bound = 0; bound < RANGING_BUFFER_SIZE; bound++) {
        if(index == NULL_INDEX) {
            break;
        }
        DEBUG_PRINT("ReceiveBuffer[%d]: sendTxTimestamp: %llu,sendRxTimestamp: %llu,receiveTxTimestamp: %llu,receiveRxTimestamp: %llu,sumTof: %lld",
            index,buffer->receiveBuffer[index].sendTxTimestamp.full,buffer->receiveBuffer[index].sendRxTimestamp.full,
            buffer->receiveBuffer[index].receiveTxTimestamp.full,buffer->receiveBuffer[index].receiveRxTimestamp.full,
            buffer->receiveBuffer[index].sumTof);
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(",sendCoordinate: (%d,%d,%d),receiveCoordinate: (%d,%d,%d)\n",
                buffer->receiveBuffer[index].sendRxCoordinate.x,buffer->receiveBuffer[index].sendRxCoordinate.y,buffer->receiveBuffer[index].sendRxCoordinate.z,
                buffer->receiveBuffer[index].receiveRxCoordinate.x,buffer->receiveBuffer[index].receiveRxCoordinate.y,buffer->receiveBuffer[index].receiveRxCoordinate.z);
        #else
            DEBUG_PRINT("\n");
        #endif
        index = (index - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
    }
}