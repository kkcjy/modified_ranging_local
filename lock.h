#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>
#include <string.h>
#include "defs.h"
#include "debug.h"

typedef enum {
    FREE,
    BUSY
} SendMutex;

typedef struct {
    void *data;
    size_t data_size;
} QueueTask_t;

typedef struct {
    QueueTask_t queueTask[QUEUE_TASK_LENGTH];   // only used for Rx
    pthread_mutex_t queueMutex;                 // for modifying queueTask
    pthread_mutex_t mutex;                      // for modifying data
    SendMutex generateMutex;                    // for generating data
    SendMutex processMutex;                     // for processing data
    uint8_t TxCount;                            // counter for Tx(for debug)
    uint8_t RxCount;                            // counter for Rx(for debug)
    uint8_t head;               
    uint8_t tail;               
    uint8_t count;                              // number of tasks in queueTask
} QueueTaskLock_t;

void initQueue(QueueTaskLock_t *queue);

#endif