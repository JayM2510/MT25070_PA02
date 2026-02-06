// This header file defines the message structure used by all client and server implementations (two-copy, one-copy, zero-copy).
#ifndef MT25070_PART_A_MESSAGE_H
#define MT25070_PART_A_MESSAGE_H

#include <stdlib.h>        // For malloc() and free()
#include <string.h>        // For memset()

// NUM_FIELDS defines how many separate buffers are present in a single message. Using multiple fields allows us to 
// study scatter-gather I/O (sendmsg) behavior.

#define NUM_FIELDS 8

typedef struct {
    char *fields[NUM_FIELDS];
    size_t field_size;
} message_t;

// Allocates and initializes a message structure.
static inline message_t *create_message(size_t field_size) {

    // Allocate memory for the message structure itself
    message_t *msg = (message_t *)malloc(sizeof(message_t));

    // Store the field size for later use
    msg->field_size = field_size;
    
    // Allocate and initialize each message field
    for (int i = 0; i < NUM_FIELDS; i++) {
        msg->fields[i] = (char *)malloc(field_size);

        // Fill the buffer with a repeating character (e.g., 'A', 'B', 'C', ...) to simulate real data
        memset(msg->fields[i], 'A' + i, field_size);
    }
    return msg;
}


static inline void destroy_message(message_t *msg) {

    // Free each dynamically allocated field
    for (int i = 0; i < NUM_FIELDS; i++) {
        free(msg->fields[i]);
    }
    // Free the message structure itself
    free(msg);
}

#endif
