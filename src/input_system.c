#include "input_system.h"

#include "events.h"
#include "systems.h"

#include "core/arena.h"

#include <GLFW/glfw3.h>

#include <memory.h>

struct InputSystem *input_system_create(struct Services *services) {
  struct InputSystem *system =
      ArenaAlloc(&global_static_allocator, 1, struct InputSystem);

  // This system isn't interested in any components.
  // It only consumes input from OS and produces
  // events.
  system_base_init((struct SystemBase *)system, INPUT_SYSTEM_BIT,
                   &input_system_update, 0, services, "InputSystem");

  memset(system->keys, 0x0, sizeof(system->keys));
  memset(system->events, 0x0, sizeof(system->events));

  system->keys[GLFW_KEY_SPACE].flags |= KeyFlag_IsOneShot;

  return system;
}

static inline void input_system_reset(struct InputSystem *system) {
  memset(system->events, 0x0, sizeof(system->events));
}

static inline void clear_key_pressed_flag(struct KeyState *state) {
  state->flags &= ~KeyFlag_Pressed;
}

void input_system_handle_keyboard_input(struct InputSystem *system, int key,
                                        int action) {
  // TODO: is there still some jank in this function, or
  // the update function or between the two? When pressing
  // a button, the duration seems longer than it should be
  struct KeyState *state = &system->keys[key];

  switch (action) {
  case 0:
    clear_key_pressed_flag(state);
    break;
  case 1:
    state->flags |= KeyFlag_Pressed;
    LOG_INFO("Pressed %d", key);
    break;
  default:
    LOG_WARN("Unhandled key action");
  }
}

void input_system_update(Registry *registry, struct SystemBase *sys,
                         size_t frame_nr) {
  (void)registry;
  (void)frame_nr;

  struct EventBus *bus = sys->services->event_bus;
  struct InputSystem *system = (struct InputSystem *)sys;
  size_t num_events = 0;

  for (int i = 0; i < INPUT_SYSTEM_MAX_KEY_STATES; ++i) {
    if (system->keys[i].flags & KeyFlag_Pressed) {
      struct KeyStateEventData *data = &system->events[num_events++];
      data->key = i;

      if (system->keys[i].flags & KeyFlag_IsOneShot) {
        clear_key_pressed_flag(&system->keys[i]);
      }
    }
  }

  if (num_events) {
    struct AggregatedKeyboardEvents aggregated;
    aggregated.num_events = num_events;
    aggregated.events = &system->events[0];

    struct Event events;
    events.id = KeyboardInput_Update;
    events.event_data = &aggregated;

    event_bus_emit(bus, &events);
  }

  input_system_reset(system);
}
