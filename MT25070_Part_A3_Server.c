// This version uses sendmsg() with the MSG_ZEROCOPY flag to avoid copying data into kernel socket buffers.
// Data is transmitted directly from user-space memory using DMA, reducing CPU usage and cache pollution.

#define _GNU_SOURCE     // Required for MSG_ZEROCOPY support
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#include <sys/socket.h>     // For socket options
#include <linux/errqueue.h>     // For MSG_ERRQUEUE
#include "MT25070_Part_A_Message.h"

// Maximum number of pending connections
#define BACKLOG 16

// Structure passed to each client handler thread.
// Stores the connected client socket and message size.
typedef struct {
    int client_fd;
    size_t field_size;
} client_arg_t;

// Zero-copy sends complete asynchronously. The kernel notifies completion via the socket error queue.
// This function drains the error queue to acknowledge completed zero-copy transmissions.
void drain_error_queue(int sock) {
    char ctrl[256];
    struct msghdr msg;
    struct iovec iov;
    char data;
    
    // Dummy buffer required by recvmsg() 
    iov.iov_base = &data;
    iov.iov_len = 1;

    memset(&msg, 0, sizeof(msg));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = ctrl;
    msg.msg_controllen = sizeof(ctrl);

    // Drain all available error queue messages
    while (recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) > 0) {}
}


// Handles a single client connection using zero-copy sends.
// Data buffers are passed directly to the kernel without copying. 
void *client_handler(void *arg) {
    client_arg_t *carg = arg;

    // Create and initialize message buffers
    message_t *msg = create_message(carg->field_size);
    
    // Scatter-gather buffers
    struct iovec iov[NUM_FIELDS];
    struct msghdr mh;

    memset(&mh, 0, sizeof(mh));
    
    // Initialize iovec entries
    for (int i = 0; i < NUM_FIELDS; i++) {
        iov[i].iov_base = msg->fields[i];
        iov[i].iov_len  = carg->field_size;
    }

    mh.msg_iov = iov;
    mh.msg_iovlen = NUM_FIELDS;
    
    // Continuously send data using MSG_ZEROCOPY.
    // The kernel pins user-space pages and NIC reads them directly via DMA.
    while (1) {
        ssize_t ret = sendmsg(carg->client_fd, &mh, MSG_ZEROCOPY);
        if (ret <= 0) {
            close(carg->client_fd);
            destroy_message(msg);
            free(carg);
            return NULL;
        }
        // Drain the error queue to receive zero-copy completion notifications.
        drain_error_queue(carg->client_fd);
    }
}

// Sets up a TCP server with zero-copy enabled. Each client is handled by a detached thread.
int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <port> <field_size>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    size_t field_size = atoi(argv[2]);
    
    // Create TCP socket 
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    
    // Enable zero-copy support on the server socket.
    int enable = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));
    
    // Server address configuration 
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(server_fd, BACKLOG);

    printf("[Zero-Copy Server] Listening on port %d\n", port);
    
    // Accept and handle client connections indefinitely
    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        
        // Enable zero-copy on the client socket as well
        setsockopt(client_fd, SOL_SOCKET, SO_ZEROCOPY, &enable, sizeof(enable));

        client_arg_t *arg = malloc(sizeof(client_arg_t));
        arg->client_fd = client_fd;
        arg->field_size = field_size;

        pthread_t tid;
        pthread_create(&tid, NULL, client_handler, arg);
        pthread_detach(tid);
    }
}
