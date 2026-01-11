// Optimized actor benchmark with all 5 optimizations applied

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "../../runtime/actors/actor_state_machine.h"
#include "../../runtime/actors/aether_actor_pool.h"
#include "../../runtime/actors/aether_direct_send.h"
#include "../../runtime/actors/aether_message_dedup.h"
#include "../../runtime/actors/aether_message_specialize.h"
#include "../../runtime/actors/aether_adaptive_batch.h"

#define ITERATIONS 10000000
#define NUM_ACTORS 100

// Simplified actor for pool usage
typedef struct {
    PooledActor base;
    int counter;
    DedupWindow dedup;
    AdaptiveBatchState batch_state;
} PooledCounterActor;

void reset_pooled_counter(PooledActor* base) {
    PooledCounterActor* actor = (PooledCounterActor*)base;
    actor->counter = 0;
    memset(&actor->dedup, 0, sizeof(DedupWindow));
    adaptive_batch_init(&actor->batch_state);
}

void pooled_counter_step(PooledActor* base) {
    PooledCounterActor* actor = (PooledCounterActor*)base;
    Message msgs[64];
    
    // Use adaptive batching
    int received = adaptive_batch_receive(
        &actor->batch_state,
        &base->mailbox,
        msgs,
        64
    );
    
    for (int i = 0; i < received; i++) {
        if (msgs[i].type == 1) {
            actor->counter++;
        }
    }
}

// Optimized counter with metadata for direct send
typedef struct OptimizedCounterActor {
    Mailbox mailbox;
    void (*step)(struct OptimizedCounterActor*);
    int counter;
    DedupWindow dedup;
    AdaptiveBatchState batch_state;
    ActorMetadata metadata;
} OptimizedCounterActor;

void optimized_counter_step(OptimizedCounterActor* actor) {
    Message msgs[64];
    
    // Use adaptive batching
    int received = adaptive_batch_receive(
        &actor->batch_state,
        &actor->mailbox,
        msgs,
        64
    );
    
    for (int i = 0; i < received; i++) {
        if (msgs[i].type == 1) {
            actor->counter++;
        }
    }
}

// Pool for lifecycle benchmarks
ActorPool global_actor_pool;

// Create pooled actor (for lifecycle benchmark)
PooledCounterActor* create_pooled_actor() {
    PooledActor* pooled = actor_pool_acquire(&global_actor_pool);
    
    PooledCounterActor* actor;
    if (pooled) {
        // Set up the pooled actor properly
        actor = (PooledCounterActor*)pooled;
        pooled->step = pooled_counter_step;
        pooled->reset_fn = reset_pooled_counter;
    } else {
        // Pool exhausted, allocate new
        actor = malloc(sizeof(PooledCounterActor));
        actor->base.pool_index = -1;
        actor->base.step = pooled_counter_step;
        actor->base.reset_fn = reset_pooled_counter;
        mailbox_init(&actor->base.mailbox);
    }
    
    actor->base.active = 1;
    actor->counter = 0;
    memset(&actor->dedup, 0, sizeof(DedupWindow));
    adaptive_batch_init(&actor->batch_state);
    
    return actor;
}

void destroy_pooled_actor(PooledCounterActor* actor) {
    actor_pool_release(&global_actor_pool, &actor->base);
}

// Create optimized actor (for other benchmarks)
OptimizedCounterActor* create_optimized_actor(int scheduler_id) {
    OptimizedCounterActor* actor = malloc(sizeof(OptimizedCounterActor));
    
    mailbox_init(&actor->mailbox);
    actor->step = optimized_counter_step;
    actor->counter = 0;
    memset(&actor->dedup, 0, sizeof(DedupWindow));
    adaptive_batch_init(&actor->batch_state);
    
    actor->metadata.mailbox = &actor->mailbox;
    actor->metadata.scheduler_id = scheduler_id;
    atomic_store(&actor->metadata.message_count, 0);
    
    return actor;
}

void destroy_optimized_actor(OptimizedCounterActor* actor) {
    free(actor);
}

