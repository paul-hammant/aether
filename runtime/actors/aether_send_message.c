#include "actor_state_machine.h"
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

// Minimal actor base structure for accessing common fields
// All generated actors have these fields at the beginning
// MUST match the layout generated in codegen.c
typedef struct {
    int active;
    int id;
    Mailbox mailbox;
    void (*step)(void*);
    pthread_t thread;
    int auto_process;
    int assigned_core;
} ActorBase;

// Send a typed message to an actor
// The message data is copied into the mailbox
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

    // Send to actor's mailbox
    mailbox_send(&actor->mailbox, msg);
    actor->active = 1;
}
