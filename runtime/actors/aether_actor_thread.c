#include "aether_actor.h"
#include "aether_actor_thread.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void* aether_actor_thread(void* arg) {
    void* actor_ptr = arg;
    typedef struct {
        int active;
        int id;
        Mailbox mailbox;
        void (*step)(void*);
        pthread_t thread;
        int auto_process;
        int assigned_core;
    } GenericActor;

    GenericActor* actor = (GenericActor*)actor_ptr;
    
    while (actor->active) {
        if (actor->mailbox.count > 0) {
            if (actor->step) {
                actor->step(actor_ptr);
            }
        } else {
            usleep(100);
        }
    }
    
    return NULL;
}
