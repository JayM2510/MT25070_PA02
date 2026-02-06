// This server accepts multiple concurrent client connections and uses one thread per client to send data continuously.
// Data is sent using the standard send() system call, which results in a user-space to kernel-space copy for each send.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include "MT25070_Part_A_Message.h"

// Maximum number of pending connections
#define BACKLOG 16

// Structure passed to each client handler thread.
// Stores the connected client socket and message size.
typedef struct {
    int client_fd;
    size_t field_size;
} client_arg_t;

// Each client connection is handled by a separate thread.
// The thread repeatedly sends message data to the client until the connection is closed or an error occurs.
void *client_handler(void *arg) {
    client_arg_t *carg = arg;

    // Create and initialize a message with multiple fields
    message_t *msg = create_message(carg->field_size);
    
    // Continuously send data to the connected client
    while (1) {
        for (int i = 0; i < NUM_FIELDS; i++) {

            // send() copies data from user space (msg->fields[i]) into kernel socket buffers before transmission.
            if (send(carg->client_fd,
                     msg->fields[i],
                     carg->field_size,
                     0) <= 0) {
                close(carg->client_fd);
                destroy_message(msg);
                free(carg);
                return NULL;
            }
        }
    }
}

// Sets up the TCP server, listens for incoming connections, and creates a detached thread for each connected client.
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <field_size>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    size_t field_size = atoi(argv[2]);
    
    // Create TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Server address configuration
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };
    
    // Bind socket to the specified port
    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));

    // Start listening for incoming connections
    listen(server_fd, BACKLOG);

    printf("[Server] Listening on port %d\n", port);
    
    // Accept and handle client connections indefinitely
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        
        // Allocate argument structure for the client thread
        client_arg_t *arg = malloc(sizeof(client_arg_t));
        arg->client_fd = client_fd;
        arg->field_size = field_size;
        
        // Create a detached thread for this client
        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);
    }
}
