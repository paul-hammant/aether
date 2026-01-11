# Implemented Optimizations

Benchmarks for optimizations integrated into the production runtime.

## Available Benchmarks

### Core Runtime Optimizations
1. **bench_message_coalescing.c** - Message batching optimization
2. **bench_zerocopy.c** - Zero-copy message transfer
3. **bench_type_pools.c** - Type-specific allocation pools
4. **bench_inline_asm_atomics.c** - Optimized atomic operations
5. **bench_lockfree_mailbox.c** - Lock-free concurrent mailbox
6. **bench_simd_batch.c** - SIMD vectorization
7. **bench_computed_goto.c** - Computed goto dispatch

### Actor Runtime Optimizations
8. **bench_actor_baseline.c** - Baseline actor performance measurements
9. **bench_actor_optimized.c** - Combined optimizations (pooling, direct send, dedup, specialization, adaptive batching)

## Running Benchmarks

### Individual Optimizations

```bash
gcc -O3 -march=native -o bench_message_coalescing bench_message_coalescing.c
./bench_message_coalescing
```

### Actor Runtime Comparison

```bash
# Build baseline
gcc -O3 -I../.. bench_actor_baseline.c -o bench_baseline

# Build optimized
gcc -O3 -I../.. bench_actor_optimized.c -o bench_optimized

# Run comparison
./bench_baseline
./bench_optimized
```

## Actor Optimizations Tested

The optimized actor benchmark tests five advanced techniques:

1. **Actor Pooling** - Reuses actor instances instead of repeated allocation
2. **Direct Send** - Bypasses mailbox for same-core actors
3. **Message Deduplication** - Skips redundant messages using rolling window
4. **Compile-Time Specialization** - Zero-overhead message sends for known types
5. **Adaptive Batching** - Dynamic batch sizing based on queue depth

See [../../runtime/actors/README.md](../../runtime/actors/README.md) for implementation details.
