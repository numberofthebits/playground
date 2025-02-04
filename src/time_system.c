#include "time_system.h"

#include "component.h"
#include "system.h"

#include <core/ecs.h>
#include <core/systembase.h>
#include <core/arena.h>
#include <core/os.h>


static void time_update(Registry* reg, struct SystemBase* sys, size_t frame_nr) {
  (void)frame_nr;
    struct Pool* time_pool = registry_get_pool(reg, TIME_COMPONENT_BIT);

    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for(int i = 0; i < sys->entities.size; ++i) {
        Entity e = entities[i];

        TimeComponent* tc = PoolGetComponent(time_pool, TimeComponent, e.index);
        
        if (time_expired(tc->expires)) {
            LOG_INFO("entity id %d %zu", e.id, e.index);
            LOG_INFO("What the fuck %llu %llu", tc->created, tc->expires);
            registry_remove_entity(reg, e);
        }
    }
}

struct TimeSystem* time_system_create(struct Services* services) {
    struct TimeSystem* system = ArenaAlloc(&allocator, 1, struct TimeSystem);
    system_base_init((struct SystemBase*)system, TIME_SYSTEM_BIT, &time_update, TIME_COMPONENT_BIT, services);

    return system;
}

