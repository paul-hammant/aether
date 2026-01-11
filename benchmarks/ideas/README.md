# Potential Optimizations

Exploration of additional optimization opportunities, organized by implementation complexity.

## Quick Wins (Low effort, good payoff)

### 1. Branch Prediction Hints (Expected: 2-5%)
**Effort:** 1-2 hours  
**Risk:** Very low

```c
// Mark hot paths
if (__builtin_expect(mailbox_has_message(mbox), 1)) {
    // Common case - process message
} else {
    // Rare case - idle
}
```

**Why it works:** Guide CPU branch predictor on critical paths.

**Next step:** Add to `mailbox_receive()` and test with `bench_runtime`.

---

### 2. Power-of-2 Ring Buffer Masking (Expected: 5-10%)
**Effort:** 2-3 hours  
**Risk:** Very low

```c
// Before: size % BUFFER_SIZE (slow modulo)
// After:  size & (BUFFER_SIZE - 1) (fast AND)
```

**Why it works:** Bitwise AND is 3-5x faster than modulo on modern CPUs.

**Next step:** Change ring buffer size to 256/512/1024, use masking.

---

### 3. Cache Line Alignment (Expected: 3-8%)
**Effort:** 3-4 hours  
**Risk:** Low

```c
typedef struct __attribute__((aligned(64))) {
    _Atomic uint32_t head;  // Own cache line
    char pad1[60];
    _Atomic uint32_t tail;  // Own cache line  
    char pad2[60];
    Message messages[256];
} Mailbox;
```

**Why it works:** Prevents false sharing between threads.

**Next step:** Align hot structures, test with `bench_multicore`.

---

## Medium Wins (Medium effort, high payoff)

### 4. NUMA-Aware Actor Placement (Expected: 20-30% on multi-socket)
**Effort:** 1-2 days  
**Risk:** Medium

Keep actors and their memory on same NUMA node:
```c
#include <numa.h>

void* actor_alloc(size_t size) {
    int node = numa_node_of_cpu(sched_getcpu());
    return numa_alloc_onnode(size, node);
}
```

**Why it works:** Local memory access is 2-3x faster than remote on multi-socket.

**Next step:** Prototype with `numa_alloc_local()`, benchmark on 2+ socket system.

---

### 5. Huge Pages (Expected: 5-10%)
**Effort:** 2-3 days  
**Risk:** Medium (portability)

Reduce TLB misses for large allocations:
```c
// Linux
madvise(ptr, size, MADV_HUGEPAGE);

// Windows
VirtualAlloc(NULL, size, MEM_LARGE_PAGES, PAGE_READWRITE);
```

**Why it works:** 2MB huge pages vs 4KB normal = 512x fewer TLB entries.

**Next step:** Benchmark arena allocator with huge pages enabled.

---

### 6. Batch Actor Scheduling (Expected: 10-20%)
**Effort:** 3-5 days  
**Risk:** Medium

Schedule multiple actors at once:
```c
void schedule_batch(Actor** actors, int count) {
    for (int i = 0; i < count; i++) {
        // Prefetch next actor while processing current
        __builtin_prefetch(&actors[i+1], 0, 3);
        process_actor(actors[i]);
    }
}
```

**Why it works:** Amortizes scheduler overhead, improves cache locality.

**Next step:** Modify `multicore_scheduler.c` to batch-schedule.

---

## Big Bets (High effort, huge payoff potential)

### 7. JIT Compilation for Hot Actors (Expected: 50-100%)
**Effort:** 2-3 weeks  
**Risk:** High

Generate machine code for frequently-executed actors:
```c
// Interpret cold actors, JIT hot ones
if (actor->message_count > JIT_THRESHOLD) {
    actor->handler = jit_compile(actor->bytecode);
}
```

**Why it works:** Eliminates interpreter overhead, enables CPU-specific optimizations.

**Next step:** Integrate LLVM JIT or libgccjit.

---

### 8. Actor Fusion (Expected: 30-50%)
**Effort:** 2-4 weeks  
**Risk:** High

Merge connected actors:
```c
// Before: ActorA -> message -> ActorB (2 queue ops)
// After:  ActorA_B_fused()        (direct call)
```

**Why it works:** Eliminates message passing overhead for tightly-coupled actors.

**Next step:** Implement fusion analysis pass in compiler.

---

### 9. Memory Pooling per Message Type (Expected: 15-25%)
**Effort:** 1-2 weeks  
**Risk:** Medium

Currently: type-specific pools (done)  
Next: Pre-sized pools based on profiling:

```c
// Hot path for 90% of messages
if (size <= 32) return pool_32_alloc();
if (size <= 64) return pool_64_alloc();
// General case for remaining 10%
return general_alloc(size);
```

**Why it works:** Size-specific fast paths for common cases.

**Next step:** Profile message size distribution, create specialized pools.

---

## CRAZY Ideas (Research projects)

### 10. GPU Actor Execution (Expected: 100-1000x for parallel workloads)
**Effort:** Months  
**Risk:** Very high

Run thousands of actors on GPU:
```c
__global__ void actor_kernel(Actor* actors, int count) {
    int id = blockIdx.x * blockDim.x + threadIdx.x;
    if (id < count) {
        process_actor(&actors[id]);
    }
}
```

**Why it works:** 1000s of parallel actors on GPU vs 16-32 cores on CPU.

**Next step:** CUDA/OpenCL prototype for embarrassingly parallel workloads.

---

### 11. RDMA for Distributed Actors (Expected: 10-50x for network)
**Effort:** Months  
**Risk:** Very high

Zero-copy network messaging:
```c
// Normal: send() copies to kernel, network card copies again
// RDMA:   Direct memory access, zero copies
rdma_post_send(qp, actor->mailbox, size, actor_addr);
```

**Why it works:** Bypasses kernel, eliminates all copies.

**Next step:** Test with InfiniBand or RoCE.

---

## How to Use This

1. Pick an idea
2. Create `bench_<idea>.c` in `benchmarks/optimizations/`
3. Implement baseline and optimized versions
4. If speedup > 10% → implement in runtime
5. Document results here
6. Update main README

## Contribution Guidelines

When prototyping optimizations:
- Always include baseline comparison
- Test on multiple platforms (Linux, Windows, macOS)
- Measure memory overhead, not just speed
- Profile with `perf`, Instruments, or VTune
- Document trade-offs clearly
