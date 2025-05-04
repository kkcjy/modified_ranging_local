#include "ranging_buffer.h"
#include "table_linked_list.h"
#include "nullVal.h"

// static int64_t initTofSum;

// int64_t getInitTofSum() {
//     return initTofSum;
// }

void initRangingBufferNode(RangingBufferNode *node) {
    node->sendTx = nullTimeStamp;
    node->sendRx = nullTimeStamp;
    node->receiveTx = nullTimeStamp;
    node->receiveRx = nullTimeStamp;
    node->T1 = NULL_TOF;
    node->T2 = NULL_TOF;
    node->sumTof = NULL_TOF;
    node->localSeq = NULL_SEQ;
    node->preLocalSeq = NULL_SEQ;

    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
        node->sendTxCoordinate = nullCoordinate;
        node->sendRxCoordinate = nullCoordinate;
        node->receiveTxCoordinate = nullCoordinate;
        node->receiveRxCoordinate = nullCoordinate;
    #endif
}

void initRangingBuffer(RangingBuffer *buffer) {
    buffer->topSendBuffer = NULL_INDEX;
    buffer->topReceiveBuffer = NULL_INDEX;
    buffer->receiveLength = 0;
    buffer->sendLength = 0;
    for (int i = 0; i < RANGING_BUFFER_SIZE; i++) {
        initRangingBufferNode(&buffer->sendBuffer[i]);
        initRangingBufferNode(&buffer->receiveBuffer[i]);
    }
    // initTofSum = 0;
}

// add RangingBufferNode to RangingBuffer(send/receive)
void addRangingBuffer(RangingBuffer *buffer, RangingBufferNode *node, StatusType status) {
    if (status == SENDER) {
        buffer->topSendBuffer = (buffer->topSendBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->sendLength = buffer->sendLength < RANGING_BUFFER_SIZE ? buffer->sendLength + 1 : RANGING_BUFFER_SIZE;
        buffer->sendBuffer[buffer->topSendBuffer].sendTx = node->sendTx;
        buffer->sendBuffer[buffer->topSendBuffer].sendRx = node->sendRx;
        buffer->sendBuffer[buffer->topSendBuffer].receiveTx = node->receiveTx;
        buffer->sendBuffer[buffer->topSendBuffer].receiveRx = node->receiveRx;
        buffer->sendBuffer[buffer->topSendBuffer].T1 = node->T1;
        buffer->sendBuffer[buffer->topSendBuffer].T2 = node->T2;
        buffer->sendBuffer[buffer->topSendBuffer].sumTof = node->sumTof;
        buffer->sendBuffer[buffer->topSendBuffer].localSeq = node->localSeq;
        buffer->sendBuffer[buffer->topSendBuffer].preLocalSeq = node->preLocalSeq;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            buffer->sendBuffer[buffer->topSendBuffer].sendTxCoordinate = node->sendTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].sendRxCoordinate = node->sendRxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveTxCoordinate = node->receiveTxCoordinate;
            buffer->sendBuffer[buffer->topSendBuffer].receiveRxCoordinate = node->receiveRxCoordinate;
        #endif

        DEBUG_PRINT("[SENDBUFFER_ADD]: sendTx:%llu,sendRx:%llu,receiveTx:%llu,receiveRx:%llu,sumTof:%lld,localSeq:%d,preLocalSeq:%d\n"
            ,node->sendTx.full,node->sendRx.full,node->receiveTx.full,node->receiveRx.full,node->sumTof,node->localSeq,node->preLocalSeq);
    }
    else if (status == RECEIVER) {
        buffer->topReceiveBuffer = (buffer->topReceiveBuffer + 1) % RANGING_BUFFER_SIZE;
        buffer->receiveLength = buffer->receiveLength < RANGING_BUFFER_SIZE ? buffer->receiveLength + 1 : RANGING_BUFFER_SIZE;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendTx = node->sendTx;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sendRx = node->sendRx;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTx = node->receiveTx;
        buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRx = node->receiveRx;
        buffer->receiveBuffer[buffer->topReceiveBuffer].T1 = node->T1;
        buffer->receiveBuffer[buffer->topReceiveBuffer].T2 = node->T2;
        buffer->receiveBuffer[buffer->topReceiveBuffer].sumTof = node->sumTof;
        buffer->receiveBuffer[buffer->topReceiveBuffer].localSeq = node->localSeq;
        buffer->receiveBuffer[buffer->topReceiveBuffer].preLocalSeq = node->preLocalSeq;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendTxCoordinate = node->sendTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].sendRxCoordinate = node->sendRxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveTxCoordinate = node->receiveTxCoordinate;
            buffer->receiveBuffer[buffer->topReceiveBuffer].receiveRxCoordinate = node->receiveRxCoordinate;
        #endif

        DEBUG_PRINT("[RECEIVEBUFFER_ADD]: sendTx:%llu,sendRx:%llu,receiveTx:%llu,receiveRx:%llu,sumTof:%lld,localSeq:%d,preLocalSeq:%d\n"
            ,node->sendTx.full,node->sendRx.full,node->receiveTx.full,node->receiveRx.full,node->sumTof,node->localSeq,node->preLocalSeq);
    }
}

