// Benchmark: Type-Specific Memory Pools vs General Pools
//
// Compares allocation performance of:
// 1. Type-specific pools (zero-branch, direct indexing)
// 2. General-purpose pools (size-based bucketing)
// 3. malloc/free (baseline)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>

// Include the type pool infrastructure
#include "../../runtime/memory/aether_type_pools.h"

#define ITERATIONS 10000000  // 10M allocations (Windows timing is coarser)

// Test message types
typedef struct {
    int _message_id;
    int value;
} IncrementMessage;

typedef struct {
    int _message_id;
    int value;
} DecrementMessage;

typedef struct {
    int _message_id;
    int active;
    int count;
} StatusMessage;

// Generate type-specific pools
DECLARE_TYPE_POOL(IncrementMessage)
DECLARE_TLS_POOL(IncrementMessage)

DECLARE_TYPE_POOL(DecrementMessage)
DECLARE_TLS_POOL(DecrementMessage)

DECLARE_TYPE_POOL(StatusMessage)
DECLARE_TLS_POOL(StatusMessage)

// Timing utility (Windows-compatible)
#ifdef _WIN32
#include <windows.h>
static inline uint64_t get_nanos() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000000ULL) / freq.QuadPart;
}
#else
static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

// Prevent compiler from optimizing away allocations
static void __attribute__((noinline)) escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

// Benchmark 1: Type-specific pools (zero-branch)
double bench_type_specific_pools() {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // 60% Increment, 30% Decrement, 10% Status
        if (i % 10 < 6) {
            IncrementMessage* msg = IncrementMessage_alloc();
            if (msg) {
                msg->value = i;
                sum += msg->value;
                escape(msg);  // Prevent optimization
                IncrementMessage_free(msg);
            }
        } else if (i % 10 < 9) {
            DecrementMessage* msg = DecrementMessage_alloc();
            if (msg) {
                msg->value = i;
                sum += msg->value;
                escape(msg);  // Prevent optimization
                DecrementMessage_free(msg);
            }
        } else {
            StatusMessage* msg = StatusMessage_alloc();
            if (msg) {
                msg->count = i;
                sum += msg->count;
                escape(msg);  // Prevent optimization
                StatusMessage_free(msg);
            }
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    printf("Type-Specific Pools: %.2f M alloc/sec (checksum: %llu)\n", ops_per_sec, sum);
    return ops_per_sec;
}

// Benchmark 2: General-purpose allocator (malloc/free)
double bench_malloc() {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Same distribution
        if (i % 10 < 6) {
            IncrementMessage* msg = (IncrementMessage*)malloc(sizeof(IncrementMessage));
            if (msg) {
                msg->value = i;
                sum += msg->value;
                escape(msg);  // Prevent optimization
                free(msg);
            }
        } else if (i % 10 < 9) {
            DecrementMessage* msg = (DecrementMessage*)malloc(sizeof(DecrementMessage));
            if (msg) {
                msg->value = i;
                sum += msg->value;
                escape(msg);  // Prevent optimization
                free(msg);
            }
        } else {
            StatusMessage* msg = (StatusMessage*)malloc(sizeof(StatusMessage));
            if (msg) {
                msg->count = i;
                sum += msg->count;
                escape(msg);  // Prevent optimization
                free(msg);
            }
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    printf("malloc/free:         %.2f M alloc/sec (checksum: %llu)\n", ops_per_sec, sum);
    return ops_per_sec;
}

// Benchmark 3: Batched allocation (realistic actor pattern)
double bench_batched_type_pools() {
    uint64_t sum = 0;
    IncrementMessage* batch[100];
    
    uint64_t start = get_nanos();
    
    int total_messages = ITERATIONS / 100;
    for (int i = 0; i < total_messages; i++) {
        // Allocate batch
        for (int j = 0; j < 100; j++) {
            batch[j] = IncrementMessage_alloc();
            if (batch[j]) {
                batch[j]->value = i * 100 + j;
                sum += batch[j]->value;
            }
        }
        
        // Free batch
        for (int j = 0; j < 100; j++) {
            if (batch[j]) {
                IncrementMessage_free(batch[j]);
            }
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = (total_messages * 100) / elapsed / 1e6;
    
    printf("Batched Type Pools:  %.2f M alloc/sec (checksum: %llu)\n", ops_per_sec, sum);
    return ops_per_sec;
}

double bench_batched_malloc() {
    uint64_t sum = 0;
    IncrementMessage* batch[100];
    
    uint64_t start = get_nanos();
    
    int total_messages = ITERATIONS / 100;
    for (int i = 0; i < total_messages; i++) {
        // Allocate batch
        for (int j = 0; j < 100; j++) {
            batch[j] = (IncrementMessage*)malloc(sizeof(IncrementMessage));
            if (batch[j]) {
                batch[j]->value = i * 100 + j;
                sum += batch[j]->value;
            }
        }
        
        // Free batch
        for (int j = 0; j < 100; j++) {
            if (batch[j]) {
                free(batch[j]);
            }
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = (total_messages * 100) / elapsed / 1e6;
    
    printf("Batched malloc:      %.2f M alloc/sec (checksum: %llu)\n", ops_per_sec, sum);
    return ops_per_sec;
}

int main() {
    printf("===========================================\n");
    printf("TYPE-SPECIFIC MEMORY POOLS BENCHMARK\n");
    printf("===========================================\n");
    printf("Operations: %d M allocations\n", ITERATIONS / 1000000);
    printf("Compiler: GCC -O3\n");
    printf("\n");
    
    // Warmup
    for (int i = 0; i < 1000000; i++) {
        IncrementMessage* msg = IncrementMessage_alloc();
        if (msg) IncrementMessage_free(msg);
    }
    
    printf("Test 1: Mixed Message Allocation\n");
    printf("-------------------------------------------\n");
    double malloc_score = bench_malloc();
    double type_pool_score = bench_type_specific_pools();
    
    double speedup1 = type_pool_score / malloc_score;
    printf("Speedup: %.2fx\n", speedup1);
    printf("\n");
    
    printf("Test 2: Batched Allocation (100 msg batches)\n");
    printf("-------------------------------------------\n");
    double malloc_batch_score = bench_batched_malloc();
    double type_pool_batch_score = bench_batched_type_pools();
    
    double speedup2 = type_pool_batch_score / malloc_batch_score;
    printf("Speedup: %.2fx\n", speedup2);
    printf("\n");
    
    printf("===========================================\n");
    printf("RESULTS SUMMARY\n");
    printf("===========================================\n");
    printf("Mixed allocation:  %.2fx faster\n", speedup1);
    printf("Batched allocation: %.2fx faster\n", speedup2);
    printf("Average speedup:    %.2fx\n", (speedup1 + speedup2) / 2.0);
    
    if (speedup1 > 2.0 && speedup2 > 2.0) {
        printf("\nOPTIMIZATION EFFECTIVE\n");
        printf("Type-specific pools provide 2-3x faster allocation!\n");
    } else if (speedup1 > 1.5) {
        printf("\nMODERATE IMPROVEMENT\n");
        printf("Type-specific pools faster but less than expected.\n");
    } else {
        printf("\nLIMITED BENEFIT\n");
        printf("Check pool size and allocation patterns.\n");
    }
    
    return 0;
}
