#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aether_runtime.h"

int main() {
    aether_runtime_init(4); // Initialize with 4 worker threads
    
    {
printf("Hello, Aether World!\n");
printf("This is a clean, professional compiler.\n");
int x = 42;
int y = 8;
if ((x > y)) {
            {
printf("x (%d) is greater than y (%d)\n", x, y);
            }
        }
printf("Aether compilation successful!\n");
    }
    
    aether_runtime_shutdown();
    return 0;
}
