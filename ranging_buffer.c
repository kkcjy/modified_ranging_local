#include "ranging_buffer.h"


static int64_t initTofSum;

int64_t getInitTofSum() {
    return initTofSum;
}

void initRangingBufferNode(RangingBufferNode_t *node) {
    node->sendTxTimestamp = nullTimeStamp;
    node->sendRxTimestamp = nullTimeStamp;
    node->receiveTxTimestamp = nullTimeStamp;
    node->receiveRxTimestamp = nullTimeStamp;
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        node->sendTxCoordinate = nullCoordinate;
        node->sendRxCoordinate = nullCoordinate;
        node->receiveTxCoordinate = nullCoordinate;
        node->receiveRxCoordinate = nullCoordinate;
    #endif
    node->sumTof = NULL_TOF;
    node->TxSeq = NULL_SEQ;
    node->RxSeq = NULL_SEQ;
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
void addRangingBuffer(RangingBuffer_t *buffer, RangingBufferNode_t *node, StatusType status) {
    if (status == SENDER) {
        buffer->topSendBuffer = (buffer->topSendBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->sendLength = buffer->sendLength < RANGING_BUFFER_SIZE ? buffer->sendLength + 1 : RANGING_BUFFER_SIZE;
        buffer->sendBuffer[buffer->topSendBuffer].sendTxTimestamp = node->sendTxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].sendRxTimestamp = node->sendRxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].receiveTxTimestamp = node->receiveTxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].receiveRxTimestamp = node->receiveRxTimestamp;
        buffer->sendBuffer[buffer->topSendBuffer].sumTof = node->sumTof;
        buffer->sendBuffer[buffer->topSendBuffer].TxSeq = node->TxSeq;
        buffer->sendBuffer[buffer->topSendBuffer].RxSeq = node->RxSeq;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            buffer->sendBuffer[buffer->topSendBuffer].sendTxCoordinate = node->sendTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].sendRxCoordinate = node->sendRxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveTxCoordinate = node->receiveTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveRxCoordinate = node->receiveRxCoordinate;
        #endif

        // DEBUG_PRINT("[SENDBUFFER_ADD]: sendTxTimestamp:%llu,sendRxTimestamp:%llu,receiveTxTimestamp:%llu,receiveRxTimestamp:%llu,TxSeq:%d,RxSeq:%d\n"
        //     ,node->sendTxTimestamp.full,node->sendRxTimestamp.full,node->receiveTxTimestamp.full,node->receiveRxTimestamp.full,node->TxSeq,node->RxSeq);
    }

    else if (status == RECEIVER) {
        buffer->topReceiveBuffer = (buffer->topReceiveBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->receiveLength = buffer->receiveLength < RANGING_BUFFER_SIZE ? buffer->receiveLength + 1 : RANGING_BUFFER_SIZE;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendTxTimestamp = node->sendTxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendRxTimestamp = node->sendRxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTxTimestamp = node->receiveTxTimestamp;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRxTimestamp = node->receiveRxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendTxCoordinate = node->sendTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendRxCoordinate = node->sendRxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTxCoordinate = node->receiveTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRxCoordinate = node->receiveRxCoordinate;
        #endif
        buffer->receiveBuffer[buffer->topReceiveBuffer].sumTof = node->sumTof;
        buffer->receiveBuffer[buffer->topReceiveBuffer].TxSeq = node->TxSeq;
        buffer->receiveBuffer[buffer->topReceiveBuffer].RxSeq = node->RxSeq;

        // DEBUG_PRINT("[RECEIVEBUFFER_ADD]: sendTxTimestamp:%llu,sendRxTimestamp:%llu,receiveTxTimestamp:%llu,receiveRxTimestamp:%llu,TxSeq:%d,RxSeq:%d\n"
        //     ,node->sendTxTimestamp.full,node->sendRxTimestamp.full,node->receiveTxTimestamp.full,node->receiveRxTimestamp.full,node->TxSeq,node->RxSeq);
    }
}

