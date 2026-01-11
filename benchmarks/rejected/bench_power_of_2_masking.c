// Benchmark: Power-of-2 Ring Buffer Masking vs Modulo
// Expected: 5-10% improvement from replacing modulo with bitwise AND

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "../../runtime/actors/actor_state_machine.h"

#define ITERATIONS 100000000
#define RING_SIZE 16
#define RING_MASK (RING_SIZE - 1)

// Baseline: modulo-based indexing
static inline int advance_modulo(int index) {
    return (index + 1) % RING_SIZE;
}

// Optimized: power-of-2 masking
static inline int advance_mask(int index) {
    return (index + 1) & RING_MASK;
}

// Simulate ring buffer operations with modulo
double bench_modulo() {
    int head = 0, tail = 0;
    int data[RING_SIZE] = {0};
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Write
        data[tail] = i;
        tail = advance_modulo(tail);
        
        // Read
        volatile int val = data[head];
        head = advance_modulo(head);
        (void)val;
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

// Simulate ring buffer operations with masking
double bench_masking() {
    int head = 0, tail = 0;
    int data[RING_SIZE] = {0};
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Write
        data[tail] = i;
        tail = advance_mask(tail);
        
        // Read
        volatile int val = data[head];
        head = advance_mask(head);
        (void)val;
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

// Test with actual mailbox structure
double bench_mailbox_modulo() {
    Mailbox mbox;
    mailbox_init(&mbox);
    Message msg = {.type = 1, .payload_int = 42};
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS / 10; i++) {
        // Fill mailbox
        for (int j = 0; j < MAILBOX_SIZE - 1; j++) {
            mailbox_send(&mbox, msg);
        }
        
        // Drain mailbox
        Message recv_msg;
        for (int j = 0; j < MAILBOX_SIZE - 1; j++) {
            mailbox_receive(&mbox, &recv_msg);
        }
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

int main() {
    printf("=== Power-of-2 Ring Buffer Masking Benchmark ===\n\n");
    
    printf("Ring buffer size: %d (power of 2)\n", RING_SIZE);
    printf("Iterations: %d\n\n", ITERATIONS);
    
    // Warmup
    for (int i = 0; i < 3; i++) {
        bench_modulo();
        bench_masking();
    }
    
    // Baseline: modulo
    printf("[Baseline] Modulo-based indexing...\n");
    double time_modulo = bench_modulo();
    printf("  Time: %.3f seconds\n", time_modulo);
    double ops_modulo = (ITERATIONS * 2) / time_modulo;  // 2 ops per iteration (read + write)
    printf("  Throughput: %.2f M ops/sec\n\n", ops_modulo / 1e6);
    
    // Optimized: masking
    printf("[Optimized] Power-of-2 masking...\n");
    double time_mask = bench_masking();
    printf("  Time: %.3f seconds\n", time_mask);
    double ops_mask = (ITERATIONS * 2) / time_mask;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_mask / 1e6);
    
    double speedup = time_modulo / time_mask;
    double improvement = ((speedup - 1.0) * 100.0);
    
    printf("[Results]\n");
    printf("  Speedup: %.2fx\n", speedup);
    printf("  Improvement: %+.1f%%\n\n", improvement);
    
    // Test with actual mailbox
    printf("[Integration Test] Mailbox operations...\n");
    double time_mailbox = bench_mailbox_modulo();
    printf("  Time: %.3f seconds\n", time_mailbox);
    printf("  Operations: %d sends + receives\n\n", (ITERATIONS / 10) * (MAILBOX_SIZE - 1) * 2);
    
    if (improvement >= 3.0) {
        printf("✓ SUCCESS: %.1f%% improvement (expected 5-10%%)\n", improvement);
        printf("  Bitwise AND is significantly faster than modulo\n");
        return 0;
    } else if (improvement >= 1.0) {
        printf("✓ MODEST: %.1f%% improvement (below 5%% threshold)\n", improvement);
        printf("  Some benefit, but compiler may already optimize modulo for power-of-2\n");
        return 0;
    } else {
        printf("✗ NO BENEFIT: %.1f%% change\n", improvement);
        printf("  Modern compilers optimize power-of-2 modulo to masking automatically\n");
        return 1;
    }
}
