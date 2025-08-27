#include "time_system.h"

#include "components.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include "core/os.h"
#include "core/systembase.h"

static void time_update(Registry *reg, struct SystemBase *sys,
                        size_t frame_nr) {
  (void)frame_nr;
  Pool *time_pool = registry_get_pool(reg, TIME_COMPONENT_BIT);

  Entity *entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity e = entities[i];

    TimeComponent *tc = PoolGetComponent(time_pool, TimeComponent, e.index);

    if (time_expired(tc->expires)) {
      registry_entity_remove(reg, e);
    }
  }
}

struct TimeSystem *time_system_create(Services *services) {
  struct TimeSystem *system =
      ArenaAlloc<TimeSystem>(&global_static_allocator, 1);
  system_base_init((struct SystemBase *)system, TIME_SYSTEM_BIT, &time_update,
                   TIME_COMPONENT_BIT, services, "TimeSystem");

  return system;
}
