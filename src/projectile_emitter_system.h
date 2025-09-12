#ifndef PROJECTILE_EMITTER_SYSTEM_H
#define PROJECTILE_EMITTER_SYSTEM_H

#include "components.h"

#include "core/systembase.h"

typedef struct {
  struct SystemBase base;

} ProjectileEmitterSystem;

ProjectileEmitterSystem *projectile_emitter_system_create(Services *services);

#endif
