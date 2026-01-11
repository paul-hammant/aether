// Comprehensive Benchmark: All Scheduler Optimizations
// Tests: Direct Send, Adaptive Batching, SIMD Processing, Lock-free Mailbox
// Compares baseline vs optimized performance

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#define sleep_ms(ms) Sleep(ms)
static inline uint64_t get_nanos() {
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (counter.QuadPart * 1000000000ULL) / freq.QuadPart;
}
#else
#include <unistd.h>
#define sleep_ms(ms) usleep((ms) * 1000)
static inline uint64_t get_nanos() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}
#endif

// Include runtime components
#include "../runtime/scheduler/multicore_scheduler.h"
#include "../runtime/scheduler/scheduler_optimizations.h"
#include "../runtime/actors/lockfree_mailbox.h"
#include "../runtime/actors/aether_simd_batch.h"
#include "../runtime/actors/aether_adaptive_batch.h"
#include "../runtime/actors/aether_actor_pool.h"

// Benchmark configuration
#define WARMUP_ITERATIONS 10000
#define BENCHMARK_ITERATIONS 1000000
#define NUM_ACTORS 64
#define BENCH_BATCH_SIZE 16

// Results structure
typedef struct {
    const char* name;
    double ops_per_sec;
    double latency_ns;
    double speedup;
} BenchResult;

#define MAX_RESULTS 20
static BenchResult results[MAX_RESULTS];
static int result_count = 0;

static void record_result(const char* name, double ops, double lat, double speedup) {
    if (result_count < MAX_RESULTS) {
        results[result_count].name = name;
        results[result_count].ops_per_sec = ops;
        results[result_count].latency_ns = lat;
        results[result_count].speedup = speedup;
        result_count++;
    }
}

// ============================================================================
// BENCHMARK 1: Mailbox Operations (Baseline vs Lock-free)
// ============================================================================

double bench_mailbox_baseline(int iterations) {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    Message msg = {.type = 1, .sender_id = 0, .payload_int = 42};
    Message recv;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        mailbox_send(&mbox, msg);
        mailbox_receive(&mbox, &recv);
    }
    
    uint64_t end = get_nanos();
    
    return (double)iterations / ((end - start) / 1e9);
}

double bench_mailbox_lockfree(int iterations) {
    LockFreeMailbox mbox;
    lockfree_mailbox_init(&mbox);
    
    Message msg = {.type = 1, .sender_id = 0, .payload_int = 42};
    Message recv;
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        lockfree_mailbox_send(&mbox, msg);
        lockfree_mailbox_receive(&mbox, &recv);
    }
    
    uint64_t end = get_nanos();
    
    return (double)iterations / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 2: Batch Processing (Fixed vs Adaptive)
// ============================================================================

double bench_batch_fixed(int iterations) {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    Message msgs[BENCH_BATCH_SIZE];
    for (int i = 0; i < BENCH_BATCH_SIZE; i++) {
        msgs[i] = (Message){.type = 1, .sender_id = 0, .payload_int = i};
    }
    
    int processed = 0;
    uint64_t start = get_nanos();
    
    for (int iter = 0; iter < iterations / BENCH_BATCH_SIZE; iter++) {
        // Send batch
        for (int i = 0; i < BENCH_BATCH_SIZE; i++) {
            mailbox_send(&mbox, msgs[i]);
        }
        
        // Receive fixed batch
        Message recv[BENCH_BATCH_SIZE];
        int got = mailbox_receive_batch(&mbox, recv, BENCH_BATCH_SIZE);
        processed += got;
    }
    
    uint64_t end = get_nanos();
    
    return (double)processed / ((end - start) / 1e9);
}

