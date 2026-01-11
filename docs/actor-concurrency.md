# Aether Runtime Guide

Understanding the actor runtime, concurrency model, and message passing system.

## Performance Optimizations

The actor runtime includes several performance optimizations that are applied automatically based on workload characteristics.

### Scheduler Design

**Partitioned Multicore Scheduler:**
- Static actor-to-core assignment at spawn time
- Each core processes only its local actors (zero cross-core traffic in fast path)
- Work stealing activated when cores become idle
- Non-blocking work stealing: failed attempts don't block progress
- NUMA-aware CPU pinning when available (Linux, Windows)

**Work Stealing:**
- Triggered after 5000 idle scheduler cycles
- Selects busiest core (most pending work)
- Steals entire actors (not individual messages)
- Preserves cache locality and minimizes synchronization

**Message Delivery:**
- Local messages: Direct mailbox writes (no queue overhead)
- Remote messages: Lock-free queue with adaptive backpressure
- Adaptive batch processing: 32-256 messages per core cycle

### Implemented Optimizations

**Message Coalescing:**
- Batches multiple messages to reduce atomic operations
- Processes up to 256 messages per scheduler cycle
- Reduces per-message overhead

**Actor Pooling:**
- Reuses actor instances instead of repeated allocation
- Lock-free pool with pre-allocated actors
- Falls back to malloc when exhausted

**Direct Send Optimization:**
- Bypasses queue for same-core actors
- Directly writes to actor mailbox
- Eliminates enqueue/dequeue latency

**Adaptive Batching:**
- Dynamically adjusts batch size based on queue depth
- Balances throughput and latency
- Increases batch size under load, decreases when idle

See [runtime/actors/README.md](../runtime/actors/README.md) for implementation details and usage examples.

## Actor Model

Aether implements a lightweight actor model where actors are state machines that communicate via asynchronous messages.

### Actor Lifecycle

1. **Spawn** - Create actor instance with `spawn_ActorName()`
2. **Send** - Send messages with `send_ActorName()`
3. **Step** - Process messages with `ActorName_step()`
4. **Terminate** - Actor becomes inactive when mailbox is empty

### Actor Structure

Each actor is a C struct:

```c
typedef struct Counter {
    int id;              // Unique actor ID
    int active;          // Active flag
    int assigned_core;   // Core assignment (multi-core)
    Mailbox mailbox;     // Message queue
    void (*step)(void*); // Step function pointer
    int count;           // User state variables
} Counter;
```

## Message Passing

### Mailbox

Each actor has a ring buffer mailbox (16 messages by default):

```c
typedef struct {
    Message messages[MAILBOX_SIZE];
    int head;
    int tail;
    int count;
} Mailbox;
```

**Performance tip:** Use `MessageBatch` API for bulk sends:

```c
MessageBatch* batch = batch_create(256);
batch_add(batch, target_id, msg1);
batch_add(batch, target_id, msg2);
// ... add up to 256 messages
batch_send(batch);  // Bulk send
```

### Message Structure

```c
typedef struct {
    int type;           // Message type
    int sender_id;      // Sender actor ID
    int payload_int;    // Integer payload
    void* payload_ptr;  // Pointer payload
} Message;
```

### Sending Messages

Messages are sent asynchronously. The mailbox enqueues the message immediately.

```aether
send_Counter(c, 1, 42);
```

This generates:

```c
void send_Counter(Counter* actor, int type, int payload) {
    Message msg = {type, 0, payload, NULL};
    if (actor->assigned_core == current_core_id) {
        scheduler_send_local((ActorBase*)actor, msg);
    } else {
        scheduler_send_remote((ActorBase*)actor, msg, current_core_id);
    }
}
```

### Receiving Messages

The `receive` block in your actor definition becomes the step function:

```aether
receive(msg) {
    if (msg.type == 1) {
        count = count + 1;
    }
}
```

This generates:

```c
void Counter_step(Counter* self) {
    Message msg;
    if (!mailbox_receive(&self->mailbox, &msg)) {
        self->active = 0;
        return;
    }
    // Generated code from receive block
    if (msg.type == 1) {
        self->count = self->count + 1;
    }
}
```

## Single-Core Runtime

In single-core mode, actors run cooperatively. You manually call step functions:

```aether
main() {
    Counter c = spawn_Counter();
    send_Counter(c, 1, 0);
    Counter_step(c);  // Process one message
}
```

### Performance

Single-core characteristics:
- Lock-free message passing
- Zero-copy transfers
- Cooperative scheduling
- Minimal per-actor overhead (264 bytes)

## Multi-Core Runtime

The multi-core scheduler uses fixed core partitioning:

- N cores = N independent schedulers
- Each scheduler runs in a pthread
- Actors assigned to cores via hash (actor_id % num_cores)
- Cross-core messages use lock-free queues

### Initialization

```c
int main() {
    scheduler_init(4);  // Use 4 cores
    scheduler_start();
    
    // Create and use actors
    
    scheduler_stop();
    scheduler_wait();
}
```

### Core Assignment

Actors are assigned to cores at spawn time:

```c
int core = actor->id % num_cores;
```

This ensures deterministic placement and good load balancing.

### Message Routing

- **Local messages** (same core): Direct mailbox enqueue
- **Remote messages** (different core): Lock-free queue to target core

Cross-core messages incur additional latency due to cache coherence.

## Memory Management

Actors use manual memory management (malloc/free). The compiler generates allocation in spawn functions, but cleanup is manual:

```c
Counter* c = spawn_Counter();
// ... use actor ...
free(c);  // Manual cleanup
```

## Best Practices

1. **Batch Processing** - Process multiple messages per step when possible
2. **Local Communication** - Prefer actors on same core for hot paths
3. **Message Types** - Use enum constants for message types
4. **State Access** - Access state via `self->` in generated code

## Limitations

- No automatic garbage collection
- No actor supervision trees (yet)
- No pattern matching in receive blocks (yet)
- Fixed mailbox size (16 messages)
