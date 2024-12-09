#include "movement_system.h"

#include "system.h"
#include "component.h"

#include <core/arena.h>

struct MovementSystem* movement_system_create(pfnSystemUpdate update_callback) {
    struct MovementSystem* system =
        ArenaAlloc(&allocator, 1, struct MovementSystem);
    
    system_base_init((SystemBase*)system, MOVEMENT_SYSTEM_BIT, update_callback, TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT, 0);
    
    return system;
}

