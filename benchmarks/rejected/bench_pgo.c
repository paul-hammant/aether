// Benchmark: Profile-Guided Optimization Impact
//
// Measures the performance improvement from PGO by running the same
// workload as the training set.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

#define BENCH_ITERATIONS 100000000  // 100M operations

// Message type
typedef struct {
    int type;
    int value;
    void* data;
} Message;

// Timing utility
static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

// Hot path: message dispatch with biased distribution
static uint64_t bench_dispatch() {
    uint64_t sum = 0;
    Message msg;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        // Same distribution as training: 80% hot, 15% warm, 5% cold
        msg.type = (i % 100 < 80) ? 0 :
                   (i % 100 < 95) ? 1 :
                   2;
        
        switch (msg.type) {
            case 0: sum += i * 2; break;
            case 1: sum += i * 3; break;
            case 2: sum += i * 5; break;
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = BENCH_ITERATIONS / elapsed / 1e6;
    
    printf("Message Dispatch:  %.2f M ops/sec (checksum: %llu)\n", ops_per_sec, sum);
    return (uint64_t)ops_per_sec;
}

// Memory allocation benchmark
static uint64_t bench_memory() {
    uint64_t sum = 0;
    int alloc_count = BENCH_ITERATIONS / 100;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < alloc_count; i++) {
        size_t size = (i % 100 < 70) ? 32 :
                      (i % 100 < 90) ? 128 :
                      512;
        
        void* ptr = malloc(size);
        if (ptr) {
            memset(ptr, i & 0xFF, size);
            sum += ((char*)ptr)[0];
            free(ptr);
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = alloc_count / elapsed / 1e6;
    
    printf("Memory Allocation: %.2f M ops/sec (checksum: %llu)\n", ops_per_sec, sum);
    return (uint64_t)ops_per_sec;
}

// String operations benchmark
static uint64_t bench_strings() {
    uint64_t sum = 0;
    int iter_count = BENCH_ITERATIONS / 100;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iter_count; i++) {
        char buf1[64], buf2[64];
        snprintf(buf1, 64, "token_%d", i);
        snprintf(buf2, 64, "value_%d", i * 2);
        
        if (strcmp(buf1, buf2) == 0) {
            sum += 1;
        } else {
            sum += strlen(buf1) + strlen(buf2);
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = iter_count / elapsed / 1e6;
    
    printf("String Operations: %.2f M ops/sec (checksum: %llu)\n", ops_per_sec, sum);
    return (uint64_t)ops_per_sec;
}

// Nested loops benchmark
static uint64_t bench_nested() {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < 10000; i++) {
        for (int j = 0; j < 1000; j++) {
            sum += i * j + (i ^ j);
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = 10000000.0 / elapsed / 1e6;
    
    printf("Nested Loops:      %.2f M ops/sec (checksum: %llu)\n", ops_per_sec, sum);
    return (uint64_t)ops_per_sec;
}

int main() {
    printf("===========================================\n");
    printf("PGO PERFORMANCE BENCHMARK\n");
    printf("===========================================\n");
#ifdef __PGO__
    printf("Build: PGO-Optimized (profile-guided)\n");
#else
    printf("Build: Standard -O3\n");
#endif
    printf("Operations: 100M\n");
    printf("\n");
    
    // Warmup
    for (int i = 0; i < 10000000; i++) {
        volatile int x = i * 2;
        (void)x;
    }
    
    printf("Running benchmarks...\n\n");
    
    uint64_t score = 0;
    
    // Run all benchmarks
    score += bench_dispatch();
    score += bench_memory();
    score += bench_strings();
    score += bench_nested();
    
    printf("\n===========================================\n");
    printf("TOTAL PERFORMANCE SCORE: %llu\n", score);
    printf("===========================================\n");
    
    return 0;
}