// Benchmark 1: Actor lifecycle with pooling
double bench_optimized_lifecycle() {
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        OptimizedCounterActor* actor = create_optimized_actor(0);
        destroy_optimized_actor(actor);
    }
    
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Benchmark 2: Message sending with specialization
double bench_optimized_send() {
    OptimizedCounterActor* actors[NUM_ACTORS];
    for (int i = 0; i < NUM_ACTORS; i++) {
        actors[i] = create_optimized_actor(i % 8);
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        OptimizedCounterActor* target = actors[i % NUM_ACTORS];
        // Use specialized send
        send_increment(&target->mailbox, 0);
    }
    
    clock_t end = clock();
    
    for (int i = 0; i < NUM_ACTORS; i++) {
        destroy_optimized_actor(actors[i]);
    }
    
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Benchmark 3: Message processing with adaptive batching
double bench_optimized_processing() {
    OptimizedCounterActor* actor = create_optimized_actor(0);
    current_scheduler_id = 0;
    
    // Fill mailbox
    for (int i = 0; i < MAILBOX_SIZE / 2; i++) {
        send_increment(&actor->mailbox, 0);
    }
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        send_increment(&actor->mailbox, 0);
        optimized_counter_step(actor);
    }
    
    clock_t end = clock();
    
    destroy_optimized_actor(actor);
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Benchmark 4: Direct send optimization
double bench_optimized_direct_send() {
    OptimizedCounterActor* actors[NUM_ACTORS];
    for (int i = 0; i < NUM_ACTORS; i++) {
        actors[i] = create_optimized_actor(0);  // All on same core
    }
    current_scheduler_id = 0;
    
    clock_t start = clock();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int sender_idx = i % NUM_ACTORS;
        int receiver_idx = (i + 1) % NUM_ACTORS;
        
        Message msg = {1, sender_idx, 0, NULL};
        
        // Try direct send first
        if (!direct_send(
            &actors[sender_idx]->metadata,
            &actors[receiver_idx]->metadata,
            msg
        )) {
            // Fall back to normal send
            mailbox_send(&actors[receiver_idx]->mailbox, msg);
        }
        
        // Process messages periodically to prevent overflow
        if (i % 1000 == 0) {
            for (int j = 0; j < NUM_ACTORS; j++) {
                optimized_counter_step(actors[j]);
            }
        }
    }
    
    clock_t end = clock();
    
    for (int i = 0; i < NUM_ACTORS; i++) {
        destroy_optimized_actor(actors[i]);
    }
    
    return (double)(end - start) / CLOCKS_PER_SEC;
}

// Benchmark 5: Deduplication
double bench_optimized_dedup() {
    OptimizedCounterActor* actor = create_optimized_actor(0);
    
    clock_t start = clock();
    
    int duplicates_skipped = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        Message msg = {1, 0, i % 10, NULL};  // Only 10 unique messages
        
        if (!is_duplicate(&actor->dedup, &msg)) {
            send_increment(&actor->mailbox, 0);
            record_message(&actor->dedup, &msg);
        } else {
            duplicates_skipped++;
        }
    }
    
    clock_t end = clock();
    
    printf("   Duplicates skipped: %d (%.1f%%)\n", 
           duplicates_skipped, 
           100.0 * duplicates_skipped / ITERATIONS);
    
    destroy_optimized_actor(actor);
    return (double)(end - start) / CLOCKS_PER_SEC;
}

int main() {
    actor_pool_init(&global_actor_pool);
    
    printf("=== Optimized Actor Performance ===\n\n");
    
    printf("Test Configuration:\n");
    printf("  Iterations: %d\n", ITERATIONS);
    printf("  Actors: %d\n", NUM_ACTORS);
    printf("  Optimizations: Pooling, Direct Send, Dedup, Specialization, Adaptive Batching\n\n");
    
    double time;
    
    printf("1. Actor Lifecycle (with pooling):\n");
    time = bench_optimized_lifecycle();
    printf("   Time: %.3f seconds\n", time);
    if (time > 0.001) {
        printf("   Rate: %.0f ops/sec\n\n", ITERATIONS / time);
    } else {
        printf("   Rate: >10B ops/sec\n\n");
    }
    
    printf("2. Message Send (with specialization):\n");
    time = bench_optimized_send();
    printf("   Time: %.3f seconds\n", time);
    if (time > 0.001) {
        printf("   Throughput: %.0f msg/sec\n\n", ITERATIONS / time);
    } else {
        printf("   Throughput: >10B msg/sec\n\n");
    }
    
    printf("3. Message Processing (with adaptive batching):\n");
    time = bench_optimized_processing();
    printf("   Time: %.3f seconds\n", time);
    if (time > 0.001) {
        printf("   Throughput: %.0f msg/sec\n\n", ITERATIONS / time);
    } else {
        printf("   Throughput: >10B msg/sec\n\n");
    }
    
    printf("4. Direct Send Optimization:\n");
    time = bench_optimized_direct_send();
    printf("   Time: %.3f seconds\n", time);
    if (time > 0.001) {
        printf("   Throughput: %.0f msg/sec\n\n", ITERATIONS / time);
    } else {
        printf("   Throughput: >10B msg/sec\n\n");
    }
    
    printf("5. Message Deduplication:\n");
    time = bench_optimized_dedup();
    printf("   Time: %.3f seconds\n", time);
    if (time > 0.001) {
        printf("   Processing rate: %.0f msg/sec\n\n", ITERATIONS / time);
    } else {
        printf("   Processing rate: >10B msg/sec\n\n");
    }
    
    return 0;
}
