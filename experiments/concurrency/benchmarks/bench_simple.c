// Simple benchmark - mailbox comparison only
#include <stdio.h>
#include <time.h>
#include "runtime/actors/actor_state_machine.h"
#include "runtime/actors/lockfree_mailbox.h"

#define ITERATIONS 1000000

int main() {
    Message msg = {1, 0, 42, NULL};
    Message recv;
    clock_t start, end;
    double simple_ops, lockfree_ops;
    
    // Test 1: Simple mailbox
    Mailbox simple;
    mailbox_init(&simple);
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        mailbox_send(&simple, msg);
        mailbox_receive(&simple, &recv);
    }
    end = clock();
    simple_ops = ITERATIONS / ((double)(end - start) / CLOCKS_PER_SEC);
    
    // Test 2: Lock-free mailbox
    LockFreeMailbox lockfree;
    lockfree_mailbox_init(&lockfree);
    start = clock();
    for (int i = 0; i < ITERATIONS; i++) {
        lockfree_mailbox_send(&lockfree, msg);
        lockfree_mailbox_receive(&lockfree, &recv);
    }
    end = clock();
    lockfree_ops = ITERATIONS / ((double)(end - start) / CLOCKS_PER_SEC);
    
    printf("=================================\n");
    printf("  Mailbox Performance Benchmark\n");
    printf("=================================\n\n");
    printf("Simple mailbox:    %.2f M ops/sec\n", simple_ops / 1e6);
    printf("Lock-free mailbox: %.2f M ops/sec\n", lockfree_ops / 1e6);
    printf("\nSpeedup: %.2fx\n", lockfree_ops / simple_ops);
    printf("=================================\n");
    
    return 0;
}
