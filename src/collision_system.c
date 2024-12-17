#include "collision_system.h"

#include "component.h"
#include "system.h"

#include <core/arena.h>


struct CollisionSystem* collision_system_create(pfnSystemUpdate callback, struct EventBus* event_bus) {
    struct CollisionSystem* system =
        ArenaAlloc(&allocator, 1, struct CollisionSystem);
    
    system_base_init((struct SystemBase*)system, COLLISION_SYSTEM_BIT, callback, COLLISION_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, 0, event_bus);
    
    return system;
}
