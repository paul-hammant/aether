// Benchmark: Zero-Copy Message Passing
//
// Compares:
// 1. Traditional copy-based message passing (memcpy)
// 2. Zero-copy ownership transfer (pointer move)
// 3. Hybrid approach (small messages inline, large zero-copy)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
static inline uint64_t get_nanos() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000000ULL) / freq.QuadPart;
}
#else
#include <time.h>
static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

#include "../../runtime/actors/aether_zerocopy.h"

#define ITERATIONS 10000000  // 10M messages

// Test message structures (varying sizes)
typedef struct {
    int _message_id;
    int value;
} SmallMessage;  // 8 bytes

typedef struct {
    int _message_id;
    int values[64];
} MediumMessage;  // 260 bytes

typedef struct {
    int _message_id;
    int values[1024];
} LargeMessage;  // 4100 bytes

// Prevent compiler optimizations
static void __attribute__((noinline)) escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

// Benchmark 1: Traditional copy-based messaging
double bench_copy_based(int message_size) {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Allocate message
        void* msg = malloc(message_size);
        if (!msg) continue;
        
        *(int*)msg = i;  // Set message_id
        sum += *(int*)msg;
        
        // Simulate sending (copy to mailbox)
        void* mailbox_copy = malloc(message_size);
        if (mailbox_copy) {
            memcpy(mailbox_copy, msg, message_size);  // COPY
            escape(mailbox_copy);
            
            // Simulate receiving (copy from mailbox)
            void* received = malloc(message_size);
            if (received) {
                memcpy(received, mailbox_copy, message_size);  // COPY
                escape(received);
                free(received);
            }
            
            free(mailbox_copy);
        }
        
        free(msg);
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    printf("Copy-based (%d bytes): %.2f M msg/sec (checksum: %llu)\n", 
           message_size, ops_per_sec, sum);
    return ops_per_sec;
}

// Benchmark 2: Zero-copy ownership transfer
double bench_zero_copy(int message_size) {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < ITERATIONS; i++) {
        // Allocate message
        void* msg = malloc(message_size);
        if (!msg) continue;
        
        *(int*)msg = i;  // Set message_id
        sum += *(int*)msg;
        
        // Initialize zero-copy message
        ZeroCopyMessage zcmsg;
        zerocopy_init_owned(&zcmsg, msg, message_size);
        
        // Simulate sending (ownership transfer - no copy!)
        ZeroCopyMessage mailbox_msg;
        zerocopy_transfer(&mailbox_msg, &zcmsg);  // MOVE, not copy
        escape(mailbox_msg.data);
        
        // Simulate receiving (ownership transfer - no copy!)
        ZeroCopyMessage received_msg;
        zerocopy_transfer(&received_msg, &mailbox_msg);  // MOVE, not copy
        escape(received_msg.data);
        
        // Clean up
        zerocopy_free(&received_msg);
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    printf("Zero-copy (%d bytes): %.2f M msg/sec (checksum: %llu)\n", 
           message_size, ops_per_sec, sum);
    return ops_per_sec;
}

// Benchmark 3: Hybrid approach (small inline, large zero-copy)
double bench_hybrid(int message_size) {
    uint64_t sum = 0;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < ITERATIONS; i++) {
        void* msg = malloc(message_size);
        if (!msg) continue;
        
        *(int*)msg = i;
        sum += *(int*)msg;
        
        if (message_size <= 64) {
            // Small message: use inline copy (faster for tiny messages)
            char inline_buf[64];
            memcpy(inline_buf, msg, message_size);
            escape(inline_buf);
            free(msg);
        } else {
            // Large message: use zero-copy
            ZeroCopyMessage zcmsg;
            zerocopy_init_owned(&zcmsg, msg, message_size);
            
            ZeroCopyMessage mailbox_msg;
            zerocopy_transfer(&mailbox_msg, &zcmsg);
            escape(mailbox_msg.data);
            
            zerocopy_free(&mailbox_msg);
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    printf("Hybrid (%d bytes):    %.2f M msg/sec (checksum: %llu)\n", 
           message_size, ops_per_sec, sum);
    return ops_per_sec;
}

int main() {
    printf("===========================================\n");
    printf("ZERO-COPY MESSAGE PASSING BENCHMARK\n");
    printf("===========================================\n");
    printf("Operations: %d M messages per test\n", ITERATIONS / 1000000);
    printf("Compiler: GCC -O3\n");
    printf("\n");
    
    // Test 1: Small messages (8 bytes)
    printf("Test 1: Small Messages (8 bytes)\n");
    printf("-------------------------------------------\n");
    double small_copy = bench_copy_based(sizeof(SmallMessage));
    double small_zerocopy = bench_zero_copy(sizeof(SmallMessage));
    double small_hybrid = bench_hybrid(sizeof(SmallMessage));
    double speedup1 = small_zerocopy / small_copy;
    printf("Zero-copy speedup: %.2fx\n", speedup1);
    printf("Hybrid speedup:    %.2fx\n", small_hybrid / small_copy);
    printf("\n");
    
    // Test 2: Medium messages (260 bytes)
    printf("Test 2: Medium Messages (260 bytes)\n");
    printf("-------------------------------------------\n");
    double medium_copy = bench_copy_based(sizeof(MediumMessage));
    double medium_zerocopy = bench_zero_copy(sizeof(MediumMessage));
    double medium_hybrid = bench_hybrid(sizeof(MediumMessage));
    double speedup2 = medium_zerocopy / medium_copy;
    printf("Zero-copy speedup: %.2fx\n", speedup2);
    printf("Hybrid speedup:    %.2fx\n", medium_hybrid / medium_copy);
    printf("\n");
    
    // Test 3: Large messages (4100 bytes)
    printf("Test 3: Large Messages (4100 bytes)\n");
    printf("-------------------------------------------\n");
    double large_copy = bench_copy_based(sizeof(LargeMessage));
    double large_zerocopy = bench_zero_copy(sizeof(LargeMessage));
    double large_hybrid = bench_hybrid(sizeof(LargeMessage));
    double speedup3 = large_zerocopy / large_copy;
    printf("Zero-copy speedup: %.2fx\n", speedup3);
    printf("Hybrid speedup:    %.2fx\n", large_hybrid / large_copy);
    printf("\n");
    
    // Summary
    printf("===========================================\n");
    printf("RESULTS SUMMARY\n");
    printf("===========================================\n");
    printf("Small messages:  %.2fx faster with zero-copy\n", speedup1);
    printf("Medium messages: %.2fx faster with zero-copy\n", speedup2);
    printf("Large messages:  %.2fx faster with zero-copy\n", speedup3);
    printf("Average speedup: %.2fx\n", (speedup1 + speedup2 + speedup3) / 3.0);
    printf("\n");
    
    if (speedup2 > 1.3 && speedup3 > 1.5) {
        printf("OPTIMIZATION EFFECTIVE\n");
        printf("Zero-copy provides 30-50%% faster messaging for large messages!\n");
    } else if (speedup2 > 1.1) {
        printf("MODERATE IMPROVEMENT\n");
        printf("Zero-copy provides modest gains.\n");
    } else {
        printf("LIMITED BENEFIT\n");
        printf("Allocation overhead dominates. Consider type-specific pools.\n");
    }
    
    return 0;
}
