#ifndef DAMAGE_SYSTEM_H
#define DAMAGE_SYSTEM_H

#include "components.h"
#include "events.h"
#include "systems.h"

#include "core/systembase.h"

typedef struct {
  struct SystemBase base;
} DamageSystem;

DamageSystem *damage_system_create(Services *services);

void damage_system_update(Registry *registry, struct SystemBase *sys,
                          size_t frame_nr);

void damage_system_handle_event(struct SystemBase *system, struct Event e);

#endif