/* select index from RangingBuffer_t(send/receive)
SENDER
select the index closest to localSeq from receiveBuffer
RECEIVER
select the index closest to localSeq from sendBuffer
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
    CALCULATE_FLAG = FIRST_CALCULATE,
        calculate the Tof using the most recent valid record
    CALCULATE_FLAG = SECOND_CALCULATE_UNQUALIFIED,
        recalculate the Tof using the next most recent valid record
    FLAS = SECOND_CALCULATE_ABNORMAL
        recalculate the Tof using the next most recent valid record
*/
double calculateTof(RangingBuffer_t *buffer, RangingListNode_t* rangingListNode, uint16_t checkLocalSeq, StatusType status, CALCULATE_FLAG flag, float *Modified, float *Classic, float *True) {
    dwTime_t Tx = rangingListNode->TxTimestamp;
    dwTime_t Rx = rangingListNode->RxTimestamp;
    uint16_t localSeq = rangingListNode->localSeq;

    RangingBufferNode_t* node = NULL;
    
    table_index_t index = searchRangingBuffer(buffer, checkLocalSeq, status);
    if(index == NULL_INDEX){
        DEBUG_PRINT("Warning: Cannot find the record with localSeq:%d\n",checkLocalSeq);
        return -1;
    }
    if(status == SENDER){
        node = &buffer->receiveBuffer[index];
    }
    else if(status == RECEIVER){
        node = &buffer->sendBuffer[index];
    }
    int64_t Ra=0,Rb=0,Da=0,Db=0;
    /* SENDER
            sRx     <--Db-->    rTx         <--Rb-->        Rx

        sTx         <--Ra-->        rRx     <--Da-->    Tx
    */
    if(status == SENDER){
        Ra = (node->receiveRxTimestamp.full - node->sendTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (node->receiveTxTimestamp.full - node->sendRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - node->receiveTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - node->receiveRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
    */
    else if(status == RECEIVER){
        Ra = (node->sendRxTimestamp.full - node->receiveTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (node->sendTxTimestamp.full - node->receiveRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - node->sendTxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - node->sendRxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    int64_t T12 = node->sumTof;
    int64_t T23 = 0;
    // DEBUG_PRINT("[CalculateTof]: status:%d,T12:%lld,Ra:%lld,Rb:%lld,Da:%lld,Db:%lld\n", status,T12,Ra, Rb, Da, Db);
    float Ra_Da = (float)Ra/Da;
    float Rb_Db = (float)Rb/Db;
    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    // DEBUG_PRINT("[CalculateTof]: Ra_Da:%f,Rb_Db:%f\n", Ra_Da, Rb_Db);

    if(Ra_Da < CONVERGENCE_THRESHOLD || Rb_Db < CONVERGENCE_THRESHOLD) {
        if(Ra_Da < Rb_Db) {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da)/2 - Ra*T12)/Da;
        }
        else {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da)/2 - Rb*T12)/Db;
        }
    }

    // fail to satisfy the convergence requirements
    else {
        #ifdef CLASSIC_TOF_ENABLE
            // calculate T23 with the latest data in classic method
            T23 = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (Ra + Db + Rb + Da);
        #else
            // calculate T23 with the data of the time before latest in modified method
            // DEBUG_PRINT("Warning: Ra/Da and Rb/Db are both greater than CONVERGENCE_THRESHOLD(%d), Ra_Da = %f, Rb_Db = %f\n",CONVERGENCE_THRESHOLD, Ra_Da, Rb_Db);
            if(flag == FIRST_CALCULATE) {
                // DEBUG_PRINT("Warning: The latest record in rangingbuffer fails, and an attempt is made to recalculate the Tof using the next most recent valid record\n");
                if(status == SENDER) {
                    return calculateTof(buffer, rangingListNode, node->TxSeq, status, SECOND_CALCULATE_UNQUALIFIED, Modified, Classic, True);
                }
                else if(status == RECEIVER) {
                    return calculateTof(buffer, rangingListNode, node->RxSeq, status, SECOND_CALCULATE_UNQUALIFIED, Modified, Classic, True);
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
                return calculateTof(buffer, rangingListNode, node->TxSeq, status, SECOND_CALCULATE_ABNORMAL, Modified, Classic, True);
            }
            else if(status == RECEIVER) {
                return calculateTof(buffer, rangingListNode, node->RxSeq, status, SECOND_CALCULATE_ABNORMAL, Modified, Classic, True);
            }
        }
        else{
            return -1;
        }
    }

    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2*sumAB);
    float classicD = classicTof * VELOCITY;

    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        float trueDx = (rangingListNode->RxCoordinate.x - rangingListNode->TxCoordinate.x);
        float trueDy = (rangingListNode->RxCoordinate.y - rangingListNode->TxCoordinate.y);
        float trueDz = (rangingListNode->RxCoordinate.z - rangingListNode->TxCoordinate.z);
        float trueD = sqrtf(trueDx*trueDx + trueDy*trueDy + trueDz*trueDz) / 1000;
        // DEBUG_PRINT("[CalculateTof%s%s]: modified_D = %f, classic_D = %f, true_D = %f\n", status == 0 ? "_SENDER" : "_RECEIVER", flag == FIRST_CALCULATE ? "_FIRST" : "_SECOND", D, classicD, trueD);
    #endif

    /* adjust
    if flag == FIRST_CALCULATE
        update the most recent valid record
    if flag == SECOND_CALCULATE_UNQUALIFIED
        update the most recent valid record
    if flag == SECOND_CALCULATE_ABNORMAL
        update the next most recent valid record
    
    for SECOND_CALCULATE, the node is disconnected, maybe: Tx1, Rx1, Tx2, Rx2, Tx, Rx
                Rx1     Tx2             Rx3     Tx4             Rx

            Tx1             Rx2     Tx3             Rx4     Tx   
    */
    // update ranging_buffer
    RangingBufferNode_t newNode;
    /* SENDER
            sRx     <--Db-->    rTx         <--Rb-->        Rx

        sTx         <--Ra-->        rRx     <--Da-->    Tx
    */
    if(status == SENDER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        newNode.sendTxTimestamp = Tx;
        newNode.sendRxTimestamp = Rx;
        newNode.TxSeq = rangingListNode->localSeq;
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.receiveTxTimestamp = buffer->receiveBuffer[last].receiveTxTimestamp;
            newNode.receiveRxTimestamp = buffer->receiveBuffer[last].receiveRxTimestamp;
            newNode.RxSeq = buffer->receiveBuffer[last].RxSeq;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.receiveTxTimestamp = node->receiveTxTimestamp;
            newNode.receiveRxTimestamp = node->receiveRxTimestamp;
            newNode.RxSeq = node->RxSeq;
        }
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newNode.sendTxCoordinate = rangingListNode->TxCoordinate;
            newNode.sendRxCoordinate = rangingListNode->RxCoordinate;
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newNode.receiveRxCoordinate = buffer->receiveBuffer[last].receiveRxCoordinate;
                newNode.receiveTxCoordinate = buffer->receiveBuffer[last].receiveTxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newNode.receiveRxCoordinate = node->receiveRxCoordinate;
                newNode.receiveTxCoordinate = node->receiveTxCoordinate;
            }
        #endif
        newNode.sumTof = T23;

        addRangingBuffer(buffer, &newNode, status);
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
    */
    else if(status == RECEIVER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.sendTxTimestamp = buffer->sendBuffer[last].sendTxTimestamp;
            newNode.sendRxTimestamp = buffer->sendBuffer[last].sendRxTimestamp;
            newNode.TxSeq = buffer->sendBuffer[last].TxSeq;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.sendTxTimestamp = node->sendTxTimestamp;
            newNode.sendRxTimestamp = node->sendRxTimestamp;
            newNode.TxSeq = node->TxSeq;
        }
        newNode.receiveTxTimestamp = Tx;
        newNode.receiveRxTimestamp = Rx;
        newNode.RxSeq = rangingListNode->localSeq;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newNode.sendTxCoordinate = buffer->sendBuffer[last].sendTxCoordinate;
                newNode.sendRxCoordinate = buffer->sendBuffer[last].sendRxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newNode.sendTxCoordinate = node->sendTxCoordinate;
                newNode.sendRxCoordinate = node->sendRxCoordinate;
            }
            newNode.receiveRxCoordinate = rangingListNode->RxCoordinate;
            newNode.receiveTxCoordinate = rangingListNode->TxCoordinate;
        #endif
        newNode.sumTof = T23;

        addRangingBuffer(buffer, &newNode, status);
    }

    *Modified = D;
    *Classic = classicD;
    *True = trueD;

    return D;
}

