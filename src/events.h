#ifndef EVENTS_H
#define EVENTS_H

#include "core/math.h"
#include "core/types.h"

enum EventType {
  CollisionSystem_Detected,
  KeyboardInput_Update,
  DebugEvent_StateChanged,
  CameraSystem_CameraChanged,
  HitDetectionSystem_MeshHit
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

typedef struct HitDetectionEvent {
  Entity mesh;
  MeshRayIntersection intersection;
} HitDetectionEvent;

typedef struct {
  Vec3f pos;
  Vec2f size;
} CameraUpdated;

#endif
