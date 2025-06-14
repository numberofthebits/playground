#ifndef EVENTS_H
#define EVENTS_H

#include "core/math.h"
#include "core/types.h"

enum EventType {
  CollisionSystem_Detected,
  KeyboardInput_Update,
  DebugEvent_StateChanged, // This is a piss poor name
  CameraSystem_CameraChanged,
  HitDetectionSystem_MeshHit,
  InputSystem_CursorMoved,
  OS_FramebufferSizeChanged,
  EventTypeCount
};

typedef struct EventTypeName {
  enum EventType type;
  const char *name;
} EventTypeName;

extern EventTypeName event_type_names[EventTypeCount];

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

typedef struct InputSystemCursorMoved {
  Vec2d pos;
} InputSystemCursorMoved;

typedef struct OSFramebufferSizeChanged {
  Vec2u16 size;
} OSFramebufferSizeChanged;

int is_expected_event_id(enum EventType expected, enum EventType actual);

#endif
