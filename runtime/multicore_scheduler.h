#ifndef MULTICORE_SCHEDULER_H
#define MULTICORE_SCHEDULER_H

#include <pthread.h>
#include <stdatomic.h>
#include "actor_state_machine.h"
#include "lockfree_queue.h"

#define MAX_ACTORS_PER_CORE 10000
#define MAX_CORES 16
#define BATCH_SIZE 32  // Process up to 32 messages per actor per round

typedef struct {
    int id;
    int active;
    int assigned_core;
    Mailbox mailbox;
    void (*step)(void*);
} ActorBase;

typedef struct {
    int core_id;
    pthread_t thread;
    ActorBase** actors;
    int actor_count;
    int capacity;
    LockFreeQueue incoming_queue;
    atomic_int running;
    atomic_int work_count;  // For work stealing
    atomic_int steal_attempts;  // Statistics
} Scheduler;

extern Scheduler schedulers[MAX_CORES];
extern int num_cores;
extern atomic_int next_actor_id;

void scheduler_init(int cores);
void scheduler_start();
void scheduler_stop();
void scheduler_wait();

int scheduler_register_actor(ActorBase* actor, int preferred_core);
void scheduler_send_local(ActorBase* actor, Message msg);
void scheduler_send_remote(ActorBase* actor, Message msg, int from_core);

#endif
