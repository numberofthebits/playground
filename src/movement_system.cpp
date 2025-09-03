#include "movement_system.h"

#include "components.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include "core/systembase.h"

static void movement_update(Registry *reg, struct SystemBase *system,
                            size_t frame_nr, TimeT now) {
  (void)frame_nr;
  (void)now;

  Entity *entities = VEC_ITER_BEGIN_T(&system->entities, Entity);
  Pool *transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  Pool *physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);

  for (int i = 0; i < system->entities.size; ++i) {
    Entity e = entities[i];
    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, e.index);
    PhysicsComponent *pc =
        PoolGetComponent(physics_pool, PhysicsComponent, e.index);
    tc->pos.x += pc->velocity.x;
    tc->pos.y += pc->velocity.y;
  }
}

struct MovementSystem *movement_system_create(Services *services) {
  struct MovementSystem *system =
      ArenaAlloc<MovementSystem>(&global_static_allocator, 1);

  RequiredComponents components = {
      .signature = TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT,
      .read_access_flags = PHYSICS_COMPONENT_BIT,
      .write_access_flags = TRANSFORM_COMPONENT_BIT};

  system_base_init((struct SystemBase *)system, MOVEMENT_SYSTEM_BIT,
                   &movement_update, components, services, "MovementSystem");

  return system;
}
