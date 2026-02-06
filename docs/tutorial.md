# Aether Language Tutorial

This tutorial covers the fundamentals of the Aether programming language. Aether is a compiled language focused on lightweight, high-performance actor-based concurrency.

## Table of Contents
1. [Hello Actor](#1-hello-actor)
2. [Arrays and Data](#2-arrays-and-data)
3. [Multiple Actors](#3-multiple-actors)
4. [Multi-Core](#4-multi-core)
5. [Complete Example](#5-complete-example)

---

## 1. Hello Actor

### Your First Actor

Actors are the core abstraction in Aether. An actor is a lightweight concurrent entity that processes messages.

```aether
message Ping {}

actor counter {
    state count = 0

    receive {
        Ping() -> {
            count = count + 1
            print(count)
        }
    }
}

main() {
    c = spawn(counter())
    c ! Ping {}
}
```

**What happens here:**
- `message Ping {}` defines a message type
- `actor counter { ... }` defines an actor named `counter`
- `state count = 0` declares the actor's private state
- `receive { ... }` is the message handler with pattern matching
- `spawn(counter())` creates a new counter actor
- `c ! Ping {}` sends a Ping message to the actor

### Key Concepts
- **State**: Each actor has private state that only it can access
- **Messages**: Actors communicate only through messages (no shared memory)
- **Lightweight**: Actors have minimal memory overhead compared to OS threads

---

## 2. Arrays and Data

### Fixed Arrays

```aether
main() {
    int[5] nums;

    values = [10, 20, 30, 40, 50];

    nums[0] = 100;
    nums[1] = 200;
    x = values[0];  // x = 10

    print(x);
}
```

### Dynamic Arrays

Use `make()` for runtime-sized arrays:

```aether
main() {
    buffer = make([]int, 1000);

    buffer[0] = 42;
    buffer[999] = 100;

    val = buffer[0];
    print(val);  // Prints 42
}
```

### Multi-Dimensional Arrays

```aether
main() {
    int[3][3] matrix;

    matrix[0][0] = 1;
    matrix[1][1] = 5;
    matrix[2][2] = 9;
}
```

### Arrays in Actors

Actors can have array state:

```aether
message Store { value: int }

actor buffer {
    state int[100] data;
    state int count = 0;

    receive {
        Store(value) -> {
            data[count] = value
            count = count + 1
        }
    }
}

main() {
    buf = spawn(buffer())

    for (i = 0; i < 10; i = i + 1) {
        buf ! Store { value: i * 10 }
    }
}
```

---

## 3. Multiple Actors

### Actor Communication

Actors can send messages to each other:

```aether
message DoWork {}

actor worker {
    state work_done = 0

    receive {
        DoWork() -> {
            work_done = work_done + 1
            print(work_done)
        }
    }
}

main() {
    w1 = spawn(worker())
    w2 = spawn(worker())
    w3 = spawn(worker())

    w1 ! DoWork {}
    w2 ! DoWork {}
    w3 ! DoWork {}
}
```

### Control Flow

Aether supports standard control structures:

```aether
message Increment {}
message Decrement {}
message Reset {}

actor processor {
    state count = 0

    receive {
        Increment() -> {
            count = count + 1
        }
        Decrement() -> {
            count = count - 1
        }
        Reset() -> {
            count = 0
        }
    }
}
```

### Loops

```aether
main() {
    sum = 0;

    for (i = 0; i < 100; i = i + 1) {
        sum = sum + i;
    }

    j = 0;
    while (j < 10) {
        print(j);
        j = j + 1;
    }

    print(sum);
}
```

---

## 4. Multi-Core

### Automatic Multi-Core

Aether automatically distributes actors across CPU cores using round-robin assignment:

```aether
message DoWork { value: int }

actor worker {
    state work_done = 0

    receive {
        DoWork(value) -> {
            work_done = work_done + 1
        }
    }
}

main() {
    actors = make([]actor worker, 100);

    for (i = 0; i < 100; i = i + 1) {
        actors[i] = spawn(worker())
    }

    for (i = 0; i < 100; i = i + 1) {
        actors[i] ! DoWork { value: i }
    }
}
```

Performance varies based on workload patterns and hardware. The runtime includes optimizations for message passing, cache locality, and multi-core scalability. See [benchmarks/cross-language](../benchmarks/cross-language/) for methodology and comparisons.

### How It Works

1. **Round-Robin**: `actor_id % num_cores` determines initial core assignment
2. **Migration**: Actors migrate toward cores that send them the most messages
3. **Lock-Free Queues**: Cross-core messages use lock-free ring buffers
4. **Thread-Local Pools**: Message payloads allocated from per-thread pools

---

## 5. Complete Example

A complete application demonstrating actors, messaging, and control flow:

```aether
message Compute { value: int }
message PrintCount {}

actor worker {
    state int[1000] results;
    state count = 0

    receive {
        Compute(value) -> {
            results[count] = value * value
            count = count + 1
        }
        PrintCount() -> {
            print(count)
        }
    }
}

message Dispatch { value: int }

actor coordinator {
    state next_worker = 0

    receive {
        Dispatch(value) -> {
            // Route work to workers round-robin
            next_worker = next_worker + 1
            if (next_worker >= 4) {
                next_worker = 0
            }
        }
    }
}

main() {
    w1 = spawn(worker())
    w2 = spawn(worker())
    w3 = spawn(worker())
    w4 = spawn(worker())

    for (i = 0; i < 100; i = i + 1) {
        // Distribute work across workers
        w1 ! Compute { value: i }
    }

    w1 ! PrintCount {}
    w2 ! PrintCount {}
    w3 ! PrintCount {}
    w4 ! PrintCount {}
}
```

---

## Next Steps

### Compile and Run

```bash
# Run directly
ae run my_program.ae

# Or build an executable
ae build my_program.ae -o my_program
./my_program
```

### Best Practices

1. **Keep actors small**: each actor should have a focused responsibility
2. **Treat messages as read-only**: do not modify message data after sending
3. **No shared state**: communicate only through messages
4. **Use descriptive message names**: `Increment` is clearer than a numeric type code

### Further Reading

- [Getting Started](getting-started.md) - Installation and setup
- [Standard Library](stdlib-reference.md) - Collections, I/O, networking
- [Architecture](architecture.md) - Compiler and runtime internals
- [Runtime Optimizations](runtime-optimizations.md) - Performance techniques

---

## Summary

This tutorial covered:
- Actors and message passing
- Arrays (fixed and dynamic)
- Control flow (if, for, while)
- Multi-core distribution and migration
- Building complete applications

Aether compiles to C with no VM overhead. The runtime handles core assignment, message routing, and memory pooling automatically.

See [benchmarks/cross-language](../benchmarks/cross-language/) for comparative analysis.
