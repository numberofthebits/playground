#include "input_system.h"

#include "events.h"
#include "systems.h"

#include <core/arena.h>

#include <GLFW/glfw3.h>

#include <memory.h>

struct InputSystem *input_system_create(struct Services *services) {
  struct InputSystem *system = ArenaAlloc(&allocator, 1, struct InputSystem);

  // This system isn't interested in any components.
  // It only consumes input from OS and produces
  // events.
  system_base_init((struct SystemBase *)system, INPUT_SYSTEM_BIT,
                   &input_system_update, 0, services);

  input_system_reset(system);

  return system;
}

void input_system_reset(struct InputSystem *system) {
  memset(system->keys, 0x0, sizeof(system->keys));
  memset(system->events, 0x0, sizeof(system->events));
}

void input_system_handle_keyboard_input(struct InputSystem *system, int key,
                                        int action) {
  struct KeyState *state = &system->keys[key];
  TimeT now = time_now();

  if (action == 0) {
    state->flags = KeyFlag_Released;
    time_append(&state->elapsed, time_elapsed(state->time_start, now));
    memset(&state->time_start, 0x0, sizeof(TimeT));
  } else {
    state->flags = KeyFlag_Pressed;
    state->time_start = now;
  }
}

void input_system_update(Registry *registry, struct SystemBase *sys,
                         size_t frame_nr) {
  (void)registry;
  (void)frame_nr;
  BeginScopedTimer(input_system_update);

  struct EventBus *bus = sys->services->event_bus;
  struct InputSystem *system = (struct InputSystem *)sys;
  size_t num_events = 0;

  for (int i = 0; i < INPUT_SYSTEM_MAX_KEY_STATES; ++i) {
    if (time_to_nanosecs(system->keys[i].elapsed) > 0) {
      struct KeyStateEventData *data = &system->events[num_events++];
      data->elapsed = system->keys[i].elapsed;
      data->key = i;
    }
  }

  struct AggregatedKeyboardEvents aggregated;
  aggregated.num_events = num_events;
  aggregated.events = &system->events[0];

  struct Event events;
  events.id = KeyboardInput_Update;
  events.event_data = &aggregated;

  event_bus_emit(bus, &events);

  input_system_reset(system);

  AppendScopedTimer(input_system_update);
  PrintScopedTimer(input_system_update);
}
