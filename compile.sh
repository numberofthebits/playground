clang \
 -DENABLE_DEBUG_TIMERS \
 ./src/animation_system.c \
 ./src/collision_system.c \
 ./src/time_system.c \
 ./src/component.c \
 ./src/core/arena.c \
 ./src/core/assetstore.c \
 ./src/core/ecs.c \
 ./src/core/eventbus.c \
 ./src/core/hashmap.c \
 ./src/core/log.c \
 ./src/core/math.c \
 ./src/core/os_linux.c \
 ./src/core/systembase.c \
 ./src/core/util.c \
 ./src/core/vec.c \
 ./src/game.c \
 ./src/main.c \
 ./src/movement_system.c \
 ./src/render_system.c \
 ./src/system.c \
 ./src/input_system.c \
 ./src/player_system.c \
 ./libs/glad/src/glad.c \
 -Wall -Werror -Wextra -Wpedantic \
 -lm \
 -lc \
 -lrt \
 -lglfw \
 -std=gnu11 \
 -I /usr/include/ \
 -I ./libs/GLFW/include \
 -I ./libs/glad/include \
 -I ./libs/stb \
 -I ./src/ \
 -o main
