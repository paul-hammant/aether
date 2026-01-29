#ifndef AETHER_SEND_MESSAGE_H
#define AETHER_SEND_MESSAGE_H

#include <stddef.h>

// Send a typed message to an actor
void aether_send_message(void* actor_ptr, void* message_data, size_t message_size);

#endif
