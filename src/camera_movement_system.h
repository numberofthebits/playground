#ifndef CAMERA_MOVEMENT_SYSTEM_H
#define CAMERA_MOVEMENT_SYSTEM_H

#include "components.h"
#include "core/eventbus.h"
#include "events.h"
#include "systems.h"

#include <core/arena.h>
#include <core/componentbase.h>
#include <core/ecs.h>
#include <core/log.h>
#include <core/services.h>
#include <core/systembase.h>

typedef struct {
  struct SystemBase base;
  Vec2f camera_area;
  Vec2u8 world_size;
} CameraMovementSystem;

static inline Vec3f clamp_camera_pos(Vec3f *object_position, Vec2u8 world_size,
                                     Vec2f *area) {
  float area_w_half = area->x / 2.f;
  float area_h_half = area->y / 2.f;
  Vec3f camera_pos = *object_position;

  if (object_position->x < area_w_half) {
    camera_pos.x = area_w_half;
  } else if (object_position->x >= world_size.x - area_w_half) {
    camera_pos.x = world_size.x - area_w_half;
  }

  if (object_position->y < area_h_half) {
    camera_pos.y = area_h_half;
  } else if (object_position->y >= (world_size.y - area_h_half)) {
    camera_pos.y = world_size.y - area_h_half;
  }

  return camera_pos;
}

static inline void camera_movement_system_update(Registry *reg,
                                                 struct SystemBase *base,
                                                 size_t frame_nr) {
  (void)reg;
  (void)base;
  (void)frame_nr;
  Entity *ptr = base->entities.storage.ptr;
  struct Pool *pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  CameraMovementSystem *camera_movement_system = (CameraMovementSystem *)base;

  for (int i = 0; i < base->entities.size || i < 1; ++i) {
    Entity e = *(ptr + i);
    TransformComponent *tc =
        PoolGetComponent(pool, TransformComponent, e.index);
    CameraUpdated data;
    data.pos = clamp_camera_pos(&tc->pos, camera_movement_system->world_size,
                                &camera_movement_system->camera_area);
    data.size = camera_movement_system->camera_area;

    struct Event ev;
    ev.id = CameraSystem_CameraChanged;

    ev.event_data = &data;

    event_bus_emit(base->services->event_bus, &ev);
  }
}

static inline CameraMovementSystem *
camera_movement_system_create(struct Services *services, Vec2f *camera_area) {
  CameraMovementSystem *sys = ArenaAlloc(&global_static_allocator, 1, CameraMovementSystem);

  system_base_init((struct SystemBase *)sys, CAMERA_MOVEMENT_SYSTEM_BIT,
                   camera_movement_system_update,
                   TRANSFORM_COMPONENT_BIT | CAMERA_MOVEMENT_COMPONENT_BIT,
                   services);

  sys->camera_area = *camera_area;

  return sys;
}

static inline void
camera_movement_system_set_world_size(CameraMovementSystem *self,
                                      Vec2u8 world_size) {
  self->world_size = world_size;
}

#endif
