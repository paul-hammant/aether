# Aether Cross-Language Benchmark Suite

Comparative benchmarking of actor/message-passing implementations across languages.

## Quick Start

```bash
./run_benchmarks.sh
```

## Benchmark Patterns

| Pattern | What It Measures |
|---------|-----------------|
| **ping_pong** | Round-trip message latency between two actors |
| **counting** | Single-actor message throughput |
| **thread_ring** | Multi-actor coordination (N actors in ring) |
| **fork_join** | Fan-out/fan-in parallelism |
| **skynet** | Recursive actor tree — 6 levels × 10 = 1M actors, measures actor creation + aggregation throughput |

The skynet benchmark is based on [atemerev/skynet](https://github.com/atemerev/skynet): a root actor
recursively spawns 10 children per level until 1M leaf actors exist. Each leaf reports its offset;
each parent sums 10 children's results and reports up. Total messages processed: ~1,111,111.

## Languages & Implementations

| Language | Implementation | Notes |
|----------|---------------|-------|
| **Aether** | Native actors | Lock-free SPSC queues, computed goto dispatch |
| **Go** | Goroutines + channels | Idiomatic Go concurrency |
| **Rust** | std::sync::mpsc | Standard library channels |
| **Erlang** | Native processes | Built-in actor model |
| **Elixir** | Native processes | Built-in actor model |
| **Pony** | Native actors | Reference capabilities |
| **C (pthreads + mutex)** | pthreads + mutex | Baseline thread primitives (no actor framework) |
| **C++ (std::mutex)** | std::mutex + condvar | Baseline thread primitives (no actor framework) |
| **Zig (std.Mutex)** | std.Thread + Mutex | Baseline thread primitives (no actor framework) |

**On C/C++/Zig baselines**: These use standard mutex/condvar — the idiomatic baseline for concurrent
code in those languages without an actor framework. Adding CAF (C++ Actor Framework) would require
Akka for Java, Quasar for Kotlin, etc., creating a different kind of inconsistency. The existing
comparison is intentionally honest: raw thread synchronization overhead vs. a purpose-built actor
runtime. For a C++ actor/tasking library shootout, see
[tzcnt/runtime-benchmarks](https://github.com/tzcnt/runtime-benchmarks).

**On skynet for Rust/C++**: Spawning 1M OS threads is not feasible (unlike Go goroutines or Erlang
processes). The Rust and C++ skynet implementations use threads for the top 3 levels (~1000 concurrent
threads) and compute sub-trees sequentially below that. This is the practical baseline — a real C++
actor framework (e.g. CAF) would perform better.

## Metrics

All benchmarks report:
- **ns/msg**: Nanoseconds per message (wall-clock time)
- **Throughput**: Messages per second (M msg/sec)

## Configuration

Edit `benchmark_config.json`:

```json
{
  "messages": 1000000,
  "timeout_seconds": 60
}
```

## What This Does NOT Measure

- I/O performance
- Memory allocation patterns
- GC pauses
- Real-world application workloads
- Distributed messaging

## Methodology

Based on the [Savina Actor Benchmark Suite](https://github.com/shamsimam/savina) and the
[Skynet Actor Benchmark](https://github.com/atemerev/skynet).

- All languages compiled with `-O3` or equivalent
- No specialized tuning or non-standard optimizations
- Message validation on every exchange (ping_pong, counting, thread_ring, fork_join)
- Results are system-dependent; run on your hardware

## References

- [Savina Benchmark Paper](http://soft.vub.ac.be/AGERE14/papers/ageresplash2014_submission_19.pdf)
- [Savina GitHub](https://github.com/shamsimam/savina)
- [Skynet Actor Benchmark](https://github.com/atemerev/skynet)
- [tzcnt/runtime-benchmarks — C++ tasking library comparison](https://github.com/tzcnt/runtime-benchmarks)
