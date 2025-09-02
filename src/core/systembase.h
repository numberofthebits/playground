#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H

#include "ecs.h"
#include "os.h"
#include "services.h"
#include "statistics.h"
#include "types.h"
#include "vec.h"
#include <stdalign.h>

#define REQUIRED_COMPONENT_ACCESS_READ_ONLY 1
#define REQUIRED_COMPONENT_ACCESS_READ_WRITE 2
#define REQUIRED_COMPONENT_ACCESS_WRITE_ONLY 3

typedef struct SystemBase SystemBase;
typedef struct Registry_t Registry;

typedef void (*pfnSystemUpdate)(Registry *, struct SystemBase *, size_t, TimeT);

#define COMPONENT_ACCESS_READ 0x1
#define COMPONENT_ACCESS_WRITE 0x2
#define COMPONENT_ACCESS_READ_WRITE                                            \
  (COMPONENT_ACCESS_READ | COMPONENT_ACCESS_WRITE)

struct RequiredComponent {
  int component_flag;
  int access_flags;
};

/* SystemBase defines invariants for concrete systems such that
   the entity component system (ECS) can work with systems in a general manner.
   SystemBase describes which entities are interesting for a concrete system,
   and tracks which entities are registered in the system.

   Each system is identified by a bit flag 'flag'. This is just an integer,
   because we don't want anything in core to know about specific systems.
 */
struct SystemBase {
  // The signature is a bitwise OR'ed set of component flags
  SignatureT signature;

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

  int flag;             /*SystemBit*/
  int evaluation_order; // unused
  Duration *update_elapsed;
};
typedef struct SystemBase SystemBase;

// This MUST be called for each concrete system implementation
void system_base_init(struct SystemBase *system, int system_id,
                      pfnSystemUpdate update_fn, int required_component_flags,
                      Services *services, const char *name);

void system_add_entity(struct SystemBase *system, Entity entity);

void system_remove_entity(struct SystemBase *system, Entity entity);

#endif // SYSTEMBASE_H
