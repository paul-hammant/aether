// NUMA-Aware Actor Allocation
// Place actors on same NUMA node as their memory for reduced latency

#ifndef AETHER_NUMA_H
#define AETHER_NUMA_H

#include <stddef.h>
#include <stdint.h>

// Platform detection
#ifdef __linux__
#include <numa.h>
#include <sched.h>
#define HAS_NUMA 1
#elif defined(_WIN32)
#include <windows.h>
#define HAS_NUMA 1
#else
#define HAS_NUMA 0
#endif

// NUMA-aware allocation interface
typedef struct {
    int numa_available;
    int num_nodes;
    int *node_cpus;  // CPUs per node
} NumaInfo;

// Initialize NUMA subsystem
static inline NumaInfo numa_init() {
    NumaInfo info = {0};
    
#if HAS_NUMA
#ifdef __linux__
    if (numa_available() != -1) {
        info.numa_available = 1;
        info.num_nodes = numa_num_configured_nodes();
    }
#elif defined(_WIN32)
    ULONG highest_node;
    if (GetNumaHighestNodeNumber(&highest_node)) {
        info.numa_available = 1;
        info.num_nodes = highest_node + 1;
    }
#endif
#endif
    
    return info;
}

// Get current thread's NUMA node
static inline int numa_get_current_node() {
#if HAS_NUMA
#ifdef __linux__
    int cpu = sched_getcpu();
    if (cpu >= 0) {
        return numa_node_of_cpu(cpu);
    }
#elif defined(_WIN32)
    PROCESSOR_NUMBER proc_num;
    GetCurrentProcessorNumberEx(&proc_num);
    
    USHORT node;
    if (GetNumaProcessorNodeEx(&proc_num, &node)) {
        return node;
    }
#endif
#endif
    return 0;  // Fallback to node 0
}

// Allocate memory on specific NUMA node
static inline void* numa_alloc_on_node(size_t size, int node) {
#if HAS_NUMA
#ifdef __linux__
    return numa_alloc_onnode(size, node);
#elif defined(_WIN32)
    return VirtualAllocExNuma(
        GetCurrentProcess(),
        NULL,
        size,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE,
        node
    );
#endif
#endif
    // Fallback to regular malloc
    return malloc(size);
}

// Allocate memory on local NUMA node
static inline void* numa_alloc_local(size_t size) {
#if HAS_NUMA
#ifdef __linux__
    return numa_alloc_local(size);
#elif defined(_WIN32)
    int node = numa_get_current_node();
    return numa_alloc_on_node(size, node);
#endif
#endif
    return malloc(size);
}

// Free NUMA-allocated memory
static inline void numa_free(void* ptr, size_t size) {
#if HAS_NUMA
#ifdef __linux__
    numa_free(ptr, size);
    return;
#elif defined(_WIN32)
    VirtualFreeEx(GetCurrentProcess(), ptr, 0, MEM_RELEASE);
    return;
#endif
#endif
    free(ptr);
}

// Get NUMA distance between nodes (latency estimate)
static inline int numa_distance(int node1, int node2) {
#if HAS_NUMA && defined(__linux__)
    return numa_distance(node1, node2);
#endif
    return (node1 == node2) ? 10 : 20;  // Estimated
}

#endif // AETHER_NUMA_H
