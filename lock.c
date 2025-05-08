#include "lock.h"
#include "modified_ranging.h"


void initQueue(QueueTaskLock_t *queue) {
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_mutex_init(&queue->countMutex, NULL);
    for(int i = 0; i < QUEUE_TASK_LENGTH; i++) {
        queue->queueTask[i].data = NULL;
        queue->queueTask[i].data_size = 0;
    }
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}

// generate data and Tx
void QueueTaskTx(QueueTaskLock_t *queue) {
    pthread_mutex_lock(&queue->mutex);

    Ranging_Message_t *rangingMessage = (Ranging_Message_t*)malloc(sizeof(uint8_t) * UWB_MAX_MESSAGE_LEN);
    if (!rangingMessage) {
        DEBUG_PRINT("[QueueTaskTx]: malloc failed\n");
        pthread_mutex_unlock(&queue->mutex);
        return;
    }

    Time_t timeDelay = generateRangingMessage(rangingMessage);

    pthread_mutex_unlock(&queue->mutex);

    /*
        Tx start(socket)
    */

    free(rangingMessage);
    rangingMessage = NULL; 

    DEBUG_PRINT("[QueueTaskTx]: sent the message\n");
}

// Rx
void QueueTaskRx(QueueTaskLock_t *queue, void *data, size_t data_size) {
    if(data_size == 0) {
        DEBUG_PRINT("[QueueTaskRx]: receive empty message\n");
        return;
    }

    pthread_mutex_lock(&queue->countMutex);
    if(queue->count == QUEUE_TASK_LENGTH) {
        DEBUG_PRINT("[QueueTaskRx]: queueTask is full, message missed\n");
        pthread_mutex_unlock(&queue->countMutex);
        return;
    }

    // receive data
    queue->queueTask[queue->tail].data = malloc(data_size);
    if (queue->queueTask[queue->tail].data == NULL) {
        DEBUG_PRINT("[QueueTaskRx]: malloc failed\n");
        pthread_mutex_unlock(&queue->countMutex);
        return;
    }
    queue->queueTask[queue->tail].data_size = data_size;
    memcpy(queue->queueTask[queue->tail].data, data, data_size);

    queue->tail = (queue->tail + 1) % QUEUE_TASK_LENGTH;
    queue->count++;
    pthread_mutex_unlock(&queue->countMutex);

    DEBUG_PRINT("[QueueTaskRx]: receive the message\n");
}

// process
bool processFromQueue(QueueTaskLock_t *queue) {
GET:

    pthread_mutex_lock(&queue->mutex);
    pthread_mutex_lock(&queue->countMutex);
    if(queue->count != 0) {
        goto PROCESS;
    }

    pthread_mutex_unlock(&queue->mutex);
    pthread_mutex_unlock(&queue->countMutex);
    usleep(100); 
    goto GET;

PROCESS:

    Ranging_Message_With_Additional_Info_t *rangingMessageWithAdditionalInfo = (Ranging_Message_With_Additional_Info_t*)queue->queueTask[queue->head].data;

    #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
        bool unSafe = processRangingMessage(rangingMessageWithAdditionalInfo);
    #else
        processRangingMessage(rangingMessageWithAdditionalInfo);
    #endif

    free(queue->queueTask[queue->head].data);
    queue->queueTask[queue->head].data = NULL;
    queue->head = (queue->head + 1) % QUEUE_TASK_LENGTH;
    queue->count--;

    pthread_mutex_unlock(&queue->countMutex);

    pthread_mutex_unlock(&queue->mutex);

    #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
        return unSafe;
    #else
        return false;
    #endif
}