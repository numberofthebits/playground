#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H

#include "ecs.h"
#include "services.h"
#include "types.h"
#include "vec.h"

#include <stdalign.h>

typedef struct SystemBase SystemBase;
typedef struct Registry_t Registry;

typedef void (*pfnSystemUpdate)(Registry *, struct SystemBase *, size_t);

/* SystemBase defines invariants for concrete systems such that
   the entity component system (ECS) can work with systems in a general manner.
   Systembase describes which entities are interesting for a concrete system,
   and tracks which entities are registered in the system.

   Each system is identified by a bit flag 'flag'. This is just an integer,
   because we don't want anything in core to know about concrete systems.
 */
struct SystemBase {
  // The signature is a bitwise OR'ed set of component flags
  SignatureT signature;

  // Dynamic array of Entity types
  Vec entities;

  // The ECS calls this when it's time to update systems.
  // TODO: Is this abstraction just for the sake of abstraction?
  //       How is this better than a hardcoded list of concrete system
  //       calling each update function in turn. A hard coded list would allow
  //       each concrete system to take the parameters it needs to update,
  //       instead of the update callback parameters becoming the set of all
  //       arguments of all concrete update functions.
  pfnSystemUpdate update_fn;

  // Give system base access to whatever services it might need
  Services services;

  // Human readable name of the system. Makes logs more readable.
  const char *name;

  int flag; /*SystemBit*/
  int evaluation_order;
};
typedef struct SystemBase SystemBase;

// This MUST be called for each concrete system implementation
void system_base_init(struct SystemBase *system, int system_id,
                      pfnSystemUpdate update_fn, int required_component_flags,
                      Services *services, const char *name);

void system_add_entity(struct SystemBase *system, Entity entity);

void system_remove_entity(struct SystemBase *system, Entity entity);

#endif // SYSTEMBASE_H
