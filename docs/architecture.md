# Aether Architecture

This document provides an overview of Aether's compiler pipeline, runtime design, and key architectural decisions.

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                     Aether Compiler                         │
│  .ae source → Lexer → Parser → TypeCheck → Optimizer →     │
│  CodeGen → .c output                                        │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                      GCC/Clang                              │
│  .c source → Native binary                                  │
└─────────────────────────────────────────────────────────────┘
                            ↓
┌─────────────────────────────────────────────────────────────┐
│                    Aether Runtime                           │
│  Scheduler → Actors → Memory Management → Message Passing  │
└─────────────────────────────────────────────────────────────┘
```

## Compiler Pipeline

### 1. Lexer (`compiler/lexer.c`)

**Purpose:** Convert source text into tokens

**Input:** Raw Aether source code (`.ae` file)

**Output:** Stream of tokens

**Process:**
1. Read source character by character
2. Identify token boundaries (whitespace, operators, keywords)
3. Classify tokens (identifier, number, string, keyword, operator)
4. Track line and column numbers for error reporting

**Example:**
```aether
x = 42 + y
```

Produces tokens:
```
IDENTIFIER("x") EQUALS NUMBER(42) PLUS IDENTIFIER("y")
```

**Key Files:**
- `compiler/lexer.c` - Implementation
- `compiler/tokens.h` - Token type definitions

### 2. Parser (`compiler/parser.c`)

**Purpose:** Build Abstract Syntax Tree (AST) from tokens

**Input:** Token stream from lexer

**Output:** AST representing program structure

**Process:**
1. Consume tokens in order
2. Apply grammar rules (recursive descent parsing)
3. Build tree structure with each node representing a language construct
4. Report syntax errors with location information

**Example AST:**
```
x = 42 + y

    ASSIGN
    /    \
   x      BINOP(+)
          /      \
        42        y
```

**Key Concepts:**
- **Recursive Descent:** Each grammar rule is a function
- **Precedence Climbing:** Handles operator precedence (*, /, +, -)
- **Error Recovery:** Attempts to continue parsing after errors

**Key Files:**
- `compiler/parser.c` - Implementation
- `compiler/ast.h` - AST node definitions

### 3. Type Checker (`compiler/type_checker.c`)

**Purpose:** Infer and verify types for local variables

**Input:** AST from parser

**Output:** Type-annotated AST

**Process:**
1. Walk AST and assign type variables to expressions
2. Generate type constraints (e.g., if `a + b`, then `a` and `b` must be numeric)
3. Solve constraints using unification
4. Report type errors if constraints cannot be satisfied

**Example:**
```aether
x = 42        // Infer: x : Int
y = x + 10    // Infer: y : Int (since x : Int and 10 : Int)
z = "test"    // Infer: z : String
w = x + z     // Error: Cannot add Int and String
```

**Key Algorithms:**
- **Type Inference:** Automatic type deduction for local variables
- **Unification:** Solving type equations
- **Occurs Check:** Preventing infinite types

**Key Files:**
- `compiler/type_checker.c` - Implementation
- `compiler/types.h` - Type definitions

### 4. Optimizer (`compiler/optimizer.c`)

**Purpose:** Apply optimizations to improve performance

**Input:** Type-checked AST

**Output:** Optimized AST

**Optimizations:**

1. **Constant Folding**
   ```aether
   x = 2 + 3 * 4    // Becomes: x = 14
   ```

2. **Dead Code Elimination**
   ```aether
   if (false) {      // Entire block removed
       expensive_call()
   }
   ```

3. **Tail Call Optimization**
   ```aether
   factorial(n, acc) {
       if (n == 0) return acc
       return factorial(n-1, n*acc)  // Optimized to loop
   }
   ```

**Key Files:**
- `compiler/optimizer.c` - Implementation

### 5. Code Generator (`compiler/codegen.c`)

**Purpose:** Generate C code from AST

**Input:** Optimized AST

**Output:** C source code (`.c` file)

**Process:**
1. Walk AST in depth-first order
2. Generate equivalent C code for each node
3. Emit actor runtime calls for actor constructs
4. Generate type-appropriate C types

**Example:**
```aether
actor Counter {
    state count = 0
    
    receive(msg) {
        count = count + 1
    }
}
```

Generates C:
```c
typedef struct {
    int count;
} Counter_State;

void Counter_receive(Actor* self, Message* msg) {
    Counter_State* state = (Counter_State*)self->state;
    state->count = state->count + 1;
}
```

**Key Files:**
- `compiler/codegen.c` - Implementation

## Runtime Architecture

### Actor System

**Design:** Lightweight process model inspired by Erlang

**Actor Lifecycle:**
```
spawn() → INITIALIZING → READY → RUNNING ⇄ WAITING → TERMINATED
                           ↑         ↓
                           └─────────┘
