#include "movement_system.h"

#include "components.h"
#include "systems.h"

#include <core/arena.h>
#include <core/ecs.h>
#include <core/systembase.h>

static void movement_update(Registry *reg, struct SystemBase *system,
                            size_t frame_nr) {
  (void)frame_nr;
  BeginScopedTimer(movement_time);

  Entity *entities = VEC_ITER_BEGIN_T(&system->entities, Entity);
  struct Pool *transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  struct Pool *physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);

  for (int i = 0; i < system->entities.size; ++i) {
    Entity e = entities[i];
    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, e.index);
    PhysicsComponent *pc =
        PoolGetComponent(physics_pool, PhysicsComponent, e.index);
    tc->pos.x += pc->velocity.x;
    tc->pos.y += pc->velocity.y;
  }

  AppendScopedTimer(movement_time);
  PrintScopedTimer(movement_time);
}

struct MovementSystem *movement_system_create(struct Services *services) {
  struct MovementSystem *system =
      ArenaAlloc(&allocator, 1, struct MovementSystem);

  system_base_init((struct SystemBase *)system, MOVEMENT_SYSTEM_BIT,
                   &movement_update,
                   TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT, services);

  return system;
}
