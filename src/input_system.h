#ifndef INPUT_SYSTEM_H
#define INPUT_SYSTEM_H

#include "core/systembase.h"
#include "core/util.h"

#include <stdint.h>

#define INPUT_SYSTEM_MAX_KEY_STATES 512

typedef enum {
  KeyFlag_Released = 0x0,
  KeyFlag_Pressed = 0x1,
  // If IsToggle, pressed state is reset to
  // released immediately
  KeyFlag_IsOneShot = 0x2
} KeyFlags;

struct KeyState {
  KeyFlags flags;
};

struct KeyStateEventData {
  int key;
};

struct AggregatedKeyboardEvents {
  size_t num_events;
  struct KeyStateEventData *events;
};

typedef struct {
  Vec2u16 pos; // Position in screen pixels
} Cursor;

struct InputSystem {
  struct SystemBase base;

  // Working data structure for callbacks from OS
  struct KeyState keys[INPUT_SYSTEM_MAX_KEY_STATES];

  // Processed list of keystrokes for event_bus
  struct KeyStateEventData events[INPUT_SYSTEM_MAX_KEY_STATES];

  Cursor cursor;
};

struct InputSystem *input_system_create(Services *services);

void input_system_handle_keyboard_input(struct InputSystem *input_system,
                                        int key, int action);

void input_system_set_cursor_pos(struct InputSystem *input_system, uint16_t x,
                                 uint16_t y, uint16_t framebuffer_width,
                                 uint16_t framebuffer_height);

void input_system_update(Registry *registry, struct SystemBase *sys,
                         size_t frame_nr);

#endif
