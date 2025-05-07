#ifndef LOCK_H
#define LOCK_H

#include "defs.h"

typedef struct {
    void *data;
    size_t data_size;
} Task_t;                                       

typedef struct {
    pthread_mutex_t mutex;                              // for generating and processing data
    pthread_mutex_t countMutex;                         // for count
    Task_t queueTask[QUEUE_TASK_LENGTH];                // head for processing, tail for receiving
    uint8_t head;               
    uint8_t tail;               
    uint8_t count;  
} QueueTaskLock_t;


void initQueue(QueueTaskLock_t *queue);
void QueueTaskTx(QueueTaskLock_t *queue);
void QueueTaskRx(QueueTaskLock_t *queue, void *data, size_t data_size);
bool processFromQueue(QueueTaskLock_t *queue);

#endif