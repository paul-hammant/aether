// PGO Training Workload - Simple Version
//
// This workload exercises the hot paths to collect profile data for
// Profile-Guided Optimization (PGO). Focuses on common code patterns
// without complex dependencies.

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define ITERATIONS 100000000  // 100M operations

// Simulate actor message dispatch pattern
typedef struct {
    int type;
    int value;
    void* data;
} Message;

// Hot path: biased branch prediction pattern
static uint64_t hot_path_dispatch(int iterations) {
    uint64_t sum = 0;
    Message msg;
    
    for (int i = 0; i < iterations; i++) {
        // Real-world distribution: 80% hot, 15% warm, 5% cold
        msg.type = (i % 100 < 80) ? 0 :   // Hot path (80%)
                   (i % 100 < 95) ? 1 :   // Warm path (15%)
                   2;                     // Cold path (5%)
        
        // Dispatch simulation
        switch (msg.type) {
            case 0: 
                sum += i * 2;           // Most common
                break;
            case 1: 
                sum += i * 3;           // Less common
                break;
            case 2: 
                sum += i * 5;           // Rare
                break;
        }
    }
    
    return sum;
}

// Memory allocation patterns
static uint64_t memory_pattern(int iterations) {
    uint64_t sum = 0;
    
    for (int i = 0; i < iterations; i++) {
        // Simulate allocation size distribution
        size_t size = (i % 100 < 70) ? 32 :     // 70% small
                      (i % 100 < 90) ? 128 :    // 20% medium
                      512;                      // 10% large
        
        void* ptr = malloc(size);
        if (ptr) {
            // Touch memory to ensure it's allocated
            memset(ptr, i & 0xFF, size);
            sum += ((char*)ptr)[0];
            free(ptr);
        }
    }
    
    return sum;
}

// String operations (common in compilers)
static uint64_t string_operations(int iterations) {
    uint64_t sum = 0;
    
    for (int i = 0; i < iterations; i++) {
        char buf1[64], buf2[64];
        snprintf(buf1, 64, "token_%d", i);
        snprintf(buf2, 64, "value_%d", i * 2);
        
        // String comparison (hot path in parsers)
        if (strcmp(buf1, buf2) == 0) {
            sum += 1;
        } else {
            sum += strlen(buf1) + strlen(buf2);
        }
    }
    
    return sum;
}

// Nested loop pattern (code generation)
static uint64_t nested_loops(int outer, int inner) {
    uint64_t sum = 0;
    
    for (int i = 0; i < outer; i++) {
        for (int j = 0; j < inner; j++) {
            // Simulate code generation work
            sum += i * j + (i ^ j);
        }
    }
    
    return sum;
}

// Main workload
int main() {
    printf("===========================================\n");
    printf("PGO TRAINING WORKLOAD\n");
    printf("===========================================\n");
    printf("Exercising hot paths for profile collection...\n\n");
    
    uint64_t total = 0;
    clock_t start, end;
    
    // 1. Message dispatch (50% of runtime)
    printf("1/4 Message dispatch simulation... ");
    fflush(stdout);
    start = clock();
    total += hot_path_dispatch(ITERATIONS);
    end = clock();
    printf("%.2fs ✓\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    // 2. Memory patterns (25% of runtime)
    printf("2/4 Memory allocation patterns...  ");
    fflush(stdout);
    start = clock();
    total += memory_pattern(ITERATIONS / 100);
    end = clock();
    printf("%.2fs ✓\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    // 3. String operations (15% of runtime)
    printf("3/4 String operations...           ");
    fflush(stdout);
    start = clock();
    total += string_operations(ITERATIONS / 100);
    end = clock();
    printf("%.2fs ✓\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    // 4. Nested loops (10% of runtime)
    printf("4/4 Nested loop patterns...        ");
    fflush(stdout);
    start = clock();
    total += nested_loops(10000, 1000);
    end = clock();
    printf("%.2fs ✓\n", (double)(end - start) / CLOCKS_PER_SEC);
    
    printf("\n===========================================\n");
    printf("PROFILE COLLECTION COMPLETE\n");
    printf("===========================================\n");
    printf("Checksum: %llu\n", total);
    printf("\nProfile data (.gcda files) generated.\n");
    printf("Run these benchmarks to see the improvement.\n");
    
    return 0;
}
