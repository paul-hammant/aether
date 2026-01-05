#include "../runtime/aether_test.h"
#include "../std/collections/aether_hashmap.h"
#include "../std/collections/aether_set.h"
#include "../std/collections/aether_vector.h"
#include "../std/collections/aether_pqueue.h"
#include <string.h>
#include <time.h>

// HashMap Tests
TEST(hashmap_create_and_destroy) {
    HashMap* map = hashmap_create_string_to_int(16);
    ASSERT_NOT_NULL(map);
    ASSERT_EQ(hashmap_size(map), 0);
    ASSERT_TRUE(hashmap_is_empty(map));
    hashmap_free(map);
}

TEST(hashmap_insert_and_get) {
    HashMap* map = hashmap_create_string_to_int(16);
    
    char* key1 = strdup("hello");
    hashmap_insert(map, key1, (void*)42);
    
    void* val = hashmap_get(map, "hello");
    ASSERT_EQ((long)val, 42);
    ASSERT_EQ(hashmap_size(map), 1);
    
    hashmap_free(map);
}

TEST(hashmap_multiple_inserts) {
    HashMap* map = hashmap_create_string_to_int(16);
    
    for (int i = 0; i < 100; i++) {
        char* key = (char*)malloc(32);
        snprintf(key, 32, "key%d", i);
        hashmap_insert(map, key, (void*)(long)i);
    }
    
    ASSERT_EQ(hashmap_size(map), 100);
    
    for (int i = 0; i < 100; i++) {
        char key[32];
        snprintf(key, 32, "key%d", i);
        void* val = hashmap_get(map, key);
        ASSERT_EQ((long)val, i);
    }
    
    hashmap_free(map);
}

TEST(hashmap_remove) {
    HashMap* map = hashmap_create_string_to_int(16);
    
    char* key1 = strdup("hello");
    hashmap_insert(map, key1, (void*)42);
    ASSERT_TRUE(hashmap_contains(map, "hello"));
    
    hashmap_remove(map, "hello");
    ASSERT_FALSE(hashmap_contains(map, "hello"));
    ASSERT_EQ(hashmap_size(map), 0);
    
    hashmap_free(map);
}

TEST(hashmap_resize) {
    HashMap* map = hashmap_create_string_to_int(4); // Small initial capacity
    
    // Insert enough to trigger resize
    for (int i = 0; i < 50; i++) {
        char* key = (char*)malloc(32);
        snprintf(key, 32, "key%d", i);
        hashmap_insert(map, key, (void*)(long)i);
    }
    
    // All values should still be retrievable
    for (int i = 0; i < 50; i++) {
        char key[32];
        snprintf(key, 32, "key%d", i);
        void* val = hashmap_get(map, key);
        ASSERT_EQ((long)val, i);
    }
    
    hashmap_free(map);
}

TEST(hashmap_iterator) {
    HashMap* map = hashmap_create_string_to_int(16);
    
    for (int i = 0; i < 10; i++) {
        char* key = (char*)malloc(32);
        snprintf(key, 32, "key%d", i);
        hashmap_insert(map, key, (void*)(long)i);
    }
    
    HashMapIterator iter = hashmap_iterator(map);
    void* key;
    void* value;
    int count = 0;
    
    while (hashmap_iterator_next(&iter, &key, &value)) {
        count++;
    }
    
    ASSERT_EQ(count, 10);
    hashmap_free(map);
}

// Set Tests
TEST(set_create_and_destroy) {
    Set* set = set_create_string(16);
    ASSERT_NOT_NULL(set);
    ASSERT_EQ(set_size(set), 0);
    ASSERT_TRUE(set_is_empty(set));
    set_free(set);
}

TEST(set_add_and_contains) {
    Set* set = set_create_string(16);
    
    char* elem1 = strdup("hello");
    set_add(set, elem1);
    
    ASSERT_TRUE(set_contains(set, "hello"));
    ASSERT_EQ(set_size(set), 1);
    
    // Adding duplicate should not increase size
    char* elem2 = strdup("hello");
    set_add(set, elem2);
    ASSERT_EQ(set_size(set), 1);
    
    set_free(set);
}