double bench_batch_adaptive(int iterations) {
    Mailbox mbox;
    mailbox_init(&mbox);
    AdaptiveBatchState batch_state;
    adaptive_batch_init(&batch_state);
    
    Message msgs[BENCH_BATCH_SIZE * 2];
    for (int i = 0; i < BENCH_BATCH_SIZE * 2; i++) {
        msgs[i] = (Message){.type = 1, .sender_id = 0, .payload_int = i};
    }
    
    int processed = 0;
    uint64_t start = get_nanos();
    
    for (int iter = 0; iter < iterations / BENCH_BATCH_SIZE; iter++) {
        // Vary load to test adaptation
        int batch = (iter % 3 == 0) ? BENCH_BATCH_SIZE / 2 : BENCH_BATCH_SIZE;
        
        // Send batch
        for (int i = 0; i < batch; i++) {
            mailbox_send(&mbox, msgs[i]);
        }
        
        // Receive with adaptive batch size
        Message recv[BENCH_BATCH_SIZE * 2];
        int adaptive_size = batch_state.current_batch_size;
        if (adaptive_size > BENCH_BATCH_SIZE * 2) adaptive_size = BENCH_BATCH_SIZE * 2;
        
        int got = mailbox_receive_batch(&mbox, recv, adaptive_size);
        processed += got;
        
        adaptive_batch_adjust(&batch_state, got);
    }
    
    uint64_t end = get_nanos();
    
    return (double)processed / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 3: SIMD Processing
// ============================================================================

double bench_simd_scalar(int iterations) {
    int values[256];
    int results_arr[256];
    
    for (int i = 0; i < 256; i++) {
        values[i] = i;
    }
    
    uint64_t start = get_nanos();
    
    for (int iter = 0; iter < iterations; iter++) {
        // Scalar processing
        for (int i = 0; i < 256; i++) {
            results_arr[i] = values[i] * 2 + 1;
        }
    }
    
    uint64_t end = get_nanos();
    
    return (double)(iterations * 256) / ((end - start) / 1e9);
}

double bench_simd_vectorized(int iterations) {
    int values[256];
    int results_arr[256];
    
    for (int i = 0; i < 256; i++) {
        values[i] = i;
    }
    
    uint64_t start = get_nanos();
    
    for (int iter = 0; iter < iterations; iter++) {
        simd_batch_process_int(values, results_arr, 256, 2, 1);
    }
    
    uint64_t end = get_nanos();
    
    return (double)(iterations * 256) / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 4: Message Deduplication
// ============================================================================

double bench_no_dedup(int iterations) {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    int processed = 0;
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        // Send potentially duplicate messages
        Message msg = {.type = i % 10, .sender_id = i % 5, .payload_int = i % 100};
        mailbox_send(&mbox, msg);
        
        Message recv;
        if (mailbox_receive(&mbox, &recv)) {
            processed++;
        }
    }
    
    uint64_t end = get_nanos();
    
    return (double)processed / ((end - start) / 1e9);
}

double bench_with_dedup(int iterations) {
    Mailbox mbox;
    mailbox_init(&mbox);
    DedupWindow dedup;
    memset(&dedup, 0, sizeof(dedup));
    
    int processed = 0;
    int deduplicated = 0;
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        // Send potentially duplicate messages
        Message msg = {.type = i % 10, .sender_id = i % 5, .payload_int = i % 100};
        
        // Check for duplicates
        if (is_duplicate(&dedup, &msg)) {
            deduplicated++;
            continue;
        }
        
        // Record in dedup window
        MessageFingerprint fp = message_fingerprint(&msg);
        dedup.window[dedup.write_index] = fp;
        dedup.write_index = (dedup.write_index + 1) & DEDUP_WINDOW_MASK;
        
        mailbox_send(&mbox, msg);
        
        Message recv;
        if (mailbox_receive(&mbox, &recv)) {
            processed++;
        }
    }
    
    uint64_t end = get_nanos();
    
    printf("    (deduplicated %d messages)\n", deduplicated);
    return (double)processed / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 5: Optimized Actor Send/Receive
// ============================================================================

double bench_optimized_actor_throughput(int iterations) {
    scheduler_opts_init();
    
    OptimizedActor sender, receiver;
    optimized_actor_init(&sender, 0, NULL);
    optimized_actor_init(&receiver, 0, NULL);  // Same core
    
    int sent = 0;
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        Message msg = {.type = 1, .sender_id = sender.id, .payload_int = i};
        
        if (optimized_send_message(&sender, &receiver, msg)) {
            sent++;
        }
        
        // Receive periodically
        if (i % BENCH_BATCH_SIZE == BENCH_BATCH_SIZE - 1) {
            Message recv[BENCH_BATCH_SIZE];
            optimized_receive_messages(&receiver, recv, BENCH_BATCH_SIZE);
        }
    }
    
    uint64_t end = get_nanos();
    
    return (double)sent / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 6: Actor Pooling (malloc vs pooled allocation)
// ============================================================================

double bench_actor_malloc(int iterations) {
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        // Simulate actor allocation with malloc
        PooledActor* actor = malloc(sizeof(PooledActor));
        if (actor) {
            mailbox_init(&actor->mailbox);
            actor->active = 1;
            actor->pool_index = -1;
            atomic_store(&actor->in_use, 1);
            
            // Simulate brief use
            Message msg = {.type = 1, .sender_id = 0, .payload_int = i};
            mailbox_send(&actor->mailbox, msg);
            
            free(actor);
        }
    }
    
    uint64_t end = get_nanos();
    
    return (double)iterations / ((end - start) / 1e9);
}

