#include "input_system.h"

#include "events.h"
#include "systems.h"

#include "core/allocators.h"

#include <GLFW/glfw3.h>

#include <memory.h>

static inline void input_system_reset(InputSystem *system) {
  memset(system->events, 0x0, sizeof(system->events));
}

static inline void clear_key_pressed_flag(KeyState *state) {
  state->flags &= ~KeyFlag_Pressed;
}

void input_system_update(Registry *registry, struct SystemBase *sys,
                         size_t frame_nr, TimeT now) {
  (void)registry;
  (void)frame_nr;
  (void)now;

  struct EventBus *bus = sys->services.event_bus;
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
    events.event_data_size = sizeof(struct AggregatedKeyboardEvents);

    event_bus_emit(bus, &events);
  }

  input_system_reset(system);
}

struct InputSystem *input_system_create(Services *services) {
  struct InputSystem *system =
      ArenaAlloc<InputSystem>(&global_static_allocator, 1);

  // This system isn't interested in any components.
  // It only consumes input from OS and produces
  // events.
  RequiredComponents components{};

  system_base_init((struct SystemBase *)system, INPUT_SYSTEM_BIT,
                   &input_system_update, components, services, "InputSystem");

  memset(system->keys, 0x0, sizeof(system->keys));
  memset(system->events, 0x0, sizeof(system->events));

  system->keys[GLFW_KEY_SPACE].flags |= KeyFlag_IsOneShot;

  return system;
}

void input_system_handle_keyboard_input(InputSystem *system, int key,
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

void input_system_set_cursor_pos(InputSystem *input_system, uint16_t x,
                                 uint16_t y, uint16_t framebuffer_width,
                                 uint16_t framebuffer_height) {
  if (x == input_system->cursor.pos.x && y == input_system->cursor.pos.y) {
    return;
  }

  LOG_INFO("Mouse cursor (px) %hu %hu framebuffer size (px) %hu %hu", x, y,
           framebuffer_width, framebuffer_height);

  EventBus *bus = input_system->base.services.event_bus;

  // Normalize coordinates to [-1, 1]
  InputSystemCursorMoved cursor_moved_event = {
      .pos = {((float)x / (float)framebuffer_width - 0.5f) * 2.f,
              ((float)y / (float)framebuffer_height - 0.5f) * 2.f}};

  Event event = {.id = InputSystem_CursorMoved,
                 .event_data = &cursor_moved_event,
                 .event_data_size = sizeof(InputSystemCursorMoved)};

  (void)event;
  (void)bus;
  event_bus_defer(bus, &event);
}
