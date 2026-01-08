// Lock-Free Mailbox Benchmark
// Compares mutex-based vs atomic lock-free performance

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include "../runtime/aether_lockfree_mailbox.h"
#include "../runtime/actor_state_machine.h"

#define MESSAGES_PER_THREAD 10000000
#define WARMUP_ITERATIONS 100000

double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}

// Lock-free mailbox test
typedef struct {
    LockFreeMailbox* mbox;
    int thread_id;
    uint64_t* sent_count;
    uint64_t* received_count;
} LockFreeThreadArgs;

void* lockfree_producer(void* arg) {
    LockFreeThreadArgs* args = (LockFreeThreadArgs*)arg;
    Message msg = {1, args->thread_id, 42, NULL};
    
    for (int i = 0; i < MESSAGES_PER_THREAD; i++) {
        while (!lockfree_mailbox_send(args->mbox, msg)) {
            // Spin (busy wait)
        }
        (*args->sent_count)++;
    }
    return NULL;
}

void* lockfree_consumer(void* arg) {
    LockFreeThreadArgs* args = (LockFreeThreadArgs*)arg;
    Message msg;
    
    while (*args->received_count < MESSAGES_PER_THREAD) {
        if (lockfree_mailbox_receive(args->mbox, &msg)) {
            (*args->received_count)++;
        }
    }
    return NULL;
}

// Mutex-based mailbox test
typedef struct {
    Mailbox* mbox;
    int thread_id;
    uint64_t* sent_count;
    uint64_t* received_count;
} MutexThreadArgs;

void* mutex_producer(void* arg) {
    MutexThreadArgs* args = (MutexThreadArgs*)arg;
    Message msg = {1, args->thread_id, 42, NULL};
    
    for (int i = 0; i < MESSAGES_PER_THREAD; i++) {
        while (!mailbox_send(args->mbox, msg)) {
            // Spin
        }
        (*args->sent_count)++;
    }
    return NULL;
}

void* mutex_consumer(void* arg) {
    MutexThreadArgs* args = (MutexThreadArgs*)arg;
    Message msg;
    
    while (*args->received_count < MESSAGES_PER_THREAD) {
        if (mailbox_receive(args->mbox, &msg)) {
            (*args->received_count)++;
        }
    }
    return NULL;
}

double benchmark_lockfree() {
    LockFreeMailbox mbox;
    lockfree_mailbox_init(&mbox);
    
    uint64_t sent = 0;
    uint64_t received = 0;
    
    LockFreeThreadArgs producer_args = {&mbox, 1, &sent, &received};
    LockFreeThreadArgs consumer_args = {&mbox, 2, &sent, &received};
    
    pthread_t producer_thread, consumer_thread;
    
    double start = get_time_seconds();
    
    pthread_create(&producer_thread, NULL, lockfree_producer, &producer_args);
    pthread_create(&consumer_thread, NULL, lockfree_consumer, &consumer_args);
    
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    double elapsed = get_time_seconds() - start;
    
    return elapsed;
}

double benchmark_mutex() {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    uint64_t sent = 0;
    uint64_t received = 0;
    
    MutexThreadArgs producer_args = {&mbox, 1, &sent, &received};
    MutexThreadArgs consumer_args = {&mbox, 2, &sent, &received};
    
    pthread_t producer_thread, consumer_thread;
    
    double start = get_time_seconds();
    
    pthread_create(&producer_thread, NULL, mutex_producer, &producer_args);
    pthread_create(&consumer_thread, NULL, mutex_consumer, &consumer_args);
    
    pthread_join(producer_thread, NULL);
    pthread_join(consumer_thread, NULL);
    
    double elapsed = get_time_seconds() - start;
    
    return elapsed;
}

int main() {
    printf("=============================================\n");
    printf("  Lock-Free vs Mutex Mailbox Benchmark\n");
    printf("=============================================\n\n");
    
    printf("Messages per thread: %d\n", MESSAGES_PER_THREAD);
    printf("Running benchmarks...\n\n");
    
    // Warm up
    printf("Warming up...\n");
    LockFreeMailbox warmup_mbox;
    lockfree_mailbox_init(&warmup_mbox);
    Message msg = {1, 0, 42, NULL};
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        lockfree_mailbox_send(&warmup_mbox, msg);
        lockfree_mailbox_receive(&warmup_mbox, &msg);
    }
    
    // Benchmark lock-free
    printf("Benchmarking lock-free mailbox...\n");
    double lockfree_time = benchmark_lockfree();
    double lockfree_throughput = MESSAGES_PER_THREAD / lockfree_time / 1e6;
    
    // Benchmark mutex
    printf("Benchmarking mutex mailbox...\n");
    double mutex_time = benchmark_mutex();
    double mutex_throughput = MESSAGES_PER_THREAD / mutex_time / 1e6;
    
    printf("\n");
    printf("Results:\n");
    printf("--------\n");
    printf("Lock-Free:\n");
    printf("  Time:       %.3f seconds\n", lockfree_time);
    printf("  Throughput: %.2f M msg/sec\n", lockfree_throughput);
    printf("\n");
    printf("Mutex-Based:\n");
    printf("  Time:       %.3f seconds\n", mutex_time);
    printf("  Throughput: %.2f M msg/sec\n", mutex_throughput);
    printf("\n");
    printf("Speedup: %.2fx\n", lockfree_throughput / mutex_throughput);
    printf("\n");
    printf("Expected: 1.5-2x faster with lock-free\n");
    
    return 0;
}
