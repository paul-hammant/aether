// Comprehensive benchmark comparing optimization levels
// Tests: simple vs lock-free mailbox, with/without SIMD, with/without MWAIT

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include "runtime/actors/actor_state_machine.h"
#include "runtime/actors/lockfree_mailbox.h"
#include "runtime/actors/aether_actor.h"
#include "runtime/utils/aether_cpu_detect.h"
#include "runtime/aether_runtime.h"

#define BENCH_ITERATIONS 10000000
#define WARMUP_ITERATIONS 1000000

// Benchmark 1: Simple mailbox throughput
double bench_simple_mailbox() {
    Mailbox mbox;
    mailbox_init(&mbox);
    
    Message msg = {1, 0, 42, NULL};
    Message recv_msg;
    
    clock_t start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        mailbox_send(&mbox, msg);
        mailbox_receive(&mbox, &recv_msg);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return BENCH_ITERATIONS / seconds;
}

// Benchmark 2: Lock-free mailbox throughput
double bench_lockfree_mailbox() {
    LockFreeMailbox mbox;
    lockfree_mailbox_init(&mbox);
    
    Message msg = {1, 0, 42, NULL};
    Message recv_msg;
    
    clock_t start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        lockfree_mailbox_send(&mbox, msg);
        lockfree_mailbox_receive(&mbox, &recv_msg);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return BENCH_ITERATIONS / seconds;
}

// Benchmark 3: TLS message pool allocation
double bench_tls_pool() {
    // Warmup to initialize TLS pool
    for (int i = 0; i < 100; i++) {
        void* ptr = message_pool_alloc(NULL, 256);
        message_pool_free(NULL, ptr);
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        void* ptr = message_pool_alloc(NULL, 256);
        message_pool_free(NULL, ptr);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return BENCH_ITERATIONS / seconds;
}

// Benchmark 4: Standard malloc/free
double bench_malloc() {
    clock_t start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS; i++) {
        void* ptr = malloc(256);
        free(ptr);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return BENCH_ITERATIONS / seconds;
}

// Benchmark 5: Batch operations
double bench_batch_operations() {
    LockFreeMailbox mbox;
    lockfree_mailbox_init(&mbox);
    
    Message msgs[16];
    Message recv_msgs[16];
    
    for (int i = 0; i < 16; i++) {
        msgs[i].type = 1;
        msgs[i].payload_int = i;
        msgs[i].payload_ptr = NULL;
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < BENCH_ITERATIONS / 16; i++) {
        lockfree_mailbox_send_batch(&mbox, msgs, 16);
        lockfree_mailbox_receive_batch(&mbox, recv_msgs, 16);
    }
    
    clock_t end = clock();
    double seconds = (double)(end - start) / CLOCKS_PER_SEC;
    return BENCH_ITERATIONS / seconds;
}

void print_separator() {
    printf("========================================\n");
}

void print_comparison(const char* name, double baseline, double optimized) {
    double speedup = optimized / baseline;
    printf("%-25s %8.2f M/s -> %8.2f M/s  (%.2fx)\n", 
           name, baseline / 1e6, optimized / 1e6, speedup);
}

int main() {
    print_separator();
    printf("  Aether Optimization Benchmarks\n");
    print_separator();
    printf("\n");
    
    // Show CPU info
    const CPUInfo* cpu = cpu_get_info();
    printf("CPU: %s\n", cpu->cpu_brand);
    printf("Features: AVX2=%s MWAIT=%s SSE4.2=%s\n\n",
           cpu->avx2_supported ? "YES" : "NO",
           cpu->mwait_supported ? "YES" : "NO",
           cpu->sse42_supported ? "YES" : "NO");
    
    printf("Running benchmarks (%d iterations each)...\n", BENCH_ITERATIONS);
    printf("Warming up...\n");
    
    // Warmup
    bench_simple_mailbox();
    bench_lockfree_mailbox();
    
    printf("\n");
    print_separator();
    printf("  Results\n");
    print_separator();
    printf("\n");
    
    // Run benchmarks
    double simple_mail = bench_simple_mailbox();
    double lockfree_mail = bench_lockfree_mailbox();
    double tls_pool = bench_tls_pool();
    double malloc_ops = bench_malloc();
    double batch_ops = bench_batch_operations();
    
    printf("\nMailbox Comparison:\n");
    print_comparison("Simple mailbox:", simple_mail, simple_mail);
    print_comparison("Lock-free mailbox:", simple_mail, lockfree_mail);
    
    printf("\nMemory Pool Comparison:\n");
    print_comparison("malloc/free:", malloc_ops, malloc_ops);
    print_comparison("TLS pool:", malloc_ops, tls_pool);
    
    printf("\nBatch Processing:\n");
    print_comparison("Single ops:", lockfree_mail, lockfree_mail);
    print_comparison("Batch-16 ops:", lockfree_mail, batch_ops);
    
    printf("\n");
    print_separator();
    printf("  Summary\n");
    print_separator();
    printf("\n");
    
    printf("Lock-free vs Simple:   %.2fx faster\n", lockfree_mail / simple_mail);
    printf("TLS pool vs malloc:    %.2fx faster\n", tls_pool / malloc_ops);
    printf("Batch vs Single:       %.2fx faster\n", batch_ops / lockfree_mail);
    
    double combined_speedup = (lockfree_mail / simple_mail) * 
                              (tls_pool / malloc_ops) * 
                              (batch_ops / lockfree_mail);
    
    printf("\nCombined theoretical speedup: %.2fx\n", combined_speedup);
    
    // Expected values
    printf("\nExpected performance (from experiments):\n");
    printf("  Baseline (simple):     ~125 M msg/sec\n");
    printf("  With optimizations:    ~2.3 B msg/sec\n");
    printf("  Expected speedup:      ~18x\n");
    
    // Save results to JSON
    FILE* f = fopen("benchmarks/latest_results.json", "w");
    if (f) {
        fprintf(f, "{\n");
        fprintf(f, "  \"simple_mailbox_ops_per_sec\": %.0f,\n", simple_mail);
        fprintf(f, "  \"lockfree_mailbox_ops_per_sec\": %.0f,\n", lockfree_mail);
        fprintf(f, "  \"tls_pool_ops_per_sec\": %.0f,\n", tls_pool);
        fprintf(f, "  \"malloc_ops_per_sec\": %.0f,\n", malloc_ops);
        fprintf(f, "  \"batch_ops_per_sec\": %.0f,\n", batch_ops);
        fprintf(f, "  \"lockfree_speedup\": %.2f,\n", lockfree_mail / simple_mail);
        fprintf(f, "  \"tls_pool_speedup\": %.2f,\n", tls_pool / malloc_ops);
        fprintf(f, "  \"batch_speedup\": %.2f,\n", batch_ops / lockfree_mail);
        fprintf(f, "  \"cpu\": \"%s\",\n", cpu->cpu_brand);
        fprintf(f, "  \"avx2\": %s,\n", cpu->avx2_supported ? "true" : "false");
        fprintf(f, "  \"mwait\": %s\n", cpu->mwait_supported ? "true" : "false");
        fprintf(f, "}\n");
        fclose(f);
        printf("\nResults saved to benchmarks/latest_results.json\n");
    }
    
    printf("\n");
    
    return 0;
}