void initializeRecordBuffer(RangingList_t *listA, RangingList_t *listB, table_index_t firstIndex, RangingBuffer_t* rangingBuffer, StatusType status) {
    // fetch data successively from listA, listB, and listA 
    table_index_t indexA1 = firstIndex;
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
    // int64_t A3TX = (listA->rangingList[indexA3].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    // int64_t A3RX = (listA->rangingList[indexA3].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    // int64_t B2TX = (listB->rangingList[indexB2].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    // int64_t B2RX = (listB->rangingList[indexB2].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    // int64_t A1TX = (listA->rangingList[indexA1].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    // int64_t A1RX = (listA->rangingList[indexA1].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;

    // DEBUG_PRINT("[initializeRecordBuffer]: A3TX:%lld,B2RX:%lld,A1TX:%lld\n",A3TX,B2RX,A1TX);
    // DEBUG_PRINT("[initializeRecordBuffer]: A3RX:%lld,B2TX:%lld,A1RX:%lld\n",A3RX,B2TX,A1RX);

    int64_t Ra = (listB->rangingList[indexB2].RxTimestamp.full - listA->rangingList[indexA3].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Db = (listB->rangingList[indexB2].TxTimestamp.full - listA->rangingList[indexA3].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Rb = (listA->rangingList[indexA1].RxTimestamp.full - listB->rangingList[indexB2].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Da = (listA->rangingList[indexA1].TxTimestamp.full - listB->rangingList[indexB2].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;

    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2*sumAB);

    if(classicTof < 0){
        DEBUG_PRINT("[initializeRecordBuffer]: Tof is less than 0\n");
        return;
    }
    DEBUG_PRINT("[initializeRecordBuffer]: Tof = %lld\n",classicTof);

    RangingBufferNode_t newNode1,newNode2;

    // update ranging_buffer
    if(status == SENDER){
        /*
                    A3.Rx   B2.Tx                   A1.Rx

            A3.Tx                   B2.Rx   A1.Tx
        */  
        newNode1.sendTxTimestamp = listA->rangingList[indexA3].TxTimestamp;
        newNode1.sendRxTimestamp = listA->rangingList[indexA3].RxTimestamp;
        newNode1.receiveTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        newNode1.receiveRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newNode1.sendTxCoordinate = listA->rangingList[indexA3].TxCoordinate;
            newNode1.sendRxCoordinate = listA->rangingList[indexA3].RxCoordinate;
            newNode1.receiveTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            newNode1.receiveRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
        #endif
        newNode1.sumTof = classicTof * 2;
        newNode1.TxSeq = listA->rangingList[indexA3].localSeq;
        newNode1.RxSeq = listB->rangingList[indexB2].localSeq;
        addRangingBuffer(rangingBuffer, &newNode1, RECEIVER);

        newNode2.sendTxTimestamp = listA->rangingList[indexA1].TxTimestamp;
        newNode2.sendRxTimestamp = listA->rangingList[indexA1].RxTimestamp;
        newNode2.receiveTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        newNode2.receiveRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newNode2.sendTxCoordinate = listA->rangingList[indexA1].TxCoordinate;
            newNode2.sendRxCoordinate = listA->rangingList[indexA1].RxCoordinate;
            newNode2.receiveTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            newNode2.receiveRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
        #endif
        newNode2.sumTof = classicTof * 2;
        newNode2.TxSeq = listA->rangingList[indexA1].localSeq;
        newNode2.RxSeq = listB->rangingList[indexB2].localSeq;
        addRangingBuffer(rangingBuffer, &newNode2, SENDER);
    }
    else if(status == RECEIVER){
        /*
            A3.Tx                   B2.Rx   A1.Tx

                    A3.Rx   B2.Tx                   A1.Rx
        */
        newNode1.sendTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        newNode1.sendRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        newNode1.receiveTxTimestamp = listA->rangingList[indexA3].TxTimestamp;
        newNode1.receiveRxTimestamp = listA->rangingList[indexA3].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newNode1.sendTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            newNode1.sendRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
            newNode1.receiveTxCoordinate = listA->rangingList[indexA3].TxCoordinate;
            newNode1.receiveRxCoordinate = listA->rangingList[indexA3].RxCoordinate;
        #endif
        newNode1.sumTof = classicTof * 2;
        newNode1.TxSeq = listB->rangingList[indexB2].localSeq;
        newNode1.RxSeq = listA->rangingList[indexA3].localSeq;
        addRangingBuffer(rangingBuffer, &newNode1, SENDER);

        newNode2.sendTxTimestamp = listB->rangingList[indexB2].TxTimestamp;
        newNode2.sendRxTimestamp = listB->rangingList[indexB2].RxTimestamp;
        newNode2.receiveTxTimestamp = listA->rangingList[indexA1].TxTimestamp;
        newNode2.receiveRxTimestamp = listA->rangingList[indexA1].RxTimestamp;
        #ifdef COMMUNICATION_SEND_POSITION_ENABLE
            newNode2.sendTxCoordinate = listB->rangingList[indexB2].TxCoordinate;
            newNode2.sendRxCoordinate = listB->rangingList[indexB2].RxCoordinate;
            newNode2.receiveTxCoordinate = listA->rangingList[indexA1].TxCoordinate;
            newNode2.receiveRxCoordinate = listA->rangingList[indexA1].RxCoordinate;
        #endif
        newNode2.sumTof = classicTof * 2;
        newNode2.TxSeq = listB->rangingList[indexB2].localSeq;
        newNode2.RxSeq = listA->rangingList[indexA1].localSeq;
        addRangingBuffer(rangingBuffer, &newNode2, RECEIVER);
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