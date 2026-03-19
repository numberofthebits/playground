#ifndef ECS_H
#define ECS_H

#include "allocators.h"
#include "assetstore.h"
#include "componentbase.h"
#include "hashmap.h"
#include "sparsecomponentpool.h"
#include "types.h"
#include "vec.h"

#define COMPONENT_POOLS_MAX 32
#define SYSTEMS_MAX 32

struct SystemBase;

struct SystemUpdateGroup {
  SystemBase *systems[SYSTEMS_MAX];
  uint32_t count;
};
// Track our entity indexes so we can reuse destroyed entities
// Note this is not a component pool
struct EntityIdPool {
  uint32_t size;
  uint32_t used;
  uint32_t *pool;
};

typedef struct {
  Entity *to_add;
  uint32_t count_to_add;

  uint32_t counts[SYSTEMS_MAX];
} StagedAdd;

typedef struct {
  // Entity to const char*
  HashMap entity_tag_map;
  // const char* to Entity
  HashMap tag_entity_map;
} EntityTags;

typedef struct {
  // Entity to const char*
  HashMap entity_group_map;
  // const char* to Vec of Entity
  HashMap group_entity_map;
} EntityGroups;

struct Registry_t {
  Pool pools[COMPONENT_POOLS_MAX];
  uint32_t num_pools;

  struct SystemBase *systems[SYSTEMS_MAX];
  uint32_t num_systems;

  struct EntityIdPool entity_id_pool;

  uint32_t num_entities;
  uint32_t max_entities;

  /* Entity *to_add; */
  /* uint32_t count_to_add; */

  StagedAdd staged_add;

  Entity *to_remove;
  uint32_t count_to_remove;

  // Array must must be 'num_entities' size always
  ComponentSignatureT *entity_component_signatures;

  // Array must be 'num_entities' size always
  EntityFlags *entity_flags;

  struct Components components;

  EntityTags entity_tags;
  EntityGroups entity_groups;

  FixedSizeStack<SystemUpdateGroup, SYSTEMS_MAX> system_update_order;
};
typedef struct Registry_t Registry;

/*
 Registry API
 */
void registry_init(Registry *registry, uint32_t max_entity_count,
                   const struct Component *components, uint32_t component_count);

Pool *registry_get_pool(Registry *reg, int component_bit);

void registry_add_system(Registry *reg, struct SystemBase *s);

struct SystemBase *registry_get_system(Registry *reg, int system_id);

// Process 'to_add' and 'to_remove' lists
void registry_update(Registry *reg, uint64_t frame_index, TimeT frame_time_now);

// Build a naive parallellization of system execution order
void registry_resolve_systems_update_order(Registry *registry);

/*******************
  Entity API
********************/
Entity registry_entity_create(Registry *reg);
void registry_entity_add(Registry *reg, Entity entity);
void registry_entity_remove(Registry *reg, Entity e);
void registry_entity_commit_entities(Registry *reg);
void registry_entity_component_add(Registry *reg, Entity e, int component_bit,
                                   void *data);
void registry_entity_component_remove(Registry *reg, Entity e,
                                      int component_bit);

/*
 Entity tag
 */
void registry_entity_tag(Registry *reg, Entity entity, const char *tag);
void registry_entity_untag(Registry *reg, Entity entity);
int registry_entity_has_tag(Registry *reg, Entity e, const char *tag);

/*
 Entity group
 */
int registry_entity_in_group(Registry *reg, Entity e, const char *group);
void registry_entity_group(Registry *reg, Entity entity, const char *group);
void registry_entity_ungroup(Registry *reg, Entity entity);
Vec *registry_entity_group_get(Registry *reg, const char *group);

/*
 Entity flag
 */
void registry_entity_set_flags(Registry *reg, Entity entity, EntityFlags flag);
void registry_entity_unset_flags(Registry *reg, Entity entity,
                                 EntityFlags flag);
EntityFlags registry_entity_get_flags(Registry *reg, Entity e);

int registry_entity_has_component(Registry *reg, Entity e, int component_bit);

#ifdef BUILD_TESTS
void registry_test();
#endif

#endif // ECS_H
