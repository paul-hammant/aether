// Benchmark: SIMD Message Batching
//
// Compares:
// 1. Traditional single-message processing
// 2. Batched processing with SIMD (process 4/8 messages simultaneously)
// 3. Measures cache efficiency and throughput

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#include <malloc.h>  // For _aligned_malloc on Windows
#define aligned_alloc(alignment, size) _aligned_malloc(size, alignment)
#define aligned_free(ptr) _aligned_free(ptr)
#else
#define aligned_free(ptr) free(ptr)
#endif

#include <immintrin.h>  // AVX2

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

#define ITERATIONS 10000000
#define BATCH_SIZE 8

typedef struct {
    int message_id;
    int value;
    int result;
    int _padding;
} Message;

// Prevent compiler optimizations
static void __attribute__((noinline)) escape(void* p) {
    asm volatile("" : : "g"(p) : "memory");
}

// Benchmark 1: Traditional single-message processing
double bench_scalar() {
    Message* messages = (Message*)aligned_alloc(32, ITERATIONS * sizeof(Message));
    
    // Initialize messages
    for (int i = 0; i < ITERATIONS; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
        messages[i].result = 0;
    }
    
    uint64_t start = get_nanos();
    
    // Process messages one by one
    for (int i = 0; i < ITERATIONS; i++) {
        // Simulate message processing: result = value * 2 + message_id
        messages[i].result = messages[i].value * 2 + messages[i].message_id;
        escape(&messages[i]);
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = ITERATIONS / elapsed / 1e6;
    
    uint64_t checksum = 0;
    for (int i = 0; i < 1000; i++) {
        checksum += messages[i].result;
    }
    
    printf("Scalar:      %.2f M msg/sec (checksum: %llu)\n", ops_per_sec, checksum);
    aligned_free(messages);
    return ops_per_sec;
}

// Benchmark 2: SIMD batched processing (AVX2 - 8 ints at once)
double bench_simd_avx2() {
    Message* messages = (Message*)aligned_alloc(32, ITERATIONS * sizeof(Message));
    
    // Initialize messages
    for (int i = 0; i < ITERATIONS; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
        messages[i].result = 0;
    }
    
    uint64_t start = get_nanos();
    
    // Process 8 messages at once using AVX2
    int batches = ITERATIONS / BATCH_SIZE;
    for (int i = 0; i < batches; i++) {
        int base = i * BATCH_SIZE;
        
        // Load 8 values
        __m256i values = _mm256_setr_epi32(
            messages[base+0].value,
            messages[base+1].value,
            messages[base+2].value,
            messages[base+3].value,
            messages[base+4].value,
            messages[base+5].value,
            messages[base+6].value,
            messages[base+7].value
        );
        
        // Load 8 message_ids
        __m256i ids = _mm256_setr_epi32(
            messages[base+0].message_id,
            messages[base+1].message_id,
            messages[base+2].message_id,
            messages[base+3].message_id,
            messages[base+4].message_id,
            messages[base+5].message_id,
            messages[base+6].message_id,
            messages[base+7].message_id
        );
        
        // SIMD computation: result = value * 2 + message_id
        __m256i doubled = _mm256_slli_epi32(values, 1);  // value * 2 (shift left)
        __m256i results = _mm256_add_epi32(doubled, ids);
        
        // Store results (unaligned)
        int temp[8];
        _mm256_storeu_si256((__m256i*)temp, results);
        
        for (int j = 0; j < BATCH_SIZE; j++) {
            messages[base+j].result = temp[j];
        }
        
        escape(&messages[base]);
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = (batches * BATCH_SIZE) / elapsed / 1e6;
    
    uint64_t checksum = 0;
    for (int i = 0; i < 1000; i++) {
        checksum += messages[i].result;
    }
    
    printf("SIMD (AVX2): %.2f M msg/sec (checksum: %llu)\n", ops_per_sec, checksum);
    aligned_free(messages);
    return ops_per_sec;
}

// Benchmark 3: Cache-friendly batched processing (no SIMD)
double bench_batched_scalar() {
    Message* messages = (Message*)aligned_alloc(32, ITERATIONS * sizeof(Message));
    
    // Initialize messages
    for (int i = 0; i < ITERATIONS; i++) {
        messages[i].message_id = i % 256;
        messages[i].value = i;
        messages[i].result = 0;
    }
    
    uint64_t start = get_nanos();
    
    // Process in batches for better cache utilization
    int batches = ITERATIONS / BATCH_SIZE;
    for (int i = 0; i < batches; i++) {
        int base = i * BATCH_SIZE;
        
        // Process batch sequentially (cache-friendly)
        for (int j = 0; j < BATCH_SIZE; j++) {
            int idx = base + j;
            messages[idx].result = messages[idx].value * 2 + messages[idx].message_id;
        }
        
        escape(&messages[base]);
    }
    
    uint64_t end = get_nanos();
    double elapsed = (end - start) / 1e9;
    double ops_per_sec = (batches * BATCH_SIZE) / elapsed / 1e6;
    
    uint64_t checksum = 0;
    for (int i = 0; i < 1000; i++) {
        checksum += messages[i].result;
    }
    
    printf("Batched:     %.2f M msg/sec (checksum: %llu)\n", ops_per_sec, checksum);
    aligned_free(messages);
    return ops_per_sec;
}

int main() {
    printf("===========================================\n");
    printf("SIMD MESSAGE BATCHING BENCHMARK\n");
    printf("===========================================\n");
    printf("Operations: %d M messages\n", ITERATIONS / 1000000);
    printf("Batch size: %d messages\n", BATCH_SIZE);
    printf("CPU: AVX2 support required\n");
    printf("\n");
    
    // Check AVX2 support
    int avx2_supported = __builtin_cpu_supports("avx2");
    if (!avx2_supported) {
        printf("WARNING: AVX2 not supported, SIMD benchmark will be inaccurate\n\n");
    }
    
    printf("Test: Message Processing Throughput\n");
    printf("-------------------------------------------\n");
    double scalar_score = bench_scalar();
    double batched_score = bench_batched_scalar();
    double simd_score = bench_simd_avx2();
    
    printf("\n");
    printf("===========================================\n");
    printf("RESULTS SUMMARY\n");
    printf("===========================================\n");
    printf("Batched speedup: %.2fx\n", batched_score / scalar_score);
    printf("SIMD speedup:    %.2fx\n", simd_score / scalar_score);
    printf("\n");
    
    if (simd_score > scalar_score * 1.5) {
        printf("OPTIMIZATION EFFECTIVE\n");
        printf("SIMD batching provides significant throughput improvement!\n");
        printf("\nRecommendation: Implement SIMD batching for message processing.\n");
    } else if (batched_score > scalar_score * 1.2) {
        printf("BATCHING HELPS, SIMD MARGINAL\n");
        printf("Cache-friendly batching helps, but SIMD overhead is high.\n");
        printf("\nRecommendation: Use scalar batching, skip SIMD complexity.\n");
    } else {
        printf("LIMITED BENEFIT\n");
        printf("Message processing is not compute-bound.\n");
        printf("\nRecommendation: Focus on reducing memory latency instead.\n");
    }
    
    return 0;
}
