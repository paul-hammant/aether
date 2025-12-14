#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aether_runtime.h"

int main() {
    aether_runtime_init(4); // Initialize with 4 worker threads
    
    {
printf("Testing simple for loop\n");
int i = 1;
for (; (i <= 3); ++i) {
            {
printf("i = %d\n", i);
            }
        }
printf("Done\n");
    }
    
    aether_runtime_shutdown();
    return 0;
}
