#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aether_runtime.h"

typedef struct Point {
    int x;
    int y;
} Point;

int main() {
    aether_runtime_init(4); // Initialize with 4 worker threads
    
    {
printf("Struct test\n");
    }
    
    aether_runtime_shutdown();
    return 0;
}
