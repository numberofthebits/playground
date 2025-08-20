#ifndef EVENTS_H
#define EVENTS_H

#include "core/math.h"
#include "core/types.h"

enum EventType {
  CollisionSystem_Detected,
  KeyboardInput_Update,
  DebugEvent_StateChanged, // This is a piss poor name
  CameraSystem_CameraUpdated,
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
  Vec3f position; // Camera center
  Vec2f area;     // Extent of camera view
  Mat4x4 projection;
  Mat4x4 view;
  Mat4x4 view_projection;
} CameraUpdatedEventData;

typedef struct InputSystemCursorMoved {
  Vec2f pos;
} InputSystemCursorMoved;

typedef struct OSFramebufferSizeChanged {
  Vec2u32 size;
} OSFramebufferSizeChanged;

int is_expected_event_id(enum EventType expected, enum EventType actual);

const char *event_type_name(enum EventType);

#endif
