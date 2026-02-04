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
actor counter {
    state int count = 0;

    receive(msg) {
        count++;
        print(count);
    }
}

main() {
    c = spawn_counter();
    send_counter(c, 1, 0);
}
```

**What happens here:**
- `actor counter { ... }` defines an actor named `counter`
- `state int count = 0` declares the actor's private state
- `receive(msg) { ... }` is the message handler (called when messages arrive)
- `spawn_counter()` creates a new counter actor
- `send_counter(c, 1, 0)` sends a message to the actor

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
actor buffer {
    state int[100] data;
    state int count = 0;

    receive(msg) {
        if (msg.type == 1) {
            data[count] = msg.payload;
            count++;
        }
    }
}

main() {
    buf = spawn_buffer();

    for (i = 0; i < 10; i++) {
        send_buffer(buf, 1, i * 10);
    }
}
```

---

## 3. Multiple Actors

### Actor Communication

Actors can send messages to each other:

```aether
actor worker {
    state int id;
    state int work_done = 0;

    receive(msg) {
        if (msg.type == 1) {
            work_done++;
            print(work_done);
        }
    }
}

main() {
    w1 = spawn_worker();
    w2 = spawn_worker();
    w3 = spawn_worker();

    send_worker(w1, 1, 0);
    send_worker(w2, 1, 0);
    send_worker(w3, 1, 0);
}
```

### Control Flow

Aether supports standard control structures:

```aether
actor processor {
    state int count = 0;

    receive(msg) {
        switch (msg.type) {
            case 1:
                count++;
                break;
            case 2:
                count--;
                break;
            case 3:
                count = 0;
                break;
            default:
                print(0);
        }

        if (count > 100) {
            count = 0;
        } else if (count < 0) {
            count = 0;
        }

        print(count);
    }
}
```

### Loops

```aether
main() {
    sum = 0;

    for (i = 0; i < 100; i++) {
        sum = sum + i;
    }

    j = 0;
    while (j < 10) {
        print(j);
        j++;
    }

    print(sum);
}
```

---

## 4. Multi-Core

### Automatic Multi-Core

Aether automatically distributes actors across CPU cores using round-robin assignment:

```aether
main() {
    actors = make([]actor worker, 100);

    for (i = 0; i < 100; i++) {
        actors[i] = spawn_worker();
    }

    for (i = 0; i < 100; i++) {
        send_worker(actors[i], 1, i);
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
actor worker {
    state int id;
    state int[1000] results;
    state int count = 0;

    receive(msg) {
        switch (msg.type) {
            case 1:
                results[count] = msg.payload * msg.payload;
                count++;
                break;

            case 2:
                print(count);
                break;

            default:
                print(0);
        }
    }
}

actor coordinator {
    state worker[4] workers;
    state int next_worker = 0;

    receive(msg) {
        if (msg.type == 1) {
            w = workers[next_worker];
            send_worker(w, 1, msg.payload);

            next_worker++;
            if (next_worker >= 4) {
                next_worker = 0;
            }
        }
    }
}

main() {
    coord = spawn_coordinator();

    w1 = spawn_worker();
    w2 = spawn_worker();
    w3 = spawn_worker();
    w4 = spawn_worker();

    for (i = 0; i < 100; i++) {
        send_coordinator(coord, 1, i);
    }

    send_worker(w1, 2, 0);
    send_worker(w2, 2, 0);
    send_worker(w3, 2, 0);
    send_worker(w4, 2, 0);
}
```

---

## Next Steps

### Compile and Run

```bash
# Compile your Aether program
./build/aetherc my_program.ae output.c

# Compile the generated C code
gcc -O3 -march=native output.c -Iruntime runtime/scheduler/multicore_scheduler.c \
    runtime/actors/aether_send_message.c runtime/aether_numa.c \
    -o my_program -pthread -lm

# Run
./my_program
```

### Best Practices

1. **Keep actors small**: each actor should have a focused responsibility
2. **Treat messages as read-only**: do not modify message data after sending
3. **No shared state**: communicate only through messages
4. **Handle all cases**: include a `default` case in switch statements

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
- Control flow (if, for, while, switch)
- Multi-core distribution and migration
- Building complete applications

Aether compiles to C with no VM overhead. The runtime handles core assignment, message routing, and memory pooling automatically.

See [benchmarks/cross-language](../benchmarks/cross-language/) for comparative analysis.
