#ifndef COLLISION_SYSTEM_H
#define COLLISION_SYSTEM_H

#include <core/systembase.h>

struct CollisionSystem {
    SystemBase base;
};

struct CollisionSystem* collision_system_create(pfnSystemUpdate callback);


#endif COLLITION_SYSTEM_H