TEST(set_union) {
    Set* a = set_create_string(16);
    Set* b = set_create_string(16);
    
    set_add(a, strdup("a"));
    set_add(a, strdup("b"));
    set_add(b, strdup("b"));
    set_add(b, strdup("c"));
    
    Set* result = set_union(a, b);
    ASSERT_EQ(set_size(result), 3);
    ASSERT_TRUE(set_contains(result, "a"));
    ASSERT_TRUE(set_contains(result, "b"));
    ASSERT_TRUE(set_contains(result, "c"));
    
    set_free(a);
    set_free(b);
    set_free(result);
}

TEST(set_intersection) {
    Set* a = set_create_string(16);
    Set* b = set_create_string(16);
    
    set_add(a, strdup("a"));
    set_add(a, strdup("b"));
    set_add(b, strdup("b"));
    set_add(b, strdup("c"));
    
    Set* result = set_intersection(a, b);
    ASSERT_EQ(set_size(result), 1);
    ASSERT_TRUE(set_contains(result, "b"));
    ASSERT_FALSE(set_contains(result, "a"));
    ASSERT_FALSE(set_contains(result, "c"));
    
    set_free(a);
    set_free(b);
    set_free(result);
}

TEST(set_difference) {
    Set* a = set_create_string(16);
    Set* b = set_create_string(16);
    
    set_add(a, strdup("a"));
    set_add(a, strdup("b"));
    set_add(b, strdup("b"));
    set_add(b, strdup("c"));
    
    Set* result = set_difference(a, b);
    ASSERT_EQ(set_size(result), 1);
    ASSERT_TRUE(set_contains(result, "a"));
    
    set_free(a);
    set_free(b);
    set_free(result);
}

// Vector Tests
TEST(vector_create_and_destroy) {
    Vector* vec = vector_create(16, NULL, NULL);
    ASSERT_NOT_NULL(vec);
    ASSERT_EQ(vector_size(vec), 0);
    ASSERT_TRUE(vector_is_empty(vec));
    vector_free(vec);
}

TEST(vector_push_and_get) {
    Vector* vec = vector_create(16, NULL, NULL);
    
    vector_push(vec, (void*)10);
    vector_push(vec, (void*)20);
    vector_push(vec, (void*)30);
    
    ASSERT_EQ(vector_size(vec), 3);
    ASSERT_EQ((long)vector_get(vec, 0), 10);
    ASSERT_EQ((long)vector_get(vec, 1), 20);
    ASSERT_EQ((long)vector_get(vec, 2), 30);
    
    vector_free(vec);
}

TEST(vector_pop) {
    Vector* vec = vector_create(16, NULL, NULL);
    
    vector_push(vec, (void*)10);
    vector_push(vec, (void*)20);
    
    void* val = vector_pop(vec);
    ASSERT_EQ((long)val, 20);
    ASSERT_EQ(vector_size(vec), 1);
    
    vector_free(vec);
}

TEST(vector_set) {
    Vector* vec = vector_create(16, NULL, NULL);
    
    vector_push(vec, (void*)10);
    vector_set(vec, 0, (void*)42);
    
    ASSERT_EQ((long)vector_get(vec, 0), 42);
    
    vector_free(vec);
}

TEST(vector_insert) {
    Vector* vec = vector_create(16, NULL, NULL);
    
    vector_push(vec, (void*)10);
    vector_push(vec, (void*)30);
    vector_insert(vec, 1, (void*)20);
    
    ASSERT_EQ(vector_size(vec), 3);
    ASSERT_EQ((long)vector_get(vec, 0), 10);
    ASSERT_EQ((long)vector_get(vec, 1), 20);
    ASSERT_EQ((long)vector_get(vec, 2), 30);
    
    vector_free(vec);
}

TEST(vector_remove) {
    Vector* vec = vector_create(16, NULL, NULL);
    
    vector_push(vec, (void*)10);
    vector_push(vec, (void*)20);
    vector_push(vec, (void*)30);
    
    vector_remove(vec, 1);
    
    ASSERT_EQ(vector_size(vec), 2);
    ASSERT_EQ((long)vector_get(vec, 0), 10);
    ASSERT_EQ((long)vector_get(vec, 1), 30);
    
    vector_free(vec);
}

TEST(vector_resize) {
    Vector* vec = vector_create(2, NULL, NULL); // Small initial capacity
    
    for (int i = 0; i < 100; i++) {
        vector_push(vec, (void*)(long)i);
    }
    
    ASSERT_EQ(vector_size(vec), 100);
    
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ((long)vector_get(vec, i), i);
    }
    
    vector_free(vec);
}