double bench_actor_pooled(int iterations) {
    ActorPool* pool = malloc(sizeof(ActorPool));
    if (!pool) return 0;
    actor_pool_init(pool);
    
    uint64_t start = get_nanos();
    
    for (int i = 0; i < iterations; i++) {
        // Get actor from pool
        PooledActor* actor = actor_pool_acquire(pool);
        if (actor) {
            // Simulate brief use
            Message msg = {.type = 1, .sender_id = 0, .payload_int = i};
            mailbox_send(&actor->mailbox, msg);
            
            actor_pool_release(pool, actor);
        }
    }
    
    uint64_t end = get_nanos();
    free(pool);
    
    return (double)iterations / ((end - start) / 1e9);
}

// ============================================================================
// BENCHMARK 7: End-to-End Message Processing (No scheduler threads)
// ============================================================================

static atomic_int baseline_counter;
static atomic_int optimized_counter;

double bench_message_processing_baseline(int iterations) {
    atomic_store(&baseline_counter, 0);
    
    // Create actors with mailboxes
    ActorBase actors[4];
    for (int i = 0; i < 4; i++) {
        actors[i].id = i;
        actors[i].active = 1;
        actors[i].assigned_core = i % 2;
        mailbox_init(&actors[i].mailbox);
    }
    
    uint64_t start = get_nanos();
    
    // Send messages
    for (int i = 0; i < iterations; i++) {
        Message msg = {.type = 1, .sender_id = 0, .payload_int = i};
        mailbox_send(&actors[i % 4].mailbox, msg);
    }
    
    // Process all messages
    for (int i = 0; i < 4; i++) {
        Message msg;
        while (mailbox_receive(&actors[i].mailbox, &msg)) {
            atomic_fetch_add(&baseline_counter, 1);
        }
    }
    
    uint64_t end = get_nanos();
    
    int processed = atomic_load(&baseline_counter);
    printf("    (processed %d messages)\n", processed);
    
    return (double)processed / ((end - start) / 1e9);
}

double bench_message_processing_optimized(int iterations) {
    atomic_store(&optimized_counter, 0);
    scheduler_opts_init();
    
    // Create optimized actors
    OptimizedActor actors[4];
    for (int i = 0; i < 4; i++) {
        optimized_actor_init(&actors[i], i % 2, NULL);
        actors[i].id = i;
    }
    
    uint64_t start = get_nanos();
    
    // Send messages with optimizations
    for (int i = 0; i < iterations; i++) {
        Message msg = {.type = 1, .sender_id = actors[i % 4].id, .payload_int = i};
        int sender_idx = i % 4;
        int receiver_idx = (i + 1) % 4;
        optimized_send_message(&actors[sender_idx], &actors[receiver_idx], msg);
    }
    
    // Receive and process with batch optimization
    for (int i = 0; i < 4; i++) {
        Message recv[64];
        int total = 0;
        int got;
        while ((got = optimized_receive_messages(&actors[i], recv, 64)) > 0) {
            total += got;
        }
        atomic_fetch_add(&optimized_counter, total);
    }
    
    uint64_t end = get_nanos();
    
    scheduler_opts_print_stats();
    
    int processed = atomic_load(&optimized_counter);
    printf("    (processed %d messages)\n", processed);
    
    return (double)processed / ((end - start) / 1e9);
}

// ============================================================================
// MAIN: Run All Benchmarks
// ============================================================================

void print_separator() {
    printf("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n");
}

