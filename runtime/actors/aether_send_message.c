#include "actor_state_machine.h"
#include "../scheduler/multicore_scheduler.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

// Thread-local core ID for optimization
extern __thread int current_core_id;

// ActorBase is defined in multicore_scheduler.h

// Send a typed message to an actor using optimized scheduler paths
// Uses SPSC queues, direct sends, and other optimizations automatically
void aether_send_message(void* actor_ptr, void* message_data, size_t message_size) {
    ActorBase* actor = (ActorBase*)actor_ptr;

    // Allocate and copy message data
    void* msg_copy = malloc(message_size);
    if (!msg_copy) return;
    memcpy(msg_copy, message_data, message_size);

    // Create mailbox message with the data pointer
    Message msg;
    msg.type = *(int*)message_data;  // First field is _message_id
    msg.sender_id = 0;
    msg.payload_int = 0;
    msg.payload_ptr = msg_copy;  // Use payload_ptr for the message data
    msg.zerocopy.data = NULL;
    msg.zerocopy.size = 0;
    msg.zerocopy.owned = 0;

    // Use optimized scheduler send paths:
    // - Same-core: direct mailbox send (no queue overhead)
    // - Cross-core: lock-free queue with batching
    // - Automatic SPSC queue usage where beneficial
    if (current_core_id >= 0 && current_core_id == actor->assigned_core) {
        // TIER 1 OPTIMIZATION: Same-core direct send
        scheduler_send_local(actor, msg);
    } else {
        // Cross-core send with lock-free queue
        scheduler_send_remote(actor, msg, current_core_id);
    }
}
