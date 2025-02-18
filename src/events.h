#ifndef EVENTS_H
#define EVENTS_H

#include <core/math.h>
#include <core/types.h>

enum EventType {
  CollisionSystem_Detected,
  KeyboardInput_Update,
  DebugEvent_StateChanged,
};

struct KeyboardInputUpdate {
  int code;
};

struct CollisionDetectedEvent {
  Entity entity_a;
  Entity entity_b;
};

struct DebugEventStateChangedEvent {
    int debug_enabled;
};


#endif
