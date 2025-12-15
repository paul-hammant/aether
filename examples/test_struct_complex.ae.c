#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "aether_runtime.h"

typedef struct Vector2D {
    int x;
    int y;
} Vector2D;

typedef struct Player {
    int health;
    int score;
} Player;

int main() {
    aether_runtime_init(4); // Initialize with 4 worker threads
    
    {
int x = 10;
int y = 20;
printf("Testing structs...\n");
printf("x=%d, y=%d\n", x, y);
    }
    
    aether_runtime_shutdown();
    return 0;
}
