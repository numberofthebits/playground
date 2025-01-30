#ifndef EVENTS_H
#define EVENTS_H

#include <core/math.h>
#include <core/types.h>

enum EventType {
  CollisionSystem_Detected,
  KeyboardInput_Update
};

struct KeyboardInputUpdate {
  int code;
};

struct CollisionDetectedEvent {
  Entity entity_a;
  Entity entity_b;
};


#endif
