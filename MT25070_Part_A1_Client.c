#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>   // For socket APIs
#include <time.h>
#include <sys/time.h>    // For gettimeofday()


// Structure to pass common parameters to each client thread. 
// All threads connect to the same server but run independently.
typedef struct {
    char ip[32];          // Server IP address
    int port;             // Server port number
    size_t field_size;    // Size of data to receive per recv() call
    int duration;         
} client_arg_t;


// Structure to store aggregated statistics across all threads.
typedef struct {
    long long total_bytes;        // Total bytes received
    long long recv_count;         // Number of recv() calls
    long long total_latency_us;   // Sum of per-recv latency (microseconds)
} stats_t;

// Mutex to protect shared statistics 
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Global statistics structure
stats_t global_stats = {0};

// Returns the current time in microseconds. Used for measuring per-recv latency.
long long now_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void *client_thread(void *arg) {
    client_arg_t *c = arg;

    // Create TCP socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    // Server address structure
    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port = htons(c->port)
    };

    // Convert IP address from string to binary form
    inet_pton(AF_INET, c->ip, &srv.sin_addr);

    // Connect to the server
    connect(sock, (struct sockaddr *)&srv, sizeof(srv));
    
    // Allocate receive buffer
    char *buf = malloc(c->field_size);

    // Record experiment start time
    time_t start = time(NULL);
    
    // Thread-local statistics
    long long local_bytes = 0;
    long long local_recvs = 0;
    long long local_latency = 0;

    // Receive data until duration expires
    while (time(NULL) - start < c->duration) {
        long long t1 = now_us();

        // Receive data from server
        ssize_t r = recv(sock, buf, c->field_size, 0);
        long long t2 = now_us();  // End timestamp

        if (r <= 0)
            break;
        
        // Update thread-local statistics
        local_bytes += r;
        local_recvs++;
        local_latency += (t2 - t1);
    }
    // Safely update global statistics
    pthread_mutex_lock(&lock);
    global_stats.total_bytes += local_bytes;
    global_stats.recv_count += local_recvs;
    global_stats.total_latency_us += local_latency;
    pthread_mutex_unlock(&lock);

    free(buf);
    close(sock);
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr,
                "Usage: %s <server_ip> <port> <field_size> <threads> <duration>\n",
                argv[0]);
        exit(1);
    }

    int threads = atoi(argv[4]);
    pthread_t tids[threads];
    
    // Populate common client arguments
    client_arg_t carg;
    snprintf(carg.ip, sizeof(carg.ip), "%s", argv[1]);
    carg.port = atoi(argv[2]);
    carg.field_size = atoi(argv[3]);
    carg.duration = atoi(argv[5]);
    
    // Create client threads
    for (int i = 0; i < threads; i++)
        pthread_create(&tids[i], NULL, client_thread, &carg);
    
    // Wait for all threads to finish 
    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);
    
    // Compute Throughput 
    double throughput_gbps =
        (global_stats.total_bytes * 8.0) /
        (carg.duration * 1e9);
    
    // Compute average latency per recv() call
    double avg_latency_us =
        (global_stats.recv_count > 0)
            ? (double)global_stats.total_latency_us / global_stats.recv_count
            : 0;

    printf("THROUGHPUT_GBPS=%.6f\n", throughput_gbps);
    printf("AVG_LATENCY_US=%.2f\n", avg_latency_us);

    return 0;
}
