#ifndef MOVEMENT_SYSTEM_H
#define MOVEMENT_SYSTEM_H

#include "core/systembase.h"

struct MovementSystem {
  struct SystemBase base;
};

struct MovementSystem *movement_system_create(struct Services *services);

#endif
