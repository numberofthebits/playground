#include "hit_detection_system.h"

#include "events.h"

static void hit_detection_system_update(SystemUpdateArgs args) {

  HitDetectionSystem *system = (HitDetectionSystem *)args.system;
  Pool *mesh_pool = registry_get_pool(args.registry, MESH_COMPONENT_BIT);
  Pool *transform_pool =
      registry_get_pool(args.registry, TRANSFORM_COMPONENT_BIT);
  Entity *entities = VEC_ITER_BEGIN_T(&args.system->entities, Entity);
  struct EventBus *event_bus = args.system->services.event_bus;

  for (uint32_t r = 0; r < system->num_rays; ++r) {
    for (uint32_t i = 0; i < args.system->entities.size; ++i) {
      Entity entity = entities[i];
      MeshComponent *mesh =
          PoolGetComponent(mesh_pool, MeshComponent, entity.index);
      TransformComponent *transform =
          PoolGetComponent(transform_pool, TransformComponent, entity.index);

      Mat4x4 t;
      Vec3f axis = {.x = 0.f, .y = 0.f, .z = 1.f};
      mat4_transform(&t, &axis, transform->rotation, transform->scale,
                     &transform->pos);
      MeshRayIntersection intersection = {};
      if (mesh_transform_intersect_ray(&mesh->mesh, &system->rays[r], &t,
                                       &intersection)) {

        struct HitDetectionEvent hit_event;
        hit_event.mesh = entities[i];
        hit_event.intersection = intersection;

        struct Event e;
        e.id = HitDetectionSystem_MeshHit;
        e.event_data = &hit_event;
        e.event_data_size = sizeof(HitDetectionEvent);

        event_bus_emit(event_bus, &e);
      }
    }
  }

  system->num_rays = 0;
}

HitDetectionSystem *hit_detection_system_create(Services *services) {
  HitDetectionSystem *system =
      ArenaAlloc<HitDetectionSystem>(&global_static_allocator, 1);

  RequiredComponents components = {
      .signature = MESH_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT,
      .read_access_flags = MESH_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT,
      .write_access_flags = 0};

  system_base_init((struct SystemBase *)system, HIT_DETECTION_SYSTEM_BIT,
                   &hit_detection_system_update, components, services,
                   "HitDetectionSystem");
  system->rays = ArenaAlloc<Ray3f>(&global_static_allocator, 1);
  system->num_rays = 0;

  return system;
}

void hit_detection_system_handle_cursor_moved(struct SystemBase *base,
                                              Event e) {
  HitDetectionSystem *system = (HitDetectionSystem *)base;

  if (!is_expected_event_id(InputSystem_CursorMoved, (EventType)e.id)) {
    return;
  }

  if (system->num_rays) {
    LOG_WARN("Only 1 ray per frame supported right now");
    return;
  }

  InputSystemCursorMoved *event = (InputSystemCursorMoved *)e.event_data;
  Vec3f normalized_cursor_pos = {
      .x = event->pos.x, .y = event->pos.y, .z = 0.f};
  Vec3f ray_direction = {.x = 0.f, .y = 0.f, .z = -1.f};

  Ray3f *ray = &system->rays[system->num_rays++];
  ray->origin = normalized_cursor_pos;
  ray->direction = ray_direction;

  // Let the update method deal with the actual ray casting
}
