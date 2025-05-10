#ifndef LOCK_H
#define LOCK_H

#include "defs.h"
#include "debug.h"
#include "modified_ranging.h"

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


typedef void (*SendFunction)(int, const char*, const Ranging_Message_t *);

void initQueueTaskLock(QueueTaskLock_t *queue);
Time_t QueueTaskTx(QueueTaskLock_t *queue, int msgSize, SendFunction send_func, int centerSocket, const char* droneId);
void QueueTaskRx(QueueTaskLock_t *queue, void *data, size_t data_size);
bool processFromQueue(QueueTaskLock_t *queue);

#endif