#include "game.h"

#include <core/log.h>
#include <core/arena.h>

#include <stdio.h>

// Try to define all the memory we will ever need up front.
// 
// NOTE: There's quite a few dynamic arrays and hash maps
//       left to remove.
#define STATIC_ARENA_SIZE 1024*1024*64

int main(void) {

    if (!log_init(0)) {
        printf("Failed to initialize logger");
        return -1;
    }

    arena_init(&allocator, STATIC_ARENA_SIZE);
    
    Game* game = game_create();
    if (!game) {
        LOG_ERROR("Failed to initialize game");
        return -1;
    }
    game_setup(game);
    game_run(game);
    
    game_destroy(game);
    log_destroy();
    return 0;
}

