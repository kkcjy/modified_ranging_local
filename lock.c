#include "lock.h"


void initQueue(QueueTaskLock_t *queue) {
    pthread_mutex_init(&queue->mutex, NULL);
    queue->TxMutex = FREE;
    queue->processMutex = FREE;
    for(int i = 0; i < QUEUE_TASK_LENGTH; i++) {
        queue->queueTask->data = NULL;
        queue->queueTask->data_size = 0;
    }
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->TxCount = 0;
    queue->RxCount = 0;
}

/* generate data and Tx(top priority) 
    prohibit processing, permit receiving
*/
void QueueTaskTx(QueueTaskLock_t *queue) {
    pthread_mutex_lock(&queue->mutex);

    // wait for processing task finished
    while (queue->processMutex);

    queue->TxMutex = BUSY;

    Ranging_Message_t *rangingMessage = (Ranging_Message_t*)malloc(sizeof(uint8_t) * UWB_MAX_MESSAGE_LEN);

    Time_t timeDelay = generateRangingMessage(rangingMessage);

    pthread_mutex_unlock(&queue->mutex);

    // wait for last Tx task finished
    while (queue->TxMutex);   // TxMutex

    /*
        Tx start(socket)
    */

    free(rangingMessage);
    rangingMessage = NULL; 

    queue->TxMutex = FREE;

    queue->TxCount++;
    DEBUG_PRINT("[QueueTaskTx]: sent the %d-th message\n", queue->TxCount);
}

/* Rx
    permit everything
*/
void QueueTaskRx(QueueTaskLock_t *queue, void *data, size_t data_size) {
    if(queue->count == QUEUE_TASK_LENGTH) {
        DEBUG_PRINT("[QueueTaskRx]: queueTask is full\n");
        return;
    }

    if(data_size == 0) {
        DEBUG_PRINT("[QueueTaskRx]: receive empty message\n");
        return;
    }

    // with additional info
    queue->queueTask[queue->tail].data = malloc(data_size);
    if (queue->queueTask[queue->tail].data == NULL) {
        DEBUG_PRINT("[QueueTaskRx]: malloc failed\n");
        return -1;
    }
    queue->queueTask[queue->tail].data_size = data_size;
    memcpy(queue->queueTask[queue->tail].data, data, data_size);

    queue->tail = (queue->tail + 1) % QUEUE_TASK_LENGTH;
    queue->count++;

    queue->RxCount++;
    DEBUG_PRINT("[QueueTaskRx]: receive the %d-th message\n", queue->RxCount);
}

/* process data
    prohibit generate
*/
bool processFromQueue(QueueTaskLock_t *queue) {
    while(queue->count = 0);

    pthread_mutex_lock(&queue->mutex);

    // wait for last task fished
    while (queue->processMutex);

    queue->processMutex = BUSY;

    Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo = (Ranging_Message_With_Additional_Info_t*)queue->queueTask[queue->head].data;

    #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
        bool unSafe = processRangingMessage(rangingMessageWithAdditionalInfo);
    #else
        processRangingMessage(rangingMessageWithAdditionalInfo);
    #endif

    pthread_mutex_unlock(&queue->mutex);

    queue->processMutex = FREE;

    #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
        return unSafe;
    #else
        return false;
    #endif
}