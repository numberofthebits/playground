#include "input_system.h"

#include <core/arena.h>

#include <GLFW/glfw3.h>

#include <memory.h>

struct InputSystem* input_system_create() {
    struct InputSystem* impl =
        ArenaAlloc(&allocator, 1, struct InputSystem);

    input_system_reset(impl);
    return impl;
}

void input_system_handle_keyboard(struct InputSystem* system, int key, int action) {
    system->key_state[key] = (uint8_t)action == 0 ? 0 : 1;
}

void input_system_reset(struct InputSystem* input_system) {
    memset(input_system->key_state, (uint8_t)GLFW_RELEASE, sizeof(input_system->key_state));

}
