#include "systembase.h"

#include "types.h"
#include <core/arena.h>

#include <stdlib.h>

#define SYSTEM_ENTITIES_DEFAULT_CAPACITY 1024

static int find_entity_index(Vec *entities, Entity e) {
  Entity *iter = VEC_ITER_BEGIN_T(entities, Entity);
  for (int i = 0; i < entities->size; ++i, ++iter) {
    if (iter->id == e.id) {
      return i;
    }
  }
  return ENTITY_INVALID_INDEX;
}

void system_base_init(struct SystemBase *system, int system_flag,
                      pfnSystemUpdate update_fn, int required_component_flags,
                      struct Services *services, const char *name) {
  system->flag = system_flag;
  system->signature = required_component_flags;
  system->update_fn = update_fn;
  system->entities = vec_create();
  system->services = services;
  system->name = name;

  VEC_RESERVE_T(&system->entities, Entity, SYSTEM_ENTITIES_DEFAULT_CAPACITY);
}

void system_add_entity(struct SystemBase *system, Entity e) {
  int index = find_entity_index(&system->entities, e);
  if (index != ENTITY_INVALID_INDEX) {
    LOG_WARN("Entity %d already in System %s", e.id, system->name);
    return;
  }

  VEC_PUSH_T(&system->entities, Entity, e);
}

void system_remove_entity(struct SystemBase *system, Entity e) {
  int index = find_entity_index(&system->entities, e);
  if (index == ENTITY_INVALID_INDEX) {
    LOG_WARN("Entity %d not found in System %s", e.id, system->name);
    return;
  }

  LOG_INFO("Remove entity ID %d with index %d from system %s", e.id, e.index,
           system->name);
  VEC_ERASE_T(&system->entities, Entity, index);
}

void system_require_component(struct SystemBase *sys, int bit) {
  sys->signature |= bit;
}
