// Benchmark: Computed Goto vs Function Pointer Dispatch
// Tests the performance improvement of computed goto dispatch

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 100000000
#define NUM_MESSAGE_TYPES 10

// Simulated actor state
typedef struct {
    int counter;
    int value;
} Actor;

typedef struct {
    int type;
    int payload;
} Message;

// Message handlers
void handle_increment(Actor* self, Message* msg) {
    self->counter += msg->payload;
}

void handle_decrement(Actor* self, Message* msg) {
    self->counter -= msg->payload;
}

void handle_set_value(Actor* self, Message* msg) {
    self->value = msg->payload;
}

void handle_get_value(Actor* self, Message* msg) {
    msg->payload = self->value;
}

void handle_reset(Actor* self, Message* msg) {
    self->counter = 0;
    self->value = 0;
}

void handle_multiply(Actor* self, Message* msg) {
    self->counter *= msg->payload;
}

void handle_divide(Actor* self, Message* msg) {
    if (msg->payload != 0) {
        self->counter /= msg->payload;
    }
}

void handle_nop(Actor* self, Message* msg) {
    // Do nothing
}

// Function pointer dispatch (baseline)
typedef void (*handler_fn)(Actor*, Message*);

handler_fn handlers[NUM_MESSAGE_TYPES];

void init_handlers() {
    handlers[0] = handle_increment;
    handlers[1] = handle_decrement;
    handlers[2] = handle_set_value;
    handlers[3] = handle_get_value;
    handlers[4] = handle_reset;
    handlers[5] = handle_multiply;
    handlers[6] = handle_divide;
    handlers[7] = handle_nop;
    handlers[8] = handle_nop;
    handlers[9] = handle_nop;
}

void dispatch_function_pointer(Actor* self, Message* msg) {
    if (msg->type >= 0 && msg->type < NUM_MESSAGE_TYPES) {
        handlers[msg->type](self, msg);
    }
}

// Computed goto dispatch (optimized)
void dispatch_computed_goto(Actor* self, Message* msg) {
    static void* dispatch_table[] = {
        &&handle_0, &&handle_1, &&handle_2, &&handle_3, &&handle_4,
        &&handle_5, &&handle_6, &&handle_7, &&handle_8, &&handle_9
    };
    
    int type = msg->type;
    if (type >= 0 && type < NUM_MESSAGE_TYPES) {
        goto *dispatch_table[type];
    }
    return;
    
handle_0:
    handle_increment(self, msg);
    return;
handle_1:
    handle_decrement(self, msg);
    return;
handle_2:
    handle_set_value(self, msg);
    return;
handle_3:
    handle_get_value(self, msg);
    return;
handle_4:
    handle_reset(self, msg);
    return;
handle_5:
    handle_multiply(self, msg);
    return;
handle_6:
    handle_divide(self, msg);
    return;
handle_7:
    handle_nop(self, msg);
    return;
handle_8:
    handle_nop(self, msg);
    return;
handle_9:
    handle_nop(self, msg);
    return;
}

// Switch statement dispatch (comparison)
void dispatch_switch(Actor* self, Message* msg) {
    switch (msg->type) {
        case 0: handle_increment(self, msg); break;
        case 1: handle_decrement(self, msg); break;
        case 2: handle_set_value(self, msg); break;
        case 3: handle_get_value(self, msg); break;
        case 4: handle_reset(self, msg); break;
        case 5: handle_multiply(self, msg); break;
        case 6: handle_divide(self, msg); break;
        case 7: handle_nop(self, msg); break;
        case 8: handle_nop(self, msg); break;
        case 9: handle_nop(self, msg); break;
    }
}

double benchmark(const char* name, void (*dispatch)(Actor*, Message*)) {
    Actor actor = {0, 100};
    Message msg;
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    // Random message pattern to prevent perfect branch prediction
    for (int i = 0; i < ITERATIONS; i++) {
        msg.type = (i * 7) % NUM_MESSAGE_TYPES;  // Pseudo-random
        msg.payload = 1;
        dispatch(&actor, &msg);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    double ops_per_sec = ITERATIONS / elapsed;
    
    printf("%-25s %.2f M dispatches/sec  (%.3f sec)\n", 
           name, ops_per_sec / 1e6, elapsed);
    
    return ops_per_sec;
}

int main() {
    printf("========================================\n");
    printf("  Message Dispatch Performance Test\n");
    printf("========================================\n");
    printf("Iterations: %d\n", ITERATIONS);
    printf("Message types: %d\n\n", NUM_MESSAGE_TYPES);
    
    init_handlers();
    
    // Warmup
    Actor warmup_actor = {0, 100};
    Message warmup_msg = {0, 1};
    for (int i = 0; i < 1000000; i++) {
        dispatch_function_pointer(&warmup_actor, &warmup_msg);
        dispatch_computed_goto(&warmup_actor, &warmup_msg);
        dispatch_switch(&warmup_actor, &warmup_msg);
    }
    
    printf("Running benchmarks...\n\n");
    
    double switch_speed = benchmark("Switch statement:", dispatch_switch);
    double func_ptr_speed = benchmark("Function pointer:", dispatch_function_pointer);
    double computed_goto_speed = benchmark("Computed goto:", dispatch_computed_goto);
    
    printf("\n========================================\n");
    printf("  Results\n");
    printf("========================================\n\n");
    
    printf("Switch baseline:        %.2f M/sec (1.00x)\n", switch_speed / 1e6);
    printf("Function pointer:       %.2f M/sec (%.2fx)\n", 
           func_ptr_speed / 1e6, func_ptr_speed / switch_speed);
    printf("Computed goto:          %.2f M/sec (%.2fx)\n", 
           computed_goto_speed / 1e6, computed_goto_speed / switch_speed);
    
    printf("\n");
    printf("Computed goto speedup:  %.1f%% faster than switch\n", 
           (computed_goto_speed / switch_speed - 1.0) * 100);
    printf("                        %.1f%% faster than function ptr\n",
           (computed_goto_speed / func_ptr_speed - 1.0) * 100);
    
    printf("\n");
    if (computed_goto_speed > func_ptr_speed * 1.10) {
        printf("✅ SIGNIFICANT IMPROVEMENT - Use computed goto\n");
    } else {
        printf("⚠️  Marginal improvement - Compiler may be optimizing\n");
    }
    
    return 0;
}
