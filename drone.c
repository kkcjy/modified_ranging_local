#include "socket_frame.h"

const char *local_drone_id;

void send_to_center(int center_socket, const char *node_id, const char *message) {
    NodeMessage msg;
    strncpy(msg.sender_id, node_id, sizeof(msg.sender_id));
    msg.data_size = strlen(message) + 1;
    strncpy(msg.data, message, sizeof(msg.data));
    
    if (send(center_socket, &msg, sizeof(msg), 0) < 0) {
        perror("Send failed");
    }
}

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
            printf("[%s] %.*s\n", msg.sender_id, (int)msg.data_size, msg.data);
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

    // Start receive thread
    pthread_t receive_thread;
    if (pthread_create(&receive_thread, NULL, receive_from_center, &center_socket)) {
        perror("pthread_create");
        close(center_socket);
        return -1;
    }

    printf("Node %s connected to center\n", local_drone_id);

    // Main input loop
    char input[BUFFER_SIZE];
    while (1) {
        printf("%s> ", local_drone_id);
        fgets(input, BUFFER_SIZE, stdin);
        input[strcspn(input, "\n")] = 0;
        
        if (strcmp(input, "exit") == 0) break;
        if (strlen(input) == 0) continue;
        
        send_to_center(center_socket, local_drone_id, input);
    }

    close(center_socket);
    return 0;
}