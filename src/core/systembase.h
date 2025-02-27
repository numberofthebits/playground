#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H

#include <core/ecs.h>
#include <core/services.h>
#include <core/types.h>
#include <core/vec.h>

typedef struct Registry_t Registry;
typedef void (*pfnSystemUpdate)(Registry *, struct SystemBase *, size_t);

struct SystemBase {
  int id;
  SignatureT signature;
  Vec entities;
  pfnSystemUpdate update_fn;
  struct Services *services;
};

void system_base_init(struct SystemBase *system, int system_id,
                      pfnSystemUpdate update_fn, int required_component_flags,
                      struct Services *services);

void system_add_entity(struct SystemBase *system, Entity entity);

void system_remove_entity(struct SystemBase *system, Entity entity);

void system_require_component(struct SystemBase *system, int bit);

#endif // SYSTEMBASE_H
