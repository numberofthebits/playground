#include "collision_system.h"

#include "components.h"
#include "events.h"
#include "systems.h"

#include <core/arena.h>
#include <core/ecs.h>
#include <core/os.h>
#include <core/systembase.h>

static void collision_update(Registry *reg, struct SystemBase *sys,
                             size_t frame_nr) {
  (void)frame_nr;
  BeginScopedTimer(collision_time);

  struct Pool *collision_pool = registry_get_pool(reg, COLLISION_COMPONENT_BIT);
  struct Pool *transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  struct EventBus *event_bus = sys->services->event_bus;
  Entity *entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity self = entities[i];
    CollisionComponent *self_collision =
        PoolGetComponent(collision_pool, CollisionComponent, self.id);
    TransformComponent *self_transform =
        PoolGetComponent(transform_pool, TransformComponent, self.id);

    Rectf self_rect = self_collision->aabr;
    self_rect.pos.x += self_transform->pos.x;
    self_rect.pos.y += self_transform->pos.y;

    for (int j = i; j < sys->entities.size; ++j) {
      Entity other = entities[j];

      if (other.id == self.id) {
        continue;
      }

      CollisionComponent *other_collision =
          PoolGetComponent(collision_pool, CollisionComponent, other.id);
      TransformComponent *other_transform =
          PoolGetComponent(transform_pool, TransformComponent, other.id);

      Rectf other_rect = other_collision->aabr;
      other_rect.pos.x += other_transform->pos.x;
      other_rect.pos.y += other_transform->pos.y;

      if (intersect_rectf(&self_rect, &other_rect)) {
        struct CollisionDetectedEvent event;
        event.entity_a = self;
        event.entity_b = other;

        struct Event e;
        e.id = CollisionSystem_Detected;
        e.event_data = &event;

        // Queue both entities for removal
        registry_remove_entity(reg, self);
        registry_remove_entity(reg, other);

        // And notify anybodyt interested in the event
        event_bus_emit(event_bus, &e);
      }
    }
  }

  AppendScopedTimer(collision_time);
  PrintScopedTimer(collision_time);
}

struct CollisionSystem *collision_system_create(struct Services *services) {
  struct CollisionSystem *system =
      ArenaAlloc(&global_static_allocator, 1, struct CollisionSystem);

  system_base_init((struct SystemBase *)system, COLLISION_SYSTEM_BIT,
                   &collision_update,
                   COLLISION_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, services);

  return system;
}

void collision_system_handle_event(struct SystemBase *system, struct Event e) {
  // struct CollisionSystem* cs = (struct CollisionSystem*)system;
  switch (e.id) {
  case DebugEvent_StateChanged:
    for (int i = 0; i < system->entities.size; ++i) {
      // Entity entity = VEC_GET_T(&system->entities, Entity, i);
      // RenderComponent rc;
      // registry_add_component( , e, RENDER_COMPONENT_BIT, &rc);
      // #error "Do we want to make Registry* a member of SystemBase?"
    }
    break;
  default:
    LOG_WARN("Unhandled event ID %d", e.id);
  }
}