/* select index from RangingBuffer(send/receive)
SENDER
select the index closest to localSeq from receiveBuffer
RECEIVER
select the index closest to localSeq from sendBuffer
*/
table_index_t searchRangingBuffer(RangingBuffer *buffer, uint16_t localSeq, StatusType status) {
    table_index_t index = NULL_INDEX;
    if(status == SENDER) {
        table_index_t i = buffer->topReceiveBuffer;
        for (int j = 0; j < buffer->receiveLength; j++) {
            if(buffer->receiveBuffer[i].localSeq < localSeq) {
                if(index == NULL_INDEX || buffer->receiveBuffer[i].localSeq > buffer->receiveBuffer[index].localSeq) {
                    index = i;
                }
            }
            i = (i - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
        }
    }
    else if(status == RECEIVER) {
        table_index_t i = buffer->topSendBuffer;
        for (int j = 0; j < buffer->sendLength; j++) {
            if(buffer->sendBuffer[i].localSeq < localSeq) {
                if(index == NULL_INDEX || buffer->sendBuffer[i].localSeq > buffer->sendBuffer[index].localSeq) {
                    index = i;
                }
            }
            i = (i - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
        }
    }
    return index;
}

/* calculateTof
    FLAG = FIRST_CALCULATE,
        calculate the Tof using the most recent valid record
    FLAG = SECOND_CALCULATE_UNQUALIFIED,
        recalculate the Tof using the next most recent valid record
    FLAS = SECOND_CALCULATE_ABNORMAL
        recalculate the Tof using the next most recent valid record
*/
double calculateTof(RangingBuffer *buffer, TableNode_t* tableNode, uint16_t checkLocalSeq, StatusType status, FLAG flag) {
    // calculate D
    dwTime_t Tx = tableNode->TxTimestamp;
    dwTime_t Rx = tableNode->RxTimestamp;
    uint16_t localSeq = tableNode->localSeq;
    RangingBufferNode* node = NULL;
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
        Ra = (node->receiveRx.full-node->sendTx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (node->receiveTx.full-node->sendRx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - node->receiveTx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - node->receiveRx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
    */
    else if(status == RECEIVER){
        Ra = (node->sendRx.full - node->receiveTx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Db = (node->sendTx.full - node->receiveRx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Rb = (Rx.full - node->sendTx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
        Da = (Tx.full - node->sendRx.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    }
    int64_t T12 = node->sumTof;
    int64_t T23 = 0;
    DEBUG_PRINT("[CalculateTof]: status:%d,T12:%lld,Ra:%lld,Rb:%lld,Da:%lld,Db:%lld\n", status,T12,Ra, Rb, Da, Db);
    float Ra_Da = (float)Ra/Da;
    float Rb_Db = (float)Rb/Db;
    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    DEBUG_PRINT("[CalculateTof]: Ra_Da:%f,Rb_Db:%f\n", Ra_Da, Rb_Db);

    // fail to satisfy the convergence requirements
    if(Ra_Da < CONVERGENCE_THRESHOLD || Rb_Db < CONVERGENCE_THRESHOLD) {
        if(Ra_Da < Rb_Db) {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da)/2 - Ra*T12)/Da;
        }
        else {
            T23 = ((diffA * Rb + diffA * Db + diffB * Ra + diffB * Da)/2 - Rb*T12)/Db;
        }
    }
    else {
        DEBUG_PRINT("Warning: Ra/Da and Rb/Db are both greater than CONVERGENCE_THRESHOLD(%f)\n",CONVERGENCE_THRESHOLD);
        if(flag) {
            DEBUG_PRINT("Warning: The latest record in rangingbuffer fails, and an attempt is made to recalculate the Tof using the next most recent valid record\n");
            return calculateTof(buffer, tableNode, node->localSeq, status, SECOND_CALCULATE_UNQUALIFIED);
        }
        else {
            DEBUG_PRINT("Warning: Recalculate Tof failed\n");
            return -1;
        }
    }

    // abnormal result
    float Tof = T23 / 2;
    float D = Tof * VELOCITY;
    if(D < 0 || D > 1000){
        DEBUG_PRINT("Warning: D = %f is out of range(0,1000)\n",D);
        if(flag){
            DEBUG_PRINT("Warning: The latest record in rangingbuffer fails, and an attempt is made to recalculate the Tof using the next most recent valid record\n");
            return calculateTof(buffer, tableNode, node->localSeq, status, SECOND_CALCULATE_ABNORMAL);
        }
        else{
            DEBUG_PRINT("Warning: Recalculate Tof failed\n");
            return -1;
        }
    }

    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2*sumAB);
    float classicD = classicTof * VELOCITY;

    #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
        float trueDx = (tableNode->RxCoordinate.x - tableNode->TxCoordinate.x)/10.0;
        float trueDy = (tableNode->RxCoordinate.y - tableNode->TxCoordinate.y)/10.0;
        float trueDz = (tableNode->RxCoordinate.z - tableNode->TxCoordinate.z)/10.0;
        float trueD = sqrt(trueDx*trueDx + trueDy*trueDy + trueDz*trueDz);
        DEBUG_PRINT("[CalculateTof Finished]: modified_D = %f, classic_D = %f, true_D = %f", D, classicD, trueD);
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
    RangingBufferNode newNode;
    /* SENDER
            sRx     <--Db-->    rTx         <--Rb-->        Rx

        sTx         <--Ra-->        rRx     <--Da-->    Tx
    */
    if(status == SENDER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        newNode.sendTx = Tx;
        newNode.sendRx = Rx;
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.receiveTx = buffer->receiveBuffer[last].receiveTx;
            newNode.receiveRx = buffer->receiveBuffer[last].receiveRx;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.receiveTx = node->receiveTx;
            newNode.receiveRx = node->receiveRx;
        }
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            newNode.sendTxCoordinate = tableNode->TxCoordinate;
            newNode.sendRxCoordinate = tableNode->RxCoordinate;
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newNode.receiveRxCoordinate = buffer->receiveBuffer[last].receiveRxCoordinate;
                newNode.receiveTxCoordinate = buffer->receiveBuffer[last].receiveTxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newNode.receiveRxCoordinate = node->receiveRxCoordinate;
                newNode.receiveTxCoordinate = node->receiveTxCoordinate;
            }
        #endif
        newNode.localSeq = localSeq;
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.preLocalSeq = buffer->receiveBuffer[last].localSeq;
        }
        else if(flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.preLocalSeq = node->localSeq;
        }
        newNode.sumTof = T23;
        newNode.T1 = node->T2;
        newNode.T2 = T23 - newNode.T1;
        addRangingBuffer(buffer, &newNode, status);
    }
    /* RECEIVER
        rTx         <--Ra-->        sRx     <--Da-->    Tx

            rRx     <--Db-->    sTx         <--Rb-->        Rx
    */
    else if(status == RECEIVER) {
        table_index_t last = searchRangingBuffer(buffer, localSeq, status);
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.sendTx = buffer->sendBuffer[last].sendTx;
            newNode.sendRx = buffer->sendBuffer[last].sendRx;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.sendTx = node->sendTx;
            newNode.sendRx = node->sendRx;
        }
        newNode.receiveTx = Tx;
        newNode.receiveRx = Rx;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
                newNode.sendTxCoordinate = buffer->sendBuffer[last].sendTxCoordinate;
                newNode.sendRxCoordinate = buffer->sendBuffer[last].sendRxCoordinate;
            }
            else if (flag == SECOND_CALCULATE_ABNORMAL) {
                newNode.sendTxCoordinate = node->sendTxCoordinate;
                newNode.sendRxCoordinate = node->sendRxCoordinate;
            }
            newNode.receiveRxCoordinate = tableNode->RxCoordinate;
            newNode.receiveTxCoordinate = tableNode->TxCoordinate;
        #endif
        newNode.localSeq = localSeq;
        if(flag == FIRST_CALCULATE || flag == SECOND_CALCULATE_UNQUALIFIED) {
            newNode.preLocalSeq = buffer->sendBuffer[last].localSeq;
        }
        else if (flag == SECOND_CALCULATE_ABNORMAL) {
            newNode.preLocalSeq = node->localSeq;
        }
        newNode.sumTof = T23;
        newNode.T1 = node->T2;
        newNode.T2 = T23 - newNode.T1;
        addRangingBuffer(buffer, &newNode, status);
    }
    return D;
}

