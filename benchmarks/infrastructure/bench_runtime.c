#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "actor_state_machine.h"

#define ITERATIONS 10000000

// Benchmark mailbox operations
double bench_mailbox() {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    Message msg = {1, 0, 42, NULL};
    Message recv_msg;
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        mailbox_send(&mbox, msg);
        mailbox_receive(&mbox, &recv_msg);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return ITERATIONS / seconds;
}

// Benchmark aligned allocation
double bench_allocation() {
    clock_t start = clock();
    
    void** ptrs = malloc(1000 * sizeof(void*));
    
    for (int i = 0; i < ITERATIONS / 1000; i++) {
        for (int j = 0; j < 1000; j++) {
            ptrs[j] = _aligned_malloc(256, 64);  // Windows-compatible
        }
        for (int j = 0; j < 1000; j++) {
            _aligned_free(ptrs[j]);
        }
    }
    
    free(ptrs);
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return ITERATIONS / seconds;
}

int main() {
    printf("=== Aether Runtime Benchmark ===\n\n");
    
    // Warmup
    printf("Warming up...\n");
    bench_mailbox();
    bench_allocation();
    
    printf("\nRunning benchmarks (3 runs each)...\n\n");
    
    // Mailbox benchmark
    double mailbox_total = 0;
    for (int i = 0; i < 3; i++) {
        double ops = bench_mailbox();
        mailbox_total += ops;
        printf("Mailbox ops/sec (run %d): %.2f M/sec\n", i+1, ops / 1000000.0);
    }
    double mailbox_avg = mailbox_total / 3.0;
    
    printf("\n");
    
    // Allocation benchmark
    double alloc_total = 0;
    for (int i = 0; i < 3; i++) {
        double ops = bench_allocation();
        alloc_total += ops;
        printf("Aligned alloc ops/sec (run %d): %.2f M/sec\n", i+1, ops / 1000000.0);
    }
    double alloc_avg = alloc_total / 3.0;
    
    printf("\n=== Results (Average) ===\n");
    printf("Mailbox throughput: %.2f M ops/sec\n", mailbox_avg / 1000000.0);
    printf("Allocation throughput: %.2f M ops/sec\n", alloc_avg / 1000000.0);
    
    // Save results
    FILE* f = fopen("benchmark_results.txt", "w");
    if (f) {
        fprintf(f, "mailbox_ops_per_sec=%.0f\n", mailbox_avg);
        fprintf(f, "allocation_ops_per_sec=%.0f\n", alloc_avg);
        fclose(f);
    }
    
    return 0;
}
