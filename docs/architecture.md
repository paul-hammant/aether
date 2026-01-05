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

**Purpose:** Infer and verify types using Hindley-Milner algorithm

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
- **Hindley-Milner Type Inference:** Automatic type deduction
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

3. **Message Queue** (`runtime/lockfree_queue.h`)
   - Lock-free implementation (Michael-Scott algorithm)
   - CAS (Compare-And-Swap) operations for thread safety
   - No mutex contention = high throughput

### Scheduler

**Design:** Multicore work-stealing scheduler

**Architecture:**
```
┌─────────────────────────────────────────────────────────┐
│                  Scheduler (Main)                       │
│  ┌────────────┬────────────┬────────────┬────────────┐ │
│  │  Core 0    │  Core 1    │  Core 2    │  Core 3    │ │
│  │  [A1, A2]  │  [A3, A4]  │  [A5]      │  []        │ │
│  │            │            │            │   ↑ steal  │ │
│  └────────────┴────────────┴────────────┴────────────┘ │
└─────────────────────────────────────────────────────────┘
```

**Work Stealing Algorithm:**
1. Each core has a local deque of ready actors
2. Core runs actors from its local deque (LIFO - cache friendly)
3. If local deque empty, steal from another core (FIFO - fair)
4. Stolen actors moved to local deque

**Benefits:**
- Load balancing (idle cores steal work)
- Cache locality (LIFO for local work)
- Fairness (FIFO for stealing)

**NUMA Awareness:**
- Threads pinned to specific cores (`pthread_setaffinity_np`)
- Memory allocated on local NUMA node
- Reduces cross-node memory access latency

**Batch Processing:**
- Process up to 32 messages per actor activation
- Reduces context switch overhead
- Amortizes scheduling cost

**Key Files:**
- `runtime/multicore_scheduler.c` - Implementation
- `runtime/multicore_scheduler.h` - API

### Memory Management

**Design:** Arena-based allocation with size classes

**Memory Layout:**
```
┌──────────────────────────────────────────────────────┐
│              Thread-Local Arena                      │
│  ┌────────────┬────────────┬────────────────────┐   │
│  │  Small     │  Medium    │  Large             │   │
│  │  < 128B    │  < 4KB     │  > 4KB             │   │
│  │  [bump]    │  [bump]    │  [malloc]          │   │
│  └────────────┴────────────┴────────────────────┘   │
└──────────────────────────────────────────────────────┘
```

**Size Classes:**

1. **Small Objects (< 128 bytes)**
   - Bump allocation: `ptr += size` (< 20ns)
   - No overhead, no fragmentation
   - Used for: Messages, small structs

2. **Medium Objects (128B - 4KB)**
   - Bump allocation from medium arena
   - Rare resets to avoid waste
   - Used for: Buffers, arrays

3. **Large Objects (> 4KB)**
   - Direct malloc/free
   - Too large for efficient bump allocation
   - Used for: Large buffers, file data

**Thread-Local Arenas:**
- Each thread has its own arena
- No synchronization needed
- Zero contention between threads
- Cache-friendly (allocations stay in L1/L2)

**Memory Pooling:**
- Fixed-size pools for common allocations
- O(1) alloc/free
- Used for: Actor state, messages

**Key Files:**
- `runtime/aether_arena_optimized.c` - Optimized arena implementation
- `runtime/aether_memory.c` - Memory pools

### Performance Characteristics

**Message Throughput:**
- P50: 200M+ msg/sec (5-actor ring, high load)
- P95: 20M+ msg/sec (100-actor system, many actors)
- Limited by cache misses, not lock contention

**Message Latency:**
- P50: ~100ns (local messages, cache hot)
- P95: ~450ns (cross-core messages)
- P99: ~2-3µs (includes scheduler overhead)

**Memory Allocation:**
- Small objects: < 20ns (bump allocation)
- Medium objects: < 50ns (bump allocation)
- Large objects: ~100-200ns (malloc)

**Actor Capacity:**
- Practical limit: ~100K concurrent actors
- Per-actor overhead: ~512 bytes
- 100K actors = ~50MB baseline RAM

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
- 10-50x faster than malloc for small allocations
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
- 10-100x faster than mutex-based queues under contention
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

- Hindley-Milner Type Inference: Damas & Milner (1982)
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
