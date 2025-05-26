cmake_minimum_required(VERSION 3.5)

project(pikuma_2d_game_engine C)

# This is apparently ignored for non make/ninja generators so
# leave it on so i never have to specify this manually again
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(BUILD_TESTS "Compile test functions" ON)

add_executable(game)

target_sources(game
  PRIVATE
  ${PROJECT_SOURCE_DIR}/libs/glad/src/glad.c
  ${PROJECT_SOURCE_DIR}/src/animation_system.c
  ${PROJECT_SOURCE_DIR}/src/animation_system.h
  ${PROJECT_SOURCE_DIR}/src/collision_system.c
  ${PROJECT_SOURCE_DIR}/src/collision_system.h
  ${PROJECT_SOURCE_DIR}/src/components.c
  ${PROJECT_SOURCE_DIR}/src/components.h
  ${PROJECT_SOURCE_DIR}/src/core/allocators.c
  ${PROJECT_SOURCE_DIR}/src/core/allocators.h
  ${PROJECT_SOURCE_DIR}/src/core/assetstore.c
  ${PROJECT_SOURCE_DIR}/src/core/assetstore.h
  ${PROJECT_SOURCE_DIR}/src/core/componentbase.c
  ${PROJECT_SOURCE_DIR}/src/core/componentbase.h
  ${PROJECT_SOURCE_DIR}/src/core/ecs.c
  ${PROJECT_SOURCE_DIR}/src/core/ecs.h
  ${PROJECT_SOURCE_DIR}/src/core/eventbus.c
  ${PROJECT_SOURCE_DIR}/src/core/eventbus.h
  ${PROJECT_SOURCE_DIR}/src/core/hashmap.c
  ${PROJECT_SOURCE_DIR}/src/core/hashmap.h
  ${PROJECT_SOURCE_DIR}/src/core/log.c
  ${PROJECT_SOURCE_DIR}/src/core/log.h
  ${PROJECT_SOURCE_DIR}/src/core/math.c
  ${PROJECT_SOURCE_DIR}/src/core/math.h
  ${PROJECT_SOURCE_DIR}/src/core/os.h
  ${PROJECT_SOURCE_DIR}/src/core/renderer.c
  ${PROJECT_SOURCE_DIR}/src/core/renderer.h
  ${PROJECT_SOURCE_DIR}/src/core/services.c
  ${PROJECT_SOURCE_DIR}/src/core/services.h
  ${PROJECT_SOURCE_DIR}/src/core/sparsecomponentpool.c
  ${PROJECT_SOURCE_DIR}/src/core/sparsecomponentpool.h
  ${PROJECT_SOURCE_DIR}/src/core/systembase.c
  ${PROJECT_SOURCE_DIR}/src/core/systembase.h
  ${PROJECT_SOURCE_DIR}/src/core/types.h
  ${PROJECT_SOURCE_DIR}/src/core/util.c
  ${PROJECT_SOURCE_DIR}/src/core/util.h
  ${PROJECT_SOURCE_DIR}/src/core/vec.c
  ${PROJECT_SOURCE_DIR}/src/core/vec.h
  ${PROJECT_SOURCE_DIR}/src/camera_movement_system.h
  ${PROJECT_SOURCE_DIR}/src/damage_system.c
  ${PROJECT_SOURCE_DIR}/src/damage_system.h
  ${PROJECT_SOURCE_DIR}/src/events.h
  ${PROJECT_SOURCE_DIR}/src/game.c
  ${PROJECT_SOURCE_DIR}/src/game.h
  ${PROJECT_SOURCE_DIR}/src/input_system.c
  ${PROJECT_SOURCE_DIR}/src/input_system.h
  ${PROJECT_SOURCE_DIR}/src/main.c
  ${PROJECT_SOURCE_DIR}/src/movement_system.c
  ${PROJECT_SOURCE_DIR}/src/movement_system.h
  ${PROJECT_SOURCE_DIR}/src/player_system.c
  ${PROJECT_SOURCE_DIR}/src/player_system.h
  ${PROJECT_SOURCE_DIR}/src/projectile_emitter_system.c  
  ${PROJECT_SOURCE_DIR}/src/projectile_emitter_system.h
  ${PROJECT_SOURCE_DIR}/src/render_system.c
  ${PROJECT_SOURCE_DIR}/src/render_system.h
  ${PROJECT_SOURCE_DIR}/src/systems.c
  ${PROJECT_SOURCE_DIR}/src/systems.h
  ${PROJECT_SOURCE_DIR}/src/time_system.c
  ${PROJECT_SOURCE_DIR}/src/time_system.h
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/src/core/os_win.c>
  $<$<PLATFORM_ID:Linux>:${PROJECT_SOURCE_DIR}/src/core/os_linux.c>
)

target_include_directories(game
  PUBLIC
  ${PROJECT_SOURCE_DIR}/libs/glad/include
  ${PROJECT_SOURCE_DIR}/libs/stb
  ${PROJECT_SOURCE_DIR}/src/
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/libs/GLFW/include>
  $<$<PLATFORM_ID:Linux>:/usr/include/>
)

target_link_libraries(game
  PRIVATE
  $<$<PLATFORM_ID:Linux>:m;c;rt;glfw>
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/libs/GLFW/lib/glfw3dll.lib Shlwapi.lib>
)

target_compile_features(game
  PRIVATE
  c_std_11
)

if ( CMAKE_C_COMPILER_ID STREQUAL "Clang" OR CMAKE_C_COMPILER_ID STREQUAL "GNU")
  target_compile_options(game
    PRIVATE
    -Wall
    -Wextra
    -Werror
  )
endif()

if(BUILD_TESTS) 
  target_compile_options(game PRIVATE -DBUILD_TESTS)
endif()

if (CMAKE_C_COMPILER_ID STREQUAL "Clang") 
  if (ENABLE_SANITIZERS)
     message(STATUS "Enabling sanitizers")
     target_compile_options(game PRIVATE -fsanitize=address,undefined,leak)
     target_link_libraries(game PRIVATE ubsan asan)
  else()
     message(STATUS "No sanitizers enabled")
   endif()

   # Clang on windows uses -D, not /D
   target_compile_options(game
     PRIVATE
     $<$<PLATFORM_ID:Windows>:-D_CRT_SECURE_NO_WARNINGS>
   )
endif()
  
if ( MSVC )
  target_compile_options(game
    PRIVATE
     /D_CRT_SECURE_NO_WARNINGS
     /MP16
     /arch:AVX
     /Zi
     /W3
     /std:c11
   )
   # Make VS understand from where we should run this thing
   set_target_properties(game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()


