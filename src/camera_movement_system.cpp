#include "camera_movement_system.h"

#include "components.h"
#include "core/eventbus.h"
#include "events.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include "core/services.h"
#include "core/systembase.h"

static inline Vec3f clamp_camera_pos(Vec3f *object_position, Vec2u32 world_size,
                                     Camera *camera) {
  float area_w_half = camera->area.x / 2.f;
  float area_h_half = camera->area.y / 2.f;
  Vec3f camera_pos = *object_position;

  if (camera_pos.x < area_w_half) {
    camera_pos.x = area_w_half;
  } else if (camera_pos.x >= world_size.x - area_w_half) {
    camera_pos.x = world_size.x - area_w_half;
  }

  if (camera_pos.y < area_h_half) {
    camera_pos.y = area_h_half;
  } else if (camera_pos.y >= (world_size.y - area_h_half)) {
    camera_pos.y = world_size.y - area_h_half;
  }

  camera_pos.z = 1.f;

  return camera_pos;
}

static inline void camera_movement_system_update(Registry *reg,
                                                 struct SystemBase *base,
                                                 size_t frame_nr,
                                                 TimeT frame_time_now) {
  (void)frame_nr;
  (void)frame_time_now;

  Entity *ptr = (Entity *)base->entities.storage.ptr;
  Pool *pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  CameraMovementSystem *cms = (CameraMovementSystem *)base;

  for (int i = 0; i < base->entities.size && i < 1; ++i) {

    Entity e = *(ptr + i);
    TransformComponent *tc =
        PoolGetComponent(pool, TransformComponent, e.index);

    cms->camera.position =
        clamp_camera_pos(&tc->pos, cms->world_size, &cms->camera);

    Vec3f pos_neg = {-cms->camera.position.x, -cms->camera.position.y, 1.f};
    (void)pos_neg;

    float f = 4.f;
    Mat4x4 projection = ortho(1.0f, -1.0f, f, -f, f, -f);
    cms->camera.projection = projection;

    Vec3f target = cms->camera.position;
    Vec3f up = {0.f, 1.f, 0.f};
    target.z = -1.f;

    Mat4x4 view = look_at(&cms->camera.position, &target, &up);
    // Mat4x4 view = look_at(&pos_neg, &target, &up);
    cms->camera.view = view;

    Mat4x4 view_projection = mat4_mul(&projection, &view);
    cms->camera.view_projection = view_projection;

    CameraUpdatedEventData event_data;
    event_data.position = cms->camera.position;
    event_data.area = cms->camera.area;
    event_data.projection = projection;
    event_data.view = view;
    event_data.view_projection = view_projection;

    struct Event ev;
    ev.id = CameraSystem_CameraUpdated;

    ev.event_data = &event_data;
    ev.event_data_size = sizeof(event_data);

    event_bus_emit(base->services.event_bus, &ev);
  }
}

CameraMovementSystem *camera_movement_system_create(Services *services,
                                                    Camera *camera) {
  CameraMovementSystem *sys =
      ArenaAlloc<CameraMovementSystem>(&global_static_allocator, 1);

  RequiredComponents components = {
      .signature = TRANSFORM_COMPONENT_BIT | CAMERA_MOVEMENT_COMPONENT_BIT,
      .read_access_flags = TRANSFORM_COMPONENT_BIT,
      .write_access_flags = CAMERA_MOVEMENT_COMPONENT_BIT};

  system_base_init((struct SystemBase *)sys, CAMERA_MOVEMENT_SYSTEM_BIT,
                   camera_movement_system_update, components, services,
                   "CameraMovementSystem");

  sys->camera = *camera;

  return sys;
}

void camera_movement_system_set_world_size(CameraMovementSystem *self,
                                           Vec2u32 world_size) {
  self->world_size = world_size;
}
