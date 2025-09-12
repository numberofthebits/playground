#ifndef CAMERA_MOVEMENT_SYSTEM_H
#define CAMERA_MOVEMENT_SYSTEM_H

#include "components.h"
#include "core/eventbus.h"
#include "events.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/componentbase.h"
#include "core/ecs.h"
#include "core/log.h"
#include "core/services.h"
#include "core/systembase.h"

typedef struct Camera {
  Mat4x4 projection;
  Mat4x4 view;
  Mat4x4 view_projection; // mat4_mul(&Proj, &View)
  Vec3f position;         // Camera center
  Vec2f area;             // Extent of camera view
} Camera;

typedef struct CameraMovementSystem {
  struct SystemBase base;
  Camera camera; // We only have one for now :)
  Vec2u32 world_size;
} CameraMovementSystem;

void camera_movement_system_update(SystemUpdateArgs args);

CameraMovementSystem *camera_movement_system_create(Services *services,
                                                    Camera *camera);

void camera_movement_system_set_world_size(CameraMovementSystem *self,
                                           Vec2u32 world_size);

#endif
