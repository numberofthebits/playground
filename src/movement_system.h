#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include <core/systembase.h>

struct MovementSystem {
    SystemBase base;
};

struct MovementSystem* movement_system_create(pfnSystemUpdate update_callback);


#endif
