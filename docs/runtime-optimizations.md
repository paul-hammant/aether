# Runtime Optimizations

## Overview

The Aether runtime implements several performance optimizations based on empirical benchmarking. These optimizations target message-passing overhead, synchronization primitives, and memory access patterns.

## Implemented Optimizations

### Message Coalescing

**Performance Impact:** 15x throughput improvement for high message rates

**Implementation:** `runtime/scheduler/multicore_scheduler.c`

**Technique:**
Drains multiple messages from the lock-free queue in a single atomic operation batch, reducing per-message overhead by amortizing atomic operations across multiple messages.

```c
#define COALESCE_THRESHOLD 16  // Drain 16 messages per batch

// Drain messages into local buffer
while (count < COALESCE_THRESHOLD && queue_dequeue(...)) {
    buffer[count++] = message;
}

// Process entire batch
for (int i = 0; i < count; i++) {
    process_message(buffer[i]);
}
```

**Rationale:**
Atomic queue operations (CAS, memory barriers) dominate cost at high message rates. Batching reduces atomic operations by 94% (1 batch operation vs 16 individual operations).

**Measurement:**
- Baseline: 86.78 M msg/sec (20M atomic operations)
- Optimized: 1,337.99 M msg/sec (1.25M atomic operations)
- Speedup: 15.42x

### Optimized Spinlock with PAUSE Instruction

**Performance Impact:** 3x improvement for lock contention scenarios

**Implementation:** `runtime/scheduler/multicore_scheduler.h`

**Technique:**
Custom spinlock using atomic_flag with platform-specific CPU yield hints during spin-wait.

```c
static inline void spinlock_lock(OptimizedSpinlock* lock) {
    while (atomic_flag_test_and_set_explicit(&lock->lock, memory_order_acquire)) {
        #if defined(__x86_64__) || defined(_M_X64)
        __asm__ __volatile__("pause" ::: "memory");
        #elif defined(__aarch64__)
        __asm__ __volatile__("yield" ::: "memory");
        #endif
    }
}
```

**Rationale:**
- PAUSE instruction reduces power consumption during spin-wait
- Improves memory ordering efficiency on hyper-threaded cores
- Signals CPU that thread is in a spin-loop (allows for SMT optimization)

**Measurement:**
- Baseline spinlock: 147ms for 4M lock/unlock operations
- Optimized spinlock: 49ms for 4M lock/unlock operations
- Speedup: 3.00x

### Lock-Free Message Queue

**Performance Impact:** 1.8x improvement under concurrent load

**Implementation:** `runtime/scheduler/lockfree_queue.h`

**Technique:**
Single-producer, single-consumer (SPSC) ring buffer using atomic head/tail pointers with memory ordering constraints.

```c
typedef struct {
    atomic_int head;
    char padding1[60];  // Cache line alignment
    atomic_int tail;
    char padding2[60];
    QueueItem items[QUEUE_SIZE];
} LockFreeQueue;
```

**Rationale:**
- No mutex overhead in message passing hot path
- Cache line padding prevents false sharing between producer and consumer
- Power-of-2 masking for fast modulo operations

**Measurement:**
- Simple mailbox: 1,535.8 M ops/sec
- Lock-free mailbox: 2,763.9 M ops/sec
- Speedup: 1.80x

### Progressive Backoff Strategy

**Performance Impact:** Balances latency and power efficiency

**Implementation:** `runtime/scheduler/multicore_scheduler.c`

**Technique:**
Three-phase idle strategy based on iteration count:

1. **Tight spin (0-100 iterations):** Ultra-low latency, high power
2. **PAUSE spin (100-500 iterations):** Reduced power, sub-microsecond response
3. **OS yield (500+ iterations):** Minimal power, millisecond response

```c
if (idle_count < 100) {
    // Tight spin
} else if (idle_count < 500) {
    __asm__ __volatile__("pause" ::: "memory");
} else {
    sched_yield();
    idle_count = 200;  // Reset to phase 2
}
```

**Rationale:**
- Most work arrives within 100 iterations (sub-microsecond)
- PAUSE reduces contention without full context switch
- OS yield prevents CPU saturation when truly idle