bool initializeRecordBuffer(TableLinkedList_t *listA, TableLinkedList_t *listB, table_index_t firstIndex, RangingBuffer* rangingBuffer, StatusType status) {
    // fetch data successively from listA, listB, and listA 
    table_index_t indexA1 = firstIndex;
    if (indexA1 == NULL_INDEX || listA->tableBuffer[indexA1].TxTimestamp.full == NULL_TIMESTAMP || listA->tableBuffer[indexA1].RxTimestamp.full == NULL_TIMESTAMP || listA->tableBuffer[indexA1].Tf != NULL_TOF) {
        DEBUG_PRINT("Warning: The lastest record in listA is invalid or the record has owned Tof\n");
        return false;
    }
    table_index_t indexB2 = searchTableLinkedList(listB, listA->tableBuffer[indexA1].localSeq);
    if (indexB2 == NULL_INDEX) {
        DEBUG_PRINT("No valid record in listB\n");
        return false;
    }
    table_index_t indexA3 = searchTableLinkedList(listA, listB->tableBuffer[indexB2].localSeq);
    if (indexA3 == NULL_INDEX) {
        DEBUG_PRINT("No valid record in listA\n");
        return false;
    }

    /*
        A3.Tx          <--Ra-->          B2.Rx      <--Da-->      A1.Tx

                A3.Rx  <--Db-->  B2.Tx              <--Rb-->              A1.Rx
    */
    int64_t A3TX = (listA->tableBuffer[indexA3].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t A3RX = (listA->tableBuffer[indexA3].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t B2TX = (listB->tableBuffer[indexB2].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t B2RX = (listB->tableBuffer[indexB2].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t A1TX = (listA->tableBuffer[indexA1].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t A1RX = (listA->tableBuffer[indexA1].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;

    DEBUG_PRINT("[firstRecordBuffer]: A3TX:%lld,B2RX:%lld,A1TX:%lld\n",A3TX,B2RX,A1TX);
    DEBUG_PRINT("[firstRecordBuffer]: A3RX:%lld,B2TX:%lld,A1RX:%lld\n",A3RX,B2TX,A1RX);

    int64_t Ra = (listB->tableBuffer[indexB2].RxTimestamp.full - listA->tableBuffer[indexA3].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Db = (listB->tableBuffer[indexB2].TxTimestamp.full - listA->tableBuffer[indexA3].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Rb = (listA->tableBuffer[indexA1].RxTimestamp.full - listB->tableBuffer[indexB2].TxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;
    int64_t Da = (listA->tableBuffer[indexA1].TxTimestamp.full - listB->tableBuffer[indexB2].RxTimestamp.full + UWB_MAX_TIMESTAMP) % UWB_MAX_TIMESTAMP;

    int64_t diffA = Ra - Da;
    int64_t diffB = Rb - Db;
    int64_t sumAB = Ra + Db + Rb + Da;
    int64_t classicTof = (diffA * Rb + diffA * Db + diffB * Ra + diffB * Da) / (2*sumAB);

    if(classicTof < 0){
        DEBUG_PRINT("[firstRecordBuffer]: Tof is less than 0\n");
        return false;
    }
    DEBUG_PRINT("[firstRecordBuffer]: Tof = %lld\n",classicTof);
    
    RangingBufferNode newNode1,newNode2;

    // update ranging_buffer
    if(status == SENDER){
        /*
                    A3.Rx   B2.Tx                   A1.Rx

            A3.Tx                   B2.Rx   A1.Tx
        */  
        newNode1.sendTx = listA->tableBuffer[indexA3].TxTimestamp;
        newNode1.sendRx = listA->tableBuffer[indexA3].RxTimestamp;
        newNode1.receiveTx = listB->tableBuffer[indexB2].TxTimestamp;
        newNode1.receiveRx = listB->tableBuffer[indexB2].RxTimestamp;
        newNode1.localSeq = listB->tableBuffer[indexB2].localSeq;
        newNode1.preLocalSeq = listA->tableBuffer[indexA3].localSeq;
        newNode1.sumTof = classicTof*2;
        newNode1.T1 = classicTof;
        newNode1.T2 = classicTof;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            newNode1.sendTxCoordinate = listA->tableBuffer[indexA3].TxCoordinate;
            newNode1.sendRxCoordinate = listA->tableBuffer[indexA3].RxCoordinate;
            newNode1.receiveTxCoordinate = listB->tableBuffer[indexB2].TxCoordinate;
            newNode1.receiveRxCoordinate = listB->tableBuffer[indexB2].RxCoordinate;
        #endif
        addRangingBuffer(rangingBuffer, &newNode1, RECEIVER);

        newNode2.sendTx = listA->tableBuffer[indexA1].TxTimestamp;
        newNode2.sendRx = listA->tableBuffer[indexA1].RxTimestamp;
        newNode2.receiveTx = listB->tableBuffer[indexB2].TxTimestamp;
        newNode2.receiveRx = listB->tableBuffer[indexB2].RxTimestamp;
        newNode2.localSeq = listA->tableBuffer[indexA1].localSeq;
        newNode2.preLocalSeq = listB->tableBuffer[indexB2].localSeq;
        newNode2.sumTof = classicTof*2;
        newNode2.T1 = classicTof;
        newNode2.T2 = classicTof;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            newNode2.sendTxCoordinate = listA->tableBuffer[indexA1].TxCoordinate;
            newNode2.sendRxCoordinate = listA->tableBuffer[indexA1].RxCoordinate;
            newNode2.receiveTxCoordinate = listB->tableBuffer[indexB2].TxCoordinate;
            newNode2.receiveRxCoordinate = listB->tableBuffer[indexB2].RxCoordinate;
        #endif
        addRangingBuffer(rangingBuffer, &newNode2, SENDER);
    }
    else if(status == RECEIVER){
        /*
            A3.Tx                   B2.Rx   A1.Tx

                    A3.Rx   B2.Tx                   A1.Rx
        */
        newNode1.sendTx = listB->tableBuffer[indexB2].TxTimestamp;
        newNode1.sendRx = listB->tableBuffer[indexB2].RxTimestamp;
        newNode1.receiveTx = listA->tableBuffer[indexA3].TxTimestamp;
        newNode1.receiveRx = listA->tableBuffer[indexA3].RxTimestamp;
        newNode1.localSeq = listB->tableBuffer[indexB2].localSeq;
        newNode1.preLocalSeq = listA->tableBuffer[indexA3].localSeq;
        newNode1.sumTof = classicTof*2;
        newNode1.T1 = classicTof;
        newNode1.T2 = classicTof;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            newNode1.sendTxCoordinate = listB->tableBuffer[indexB2].TxCoordinate;
            newNode1.sendRxCoordinate = listB->tableBuffer[indexB2].RxCoordinate;
            newNode1.receiveTxCoordinate = listA->tableBuffer[indexA3].TxCoordinate;
            newNode1.receiveRxCoordinate = listA->tableBuffer[indexA3].RxCoordinate;
        #endif
        addRangingBuffer(rangingBuffer, &newNode1, SENDER);

        newNode2.sendTx = listB->tableBuffer[indexB2].TxTimestamp;
        newNode2.sendRx = listB->tableBuffer[indexB2].RxTimestamp;
        newNode2.receiveTx = listA->tableBuffer[indexA1].TxTimestamp;
        newNode2.receiveRx = listA->tableBuffer[indexA1].RxTimestamp;
        newNode2.localSeq = listA->tableBuffer[indexA1].localSeq;
        newNode2.preLocalSeq = listB->tableBuffer[indexB2].localSeq;
        newNode2.sumTof = classicTof*2;
        newNode2.T1 = classicTof;
        newNode2.T2 = classicTof;
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            newNode2.sendTxCoordinate = listB->tableBuffer[indexB2].TxCoordinate;
            newNode2.sendRxCoordinate = listB->tableBuffer[indexB2].RxCoordinate;
            newNode2.receiveTxCoordinate = listA->tableBuffer[indexA1].TxCoordinate;
            newNode2.receiveRxCoordinate = listA->tableBuffer[indexA1].RxCoordinate;
        #endif
        addRangingBuffer(rangingBuffer, &newNode2, RECEIVER);
    }
    // initTofSum += Tof;
}

void printRangingBuffer(RangingBuffer *buffer) {
    DEBUG_PRINT("[ValidBuffer]:\n");
    int index = buffer->topSendBuffer;
    for(int bound = 0; bound < RANGING_BUFFER_SIZE; bound++){
        if(index == NULL_INDEX){
            break;
        }
        DEBUG_PRINT("SendBuffer[%d]: receiveTx: %llu,receiveRx: %llu,sendTx: %llu,sendRx: %llu,sumTof: %lld",
            index,buffer->sendBuffer[index].receiveTx.full,buffer->sendBuffer[index].receiveRx.full,
            buffer->sendBuffer[index].sendTx.full,buffer->sendBuffer[index].sendRx.full,
            buffer->sendBuffer[index].sumTof
        );
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
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
        DEBUG_PRINT("ReceiveBuffer[%d]: sendTx: %llu,sendRx: %llu,receiveTx: %llu,receiveRx: %llu,sumTof: %lld",
            index,buffer->receiveBuffer[index].sendTx.full,buffer->receiveBuffer[index].sendRx.full,
            buffer->receiveBuffer[index].receiveTx.full,buffer->receiveBuffer[index].receiveRx.full,
            buffer->receiveBuffer[index].sumTof);
        #ifdef UWB_COMMUNICATION_SEND_POSITION_ENABLE
            DEBUG_PRINT(",sendCoordinate: (%d,%d,%d),receiveCoordinate: (%d,%d,%d)\n",
                buffer->receiveBuffer[index].sendRxCoordinate.x,buffer->receiveBuffer[index].sendRxCoordinate.y,buffer->receiveBuffer[index].sendRxCoordinate.z,
                buffer->receiveBuffer[index].receiveRxCoordinate.x,buffer->receiveBuffer[index].receiveRxCoordinate.y,buffer->receiveBuffer[index].receiveRxCoordinate.z);
        #else
            DEBUG_PRINT("\n");
        #endif
        index = (index - 1 + RANGING_BUFFER_SIZE) % RANGING_BUFFER_SIZE;
    }
}