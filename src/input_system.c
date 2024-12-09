#include "input_system.h"

#include "system.h"
#include "component.h"

#include <core/arena.h>

#include <GLFW/glfw3.h>

#include <memory.h>

struct InputSystem* input_system_create(pfnSystemUpdate update_callback) {
    struct InputSystem* system =
        ArenaAlloc(&allocator, 1, struct InputSystem);
    
    system_base_init((SystemBase*)system, INPUT_SYSTEM_BIT, update_callback, INPUT_COMPONENT_BIT, 0);
    
    input_system_reset(system);
    
    return system;
}

void input_system_handle_keyboard(struct InputSystem* system, int key, int action) {
    struct EventBus* bus = system->base.event_bus;

    system->key_state[key] = (uint8_t)action == 0 ? 0 : 1;
}

void input_system_reset(struct InputSystem* input_system) {
    memset(input_system->key_state, (uint8_t)GLFW_RELEASE, sizeof(input_system->key_state));

}

int input_system_is_key_pressed(struct InputSystem* input_system, int key_code) {
    return input_system->key_state[key_code] != 0;
}
