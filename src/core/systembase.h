#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H

#include "ecs.h"
#include "os.h"
#include "services.h"
#include "statistics.h"
#include "types.h"
#include "vec.h"
#include <stdalign.h>

typedef struct SystemBase SystemBase;
typedef struct Registry_t Registry;

struct SystemUpdateArgs {
  SystemBase *system;
  Registry *registry;
  TimeT now;
  TimeT delta;
  size_t frame_nr;
};

typedef void (*pfnSystemUpdate)(SystemUpdateArgs);

struct RequiredComponents {
  // The signature is a bitwise OR'ed set of component flags
  ComponentSignatureT signature;

  // Read access bit flags corresponding to component bit flags
  ComponentReadAccessT read_access_flags;

  // Write acccess bit flags corresponding to component bit flag
  ComponentWriteAccessT write_access_flags;
};

/* SystemBase defines invariants for concrete systems such that
   the entity component system (ECS) can work with systems in a general manner.
   SystemBase describes which entities are interesting for a concrete system,
   and tracks which entities are registered in the system.

   Each system is identified by a bit flag 'flag'. This is just an integer,
   because we don't want anything in core to know about specific systems.
 */
struct SystemBase {
  // Describe which components this system deals with, and what type of
  // access is required
  RequiredComponents components;

  // Dynamic array of Entity types
  Vec entities;

  // The ECS calls this when it's time to update systems.
  // TODO: Is this abstraction just for the sake of abstraction?
  //       How is this better than a hardcoded list of specific system
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
  Duration *update_elapsed;
};
typedef struct SystemBase SystemBase;

// This MUST be called for each concrete system implementation
void system_base_init(struct SystemBase *system, int system_id,
                      pfnSystemUpdate update_fn, RequiredComponents components,
                      Services *services, const char *name);

void system_add_entity(struct SystemBase *system, Entity entity);

void system_remove_entity(struct SystemBase *system, Entity entity);

#endif // SYSTEMBASE_H
