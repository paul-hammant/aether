#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>
#include "runtime/actors/actor_state_machine.h"
#include "runtime/actors/lockfree_mailbox.h"

#define MESSAGES_PER_PAIR 1000000
#define NUM_PAIRS 4

typedef struct {
    int pair_id;
    void* mailbox;
    atomic_int* done;
} PairArgs;

void* simple_pair(void* arg) {
    PairArgs* args = (PairArgs*)arg;
    Mailbox* mbox = (Mailbox*)args->mailbox;
    Message msg = {1, args->pair_id, 0, NULL};
    Message recv_msg;
    
    for (int i = 0; i < MESSAGES_PER_PAIR; i++) {
        msg.payload_int = i;
        while (!mailbox_send(mbox, msg));
        while (!mailbox_receive(mbox, &recv_msg));
    }
    
    atomic_store(args->done, 1);
    return NULL;
}

void* lockfree_pair(void* arg) {
    PairArgs* args = (PairArgs*)arg;
    LockFreeMailbox* mbox = (LockFreeMailbox*)args->mailbox;
    Message msg = {1, args->pair_id, 0, NULL};
    Message recv_msg;
    
    for (int i = 0; i < MESSAGES_PER_PAIR; i++) {
        msg.payload_int = i;
        while (!lockfree_mailbox_send(mbox, msg));
        while (!lockfree_mailbox_receive(mbox, &recv_msg));
    }
    
    atomic_store(args->done, 1);
    return NULL;
}

double benchmark_simple_multicore() {
    Mailbox mailboxes[NUM_PAIRS];
    pthread_t threads[NUM_PAIRS];
    PairArgs args[NUM_PAIRS];
    atomic_int done[NUM_PAIRS];
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        mailbox_init(&mailboxes[i]);
        atomic_init(&done[i], 0);
        args[i].pair_id = i;
        args[i].mailbox = &mailboxes[i];
        args[i].done = &done[i];
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        pthread_create(&threads[i], NULL, simple_pair, &args[i]);
    }
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    return (MESSAGES_PER_PAIR * NUM_PAIRS * 2) / elapsed;
}

double benchmark_lockfree_multicore() {
    LockFreeMailbox mailboxes[NUM_PAIRS];
    pthread_t threads[NUM_PAIRS];
    PairArgs args[NUM_PAIRS];
    atomic_int done[NUM_PAIRS];
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        lockfree_mailbox_init(&mailboxes[i]);
        atomic_init(&done[i], 0);
        args[i].pair_id = i;
        args[i].mailbox = &mailboxes[i];
        args[i].done = &done[i];
    }
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        pthread_create(&threads[i], NULL, lockfree_pair, &args[i]);
    }
    
    for (int i = 0; i < NUM_PAIRS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    
    return (MESSAGES_PER_PAIR * NUM_PAIRS * 2) / elapsed;
}

int main() {
    printf("Multi-Core Mailbox Benchmark\n");
    printf("Test: %d concurrent threads, each doing send/receive pairs\n", NUM_PAIRS);
    printf("Operations: %d per thread\n", MESSAGES_PER_PAIR);
    printf("Total messages: %d\n\n", MESSAGES_PER_PAIR * NUM_PAIRS * 2);
    
    printf("Testing simple mailbox...\n");
    double simple_ops = benchmark_simple_multicore();
    
    printf("Testing lock-free mailbox...\n");
    double lockfree_ops = benchmark_lockfree_multicore();
    
    printf("\nResults:\n");
    printf("  Simple:    %.2f M msg/sec\n", simple_ops / 1e6);
    printf("  Lock-free: %.2f M msg/sec\n", lockfree_ops / 1e6);
    printf("  Speedup:   %.2fx\n\n", lockfree_ops / simple_ops);
    
    if (lockfree_ops > simple_ops * 1.1) {
        printf("Lock-free IS faster under multi-core load\n");
    } else if (lockfree_ops < simple_ops * 0.9) {
        printf("Lock-free is SLOWER (atomics overhead)\n");
    } else {
        printf("No significant difference (within 10%%)\n");
    }
    
    return 0;
}
