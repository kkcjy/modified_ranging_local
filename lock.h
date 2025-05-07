#ifndef LOCK_H
#define LOCK_H

#include "defs.h"


typedef enum {
    FREE,
    BUSY
} SendMutex;

typedef struct {
    void *data;
    size_t data_size;
} Task_t;                                       

typedef struct {
    pthread_mutex_t mutex;                              // for generating and processing data
    SendMutex TxMutex;                                  // for Tx data
    SendMutex processMutex;                             // for processing data
    Task_t queueTask[QUEUE_TASK_LENGTH];                // head for processing, tail for receiving
    uint8_t head;               
    uint8_t tail;               
    uint8_t count;  
    uint8_t TxCount;                                    // counter for Tx(for debug)
    uint8_t RxCount;                                    // counter for Rx(for debug)
} QueueTaskLock_t;

void initQueue(QueueTaskLock_t *queue);
void QueueTaskTx(QueueTaskLock_t *queue);
void QueueTaskRx(QueueTaskLock_t *queue, void *data, size_t data_size);
int processFromQueue(QueueTaskLock_t *queue);

#endif