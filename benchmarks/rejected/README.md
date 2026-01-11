# Rejected Optimizations

These optimizations were tested and **made things slower**.

## Benchmarks

- **bench_prefetch.c** - Manual `__builtin_prefetch()` hints: -16% slower
- **bench_pgo.c** - Profile-guided optimization: -19% slower  
- **pgo_workload.c** - Training workload for PGO

## Why They Failed

**Manual prefetch:** Hardware prefetcher already optimal for sequential access patterns. Adding explicit prefetch instructions just pollutes the instruction stream.

**PGO:** Simple benchmarks have perfectly predictable branches. PGO adds overhead (larger code, more instructions) without benefit. Might help complex code with unpredictable branches.

## Lesson

Don't assume "advanced" optimizations are always better. Benchmark everything.

## Keeping These

We keep rejected optimizations to:
1. Document what NOT to do
2. Show empirical testing process
3. Prevent repeating failed experiments
