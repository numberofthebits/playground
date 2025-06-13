#include "events.h"

EventTypeName event_type_names[EventTypeCount] = {
    {CollisionSystem_Detected, "CollisionSystem_Detected"},
    {KeyboardInput_Update, "KeyboardInput_Update"},
    {DebugEvent_StateChanged, "DebugEvent_StateChanged"},
    {CameraSystem_CameraChanged, "CameraSystem_CameraChanged"},
    {HitDetectionSystem_MeshHit, "HitDetectionSystem_MeshHit"}};
