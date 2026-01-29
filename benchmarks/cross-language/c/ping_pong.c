// Pure C ping-pong benchmark with pthreads
// Baseline comparison showing raw pthread performance without Aether optimizations
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#define MESSAGES 10000000

typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int ready;
    int value;
} Channel;

Channel *chan_a, *chan_b;

void* ping_thread(void* arg) {
    for (int i = 0; i < MESSAGES; i++) {
        // Send to A
        pthread_mutex_lock(&chan_a->mutex);
        chan_a->value = i;
        chan_a->ready = 1;
        pthread_cond_signal(&chan_a->cond);
        pthread_mutex_unlock(&chan_a->mutex);

        // Wait for B
        pthread_mutex_lock(&chan_b->mutex);
        while (!chan_b->ready) {
            pthread_cond_wait(&chan_b->cond, &chan_b->mutex);
        }
        int received_value = chan_b->value;
        chan_b->ready = 0;
        pthread_mutex_unlock(&chan_b->mutex);

        // VALIDATE: Must receive echo of what we sent
        if (received_value != i) {
            fprintf(stderr, "ERROR: Ping sent %d but got back %d\n", i, received_value);
        }
    }
    return NULL;
}

void* pong_thread(void* arg) {
    for (int i = 0; i < MESSAGES; i++) {
        // Wait for A
        pthread_mutex_lock(&chan_a->mutex);
        while (!chan_a->ready) {
            pthread_cond_wait(&chan_a->cond, &chan_a->mutex);
        }
        int received_value = chan_a->value;
        chan_a->ready = 0;
        pthread_mutex_unlock(&chan_a->mutex);

        // VALIDATE: Must receive expected sequence
        if (received_value != i) {
            fprintf(stderr, "ERROR: Pong expected %d but got %d\n", i, received_value);
        }

        // Send to B - echo back what we received
        pthread_mutex_lock(&chan_b->mutex);
        chan_b->value = received_value;
        chan_b->ready = 1;
        pthread_cond_signal(&chan_b->cond);
        pthread_mutex_unlock(&chan_b->mutex);
    }
    return NULL;
}

static inline uint64_t rdtsc() {
#if defined(__x86_64__) || defined(__i386__)
    unsigned int lo, hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
#elif defined(__aarch64__) || defined(__arm__)
    // ARM doesn't have RDTSC, use clock_gettime
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
#else
    return 0;
#endif
}

int main() {
    chan_a = calloc(1, sizeof(Channel));
    chan_b = calloc(1, sizeof(Channel));

    pthread_mutex_init(&chan_a->mutex, NULL);
    pthread_cond_init(&chan_a->cond, NULL);
    pthread_mutex_init(&chan_b->mutex, NULL);
    pthread_cond_init(&chan_b->cond, NULL);

    printf("=== C (pthread) Ping-Pong Benchmark ===\n");
    printf("Messages: %d\n", MESSAGES);
    printf("Using pthread mutexes and condition variables\n\n");

    uint64_t start = rdtsc();

    pthread_t t1, t2;
    pthread_create(&t1, NULL, ping_thread, NULL);
    pthread_create(&t2, NULL, pong_thread, NULL);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    uint64_t end = rdtsc();
    uint64_t total_cycles = end - start;

#if defined(__x86_64__) || defined(__i386__)
    double cycles_per_msg = (double)total_cycles / MESSAGES;
    double freq = 3.0e9; // Approximate CPU frequency
    double throughput = freq / cycles_per_msg;

    printf("Cycles/msg:     %.2f\n", cycles_per_msg);
    printf("Throughput:     %.2f M msg/sec\n", throughput / 1e6);
#elif defined(__aarch64__) || defined(__arm__)
    // ARM: convert nanoseconds to throughput
    double ns_per_msg = (double)total_cycles / MESSAGES;
    double throughput = 1e9 / ns_per_msg;
    double cycles_per_msg = ns_per_msg * 3.0; // Approximate conversion at 3GHz

    printf("Cycles/msg:     %.2f\n", cycles_per_msg);
    printf("Throughput:     %.2f M msg/sec\n", throughput / 1e6);
#endif

    pthread_mutex_destroy(&chan_a->mutex);
    pthread_cond_destroy(&chan_a->cond);
    pthread_mutex_destroy(&chan_b->mutex);
    pthread_cond_destroy(&chan_b->cond);
    free(chan_a);
    free(chan_b);

    return 0;
}
