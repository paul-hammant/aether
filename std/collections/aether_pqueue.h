#ifndef AETHER_PQUEUE_H
#define AETHER_PQUEUE_H

#include <stddef.h>
#include <stdbool.h>

// Priority Queue (Binary Heap) implementation
// Min-heap or Max-heap based on comparator

typedef struct {
    void** data;
    size_t size;
    size_t capacity;
    int (*compare)(const void*, const void*); // <0 if a<b, 0 if a==b, >0 if a>b
    void (*element_free)(void*);
    void* (*element_clone)(const void*);
} PriorityQueue;

// Creation and destruction
PriorityQueue* pqueue_create(size_t initial_capacity,
                            int (*compare)(const void*, const void*),
                            void (*element_free)(void*),
                            void* (*element_clone)(const void*));
void pqueue_free(PriorityQueue* pq);

// Core operations - O(log n)
bool pqueue_insert(PriorityQueue* pq, void* element);
void* pqueue_extract(PriorityQueue* pq);  // Extract min/max

// Query operations - O(1)
void* pqueue_peek(PriorityQueue* pq);     // Peek at min/max
size_t pqueue_size(PriorityQueue* pq);
bool pqueue_is_empty(PriorityQueue* pq);

// Utility
void pqueue_clear(PriorityQueue* pq);
bool pqueue_contains(PriorityQueue* pq, const void* element, 
                    bool (*equals)(const void*, const void*));

// Heapify from array
PriorityQueue* pqueue_from_array(void** elements, size_t count,
                                int (*compare)(const void*, const void*),
                                void (*element_free)(void*),
                                void* (*element_clone)(const void*));

// Common comparators
int pqueue_compare_int_min(const void* a, const void* b);  // Min-heap for ints
int pqueue_compare_int_max(const void* a, const void* b);  // Max-heap for ints

#endif // AETHER_PQUEUE_H

