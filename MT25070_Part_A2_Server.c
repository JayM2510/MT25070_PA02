// This version uses sendmsg() with scatter-gather I/O to send multiple buffers in a single system call.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h> 
#include <sys/uio.h>    // For iovec and sendmsg()
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
// This thread sends message data using sendmsg() with an iovec array describing multiple user-space buffers.
void *client_handler(void *arg) {
    client_arg_t *carg = arg;

    // Create and initialize the message
    message_t *msg = create_message(carg->field_size);
    
    // iovec array describing multiple buffers.
    // Each entry points to a separate message field.
    struct iovec iov[NUM_FIELDS];

    // msghdr structure used by sendmsg(). It describes the scatter-gather buffers.
    struct msghdr mh;
    memset(&mh, 0, sizeof(mh));
    
    // Initialize each iovec entry
    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg->fields[i];    // Pointer to user-space buffer
        iov[i].iov_len  = carg->field_size;  // Length of the buffer
    }
    // Attach iovec array to message header
    mh.msg_iov = iov;
    mh.msg_iovlen = NUM_FIELDS;
    
    // Continuously send data to the client.
    // sendmsg() sends all buffers in a single system call.
    while (1) {
        ssize_t sent = sendmsg(carg->client_fd, &mh, 0);
        if (sent <= 0) {
            close(carg->client_fd);
            destroy_message(msg);
            free(carg);
            return NULL;
        }
    }
}

// Sets up the TCP server and handles each client connection using a separate detached thread.
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

    printf("[One-Copy Server] Listening on port %d\n", port);
    
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
