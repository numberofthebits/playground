cmake_minimum_required(VERSION 3.5)

project(pikuma_2d_game_engine C)

add_executable(game)

target_sources(game
  PRIVATE
./src/render_system.c
./src/core/hashmap.c
./src/core/eventbus.c
./src/core/assetstore.c
./src/core/arena.c
./src/core/systembase.c
./src/core/util.c
./src/core/math.c
./src/core/ecs.c
./src/core/vec.c
./src/core/log.c
./src/player_system.c
./src/component.c
./src/collision_system.c
./src/time_system.c
./src/movement_system.c
./src/input_system.c
./src/main.c
./src/animation_system.c
./src/game.c
./src/system.c
./libs/glad/src/glad.c
 $<$<PLATFORM_ID:Windows>:./src/core/os_win.c>
 $<$<PLATFORM_ID:Linux>:./src/core/os_linux.c>
)

# TODO: Fix for Windows
target_include_directories(game
  PRIVATE
  $<$<PLATFORM_ID:Linux>:/usr/include>
  $<$<PLATFORM_ID:Windows>:./libs/GLFW/include>
  ./libs/glad/include
  ./libs/stb
  ./src/
)

# TODO: Fix for Windows
target_link_libraries(game
  PRIVATE
  $<$<PLATFORM_ID:Linux>:m;c;rt;glfw>
)
