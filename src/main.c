#include "game.h"

#include <core/log.h>
#include <core/arena.h>

#include <stdio.h>

int main(void) {

    if (!log_init(0)) {
        printf("Failed to initialize logger");
        return -1;
    }

    
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