```

**Key Components:**

1. **Actor Structure** (`runtime/actor.h`)
   ```c
   typedef struct Actor {
       ActorID id;
       void* state;              // User-defined state
       MessageQueue* mailbox;    // Incoming messages
       ReceiveFunc receive_fn;   // Message handler
       ActorStatus status;
   } Actor;
   ```

2. **Message Passing** (`runtime/message.h`)
   ```c
   typedef struct Message {
       int type;
       void* data;
       size_t data_size;
       ActorID sender;
   } Message;
   ```
   
   **Optimization:** Zero-copy transfer
   - Ownership transfer instead of memcpy
   - Significant improvement for large payloads
   - Eliminates memory copying overhead

3. **Message Queue** (`runtime/lockfree_queue.h`)
   - Lock-free implementation (SPSC atomic ring buffer)
   - CAS (Compare-And-Swap) operations for thread safety
   - Improved throughput vs mutex-based queues
   - No mutex contention enables high concurrency

4. **Message Dispatch** (computed goto pattern)
   - Direct label jumps instead of function pointers
   - Improved performance vs traditional switch statements
   - Used by CPython and LuaJIT for interpreter dispatch

### Scheduler

**Design**: Partitioned multicore scheduler with work stealing

**Architecture**:
```
┌─────────────────────────────────────────────────────────┐
│              Partitioned Scheduler                      │
│  ┌────────────┬────────────┬────────────┬────────────┐ │
│  │  Core 0    │  Core 1    │  Core 2    │  Core 3    │ │
│  │  [A1, A2]  │  [A3, A4]  │  [A5, A6]  │  [A7]      │ │
│  │  (local)   │  (local)   │  (local)   │  (idle)    │ │
│  │            │            │  ← steal ───────────────┘ │ │
│  └────────────┴────────────┴────────────┴────────────┘ │
└─────────────────────────────────────────────────────────┘
```

**Partitioned Scheduling**:
1. Actors statically assigned to cores at spawn
2. Each core processes only its local actors (fast path)
3. Cache locality: actors stay on same core, minimal NUMA traffic
4. Direct mailbox writes when sender core matches actor core

**Work Stealing (Idle Cores)**:
1. Core becomes idle after 5000 empty cycles
2. Scans all cores to find busiest (most pending work)
3. Non-blocking lock attempt on victim's actor list
4. Steals one actor if victim has >4 actors
5. Stolen actor reassigned to idle core

**Benefits**:
- Fast path has zero cross-core overhead
- Work stealing provides load balancing
- Non-blocking: failed steal attempts don't block progress
- Preserves cache locality (steals entire actors, not messages)

**NUMA-Aware Memory Allocation**:
1. Detects NUMA topology at scheduler initialization
2. Allocates actor data structures on same NUMA node as assigned core
3. Minimizes cross-NUMA-node memory access latency
4. Falls back to regular malloc on UMA (single-node) systems
5. Platform support:
   - Windows: `VirtualAllocExNuma`, `GetNumaProcessorNodeEx`
   - Linux: `numa_alloc_onnode` (requires libnuma)
   - Automatic detection: gracefully degrades if NUMA unavailable

**NUMA Awareness**:
- CPU pinning via pthread_setaffinity_np (Linux) or SetThreadAffinityMask (Windows)
- Actor mailboxes allocated on local core's NUMA node
- Minimizes cross-node memory latency

**Message Batching**:
- Adaptive batch size (32-256 messages per core cycle)
- Increases under load, decreases when idle
- Amortizes scheduling overhead

**Performance**:
- Single-core: 50M msg/sec
- Multicore: 98M msg/sec (4 cores, 2x speedup)
- Scaling efficiency: 50% (competitive with established runtimes)

**Key Files**:
- `runtime/scheduler/multicore_scheduler.c` - Implementation
- `runtime/scheduler/multicore_scheduler.h` - API
- `runtime/scheduler/lockfree_queue.h` - Cross-core message queue

### Memory Management

**Design:** Multi-tier allocation strategy with type-specific optimization

**Memory Layout:**
```
┌──────────────────────────────────────────────────────┐
│              Thread-Local Arena                      │
│  ┌────────────┬────────────┬────────────────────┐   │
│  │  Type Pool │  Small     │  Medium/Large      │   │
│  │  (msgs)    │  < 128B    │  > 128B            │   │
│  │  [freelist]│  [bump]    │  [malloc]          │   │
│  └────────────┴────────────┴────────────────────┘   │
└──────────────────────────────────────────────────────┘
```

**Allocation Strategies:**

1. **Type-Specific Pools (messages)**
   - Compile-time generated pools per message type
   - Fast free-list allocation
   - Thread-local storage eliminates locking
   - 1024 pre-allocated objects per pool
   - Used for: Actor messages, hot path allocations

2. **Small Objects (< 128 bytes)**
   - Bump allocation: `ptr += size` (< 20ns)
   - No overhead, no fragmentation
   - Used for: Temporary values, small structs

3. **Medium/Large Objects (> 128 bytes)**
   - Direct malloc for flexibility
   - Infrequent allocations
   - Used for: Large buffers, user data structures

**Thread-Local Arenas:**
- Each thread has its own arena
- No synchronization needed
- Zero contention between threads
- Cache-friendly (allocations stay in L1/L2)

**Key Files:**
- `runtime/memory/aether_type_pools.h` - Type-specific pool macros
- `runtime/aether_arena_optimized.c` - Optimized arena implementation
- `runtime/actors/aether_zerocopy.h` - Zero-copy message envelope
- `runtime/actors/aether_simd_batch.h` - SIMD batch processing functions
- `runtime/actors/aether_message_coalescing.h` - Message coalescing buffers

### Performance Optimizations

The runtime implements several empirically-validated optimization strategies:

#### 1. Lock-Free Mailboxes
**Implementation:** SPSC atomic ring buffer  
**Technique:** Compare-and-swap operations eliminate mutex overhead

#### 2. Computed Goto Dispatch
**Implementation:** Direct label jumps for message handlers  
**Technique:** Dispatch table with `goto *label[]` for reduced indirect call overhead

#### 3. Type-Specific Memory Pools
**Implementation:** Compile-time generated pools per message type  
**Technique:** Zero-branch free-list allocation with thread-local storage

#### 4. Zero-Copy Message Passing
**Implementation:** Ownership transfer instead of memcpy  
**Technique:** Move semantics with ownership flags for large payloads

#### 5. SIMD Message Batching
**Implementation:** AVX2 vectorized message processing  
**Technique:** Process 8 messages simultaneously using SIMD instructions  
**Use case:** Compute-intensive message handlers with uniform operations

#### 6. Message Coalescing
**Implementation:** Batch multiple messages into single queue operation  
**Technique:** Buffer messages and flush when threshold reached, reducing atomic operations  
**Use case:** High-frequency messaging scenarios (HFT, game engines, telemetry)

#### 7. Batch Actor Scheduling
**Implementation:** Process actors in groups for improved instruction-level parallelism  
**Technique:** Unrolled loop processing with aggressive prefetching

#### 8. Optimized Atomic Operations
**Implementation:** Platform-specific inline assembly for critical paths  
**Technique:** Custom spinlock with PAUSE instruction for reduced contention

**Empirical Testing Results:**
- Manual prefetch hints showed negative impact (hardware prefetcher is effective)
- Profile-guided optimization showed negative impact for this workload
- Power-of-2 masking redundant (compilers optimize automatically)

See [benchmarks/](../benchmarks/) for detailed benchmark results.

### Performance Characteristics

**Message Throughput:**
- Lock-free mailbox provides high concurrent throughput
- Type-specific allocation optimized for actor messaging patterns
- Zero-copy transfer efficient for large payloads
- Performance scales with core count and memory bandwidth

**Message Latency:**
- Sub-microsecond latency for cache-local operations
- Low microsecond latency for cross-core messaging
- Scheduler overhead minimized through batching

**Memory Allocation:**
- Small objects: Bump allocation (minimal overhead)
- Medium objects: Arena allocation
- Large objects: Direct system allocation

**Actor Capacity:**
- Supports large numbers of concurrent actors
- Per-actor overhead: ~512 bytes
- Scales with available system memory

## Module System

**Design:** Static module resolution with circular import detection

**Resolution Process:**
1. Parse import statement: `import std.collections.HashMap`
2. Search paths: `std/collections/HashMap.ae`, `./collections/HashMap.ae`
3. Load and parse module if not already loaded
4. Build dependency graph
5. Detect circular imports using DFS with visited tracking
6. Generate C includes for resolved modules

**Dependency Graph:**
```
    ModuleA
    /     \
