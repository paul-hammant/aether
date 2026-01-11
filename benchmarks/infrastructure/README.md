# Infrastructure Benchmarks

Testing and validation tools.

## Benchmarks

- **bench_optimizations.c** - Comprehensive suite testing all optimizations
- **bench_multicore.c** - Multi-core scaling validation
- **bench_runtime.c** - Basic runtime performance tests
- **bench_simple.c** - Minimal mailbox comparison

## Purpose

These don't test specific optimizations - they validate overall system behavior:

- Correctness under load
- Multi-core scalability
- Baseline performance measurement
- Integration testing

## Use Cases

**Before merging optimization:**
```bash
./bench_optimizations  # Verify no regressions
./bench_multicore      # Check scaling still works
```

**Establishing baseline:**
```bash
./bench_runtime > baseline.txt
```
