#include "time_system.h"

#include "components.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include "core/os.h"
#include "core/systembase.h"

static void time_update(SystemUpdateArgs args) {
  Pool *time_pool = registry_get_pool(args.registry, TIME_COMPONENT_BIT);

  Entity *entities = VEC_ITER_BEGIN_T(&args.system->entities, Entity);

  for (uint32_t i = 0; i < args.system->entities.size; ++i) {
    Entity e = entities[i];

    TimeComponent *tc = PoolGetComponent(time_pool, TimeComponent, e.index);

    if (time_gte(args.now, tc->expires)) {
      registry_entity_remove(args.registry, e);
    }
  }
}

struct TimeSystem *time_system_create(Services *services) {
  struct TimeSystem *system =
      ArenaAlloc<TimeSystem>(&global_static_allocator, 1);

  RequiredComponents components{.signature = TIME_COMPONENT_BIT,
                                .read_access_flags = TIME_COMPONENT_BIT,
                                .write_access_flags = 0};

  system_base_init((struct SystemBase *)system, TIME_SYSTEM_BIT, &time_update,
                   components, services, "TimeSystem");

  return system;
}
