#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aether_runtime.h"

int main() {
    aether_runtime_init(4); // Initialize with 4 worker threads
    
    {
printf("Hello from Aether!");
    }
    
    aether_runtime_shutdown();
    return 0;
}
