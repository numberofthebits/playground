#include "events.h"

#include "core/log.h"

EventTypeName event_type_names[EventTypeCount] = {
    {CollisionSystem_Detected, "CollisionSystem_Detected"},
    {KeyboardInput_Update, "KeyboardInput_Update"},
    {DebugEvent_StateChanged, "DebugEvent_StateChanged"},
    {CameraSystem_CameraChanged, "CameraSystem_CameraChanged"},
    {HitDetectionSystem_MeshHit, "HitDetectionSystem_MeshHit"},
    {InputSystem_CursorMoved, "InputSystem_CursorMoved"},
    {OS_FramebufferSizeChanged, "OS_FramebufferSizeChanged"}};

int is_expected_event_id(enum EventType expected, enum EventType actual) {
  if (expected != actual) {
    LOG_ERROR("Incorrect event type %s expected %s", event_type_names[actual],
              event_type_names[expected]);
    return 0;
  }
  return 1;
}