ModuleB   ModuleC
    \     /
    ModuleD  ← All depend on ModuleD
```

**Circular Import Detection:**
- DFS traversal with "visiting" flag
- If encounter node with "visiting" flag set, cycle detected
- Report cycle with path: A → B → C → A

**Key Files:**
- `compiler/aether_module.c` - Module resolution
- `compiler/aether_module.h` - Module API

## Design Decisions

### Why Compile to C?

**Advantages:**
- Leverage mature C compilers (GCC, Clang)
- Access to optimizations (inlining, loop unrolling, vectorization)
- Portable (runs anywhere C runs)
- Easy FFI (call C libraries directly)

**Disadvantages:**
- Slower compile times (two-stage compilation)
- Harder to debug (must look at generated C)
- Some optimizations harder to implement

**Mitigation:**
- Incremental compilation reduces rebuild time
- Enhanced error messages point to Aether source, not C
- Precompiled standard library speeds up linking

### Why Arena Allocation?

**Advantages:**
- Significantly faster than malloc for small allocations
- No per-allocation overhead
- No fragmentation
- Predictable performance

**Disadvantages:**
- Cannot free individual allocations
- Memory held until arena reset
- Not suitable for long-lived, variable-size data

**Use Cases:**
- Per-request memory in servers (reset after request)
- Per-actor memory (reset when actor terminates)
- Compilation (reset after each file)

### Why Work Stealing?

**Advantages:**
- Automatic load balancing
- No manual task distribution
- Cache-friendly for local work
- Fair for stolen work

**Disadvantages:**
- Complexity (lock-free deques)
- Potential cache thrashing (stealing)
- Overhead for fine-grained tasks

**Alternatives Considered:**
- Round-robin: Poor load balancing
- Task queue per core: No load balancing
- Global queue: High contention

### Why Lock-Free Queues?

**Advantages:**
- No mutex contention
- Progress guarantee (at least one thread makes progress)
- Scales with core count

**Disadvantages:**
- Complexity (CAS loops)
- ABA problem (solved with tagged pointers)
- Memory ordering subtleties

**Performance:**
- Significantly faster than mutex-based queues under contention
- Critical for high message throughput

## Trade-Offs

### Compilation Speed vs Runtime Performance
- **Choice:** Favor runtime performance
- **Implication:** Slower compilation (two-stage, optimizations)
- **Mitigation:** Incremental builds, parallel compilation, precompiled stdlib

### Memory Safety vs Performance
- **Choice:** Manual memory management (no GC)
- **Implication:** Potential memory leaks if not careful
- **Mitigation:** Valgrind in CI, AddressSanitizer, defer statements, arenas

### Expressiveness vs Simplicity
- **Choice:** Minimal syntax (no OOP, no generics yet)
- **Implication:** Some patterns require boilerplate
- **Mitigation:** Type inference reduces annotations, macros for repetitive code

### Portability vs Optimization
- **Choice:** Portable C99 code
- **Implication:** Cannot use platform-specific SIMD/atomics directly
- **Mitigation:** Compiler auto-vectorization, optional intrinsics

## Future Directions

### Planned Improvements

1. **Incremental Type Checking:** Only re-check modified modules
2. **LLVM Backend:** Direct LLVM IR generation (skip C)
3. **Generics:** Type-parametric polymorphism
4. **Effect System:** Track side effects in type system
5. **Distributed Runtime:** Actor migration across machines

### Research Ideas

1. **Hot Code Reloading:** Update actor code without restart
2. **Persistent Actors:** Save/restore actor state to disk
3. **Actor Flamegraphs:** Visualize actor CPU usage
4. **Contract Testing:** Property-based testing for actor protocols
5. **Formal Verification:** Prove actor protocol correctness

## References

- Type inference for imperative languages: Pierce & Turner (2000)
- Michael-Scott Queue: Michael & Scott (1996)
- Work Stealing: Blumofe & Leiserson (1999)
- Actor Model: Hewitt, Bishop, Steiger (1973)
- Arena Allocation: Hanson (1990)

## Appendix: Build System

### Incremental Compilation

**Dependency Tracking:**
```makefile
# Generate .d files with dependencies
CFLAGS += -MMD -MP

# Include generated dependencies
-include $(DEPS)

# Compile .c to .o
build/%.o: %.c
    $(CC) $(CFLAGS) -c $< -o $@
```

**Process:**
1. Compile each `.c` file to `.o` with `-MMD` to generate `.d` dependency file
2. `.d` file lists all headers included by `.c` file
3. Make uses `.d` to determine if `.o` needs rebuild
4. Only modified files and their dependents rebuild

**Benefits:**
- 10-15x faster for small changes (0.5-1s vs 8-10s)
- Avoids redundant compilation
- Automatic dependency tracking

### Parallel Compilation

```bash
make -j8    # 8 parallel jobs
```

**Benefits:**
- 3-4x faster on 8-core system
- Limited by dependencies (some files must build before others)
- Diminishing returns beyond number of cores

### Precompiled Standard Library

```makefile
# Build stdlib as static library
build/libaether_std.a: $(STDLIB_OBJS)
    ar rcs $@ $^

# Link user programs against precompiled stdlib
user_program: user_program.o build/libaether_std.a
    $(CC) $^ -o $@ $(LDFLAGS)
```

**Benefits:**
- 5-8x faster user program compilation
- Stdlib only recompiles when modified
- Smaller link times