void run_benchmark(const char* name, 
                   double (*baseline_fn)(int), 
                   double (*optimized_fn)(int),
                   int iterations) {
    printf("\n%s\n", name);
    print_separator();
    
    // Warmup
    printf("  Warming up...\n");
    baseline_fn(WARMUP_ITERATIONS);
    optimized_fn(WARMUP_ITERATIONS);
    
    // Baseline
    printf("  Running baseline...\n");
    double baseline = baseline_fn(iterations);
    printf("  Baseline:   %.2f M ops/sec\n", baseline / 1e6);
    
    // Optimized
    printf("  Running optimized...\n");
    double optimized = optimized_fn(iterations);
    printf("  Optimized:  %.2f M ops/sec\n", optimized / 1e6);
    
    // Calculate speedup
    double speedup = optimized / baseline;
    printf("  ✨ Speedup: %.2fx\n", speedup);
    
    // Record result
    record_result(name, optimized, 1e9 / optimized, speedup);
}

void print_summary() {
    printf("\n");
    print_separator();
    printf("                    BENCHMARK SUMMARY\n");
    print_separator();
    printf("%-35s %15s %10s\n", "Benchmark", "Ops/sec", "Speedup");
    print_separator();
    
    for (int i = 0; i < result_count; i++) {
        printf("%-35s %12.2f M %9.2fx\n", 
               results[i].name, 
               results[i].ops_per_sec / 1e6,
               results[i].speedup);
    }
    
    print_separator();
    
    // Calculate overall improvement
    double total_speedup = 0;
    for (int i = 0; i < result_count; i++) {
        total_speedup += results[i].speedup;
    }
    double avg_speedup = total_speedup / result_count;
    
    printf("Average Speedup: %.2fx\n", avg_speedup);
    printf("\n");
}

int main() {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════╗\n");
    printf("║     AETHER SCHEDULER OPTIMIZATIONS BENCHMARK SUITE           ║\n");
    printf("║     Testing: Direct Send, Adaptive Batch, SIMD, Lock-free    ║\n");
    printf("╚══════════════════════════════════════════════════════════════╝\n");
    
    // Run all benchmarks
    run_benchmark("1. Mailbox: Standard vs Lock-free",
                  bench_mailbox_baseline,
                  bench_mailbox_lockfree,
                  BENCHMARK_ITERATIONS);
    
    run_benchmark("2. Batching: Fixed vs Adaptive",
                  bench_batch_fixed,
                  bench_batch_adaptive,
                  BENCHMARK_ITERATIONS);
    
    run_benchmark("3. Processing: Scalar vs SIMD",
                  bench_simd_scalar,
                  bench_simd_vectorized,
                  BENCHMARK_ITERATIONS / 100);  // Fewer iterations, more data per iter
    
    run_benchmark("4. Deduplication: None vs With Dedup",
                  bench_no_dedup,
                  bench_with_dedup,
                  BENCHMARK_ITERATIONS / 10);
    
    run_benchmark("5. Actor Pool: malloc vs Pooled",
                  bench_actor_malloc,
                  bench_actor_pooled,
                  BENCHMARK_ITERATIONS / 10);

    printf("\n6. Optimized Actor Throughput\n");
    print_separator();
    double actor_throughput = bench_optimized_actor_throughput(BENCHMARK_ITERATIONS);
    printf("  Throughput: %.2f M msg/sec\n", actor_throughput / 1e6);
    record_result("6. Optimized Actor", actor_throughput, 1e9 / actor_throughput, 1.0);
    
    // Print summary
    print_summary();
    
    // Output JSON for baseline update
    printf("\n=== JSON Output for baseline.json ===\n");
    printf("{\n");
    printf("  \"timestamp\": \"%s\",\n", __DATE__);
    printf("  \"optimizations_benchmark\": {\n");
    for (int i = 0; i < result_count; i++) {
        printf("    \"%s\": {\n", results[i].name);
        printf("      \"ops_per_sec\": %.0f,\n", results[i].ops_per_sec);
        printf("      \"speedup\": \"%.2fx\"\n", results[i].speedup);
        printf("    }%s\n", i < result_count - 1 ? "," : "");
    }
    printf("  }\n");
    printf("}\n");
    
    return 0;
}
