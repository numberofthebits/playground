#include "hit_detection_system.h"

static void hit_detection_system_update(Registry *reg, struct SystemBase *base,
                                        size_t frame_nr) {
  (void)frame_nr;
  HitDetectionSystem *system = (HitDetectionSystem *)base;
  Pool *mesh_pool = registry_get_pool(reg, MESH_COMPONENT_BIT);
  Entity *entities = VEC_ITER_BEGIN_T(&base->entities, Entity);
  struct EventBus *event_bus = base->services.event_bus;

  for (size_t r = 0; r < system->num_rays; ++r) {
    for (int i = 0; i < base->entities.size; ++i) {
      Entity entity = entities[i];
      MeshComponent *mesh =
          PoolGetComponent(mesh_pool, MeshComponent, entity.index);

      MeshRayIntersection intersection = {0};
      if (mesh_intersect_ray(&mesh->mesh, &system->rays[r], &intersection)) {

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
}

HitDetectionSystem *hit_detection_system_create(Services *services) {
  HitDetectionSystem *system =
      ArenaAlloc(&global_static_allocator, 1, HitDetectionSystem);

  system_base_init((struct SystemBase *)system, HIT_DETECTION_SYSTEM_BIT,
                   &hit_detection_system_update, MESH_COMPONENT_BIT, services,
                   "HitDetectionSystem");

  return system;
}

void hit_detection_system_handle_cursor_moved(struct SystemBase *base,
                                              Event e) {
  (void)base;
  if (!is_expected_event_id(InputSystem_CursorMoved, e.id)) {
    return;
  }

  // TODO: Map from screen space to view space? Model space? World space?
}

void hit_detection_system_handle_framebuffer_size_changed(
    struct SystemBase *base, struct Event e) {

  if (!is_expected_event_id(OS_FramebufferSizeChanged, e.id)) {
    return;
  }

  OSFramebufferSizeChanged *ev = e.event_data;

  HitDetectionSystem *system = (HitDetectionSystem *)base;
  system->framebuffer_size = ev->size;
}
