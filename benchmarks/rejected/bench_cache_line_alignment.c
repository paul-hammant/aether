// Benchmark: Cache Line Alignment to Prevent False Sharing
// Expected: 3-8% improvement in multi-threaded scenarios

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>

#define ITERATIONS 50000000
#define NUM_THREADS 4

// Baseline: No alignment (false sharing likely)
typedef struct {
    atomic_int counter1;
    atomic_int counter2;
    atomic_int counter3;
    atomic_int counter4;
} UnalignedCounters;

// Optimized: Cache-line aligned (64 bytes per counter)
typedef struct {
    atomic_int counter1;
    char pad1[60];
    atomic_int counter2;
    char pad2[60];
    atomic_int counter3;
    char pad3[60];
    atomic_int counter4;
    char pad4[60];
} __attribute__((aligned(64))) AlignedCounters;

// Thread function for unaligned test
void* worker_unaligned(void* arg) {
    UnalignedCounters* counters = (UnalignedCounters*)arg;
    int thread_id = *(int*)&arg;
    
    atomic_int* counter = NULL;
    switch (thread_id % NUM_THREADS) {
        case 0: counter = &counters->counter1; break;
        case 1: counter = &counters->counter2; break;
        case 2: counter = &counters->counter3; break;
        case 3: counter = &counters->counter4; break;
    }
    
    for (int i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }
    
    return NULL;
}

// Thread function for aligned test
void* worker_aligned(void* arg) {
    AlignedCounters* counters = (AlignedCounters*)arg;
    int thread_id = *(int*)&arg;
    
    atomic_int* counter = NULL;
    switch (thread_id % NUM_THREADS) {
        case 0: counter = &counters->counter1; break;
        case 1: counter = &counters->counter2; break;
        case 2: counter = &counters->counter3; break;
        case 3: counter = &counters->counter4; break;
    }
    
    for (int i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }
    
    return NULL;
}

double bench_unaligned() {
    UnalignedCounters counters = {0};
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_unaligned, 
                      (void*)(intptr_t)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double bench_aligned() {
    AlignedCounters counters = {0};
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_aligned, 
                      (void*)(intptr_t)i);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

int main() {
    printf("=== Cache Line Alignment Benchmark ===\n\n");
    
    printf("Test: %d threads, each incrementing separate counters\n", NUM_THREADS);
    printf("Iterations per thread: %d\n", ITERATIONS);
    printf("Cache line size: 64 bytes\n\n");
    
    printf("Structure sizes:\n");
    printf("  Unaligned: %zu bytes (all counters in same cache line)\n", 
           sizeof(UnalignedCounters));
    printf("  Aligned: %zu bytes (each counter on separate cache line)\n\n", 
           sizeof(AlignedCounters));
    
    // Warmup
    for (int i = 0; i < 2; i++) {
        bench_unaligned();
        bench_aligned();
    }
    
    // Baseline: unaligned (false sharing)
    printf("[Baseline] Unaligned counters (false sharing)...\n");
    double time_unaligned = bench_unaligned();
    printf("  Time: %.3f seconds\n", time_unaligned);
    double ops_unaligned = (NUM_THREADS * ITERATIONS) / time_unaligned;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_unaligned / 1e6);
    
    // Optimized: aligned (no false sharing)
    printf("[Optimized] Cache-line aligned counters...\n");
    double time_aligned = bench_aligned();
    printf("  Time: %.3f seconds\n", time_aligned);
    double ops_aligned = (NUM_THREADS * ITERATIONS) / time_aligned;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_aligned / 1e6);
    
    double speedup = time_unaligned / time_aligned;
    double improvement = ((speedup - 1.0) * 100.0);
    
    printf("[Results]\n");
    printf("  Speedup: %.2fx\n", speedup);
    printf("  Improvement: %+.1f%%\n\n", improvement);
    
    if (improvement >= 3.0) {
        printf("✓ SUCCESS: %.1f%% improvement (expected 3-8%%)\n", improvement);
        printf("  Cache line alignment eliminates false sharing\n");
        printf("  Impact scales with core count and contention\n");
        return 0;
    } else if (improvement >= 1.0) {
        printf("✓ MODEST: %.1f%% improvement (below 3%% threshold)\n", improvement);
        printf("  Some benefit, may be more significant on higher core counts\n");
        return 0;
    } else {
        printf("✗ NO BENEFIT: %.1f%% change\n", improvement);
        printf("  Either not enough contention or CPU cache coherency is very fast\n");
        return 1;
    }
}
