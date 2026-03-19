cmake_minimum_required(VERSION 3.5)

#set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/toolchain.cmake")

# This is apparently ignored for non make/ninja generators so
# leave it on so i never have to specify this manually again
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C STANDARD 23)

project(pikuma_2d_game_engine C CXX)

option(ENABLE_TESTS "Compile test functions" ON)
option(ENABLE_DEBUG_TIMERS "Log debug timers" ON)
option(ENABLE_SANITIZERS "Enable sanitizers" OFF)

add_executable(game)
#target_compile_options(game PRIVATE -fsanitize=addresss,leak,undefined -fno-omit-frame-pointer)

target_sources(game
  PRIVATE
  ${PROJECT_SOURCE_DIR}/libs/glad/src/glad.c
  ${PROJECT_SOURCE_DIR}/libs/glad/include/glad/glad.h
  ${PROJECT_SOURCE_DIR}/libs/imgui/imconfig.h
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui.h
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_demo.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_draw.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_internal.h
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_tables.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_widgets.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_impl_glfw.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_impl_glfw.h
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_impl_opengl3.cpp
  ${PROJECT_SOURCE_DIR}/libs/imgui/imgui_impl_opengl3.h
  ${PROJECT_SOURCE_DIR}/src/animation_system.cpp
  ${PROJECT_SOURCE_DIR}/src/animation_system.h
  ${PROJECT_SOURCE_DIR}/src/collision_system.cpp
  ${PROJECT_SOURCE_DIR}/src/collision_system.h
  ${PROJECT_SOURCE_DIR}/src/components.cpp
  ${PROJECT_SOURCE_DIR}/src/components.h
  ${PROJECT_SOURCE_DIR}/src/core/allocators.cpp
  ${PROJECT_SOURCE_DIR}/src/core/allocators.h
  ${PROJECT_SOURCE_DIR}/src/core/assetstore.cpp
  ${PROJECT_SOURCE_DIR}/src/core/assetstore.h
  ${PROJECT_SOURCE_DIR}/src/core/componentbase.cpp
  ${PROJECT_SOURCE_DIR}/src/core/componentbase.h
  ${PROJECT_SOURCE_DIR}/src/core/ecs.cpp
  ${PROJECT_SOURCE_DIR}/src/core/ecs.h
  ${PROJECT_SOURCE_DIR}/src/core/eventbus.cpp
  ${PROJECT_SOURCE_DIR}/src/core/eventbus.h
  ${PROJECT_SOURCE_DIR}/src/core/hashmap.cpp
  ${PROJECT_SOURCE_DIR}/src/core/hashmap.h
  ${PROJECT_SOURCE_DIR}/src/core/keyvalueparser.cpp
  ${PROJECT_SOURCE_DIR}/src/core/keyvalueparser.h
  ${PROJECT_SOURCE_DIR}/src/core/log.cpp
  ${PROJECT_SOURCE_DIR}/src/core/log.h
  ${PROJECT_SOURCE_DIR}/src/core/math.cpp
  ${PROJECT_SOURCE_DIR}/src/core/math.h
  ${PROJECT_SOURCE_DIR}/src/core/os.h
  ${PROJECT_SOURCE_DIR}/src/core/renderer.cpp
  ${PROJECT_SOURCE_DIR}/src/core/renderer.h
  ${PROJECT_SOURCE_DIR}/src/core/ringbuf.h
  ${PROJECT_SOURCE_DIR}/src/core/ringbuf.cpp
  ${PROJECT_SOURCE_DIR}/src/core/services.cpp
  ${PROJECT_SOURCE_DIR}/src/core/services.h
  ${PROJECT_SOURCE_DIR}/src/core/sparsecomponentpool.cpp
  ${PROJECT_SOURCE_DIR}/src/core/sparsecomponentpool.h
  ${PROJECT_SOURCE_DIR}/src/core/statistics.cpp
  ${PROJECT_SOURCE_DIR}/src/core/statistics.h  
  ${PROJECT_SOURCE_DIR}/src/core/systembase.cpp
  ${PROJECT_SOURCE_DIR}/src/core/systembase.h
  ${PROJECT_SOURCE_DIR}/src/core/types.h
  ${PROJECT_SOURCE_DIR}/src/core/util.cpp
  ${PROJECT_SOURCE_DIR}/src/core/util.h
  ${PROJECT_SOURCE_DIR}/src/core/vec.cpp
  ${PROJECT_SOURCE_DIR}/src/core/vec.h
  ${PROJECT_SOURCE_DIR}/src/core/worker.cpp
  ${PROJECT_SOURCE_DIR}/src/core/worker.h
  ${PROJECT_SOURCE_DIR}/src/camera_movement_system.cpp
  ${PROJECT_SOURCE_DIR}/src/camera_movement_system.h
  ${PROJECT_SOURCE_DIR}/src/damage_system.cpp
  ${PROJECT_SOURCE_DIR}/src/damage_system.h
  ${PROJECT_SOURCE_DIR}/src/events.cpp
  ${PROJECT_SOURCE_DIR}/src/events.h
  ${PROJECT_SOURCE_DIR}/src/game.cpp
  ${PROJECT_SOURCE_DIR}/src/game.h
  ${PROJECT_SOURCE_DIR}/src/hit_detection_system.cpp
  ${PROJECT_SOURCE_DIR}/src/hit_detection_system.h  
  ${PROJECT_SOURCE_DIR}/src/input_system.cpp
  ${PROJECT_SOURCE_DIR}/src/input_system.h
  ${PROJECT_SOURCE_DIR}/src/main.cpp
  ${PROJECT_SOURCE_DIR}/src/movement_system.cpp
  ${PROJECT_SOURCE_DIR}/src/movement_system.h
  ${PROJECT_SOURCE_DIR}/src/player_system.cpp
  ${PROJECT_SOURCE_DIR}/src/player_system.h
  ${PROJECT_SOURCE_DIR}/src/projectile_emitter_system.cpp  
  ${PROJECT_SOURCE_DIR}/src/projectile_emitter_system.h
  ${PROJECT_SOURCE_DIR}/src/render_system.cpp
  ${PROJECT_SOURCE_DIR}/src/render_system.h
  ${PROJECT_SOURCE_DIR}/src/systems.cpp
  ${PROJECT_SOURCE_DIR}/src/systems.h
  ${PROJECT_SOURCE_DIR}/src/text_system.cpp
  ${PROJECT_SOURCE_DIR}/src/text_system.h
  ${PROJECT_SOURCE_DIR}/src/time_system.cpp
  ${PROJECT_SOURCE_DIR}/src/time_system.h
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/src/core/os_win.cpp>
  $<$<PLATFORM_ID:Linux>:${PROJECT_SOURCE_DIR}/src/core/os_linux.cpp>
)

