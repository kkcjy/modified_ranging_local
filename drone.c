#include "socket_frame.h"
#include "local_host.h"
#include "lock.h"
#include "modified_ranging.h"

const char *local_drone_id;             
Local_Host_t *localHost;                        // local host
RangingTableSet_t* rangingTableSet;             // local rangingTableSet
QueueTaskLock_t queueTaskLock;                  // lock for task
uint16_t localSendSeqNumber = 1;
uint8_t *neighborIdxPriorityQueue;              // used for choosing neighbors to sent messages
#ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
    int safeRoundCounter = 0;                   // counter for safe distance
    int dynamicRangingPeriod = RANGING_PERIOD;  // ranging period
#endif
#ifdef WARM_UP_WAIT_ENABLE
    int discardCount = 0;                       // wait for device warming up and discard message
#endif


void send_to_center(int center_socket, const char *node_id, const char *message) {
    NodeMessage msg;
    strncpy(msg.sender_id, node_id, sizeof(msg.sender_id));
    msg.data_size = strlen(message) + 1;
    strncpy(msg.data, message, sizeof(msg.data));
    
    if (send(center_socket, &msg, sizeof(msg), 0) < 0) {
        perror("Send failed");
    }
}

// for processing
void *process_messages(void *arg) {
    while (1) {
        bool unSafe = processFromQueue(&queueTaskLock);
        /*  
            unsafe -> set RANGING_PERIOD_LOW
            safe more than SAFE_DISTANCE_ROUND_BORDER -> set RANGING_PERIOD
        */
        if (unSafe) {
            #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
                dynamicRangingPeriod = RANGING_PERIOD_LOW;
                safeRoundCounter = 0;
            #endif
            printf("Warning: Unsafe condition detected!\n");
        }
        else {
            safeRoundCounter++;
            if(safeRoundCounter == SAFE_DISTANCE_ROUND_BORDER) {
                dynamicRangingPeriod = RANGING_PERIOD;
            }
        }
        local_sleep(10);                      
    }
    return NULL;
}

// for Rx
void *receive_from_center(void *arg) {
    int center_socket = *(int *)arg;
    NodeMessage msg;
    
    while (1) {
        //  center_socket -> msg
        ssize_t bytes_received = recv(center_socket, &msg, sizeof(msg), 0);
        if (bytes_received <= 0) {
            printf("Disconnected from Control Center\n");
            break;
        }

        if (strcmp(msg.data, REJECT_INFO) == 0) {
            printf("Connection rejected: Maximum drones reached (%d)\n", MAX_NODES);
            exit(1);
        }
        
        // don't display messages from self
        if (strcmp(msg.sender_id, local_drone_id) != 0) {
            // Rx
            QueueTaskRx(&queueTaskLock, &msg, sizeof(NodeMessage));
            
            printf("Received message from %s, added to queue\n", msg.sender_id);
        }
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Usage: %s <center_ip> <local_drone_id>\n", argv[0]);
        return 1;
    }

    const char *center_ip = argv[1];
    local_drone_id = argv[2];

    // init operation
    localInit(localHost, *(uint16_t*)local_drone_id);
    initQueueTaskLock(&queueTaskLock);      // before create pthread
    initRangingTableSet();
    
    int center_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (center_socket < 0) {
        perror("Socket creation error");
        return -1;
    }

    struct sockaddr_in serv_addr = {
        .sin_family = AF_INET,
        .sin_port = htons(CENTER_PORT)
    };
    
    if (inet_pton(AF_INET, center_ip, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address");
        return -1;
    }

    if (connect(center_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Send drone ID first
    send(center_socket, local_drone_id, strlen(local_drone_id), 0);

    // start process
    pthread_t process_thread;
    if (pthread_create(&process_thread, NULL, process_messages, NULL)) {
        perror("pthread_create for process thread failed");
        close(center_socket);
        return -1;
    }

    // start receive
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_from_center, &center_socket)) {
        perror("pthread_create for receive thread failed");
        close(center_socket);
        return -1;
    }

    printf("Node %s connected to center\n", local_drone_id);

    // main send loop
    while (1) {
        Time_t time_delay = QueueTaskTx(&queueTaskLock, MESSAGE_SIZE, send_to_center, center_socket, local_drone_id);

        localSendSeqNumber++;

        local_sleep(time_delay);
    }

    pthread_join(receive_thread, NULL);
    pthread_join(process_thread, NULL);
    close(center_socket);
    return 0;
}