#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <core/util.h>

#include <stdint.h>

struct InputSystem {
    uint8_t key_state[512];
};

struct InputSystem* input_system_create();

void input_system_handle_keyboard(struct InputSystem* input_system, int key, int action);

void input_system_reset(struct InputSystem* input_system);

#endif