target_include_directories(game
  PRIVATE
  ${PROJECT_SOURCE_DIR}/libs/glad/include
  ${PROJECT_SOURCE_DIR}/libs/stb
  ${PROJECT_SOURCE_DIR}/libs
  ${PROJECT_SOURCE_DIR}/src/
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/libs/GLFW/include>
  $<$<PLATFORM_ID:Linux>:/usr/include/>
)

target_link_libraries(game
  PRIVATE
  $<$<PLATFORM_ID:Linux>:m;c;rt;glfw>
  $<$<PLATFORM_ID:Windows>:${PROJECT_SOURCE_DIR}/libs/GLFW/lib/glfw3dll.lib Shlwapi.lib>
)

if ( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message("Using clang/gnu compiler")
  target_compile_options(game
    PRIVATE
    -Wall
    -Wextra
    -Werror
    -Wshadow
    -mavx2
    -mfma
    -fno-exceptions
  )
endif()

if(ENABLE_TESTS) 
  target_compile_options(game PRIVATE -DBUILD_TESTS)
endif()

if(ENABLE_DEBUG_TIMERS)
  target_compile_options(game PRIVATE -DENABLE_DEBUG_TIMERS)
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  if (ENABLE_SANITIZERS)
    message(STATUS "UBSAN doesn't work after enabling C++????")
   #target_compile_options(game PRIVATE -fsanitize=undefined -fsanitize-link-c++-runtime)
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
  message("Using MSVC")
  target_compile_options(game
    PRIVATE
     /D_CRT_SECURE_NO_WARNINGS
     /MP16
     /arch:AVX
     /Zi
     /W3
   )
   # Make VS understand from where we should run this thing
   set_target_properties(game PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
endif()


