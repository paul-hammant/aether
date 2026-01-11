// Benchmark: NUMA-Aware Actor Allocation
// Expected: 20-30% improvement on multi-socket systems

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include "../../runtime/scheduler/aether_numa.h"
#include "../../runtime/actors/actor_state_machine.h"

#define ITERATIONS 10000000
#define NUM_THREADS 8
#define ACTOR_SIZE 4096

// Baseline: Regular malloc (cross-NUMA access likely)
void* worker_baseline(void* arg) {
    int tid = *(int*)arg;
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS / NUM_THREADS; i++) {
        void* actor = malloc(ACTOR_SIZE);
        if (actor) {
            // Simulate work: write to memory
            memset(actor, tid, ACTOR_SIZE);
            volatile char val = ((char*)actor)[0];
            (void)val;
            free(actor);
        }
    }
    
    clock_t end = clock();
    double *result = malloc(sizeof(double));
    *result = ((double)(end - start)) / CLOCKS_PER_SEC;
    return result;
}

// Optimized: NUMA-aware allocation (local node)
void* worker_numa(void* arg) {
    int tid = *(int*)arg;
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS / NUM_THREADS; i++) {
        void* actor = numa_alloc_local(ACTOR_SIZE);
        if (actor) {
            // Simulate work: write to memory
            memset(actor, tid, ACTOR_SIZE);
            volatile char val = ((char*)actor)[0];
            (void)val;
            numa_free(actor, ACTOR_SIZE);
        }
    }
    
    clock_t end = clock();
    double *result = malloc(sizeof(double));
    *result = ((double)(end - start)) / CLOCKS_PER_SEC;
    return result;
}

double bench_baseline() {
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    double total_time = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, worker_baseline, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        double *thread_time;
        pthread_join(threads[i], (void**)&thread_time);
        if (total_time < *thread_time) {
            total_time = *thread_time;
        }
        free(thread_time);
    }
    
    return total_time;
}

double bench_numa() {
    pthread_t threads[NUM_THREADS];
    int tids[NUM_THREADS];
    double total_time = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        tids[i] = i;
        pthread_create(&threads[i], NULL, worker_numa, &tids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        double *thread_time;
        pthread_join(threads[i], (void**)&thread_time);
        if (total_time < *thread_time) {
            total_time = *thread_time;
        }
        free(thread_time);
    }
    
    return total_time;
}

int main() {
    printf("=== NUMA-Aware Actor Allocation Benchmark ===\n\n");
    
    // Initialize NUMA
    NumaInfo numa = numa_init();
    
    if (!numa.numa_available) {
        printf("NUMA not available on this system.\n");
        printf("This optimization only benefits multi-socket systems.\n");
        return 0;
    }
    
    printf("NUMA nodes: %d\n", numa.num_nodes);
    printf("Threads: %d\n", NUM_THREADS);
    printf("Actor size: %d bytes\n", ACTOR_SIZE);
    printf("Iterations: %d per thread\n\n", ITERATIONS / NUM_THREADS);
    
    if (numa.num_nodes < 2) {
        printf("Only 1 NUMA node detected.\n");
        printf("NUMA optimization requires multi-socket system.\n");
        printf("Benchmarking anyway for demonstration...\n\n");
    }
    
    // Warmup
    for (int i = 0; i < 2; i++) {
        bench_baseline();
        bench_numa();
    }
    
    // Baseline: Regular malloc
    printf("[Baseline] Regular malloc (cross-NUMA possible)...\n");
    double time_baseline = bench_baseline();
    printf("  Time: %.3f seconds\n", time_baseline);
    double ops_baseline = ITERATIONS / time_baseline;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_baseline / 1e6);
    
    // Optimized: NUMA-aware
    printf("[Optimized] NUMA-aware local allocation...\n");
    double time_numa = bench_numa();
    printf("  Time: %.3f seconds\n", time_numa);
    double ops_numa = ITERATIONS / time_numa;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_numa / 1e6);
    
    double speedup = time_baseline / time_numa;
    double improvement = ((speedup - 1.0) * 100.0);
    
    printf("[Results]\n");
    printf("  Speedup: %.2fx\n", speedup);
    printf("  Improvement: %+.1f%%\n\n", improvement);
    
    if (numa.num_nodes >= 2) {
        if (improvement >= 15.0) {
            printf("SUCCESS: %.1f%% improvement on %d-socket system\n", 
                   improvement, numa.num_nodes);
            printf("  NUMA-aware allocation reduces cross-node latency\n");
            printf("  Benefit scales with NUMA distance and memory pressure\n");
            return 0;
        } else if (improvement >= 5.0) {
            printf("MODEST: %.1f%% improvement\n", improvement);
            printf("  Some benefit, less than expected for multi-socket\n");
            printf("  May need higher memory pressure to see full benefit\n");
            return 0;
        }
    }
    
    printf("NO SIGNIFICANT BENEFIT: %.1f%% change\n", improvement);
    if (numa.num_nodes < 2) {
        printf("  Expected: Single-socket system, NUMA not beneficial\n");
    } else {
        printf("  NUMA may not be significant bottleneck for this workload\n");
    }
    return 1;
}