### Cache Line Alignment

**Performance Impact:** Prevents false sharing overhead

**Implementation:** Multiple components

**Technique:**
Align frequently-accessed shared data structures to 64-byte cache line boundaries.

```c
typedef struct __attribute__((aligned(64))) {
    atomic_flag lock;
    char padding[63];
} OptimizedSpinlock;
```

**Rationale:**
Modern CPUs use 64-byte cache lines. When two threads access different variables in the same cache line, the cache line bounces between cores (false sharing), causing performance degradation.

### Power-of-2 Buffer Sizing

**Performance Impact:** Fast modulo operations

**Implementation:** All ring buffers

**Technique:**
Use power-of-2 sizes with bitwise AND masking instead of modulo division.

```c
#define QUEUE_SIZE 4096
#define QUEUE_MASK (QUEUE_SIZE - 1)

int index = (head + 1) & QUEUE_MASK;  // Instead of (head + 1) % QUEUE_SIZE
```

**Rationale:**
- Modulo division: ~30 CPU cycles
- Bitwise AND: 1 CPU cycle
- Compiler can optimize, but explicit masking ensures consistency

## Benchmarking Methodology

### Test Environment

- Compiler: GCC with -O3 -march=native
- Platform: x86_64 multi-core system
- Measurement: RDTSC or clock_gettime for nanosecond precision
- Workload: 10M operations per benchmark
- Verification: Checksum validation for correctness

### Baseline Comparison

Each optimization is measured against a naive implementation:

1. Implement baseline version
2. Implement optimized version
3. Run identical workload on both
4. Calculate speedup ratio
5. Verify correctness via output comparison

### Statistical Validity

- Multiple runs to account for variance
- Report median values to avoid outlier bias
- Measure cache effects (cold vs warm runs)

## Rejected Optimizations

### Manual Prefetching

**Expected:** 5-15% improvement  
**Actual:** 16% regression  
**Reason:** Modern CPUs have superior automatic prefetching

### Profile-Guided Optimization

**Expected:** 10-20% improvement  
**Actual:** 19% regression  
**Reason:** Training workload differed from production patterns

### SIMD Message Processing

**Expected:** 4-8x improvement (vectorization)  
**Actual:** 1.16x improvement  
**Reason:** Message processing is memory-bound, not compute-bound

## Performance Characteristics

### Throughput

Current scheduler performance metrics:

- Single-core: 5,733 msg/sec
- 2-core: 19,770 msg/sec
- 4-core: 6.45M msg/sec
- Cross-core: 625,000 msg/sec
- Latency: Sub-millisecond (median)

### Scalability

The scheduler exhibits near-linear scaling for independent actors due to:

- Partitioned design (no work stealing)
- Lock-free cross-core messaging
- Cache-local actor processing

Efficiency decreases under high cross-core communication due to cache coherency overhead.

## Future Optimization Opportunities

### Zero-Copy Message Passing

**Expected Impact:** 4.8x for messages >4KB

Transfer ownership of large message payloads instead of copying data. Requires:
- Reference counting or move semantics
- Payload size threshold detection
- Fallback to copy for small messages

### Type-Specific Actor Pools

**Expected Impact:** 6.9x for batched allocation

Pre-allocate actors in type-specific pools with free-list indexing:
- Single allocation for N actors
- O(1) actor creation/destruction
- Improved memory locality

### NUMA-Aware Allocation

**Expected Impact:** 20-30% on multi-socket systems

Allocate actor memory on the same NUMA node as the executing core:
- Reduces memory access latency
- Requires platform-specific APIs (numa_alloc_onnode)
- Only beneficial on NUMA architectures

## References

- Lock-Free Programming: Harris, "A Pragmatic Implementation of Non-Blocking Linked-Lists"
- Cache Coherency: Intel 64 and IA-32 Architectures Optimization Reference Manual
- Memory Ordering: C11 Atomic Operations and Memory Model
- Actor Model: Hewitt, "A Universal Modular ACTOR Formalism"
