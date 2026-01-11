#include <stdio.h>
#include "../runtime/aether_numa.h"

int main() {
    printf("NUMA Topology Detection\n");
    printf("=======================\n\n");
    
    aether_numa_topology_t topo = aether_numa_init();
    
    printf("NUMA Available: %s\n", topo.available ? "Yes" : "No");
    printf("Number of NUMA nodes: %d\n", topo.num_nodes);
    printf("Number of CPUs: %d\n\n", topo.num_cpus);
    
    if (topo.available && topo.cpu_to_node) {
        printf("CPU to NUMA Node Mapping:\n");
        for (int i = 0; i < topo.num_cpus; i++) {
            int node = aether_numa_node_of_cpu(i);
            printf("  CPU %2d -> NUMA Node %d\n", i, node);
        }
    } else {
        printf("Single NUMA node (UMA) or NUMA not available\n");
    }
    
    printf("\nNUMA-aware allocation is %s in the multicore scheduler.\n", 
           topo.available ? "ENABLED" : "disabled (fallback to malloc)");
    
    aether_numa_cleanup();
    return 0;
}
