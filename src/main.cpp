#include "game.h"

#include "core/allocators.h"
#include "core/log.h"
#include "core/worker.h"

#include <stdio.h>

#ifdef BUILD_TESTS
#include "core/tests.h"
#endif

int main(int argc, char *argv[]) {
  thread_name = "main";
  (void)argv;
  (void)argc;

  // if (!log_init(NULL)) {
  if (!log_init("log.txt")) {
    printf("Failed to initialize logger");
    return -1;
  }

#ifdef BUILD_TESTS
  run_tests();
#endif

  Game *game = game_create();

  if (!game) {
    LOG_ERROR("Failed to initialize game");
    return -1;
  }

  FuncTimer(setup, game_setup(game));

  game_run(game);

  game_destroy(game);
  log_destroy();

  return 0;
}
