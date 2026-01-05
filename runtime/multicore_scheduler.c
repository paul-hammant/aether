#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "multicore_scheduler.h"

#ifdef __linux__
#define _GNU_SOURCE
#include <sched.h>
#include <errno.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

Scheduler schedulers[MAX_CORES];
int num_cores = 0;
atomic_int next_actor_id = 1;

__thread int current_core_id = -1;

// Pin thread to specific CPU core (NUMA awareness)
static void pin_to_core(int core_id) {
#ifdef __linux__
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    
    int result = pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
    if (result != 0) {
        fprintf(stderr, "Warning: Failed to pin thread to core %d: %s\n", 
                core_id, strerror(errno));
    }
#elif defined(_WIN32)
    // Windows: Use SetThreadAffinityMask
    DWORD_PTR mask = (DWORD_PTR)1 << core_id;
    if (SetThreadAffinityMask(GetCurrentThread(), mask) == 0) {
        fprintf(stderr, "Warning: Failed to pin thread to core %d\n", core_id);
    }
#endif
}

// Work stealing: try to steal work from another scheduler
static int try_steal_work(Scheduler* thief, int* last_victim_idx) {
    // Try to find a victim with work
    for (int attempt = 0; attempt < num_cores - 1; attempt++) {
        *last_victim_idx = (*last_victim_idx + 1) % num_cores;
        
        if (*last_victim_idx == thief->core_id) continue;
        
        Scheduler* victim = &schedulers[*last_victim_idx];
        int victim_work = atomic_load(&victim->work_count);
        
        // Only steal if victim has significant work
        if (victim_work > 5) {
            void* actor_ptr;
            Message msg;
            
            // Try to steal from victim's queue
            if (queue_dequeue(&victim->incoming_queue, &actor_ptr, &msg)) {
                atomic_fetch_sub(&victim->work_count, 1);
                atomic_fetch_add(&thief->work_count, 1);
                atomic_fetch_add(&thief->steal_attempts, 1);
                
                // Process stolen message immediately
                ActorBase* actor = (ActorBase*)actor_ptr;
                mailbox_send(&actor->mailbox, msg);
                actor->active = 1;
                return 1;
            }
        }
    }
    
    return 0;
}

void* scheduler_thread(void* arg) {
    Scheduler* sched = (Scheduler*)arg;
    current_core_id = sched->core_id;
    
    // NUMA awareness: pin thread to core
    pin_to_core(sched->core_id);
    
    int last_victim_idx = 0;
    int idle_count = 0;
    
    while (atomic_load(&sched->running)) {
        int work_done = 0;
        void* actor_ptr;
        Message msg;
        
        // Batch message processing: drain incoming queue in batches
        int batch_count = 0;
        while (batch_count < BATCH_SIZE && 
               queue_dequeue(&sched->incoming_queue, &actor_ptr, &msg)) {
            ActorBase* actor = (ActorBase*)actor_ptr;
            mailbox_send(&actor->mailbox, msg);
            actor->active = 1;
            atomic_fetch_sub(&sched->work_count, 1);
            batch_count++;
            work_done = 1;
        }
        
        // Process active actors
        for (int i = 0; i < sched->actor_count; i++) {
            ActorBase* actor = sched->actors[i];
            if (actor && actor->active) {
                if (actor->step) {
                    actor->step(actor);
                }
                work_done = 1;
            }
        }
        
        // Work stealing: if idle, try to steal from others
        if (!work_done) {
            idle_count++;
            
            // After a few idle cycles, attempt work stealing
            if (idle_count > 10) {
                if (try_steal_work(sched, &last_victim_idx)) {
                    idle_count = 0;
                    work_done = 1;
                }
            }
            
            // If still idle, yield to avoid busy-waiting
            if (idle_count > 100) {
                usleep(1);  // Brief sleep to reduce CPU usage
                idle_count = 50;  // Reset but don't go back to zero
            }
        } else {
            idle_count = 0;
        }
    }
    
    return NULL;
}

void scheduler_init(int cores) {
    if (cores <= 0 || cores > MAX_CORES) {
        cores = 4;
    }
    num_cores = cores;
    
    for (int i = 0; i < num_cores; i++) {
        schedulers[i].core_id = i;
        schedulers[i].actors = malloc(MAX_ACTORS_PER_CORE * sizeof(ActorBase*));
        schedulers[i].actor_count = 0;
        schedulers[i].capacity = MAX_ACTORS_PER_CORE;
        queue_init(&schedulers[i].incoming_queue);
        atomic_store(&schedulers[i].running, 0);
        atomic_store(&schedulers[i].work_count, 0);
        atomic_store(&schedulers[i].steal_attempts, 0);
    }
}

void scheduler_start() {
    for (int i = 0; i < num_cores; i++) {
        atomic_store(&schedulers[i].running, 1);
        pthread_create(&schedulers[i].thread, NULL, scheduler_thread, &schedulers[i]);
    }
}

void scheduler_stop() {
    for (int i = 0; i < num_cores; i++) {
        atomic_store(&schedulers[i].running, 0);
    }
}

void scheduler_wait() {
    for (int i = 0; i < num_cores; i++) {
        pthread_join(schedulers[i].thread, NULL);
    }
}

int scheduler_register_actor(ActorBase* actor, int preferred_core) {
    if (preferred_core < 0) {
        preferred_core = actor->id % num_cores;
    }
    
    Scheduler* sched = &schedulers[preferred_core];
    
    if (sched->actor_count >= sched->capacity) {
        return -1;
    }
    
    actor->assigned_core = preferred_core;
    sched->actors[sched->actor_count++] = actor;
    
    return preferred_core;
}

void scheduler_send_local(ActorBase* actor, Message msg) {
    mailbox_send(&actor->mailbox, msg);
    actor->active = 1;
}

void scheduler_send_remote(ActorBase* actor, Message msg, int from_core) {
    int target_core = actor->assigned_core;
    queue_enqueue(&schedulers[target_core].incoming_queue, actor, msg);
    atomic_fetch_add(&schedulers[target_core].work_count, 1);
}
