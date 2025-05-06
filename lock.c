#include "lock.h"

void initQueue(QueueTaskLock_t *queue) {
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_mutex_init(&queue->queueMutex, NULL);
    queue->generateMutex = FREE;
    queue->processMutex = FREE;
    queue->TxCount = 0;
    queue->RxCount = 0;
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
}