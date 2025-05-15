#include "socket_frame.h"
#include "local_host.h"
#include "lock.h"
#include "modified_ranging.h"

const char *local_drone_id;            
extern Local_Host_t *localHost;     
extern RangingTableSet_t* rangingTableSet;     
extern uint16_t localSendSeqNumber;
extern uint16_t localReceivedSeqNumber;
extern int RangingPeriod;
QueueTaskLock_t queueTaskLock;                  // lock for task
#ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
    int safeRoundCounter = 0;                   // counter for safe distance
#endif
#ifdef WARM_UP_WAIT_ENABLE
    extern int discardCount;                    // wait for device warming up and discard message
#endif


void send_to_center(int center_socket, const char* node_id, const Ranging_Message_t* ranging_msg) {
    NodeMessage msg;

    if(sizeof(MessageWithLocation) > MESSAGE_SIZE) {
        printf("Warning: %ld > %ld, Ranging_Message_t too large!\n", sizeof(Ranging_Message_t), MESSAGE_SIZE);
    }

    snprintf(msg.sender_id, sizeof(msg.sender_id), "%s", node_id);  

    MessageWithLocation modified_msg;
    memcpy(&modified_msg.rangingMessage, ranging_msg, sizeof(*ranging_msg));
    dwTime_t curTime;
    curTime.full = getCurrentTime();
    #ifdef COMMUNICATION_SEND_POSITION_ENABLE
        Coordinate_Tuple_t curLocation = getCurrentLocation();
    #endif
    modified_msg.location = curLocation;
    memcpy(msg.data, &modified_msg, sizeof(MessageWithLocation)); 

    msg.data_size = sizeof(MessageWithLocation);

    addLocalSendBuffer(curTime, curLocation);

    if (send(center_socket, &msg, sizeof(msg), 0) < 0) {
        perror("Send failed");
    }
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

        // don't display messages from essage dropped randomly (probabself
        if (strcmp(msg.sender_id, local_drone_id) != 0) {
            #ifdef PACKET_LOSS_ENABLE
                // for random
                int random_value = ((rand() * 2654435761U) / getpid()) % 100;

                if (random_value < PACKET_LOSS_RATE) {
                    MessageWithLocation *modified_msg = (MessageWithLocation*)msg.data;
                    Ranging_Message_t *ranging_msg = &modified_msg->rangingMessage;
                    printf("[QueueTaskRx]: message[%d] from %s dropped(rate = %d%%)\n",ranging_msg->header.msgSequence , msg.sender_id, PACKET_LOSS_RATE);
                    continue;
                }
            #endif

            if (msg.data_size != sizeof(MessageWithLocation)) {
                printf("Receiving failed, size of data_size does not match\n");
                return NULL;
            }

            MessageWithLocation *modified_msg = (MessageWithLocation*)msg.data;

            Ranging_Message_t *ranging_msg = &modified_msg->rangingMessage;

            // get curTime and curLocation
            #ifdef COMMUNICATION_SEND_POSITION_ENABLE
                Coordinate_Tuple_t curLocation = getCurrentLocation();
                Coordinate_Tuple_t remoteLocation = modified_msg->location;
                // printf("[local]:  x = %d, y = %d, z = %d\n[remote]: x = %d, y = %d, z = %d\n", curLocation.x, curLocation.y, curLocation.z, remoteLocation.x, remoteLocation.y, remoteLocation.z);
                double distance = sqrt(pow((curLocation.x - remoteLocation.x), 2) + pow((curLocation.y - remoteLocation.y), 2) + pow((curLocation.z - remoteLocation.z), 2));
                double Tof = distance / VELOCITY;
                // printf("[%s -> %s][%d]: D = %f, TOF  = %f\n", msg.sender_id, local_drone_id, ranging_msg->header.msgSequence, distance, Tof);
                local_sleep(Tof);
            #endif

            uint64_t curTime = getCurrentTime();

            Ranging_Message_With_Additional_Info_t full_info;

            full_info.rangingMessage = *ranging_msg;
            full_info.RxTimestamp.full = curTime;
            full_info.RxCoordinate = curLocation;
            
            // Rx
            // DEBUG_PRINT("[QueueTaskRx]: receive the message[%d] from %s at %ld\n", ranging_msg->header.msgSequence, msg.sender_id, curTime);
            QueueTaskRx(&queueTaskLock, &full_info, sizeof(full_info));
        }
    }
    return NULL;
}

// for processing
void *process_messages(void *arg) {
    while (1) {
        #ifdef DYNAMIC_RANGING_FREQUENCY_ENABLE
            bool unSafe = processFromQueue(&queueTaskLock);
            /*
                unsafe -> set RANGING_PERIOD_LOW
                safe more than SAFE_DISTANCE_ROUND_BORDER -> set RANGING_PERIOD
            */
            if (unSafe) {
                RangingPeriod = RANGING_PERIOD_LOW;
                safeRoundCounter = 0;
                printf("Warning: Unsafe condition detected!\n");
            }
            else {
                safeRoundCounter++;
                if(safeRoundCounter == SAFE_DISTANCE_ROUND_BORDER) {
                    RangingPeriod = RANGING_PERIOD;
                }
            }
        #else
            processFromQueue(&queueTaskLock);
        #endif
        
        if(localSendSeqNumber % 10 == 1) {
            printRangingTableSet(RECEIVER);
        }

        local_sleep(10);                      
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
    localInit(string_to_hash(local_drone_id));
    DEBUG_PRINT("[localInit]: localHost is ready: droneId = %s, localAddress = %d, x = %d, y = %d, z = %d, randOffTime = %ld\n", 
        local_drone_id, localHost->localAddress, localHost->location.x, localHost->location.y, localHost->location.z, localHost->randOffTime);
    initQueueTaskLock(&queueTaskLock);     
    DEBUG_PRINT("[initQueueTaskLock]: QueueTaskLock is ready\n");
    initRangingTableSet();
    DEBUG_PRINT("[initRangingTableSet]: RangingTableSet is ready\n");

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

    int64_t worldBaseTime;
    ssize_t bytes_received = recv(center_socket, &worldBaseTime, sizeof(worldBaseTime), 0);
    if (bytes_received != sizeof(worldBaseTime)) {
        perror("Failed to receive timestamp");
        close(center_socket);
        return -1;
    }

    localHost->baseTime = worldBaseTime;

    srand((unsigned int)(get_current_milliseconds()));

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
        // DEBUG_PRINT("[QueueTaskTx]: send the message[%d] from %s at %ld\n", localSendSeqNumber, local_drone_id, getCurrentTime());
        Time_t time_delay = QueueTaskTx(&queueTaskLock, MESSAGE_SIZE, send_to_center, center_socket, local_drone_id);
        
        // if(localReceivedSeqNumber % 10 == 1) {
        //     printRangingTableSet(SENDER);
        // }

        local_sleep(time_delay); 
    }

    pthread_join(receive_thread, NULL);
    pthread_join(process_thread, NULL);
    close(center_socket);
    return 0;
}