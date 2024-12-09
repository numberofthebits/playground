#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include <core/util.h>

#include <stdint.h>

enum InputSystemEvent {
    KeyPressed
};

struct InputSystem {
    uint8_t key_state[512];
};

struct InputSystem* input_system_create();

void input_system_handle_keyboard(struct InputSystem* input_system, int key, int action);

void input_system_reset(struct InputSystem* input_system);

int input_system_is_key_pressed(struct InputSystem* input_system, int key_code);

#endif
