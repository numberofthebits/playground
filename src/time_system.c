#include "time_system.h"

#include "component.h"
#include "system.h"

#include <core/arena.h>

struct TimeSystem* time_system_create(pfnSystemUpdate callback, Assets* assets) {
    struct TimeSystem* system = ArenaAlloc(&allocator, 1, struct TimeSystem);
    system_base_init((SystemBase*)system, TIME_SYSTEM_BIT, callback, TIME_COMPONENT_BIT, assets);

    return system;
}
