#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include <core/systembase.h>

struct CollisionSystem {
    struct SystemBase base;
};

struct CollisionSystem* collision_system_create(struct EventBus* event_bus);


#endif // COLLISION_SYSTEM_H