// PriorityQueue Tests
TEST(pqueue_create_and_destroy) {
    PriorityQueue* pq = pqueue_create(16, pqueue_compare_int_min, NULL, NULL);
    ASSERT_NOT_NULL(pq);
    ASSERT_EQ(pqueue_size(pq), 0);
    ASSERT_TRUE(pqueue_is_empty(pq));
    pqueue_free(pq);
}

TEST(pqueue_insert_and_extract_min) {
    PriorityQueue* pq = pqueue_create(16, pqueue_compare_int_min, NULL, NULL);
    
    pqueue_insert(pq, (void*)30);
    pqueue_insert(pq, (void*)10);
    pqueue_insert(pq, (void*)20);
    
    ASSERT_EQ(pqueue_size(pq), 3);
    
    void* val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 10);
    
    val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 20);
    
    val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 30);
    
    ASSERT_TRUE(pqueue_is_empty(pq));
    
    pqueue_free(pq);
}

TEST(pqueue_insert_and_extract_max) {
    PriorityQueue* pq = pqueue_create(16, pqueue_compare_int_max, NULL, NULL);
    
    pqueue_insert(pq, (void*)30);
    pqueue_insert(pq, (void*)10);
    pqueue_insert(pq, (void*)20);
    
    void* val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 30);
    
    val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 20);
    
    val = pqueue_extract(pq);
    ASSERT_EQ((long)val, 10);
    
    pqueue_free(pq);
}

TEST(pqueue_peek) {
    PriorityQueue* pq = pqueue_create(16, pqueue_compare_int_min, NULL, NULL);
    
    pqueue_insert(pq, (void*)30);
    pqueue_insert(pq, (void*)10);
    
    void* val = pqueue_peek(pq);
    ASSERT_EQ((long)val, 10);
    ASSERT_EQ(pqueue_size(pq), 2); // Peek should not remove
    
    pqueue_free(pq);
}

TEST(pqueue_large) {
    PriorityQueue* pq = pqueue_create(16, pqueue_compare_int_min, NULL, NULL);
    
    // Insert 100 random values
    for (int i = 0; i < 100; i++) {
        pqueue_insert(pq, (void*)(long)(100 - i));
    }
    
    // Extract should give sorted order
    long prev = 0;
    for (int i = 0; i < 100; i++) {
        long val = (long)pqueue_extract(pq);
        ASSERT_TRUE(val >= prev);
        prev = val;
    }
    
    pqueue_free(pq);
}

// Performance Tests
TEST(hashmap_performance) {
    HashMap* map = hashmap_create_string_to_int(256);
    
    clock_t start = clock();
    
    // Insert 10000 elements
    for (int i = 0; i < 10000; i++) {
        char* key = (char*)malloc(32);
        snprintf(key, 32, "key%d", i);
        hashmap_insert(map, key, (void*)(long)i);
    }
    
    // Lookup 10000 elements
    for (int i = 0; i < 10000; i++) {
        char key[32];
        snprintf(key, 32, "key%d", i);
        void* val = hashmap_get(map, key);
        ASSERT_EQ((long)val, i);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("    HashMap 10k insert+lookup: %.2fms\n", elapsed);
    
    hashmap_free(map);
}

TEST(vector_performance) {
    Vector* vec = vector_create(256, NULL, NULL);
    
    clock_t start = clock();
    
    // Push 100000 elements
    for (int i = 0; i < 100000; i++) {
        vector_push(vec, (void*)(long)i);
    }
    
    // Access all elements
    for (int i = 0; i < 100000; i++) {
        void* val = vector_get(vec, i);
        ASSERT_EQ((long)val, i);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("    Vector 100k push+get: %.2fms\n", elapsed);
    
    vector_free(vec);
}

TEST(pqueue_performance) {
    PriorityQueue* pq = pqueue_create(256, pqueue_compare_int_min, NULL, NULL);
    
    clock_t start = clock();
    
    // Insert 10000 elements
    for (int i = 0; i < 10000; i++) {
        pqueue_insert(pq, (void*)(long)(10000 - i));
    }
    
    // Extract all elements
    for (int i = 0; i < 10000; i++) {
        pqueue_extract(pq);
    }
    
    clock_t end = clock();
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("    PriorityQueue 10k insert+extract: %.2fms\n", elapsed);
    
    pqueue_free(pq);
}

