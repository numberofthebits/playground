#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include <core/systembase.h>

struct CollisionSystem {
    struct SystemBase base;
};

struct CollisionSystem* collision_system_create(struct Services* services);

void collision_system_handle_event(struct SystemBase* system, struct Event e);

#endif // COLLISION_SYSTEM_H
