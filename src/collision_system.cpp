#include "collision_system.h"

#include "components.h"
#include "events.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include "core/systembase.h"

static void collision_update(SystemUpdateArgs args) {
  Pool *collision_pool =
      registry_get_pool(args.registry, COLLISION_COMPONENT_BIT);
  Pool *transform_pool =
      registry_get_pool(args.registry, TRANSFORM_COMPONENT_BIT);

  struct EventBus *event_bus = args.system->services.event_bus;
  Entity *entities = VEC_ITER_BEGIN_T(&args.system->entities, Entity);

  for (uint32_t i = 0; i < args.system->entities.size; ++i) {
    Entity self = entities[i];
    CollisionComponent *self_collision =
        PoolGetComponent(collision_pool, CollisionComponent, self.index);
    TransformComponent *self_transform =
        PoolGetComponent(transform_pool, TransformComponent, self.index);

    Rectf self_rect = {.pos = {self_transform->pos.x, self_transform->pos.y},
                       .width = self_collision->width,
                       .height = self_collision->height};
    /* self_rect.pos.x += self_transform->pos.x; */
    /* self_rect.pos.y += self_transform->pos.y; */

    for (uint32_t j = i; j < args.system->entities.size; ++j) {
      Entity other = entities[j];

      if (other.id == self.id) {
        continue;
      }

      CollisionComponent *other_collision =
          PoolGetComponent(collision_pool, CollisionComponent, other.index);
      TransformComponent *other_transform =
          PoolGetComponent(transform_pool, TransformComponent, other.index);

      Rectf other_rect = {
          .pos = {.x = other_transform->pos.x, .y = other_transform->pos.y},
          .width = other_collision->width,
          .height = other_collision->height};
      /* other_rect.pos.x += other_transform->pos.x; */
      /* other_rect.pos.y += other_transform->pos.y; */

      if (intersect_rectf(&self_rect, &other_rect)) {
        struct CollisionDetectedEvent event;
        event.entity_a = self;
        event.entity_b = other;

        struct Event e;
        e.id = CollisionSystem_Detected;
        e.event_data = &event;
        e.event_data_size = sizeof(struct CollisionDetectedEvent);

        /* // Queue both entities for removal */
        /* registry_entity_remove(reg, self); */
        /* registry_entity_remove(reg, other); */

        // And notify anybody interested in the event
        event_bus_emit(event_bus, &e);
      }
    }
  }
}

struct CollisionSystem *collision_system_create(Services *services) {
  struct CollisionSystem *system =
      ArenaAlloc<CollisionSystem>(&global_static_allocator, 1);

  RequiredComponents components = {
      .signature = COLLISION_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT,
      .read_access_flags = COLLISION_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT,
      .write_access_flags = 0};

  system_base_init((struct SystemBase *)system, COLLISION_SYSTEM_BIT,
                   &collision_update, components, services, "CollisionSystem");

  return system;
}

void collision_system_handle_event(struct SystemBase *system, struct Event e) {
  // struct CollisionSystem* cs = (struct CollisionSystem*)system;
  switch (e.id) {
  case DebugEvent_StateChanged:
    for (uint32_t i = 0; i < system->entities.size; ++i) {
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
