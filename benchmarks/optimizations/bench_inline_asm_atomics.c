// Benchmark: Inline Assembly Atomic Operations
// Expected: 3-8% improvement over standard C atomics

#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <pthread.h>
#include <time.h>
#include "../../runtime/scheduler/aether_atomic_asm.h"

#define ITERATIONS 100000000
#define NUM_THREADS 4

// Baseline: Standard C atomics
void* worker_standard(void* arg) {
    atomic_int* counter = (atomic_int*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add_explicit(counter, 1, memory_order_relaxed);
    }
    
    return NULL;
}

// Optimized: Inline assembly atomics
void* worker_asm(void* arg) {
    atomic_int* counter = (atomic_int*)arg;
    
    for (int i = 0; i < ITERATIONS; i++) {
        atomic_fetch_add_asm(counter, 1);
    }
    
    return NULL;
}

double bench_standard() {
    atomic_int counter = 0;
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_standard, &counter);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double bench_asm() {
    atomic_int counter = 0;
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_asm, &counter);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

// Test spinlock performance
void* spinlock_worker_standard(void* arg) {
    atomic_int* lock = (atomic_int*)arg;
    
    for (int i = 0; i < ITERATIONS / 100; i++) {
        int expected = 0;
        while (!atomic_compare_exchange_weak_explicit(
            lock, &expected, 1,
            memory_order_acquire, memory_order_relaxed)) {
            expected = 0;
        }
        
        // Critical section (empty for benchmark)
        
        atomic_store_explicit(lock, 0, memory_order_release);
    }
    
    return NULL;
}

void* spinlock_worker_asm(void* arg) {
    FastSpinlock* lock = (FastSpinlock*)arg;
    
    for (int i = 0; i < ITERATIONS / 100; i++) {
        spinlock_lock(lock);
        
        // Critical section (empty for benchmark)
        
        spinlock_unlock(lock);
    }
    
    return NULL;
}

double bench_spinlock_standard() {
    atomic_int lock = 0;
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, spinlock_worker_standard, &lock);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

double bench_spinlock_asm() {
    FastSpinlock lock;
    spinlock_init(&lock);
    pthread_t threads[NUM_THREADS];
    
    clock_t start = clock();
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, spinlock_worker_asm, &lock);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end = clock();
    return ((double)(end - start)) / CLOCKS_PER_SEC;
}

int main() {
    printf("=== Inline Assembly Atomic Operations Benchmark ===\n\n");
    
#if HAS_X86_64_ASM
    printf("Platform: x86_64 (inline assembly enabled)\n");
#else
    printf("Platform: Non-x86_64 (fallback to standard atomics)\n");
#endif
    printf("Threads: %d\n", NUM_THREADS);
    printf("Iterations: %d per thread\n\n", ITERATIONS);
    
    // Warmup
    for (int i = 0; i < 2; i++) {
        bench_standard();
        bench_asm();
    }
    
    // Test 1: Atomic fetch-and-add
    printf("=== Test 1: Atomic Fetch-Add ===\n");
    
    printf("[Baseline] Standard C atomics...\n");
    double time_standard = bench_standard();
    printf("  Time: %.3f seconds\n", time_standard);
    double ops_standard = (NUM_THREADS * ITERATIONS) / time_standard;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_standard / 1e6);
    
    printf("[Optimized] Inline assembly atomics...\n");
    double time_asm = bench_asm();
    printf("  Time: %.3f seconds\n", time_asm);
    double ops_asm = (NUM_THREADS * ITERATIONS) / time_asm;
    printf("  Throughput: %.2f M ops/sec\n\n", ops_asm / 1e6);
    
    double speedup1 = time_standard / time_asm;
    double improvement1 = ((speedup1 - 1.0) * 100.0);
    
    printf("[Results]\n");
    printf("  Speedup: %.2fx\n", speedup1);
    printf("  Improvement: %+.1f%%\n\n", improvement1);
    
    // Test 2: Spinlock
    printf("=== Test 2: Spinlock (CAS + Store) ===\n");
    
    printf("[Baseline] Standard spinlock...\n");
    double time_lock_std = bench_spinlock_standard();
    printf("  Time: %.3f seconds\n", time_lock_std);
    printf("  Lock/unlock ops: %d\n\n", NUM_THREADS * (ITERATIONS / 100));
    
    printf("[Optimized] Assembly spinlock with PAUSE...\n");
    double time_lock_asm = bench_spinlock_asm();
    printf("  Time: %.3f seconds\n", time_lock_asm);
    printf("  Lock/unlock ops: %d\n\n", NUM_THREADS * (ITERATIONS / 100));
    
    double speedup2 = time_lock_std / time_lock_asm;
    double improvement2 = ((speedup2 - 1.0) * 100.0);
    
    printf("[Results]\n");
    printf("  Speedup: %.2fx\n", speedup2);
    printf("  Improvement: %+.1f%%\n\n", improvement2);
    
    // Overall assessment
    double avg_improvement = (improvement1 + improvement2) / 2.0;
    printf("=== Overall Assessment ===\n");
    printf("Average improvement: %+.1f%%\n\n", avg_improvement);
    
    if (avg_improvement >= 3.0) {
        printf("✓ SUCCESS: %.1f%% average improvement (expected 3-8%%)\n", avg_improvement);
        printf("  Inline assembly provides measurable benefits\n");
        printf("  Spinlock with PAUSE instruction especially effective\n");
        return 0;
    } else if (avg_improvement >= 1.0) {
        printf("✓ MODEST: %.1f%% average improvement\n", avg_improvement);
        printf("  Some benefit, but compiler generates good code already\n");
        return 0;
    } else {
        printf("✗ NO BENEFIT: %.1f%% average change\n", avg_improvement);
        printf("  Modern compilers generate optimal atomic operations\n");
        printf("  Inline assembly may not provide additional value\n");
        return 1;
    }
}
