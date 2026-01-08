// Benchmark: Message Coalescing
//
// Tests combining multiple small messages into batches to reduce:
// - Message queue operations (enqueue/dequeue overhead)
// - Cache misses (better locality)
// - Atomic operations (fewer CAS instructions)
//
// Real-world scenario: High-frequency trading, game engines, IoT telemetry

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define aligned_free(ptr) _aligned_free(ptr)
static inline uint64_t get_nanos() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000000ULL) / freq.QuadPart;
}
#else
#define aligned_free(ptr) free(ptr)
#include <time.h>
static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

#define TOTAL_MESSAGES 10000000
#define COALESCE_SIZE 16  // Combine 16 messages into one batch

typedef struct {
    int message_id;
    int value;
} SingleMessage;

typedef struct {
    int count;
    SingleMessage messages[COALESCE_SIZE];
} CoalescedBatch;

// Simulate message queue operations (atomic overhead)
static volatile int queue_ops = 0;

static void __attribute__((noinline)) simulate_enqueue() {
    __atomic_fetch_add(&queue_ops, 1, __ATOMIC_SEQ_CST);
}

static void __attribute__((noinline)) simulate_dequeue() {
    __atomic_fetch_add(&queue_ops, 1, __ATOMIC_SEQ_CST);
}

// Benchmark 1: Traditional single-message passing
double bench_single_messages() {
    SingleMessage* messages = (SingleMessage*)aligned_alloc(64, TOTAL_MESSAGES * sizeof(SingleMessage));
    
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
    }
    
    queue_ops = 0;
    uint64_t start = get_nanos();
    
    // Send each message individually
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        simulate_enqueue();  // Atomic operation
        simulate_dequeue();  // Atomic operation
        
        // Process message
        int result = messages[i].value * 2 + messages[i].message_id;
        (void)result;
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = TOTAL_MESSAGES / elapsed / 1e6;
    
    printf("Single messages: %.2f M msg/sec (%d queue ops)\n", ops_per_sec, queue_ops);
    aligned_free(messages);
    return ops_per_sec;
}

// Benchmark 2: Coalesced message batching
double bench_coalesced_messages() {
    SingleMessage* messages = (SingleMessage*)aligned_alloc(64, TOTAL_MESSAGES * sizeof(SingleMessage));
    
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
    }
    
    queue_ops = 0;
    uint64_t start = get_nanos();
    
    // Send messages in batches
    int num_batches = TOTAL_MESSAGES / COALESCE_SIZE;
    for (int batch = 0; batch < num_batches; batch++) {
        simulate_enqueue();  // One atomic operation per batch
        simulate_dequeue();  // One atomic operation per batch
        
        // Process entire batch
        for (int i = 0; i < COALESCE_SIZE; i++) {
            int idx = batch * COALESCE_SIZE + i;
            int result = messages[idx].value * 2 + messages[idx].message_id;
            (void)result;
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = TOTAL_MESSAGES / elapsed / 1e6;
    
    printf("Coalesced:       %.2f M msg/sec (%d queue ops)\n", ops_per_sec, queue_ops);
    aligned_free(messages);
    return ops_per_sec;
}

// Benchmark 3: Adaptive coalescing (coalesce only under load)
double bench_adaptive_coalescing() {
    SingleMessage* messages = (SingleMessage*)aligned_alloc(64, TOTAL_MESSAGES * sizeof(SingleMessage));
    
    for (int i = 0; i < TOTAL_MESSAGES; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
    }
    
    queue_ops = 0;
    uint64_t start = get_nanos();
    
    // Simulate adaptive behavior: coalesce when many messages pending
    int pending = 0;
    int processed = 0;
    
    while (processed < TOTAL_MESSAGES) {
        // Simulate messages arriving in bursts
        int burst_size = (processed % 100 < 80) ? COALESCE_SIZE : 1;
        
        if (burst_size == COALESCE_SIZE) {
            // High load: use coalescing
            simulate_enqueue();
            simulate_dequeue();
            
            for (int i = 0; i < COALESCE_SIZE && processed < TOTAL_MESSAGES; i++) {
                int result = messages[processed].value * 2 + messages[processed].message_id;
                (void)result;
                processed++;
            }
        } else {
            // Low load: send immediately
            simulate_enqueue();
            simulate_dequeue();
            
            if (processed < TOTAL_MESSAGES) {
                int result = messages[processed].value * 2 + messages[processed].message_id;
                (void)result;
                processed++;
            }
        }
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = TOTAL_MESSAGES / elapsed / 1e6;
    
    printf("Adaptive:        %.2f M msg/sec (%d queue ops)\n", ops_per_sec, queue_ops);
    aligned_free(messages);
    return ops_per_sec;
}

int main() {
    printf("===========================================\n");
    printf("MESSAGE COALESCING BENCHMARK\n");
    printf("===========================================\n");
    printf("Total messages: %d M\n", TOTAL_MESSAGES / 1000000);
    printf("Coalesce size:  %d messages\n", COALESCE_SIZE);
    printf("\n");
    
    printf("Test: Message Queue Throughput\n");
    printf("-------------------------------------------\n");
    double single_score = bench_single_messages();
    double coalesced_score = bench_coalesced_messages();
    double adaptive_score = bench_adaptive_coalescing();
    
    printf("\n");
    printf("===========================================\n");
    printf("RESULTS SUMMARY\n");
    printf("===========================================\n");
    printf("Coalesced speedup: %.2fx\n", coalesced_score / single_score);
    printf("Adaptive speedup:  %.2fx\n", adaptive_score / single_score);
    printf("\n");
    printf("Queue operation reduction:\n");
    printf("- Single: %d ops (baseline)\n", TOTAL_MESSAGES * 2);
    printf("- Coalesced: %d ops (%.0f%% reduction)\n", 
           (TOTAL_MESSAGES / COALESCE_SIZE) * 2,
           (1.0 - (double)(TOTAL_MESSAGES / COALESCE_SIZE) / TOTAL_MESSAGES) * 100);
    printf("\n");
    
    if (coalesced_score > single_score * 1.3) {
        printf("OPTIMIZATION HIGHLY EFFECTIVE\n");
        printf("Message coalescing significantly reduces queue overhead!\n");
        printf("\nRecommendation: Implement coalescing for high-throughput actors.\n");
    } else if (coalesced_score > single_score * 1.15) {
        printf("MODERATE IMPROVEMENT\n");
        printf("Coalescing helps but isn't dramatic.\n");
        printf("\nRecommendation: Use adaptive coalescing for bursty workloads.\n");
    } else {
        printf("LIMITED BENEFIT\n");
        printf("Queue operations are not the bottleneck.\n");
        printf("\nRecommendation: Focus on other optimizations.\n");
    }
    
    return 0;
}
