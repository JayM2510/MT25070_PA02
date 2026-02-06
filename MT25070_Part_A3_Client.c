#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/time.h>

typedef struct {
    char ip[32];
    int port;
    size_t field_size;
    int duration;
} client_arg_t;

typedef struct {
    long long total_bytes;
    long long recv_count;
    long long total_latency_us;
} stats_t;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
stats_t global_stats = {0};

long long now_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

void *client_thread(void *arg) {
    client_arg_t *c = arg;

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in srv = {
        .sin_family = AF_INET,
        .sin_port = htons(c->port)
    };

    inet_pton(AF_INET, c->ip, &srv.sin_addr);
    connect(sock, (struct sockaddr *)&srv, sizeof(srv));

    char *buf = malloc(c->field_size);
    time_t start = time(NULL);

    long long local_bytes = 0;
    long long local_recvs = 0;
    long long local_latency = 0;

    while (time(NULL) - start < c->duration) {
        long long t1 = now_us();
        ssize_t r = recv(sock, buf, c->field_size, 0);
        long long t2 = now_us();

        if (r <= 0)
            break;

        local_bytes += r;
        local_recvs++;
        local_latency += (t2 - t1);
    }

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

    client_arg_t carg;
    snprintf(carg.ip, sizeof(carg.ip), "%s", argv[1]);
    carg.port = atoi(argv[2]);
    carg.field_size = atoi(argv[3]);
    carg.duration = atoi(argv[5]);

    for (int i = 0; i < threads; i++)
        pthread_create(&tids[i], NULL, client_thread, &carg);

    for (int i = 0; i < threads; i++)
        pthread_join(tids[i], NULL);

    double throughput_gbps =
        (global_stats.total_bytes * 8.0) /
        (carg.duration * 1e9);

    double avg_latency_us =
        (global_stats.recv_count > 0)
            ? (double)global_stats.total_latency_us / global_stats.recv_count
            : 0;

    printf("THROUGHPUT_GBPS=%.6f\n", throughput_gbps);
    printf("AVG_LATENCY_US=%.2f\n", avg_latency_us);

    return 0;
}
