#ifndef SOCKET_FRAME_H
#define SOCKET_FRAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define MAX_NODES 1
#define BUFFER_SIZE 1024
#define CENTER_PORT 8888
#define BROADCAST_PORT 8889
#define REJECT_INFO "REJECT"

typedef struct {
    char sender_id[20];
    char data[BUFFER_SIZE - 20 - sizeof(size_t)];
    size_t data_size;
} NodeMessage;

typedef struct {
    int socket;
    char node_id[20];
} NodeInfo;

#endif