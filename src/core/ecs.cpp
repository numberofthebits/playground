#include "ecs.h"

#include "allocators.h"
#include "componentbase.h"
#include "log.h"
#include "systembase.h"
#include "types.h"

#include <assert.h>
#include <cstring>
#include <stdalign.h>
#include <stddef.h>

#define REGISTRY_ENTITY_COMMIT_BUFFER_CAPACITY 1024

#define REGISTRY_ARENA_SIZE 1024 * 1024 * 64

/*
Insertion:
Each time insertion happens, 'used' is incremented.
'used' should always point to the next free slot. This
is guaranteed by keeping the array sorted wiht used
entries on the left and free entries to the right.

Deletion example:

Start with this. 'size' is 5. 'used' is 3

slot   [ 0, 1, 2, 3, 4 ]
entity [ x, y, z, 0, 0 ]

Delete entity 'x'. slot 0 is now free.

slot   [ 0, 1, 2, 3, 4 ]
entity [ 0, y, z, 0, 0 ]

Swap values in pos 'used - 1' and entity "x"'s slot (0)

slot   [ 0, 1, 2, 3, 4 ]
entity [ z, y, 0, 0, 0 ]

Decrement 'used', leaving it at 2, which is precisely
the next free index.

*/
static void print_entity_id_pool(struct EntityIdPool *entity_id_pool) {
  LOG_INFO("Entity ID pool used %zu / %zu", entity_id_pool->used,
           entity_id_pool->size);
  for (size_t i = 0; i < entity_id_pool->used; ++i) {
    LOG_INFO("EntityIdPool[%d]=%zu", i, entity_id_pool->pool[i]);
  }
}

static void entity_id_pool_init(struct EntityIdPool *entity_ids,
                                size_t max_entity_count) {
  if (!max_entity_count) {
    LOG_EXIT("At least 1 entity is required");
  }

  LOG_INFO("Create entity ID pool size %zu", max_entity_count);

  entity_ids->pool =
      ArenaAlloc<EntityIndex>(&global_static_allocator, max_entity_count);
  entity_ids->size = max_entity_count;
  entity_ids->used = 0;

  for (size_t i = 0; i < max_entity_count; ++i) {
    entity_ids->pool[i] = i;
  }

  //    print_entity_id_pool(entity_ids);
}

static size_t entity_id_pool_reserve_index(struct EntityIdPool *entity_ids) {
  if (entity_ids->used >= entity_ids->size) {
    LOG_EXIT("Failed to find entity index. ID pool used %zu == size %zu",
             entity_ids->used, entity_ids->size);
    return ENTITY_INVALID_INDEX;
  }

  size_t index = entity_ids->pool[entity_ids->used];
  /* LOG_INFO("Reserve entity index %zu (in use total %zu)", index, */
  /*          entity_ids->used + 1); */
  entity_ids->used++;

  //    print_entity_id_pool(entity_ids);

  return index;
}

static void entity_id_pool_remove_entity(struct EntityIdPool *entity_id_pool,
                                         Entity entity) {

  int to_remove = -1;

  for (size_t i = 0; i < entity_id_pool->used; ++i) {
    if (entity.index == entity_id_pool->pool[i]) {
      to_remove = (int)i;
      break;
    }
  }

  if (to_remove == -1) {
    print_entity_id_pool(entity_id_pool);
    LOG_EXIT("Failed to remove entity with id %d @ index %zu. Not found",
             entity.id, entity.index);
    return;
  }

  LOG_INFO("Remove entity ID %d index %zu from ID pool index %d . In use %zu",
           entity.id, entity.index, to_remove, entity_id_pool->used);

  size_t tmp = entity_id_pool->pool[to_remove];
  entity_id_pool->used--;
  entity_id_pool->pool[to_remove] = entity_id_pool->pool[entity_id_pool->used];
  entity_id_pool->pool[entity_id_pool->used] = tmp;

  //    print_entity_id_pool(entity_id_pool);
}

static Entity create_entity(struct EntityIdPool *entity_id_pool) {
  static int entity_id_counter = 0;

  Entity entity;
  entity.id = entity_id_counter++;
  entity.index = entity_id_pool_reserve_index(entity_id_pool);

  //  LOG_INFO("Create entity with ID %d and index %zd", entity.id,
  //  entity.index);

  return entity;
}

static void zero_system_pointers(struct SystemBase **systems, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    *systems = 0;
    systems++;
  }
}

static inline void clear_staged_entities_add(Registry *reg) {
  reg->staged_add.count_to_add = 0;
  memset(reg->staged_add.counts, 0x0, sizeof(reg->staged_add.counts));
}

void registry_init(Registry *reg, size_t max_entity_count,
                   const struct Component *components, size_t component_count) {
  entity_id_pool_init(&reg->entity_id_pool, max_entity_count);

  // Initialize all pools to null size null pointer data and
  // null pointer
  memset(&reg->pools[0], 0x0, sizeof(reg->pools));
  reg->num_pools = component_count;
  for (size_t i = 0; i < component_count; ++i) {
    const struct Component *component = &components[i];

    pool_init(&reg->pools[i], component, max_entity_count);
  }

  hash_map_init(&reg->entity_tags.entity_tag_map, 10000);
  hash_map_init(&reg->entity_tags.tag_entity_map, 10000);
  hash_map_init(&reg->entity_groups.group_entity_map, 10000);
  hash_map_init(&reg->entity_groups.entity_group_map, 10000);

  zero_system_pointers(&reg->systems[0], SYSTEMS_MAX);

  /* reg->to_add = ArenaAlloc(&global_static_allocator, max_entity_count,
   * Entity); */
  /* reg->count_to_add = 0; */

  reg->to_remove =
      ArenaAlloc<Entity>(&global_static_allocator, max_entity_count);

  reg->count_to_remove = 0;

  reg->entity_component_signatures =
      ArenaAlloc<SignatureT>(&global_static_allocator, max_entity_count);

  reg->entity_flags =
      ArenaAlloc<EntityFlags>(&global_static_allocator, max_entity_count);

  reg->num_systems = 0;

  reg->components.num_components = component_count;
  reg->components.components = components;

  reg->staged_add.to_add =
      ArenaAlloc<Entity>(&global_static_allocator, max_entity_count);

  clear_staged_entities_add(reg);
}

Pool *registry_get_pool(Registry *reg, int component_bit) {
  const int index = component_index(&reg->components, component_bit);
  Assert(index >= 0);
  return &reg->pools[index];
}

Entity registry_entity_create(Registry *reg) {

  Entity entity = create_entity(&reg->entity_id_pool);

  reg->entity_component_signatures[entity.index] = 0;
  reg->entity_flags[entity.index] = 0;

  return entity;
}

static void registry_add_entity_to_systems(Registry *registry, Entity entity) {

  SignatureT entity_signature =
      registry->entity_component_signatures[entity.index];

  for (size_t i = 0; i < registry->num_systems; ++i) {
    struct SystemBase *system = registry->systems[i];
    SignatureT system_signature = system->signature;

    if (!system || !system_signature) {
      continue;
    }

    if ((entity_signature & system_signature) == system_signature) {
      LOG_INFO("Add entity %d to %s", entity.id, registry->systems[i]->name);
      system_add_entity(registry->systems[i], entity);
    }
  }
}

static void remove_entity_from_systems(Registry *registry, Entity entity) {
  size_t entity_index = entity.index;
  SignatureT entity_signature =
      registry->entity_component_signatures[entity_index];

  for (size_t i = 0; i < registry->num_systems; ++i) {
    struct SystemBase *system = registry->systems[i];
    SignatureT system_signature = system->signature;
    if (!system || !system_signature) {
      continue;
    }

    if ((entity_signature & system_signature) == system_signature) {
      system_remove_entity(system, entity);
    }
  }
}

static void remove_entity_from_pools(Registry *registry, Entity entity) {
  for (size_t i = 0; i < registry->num_pools; ++i) {
    pool_remove(&registry->pools[i], entity);
  }
}

void registry_entity_add(Registry *reg, Entity e) {
  reg->staged_add.to_add[reg->staged_add.count_to_add++] = e;

  /* LOG_INFO("Add entity ID %d index %zu", e.id, e.index); */
  SignatureT entity_signature = reg->entity_component_signatures[e.index];

  for (size_t i = 0; i < reg->num_systems; ++i) {
    struct SystemBase *system = reg->systems[i];
    SignatureT system_signature = system->signature;
    if (!system || !system_signature) {
      continue;
    }

    if ((entity_signature & system_signature) == system_signature) {
      reg->staged_add.counts[i]++;
      /* LOG_INFO("Add entity %d (index %d) to system %s", entity.id,
       * entity.index, */
      /*          system->name); */
    }
  }
}

void registry_entity_remove(Registry *reg, Entity e) {
  LOG_INFO("Remove entity ID %d index %zu", e.id, e.index);
  for (size_t i = 0; i < reg->count_to_remove; ++i) {
    if (e.id == reg->to_remove[i].id) {
      LOG_WARN("Entity ID %zu index %zu already queued for removal", e.id,
               e.index);
      return;
    }
  }

  reg->to_remove[reg->count_to_remove++] = e;
}

void registry_entity_commit_entities(Registry *reg) {
  DeclareScopedTimer(add_entity_to_systems);

  for (size_t j = 0; j < reg->count_to_remove; ++j) {
    Entity e = reg->to_remove[j];
    remove_entity_from_systems(reg, e);
    remove_entity_from_pools(reg, e);
    entity_id_pool_remove_entity(&reg->entity_id_pool, e);
    registry_entity_untag(reg, e);
    registry_entity_ungroup(reg, e);
  }
  reg->count_to_remove = 0;

  BeginScopedTimer(add_entity_to_systems);
  for (size_t i = 0; i < SYSTEMS_MAX; ++i) {
    // Reserve space for as many entities as the system will need
    // up front
    if (reg->staged_add.counts[i] != 0) {
      size_t current_free_capacity =
          reg->systems[i]->entities.capacity - reg->systems[i]->entities.size;

      if (current_free_capacity < reg->staged_add.counts[i]) {
        size_t required_capacity =
            reg->systems[i]->entities.size + reg->staged_add.counts[i];
        VEC_RESERVE_T(&reg->systems[i]->entities, Entity, required_capacity);
      }
    }
  }

  for (size_t i = 0; i < reg->staged_add.count_to_add; ++i) {
    registry_add_entity_to_systems(reg, reg->staged_add.to_add[i]);
  }

  clear_staged_entities_add(reg);

  AppendScopedTimer(add_entity_to_systems);
  PrintScopedTimer(add_entity_to_systems);
}

void registry_entity_component_add(Registry *reg, Entity e, int component_bit,
                                   void *data) {
  /* int index = ; */
  // assert(index >= 0 && index < COMPONENT_POOLS_MAX);

  /* LOG_INFO("Entity ID %d index %zu: add component %s)", e.id, e.index, */
  /*          component_name(&reg->components, index)); */

  Pool *pool = registry_get_pool(reg, component_bit);
  if (!pool) {
    LOG_EXIT("Could not find pool for component %d", component_bit);
    return;
  }

  pool_insert(pool, e, data);

  SignatureT *entity_signature = &reg->entity_component_signatures[e.index];
  *entity_signature |= component_bit;
}

void registry_entity_component_remove(Registry *reg, Entity e,
                                      int component_bit) {
  int index = component_index(&reg->components, component_bit);
  assert(index >= 0 && index < COMPONENT_POOLS_MAX);
  LOG_INFO("Entity ID %d %zu: remove component %s)", e.id, e.index,
           component_name(&reg->components, index));
  Pool *pool = registry_get_pool(reg, component_bit);
  pool_remove(pool, e);

  reg->entity_component_signatures[e.index] &= ~component_bit;
}

void registry_add_system(Registry *registry, struct SystemBase *systembase) {
  if (registry->num_systems >= SYSTEMS_MAX) {
    LOG_EXIT("%zu would exceed SYSTEMS_MAX %zu", registry->num_systems,
             SYSTEMS_MAX);
  }
  LOG_INFO("Add system %s ID %d", systembase->name, systembase->flag);
  registry->systems[registry->num_systems++] = systembase;

  systembase->update_elapsed = statistics_reserve_entry(systembase->name);
}

struct SystemBase *registry_get_system(Registry *reg, int system_flag) {
  for (size_t i = 0; i < reg->num_systems; ++i) {
    if (reg->systems[i]->flag == system_flag) {
      return reg->systems[i];
    }
  }
  return 0;
}

void registry_update(Registry *reg, size_t frame_index) {
  (void)frame_index;
  DeclareScopedTimer(commit_entities);

  for (size_t i = 0; i < reg->num_systems; ++i) {
    struct SystemBase *system = reg->systems[i];
    if (system) {

#ifdef ENABLE_DEBUG_TIMERS
      TimeT t0 = time_now();
#endif
      system->update_fn(reg, system, frame_index);
#ifdef ENABLE_DEBUG_TIMERS
      TimeT t1 = time_now();
      TimeT elapsed = time_elapsed(t0, t1);
      system->update_elapsed->value = time_to_microsecs(elapsed);
      // LOG_INFO("System '%s' update time: %lu microsecs", system->name,
      //          time_to_microsecs(elapsed));
#endif
    }
  }

  BeginScopedTimer(commit_entities);

  registry_entity_commit_entities(reg);

  AppendScopedTimer(commit_entities);
  PrintScopedTimer(commit_entities);
}

int registry_entity_has_component(Registry *reg, Entity e, int component_bit) {
  return reg->entity_component_signatures[e.index] & component_bit;
}

void registry_entity_tag(Registry *reg, Entity entity, const char *tag) {
  unsigned long long len = strlen(tag);
  if (registry_entity_has_tag(reg, entity, tag)) {
    LOG_WARN("Entity ID %d (index %d) already has tag %s", entity.id,
             entity.index, tag);
    return;
  }

  hash_map_insert(&reg->entity_tags.entity_tag_map, &entity, sizeof(entity),
                  (char *)tag);
  // It's nasty, but whatever
  Entity *e = (Entity *)malloc(sizeof(Entity));
  *e = entity;
  hash_map_insert(&reg->entity_tags.tag_entity_map, tag, len, e);
}

void registry_entity_untag(Registry *reg, Entity entity) {
  // Don't free values that strings from data segment
  void *val = 0;
  if (!hash_map_get(&reg->entity_tags.entity_tag_map, &entity, sizeof(entity),
                    &val)) {
    return;
  }
  char *value = (char *)hash_map_remove(&reg->entity_tags.entity_tag_map,
                                        &entity, sizeof(entity));
  if (value) {
    Entity *e = (Entity *)hash_map_remove(&reg->entity_tags.tag_entity_map,
                                          (char *)value, strlen(value));
    if (e) {
      free(e);
    } else {
      LOG_WARN("Tag entity mapping mismatch for entity id %d (index) %d",
               entity.id, entity.index);
    }
  }
}

void registry_entity_group(Registry *reg, Entity entity, const char *group) {

  if (registry_entity_in_group(reg, entity, group)) {
    LOG_WARN("Entity ID %d (index %d) already in group %s", entity.id,
             entity.index, group);
    return;
  }

  unsigned long long len = strlen(group);
  //  hash_map_insert(&reg->entity_groups.entity_group_map, &entity,
  //  sizeof(entity),
  hash_map_insert(&reg->entity_groups.entity_group_map, &entity, sizeof(entity),
                  (char *)group);

  void *entity_vec_ptr;
  // Check if we already have a group with this name
  if (hash_map_get(&reg->entity_groups.group_entity_map, (char *)group, len,
                   &entity_vec_ptr)) {
    // If we do, get the vector and insert our value

    Vec *vec = (Vec *)entity_vec_ptr;
    VEC_PUSH_T(vec, Entity, entity);
  } else {
    // Create a vector and stuff it in the map
    Vec *v = (Vec *)malloc(sizeof(Vec));
    vec_init(v);
    VEC_PUSH_T(v, Entity, entity);
    hash_map_insert(&reg->entity_groups.group_entity_map, group, len, v);
  }
}

void registry_entity_ungroup(Registry *reg, Entity entity) {
  void *group = 0;
  if (!hash_map_get(&reg->entity_groups.entity_group_map, &entity,
                    sizeof(entity), &group)) {
    LOG_WARN("Entity %d %d not found in entity group mapping", entity.id,
             entity.index)
    return;
  }

  hash_map_remove(&reg->entity_groups.entity_group_map, &entity,
                  sizeof(entity));

  void *vptr = 0;
  if (hash_map_get(&reg->entity_groups.group_entity_map, (const char *)group,
                   std::strlen((const char *)group), &vptr)) {
    Vec *v = (Vec *)vptr;
    if (v) {
      for (int i = 0; i < v->size; ++i) {
        Entity *e = VEC_GET_T_PTR(v, Entity, i);
        if (e->id == entity.id) {
          VEC_ERASE_T(v, Entity, i);
          break;
        }
      }
    }
  }
}

int registry_entity_has_tag(Registry *reg, Entity entity, const char *tag) {
  void *found_tag = 0;
  if (!hash_map_get(&reg->entity_tags.entity_tag_map, &entity, sizeof(entity),
                    &found_tag)) {
    return 0;
  }

  return strcmp((char *)found_tag, tag) == 0;
}

int registry_entity_in_group(Registry *reg, Entity entity, const char *group) {
  unsigned long long len = strlen(group);
  void *ptr = 0;
  if (!hash_map_get(&reg->entity_groups.group_entity_map, group, len, &ptr)) {
    return 0;
  }

  Vec *v = (Vec *)ptr;
  for (int i = 0; i < v->size; ++i) {
    Entity e = VEC_GET_T(v, Entity, i);
    if (e.id == entity.id) {
      return 1;
    }
  }
  return 0;
}

Vec *registry_entity_group_get(Registry *reg, char *group) {
  void *ptr = 0;
  hash_map_get(&reg->entity_groups.group_entity_map, group, strlen(group),
               &ptr);
  return (Vec *)ptr;
}

void registry_entity_set_flags(Registry *reg, Entity entity, EntityFlags flag) {
  reg->entity_flags[entity.index] |= flag;
}

void registry_entity_unset_flags(Registry *reg, Entity entity,
                                 EntityFlags flag) {
  reg->entity_flags[entity.index] &= ~flag;
}

EntityFlags registry_entity_get_flags(Registry *reg, Entity entity) {
  return reg->entity_flags[entity.index];
}

#ifdef BUILD_TESTS
typedef struct {
  const char *text;
} TestComponentA;

typedef struct {
  int a;
  int b;
} TestComponentB;

void registry_test() {
  struct Component components[2];
  components[0].alignment = alignof(TestComponentA);
  components[0].flag = 1;
  components[0].name = "TestComponentA";
  components[0].size = sizeof(TestComponentA);

  components[1].alignment = alignof(TestComponentB);
  components[1].flag = 2;
  components[1].name = "TestComponentB";
  components[1].size = sizeof(TestComponentB);

  TestComponentA a;
  a.text = "entity1";

  TestComponentB b;
  b.a = 2;
  b.b = 2;

  TestComponentA c;
  c.text = "entity3";
  TestComponentB d;
  b.a = 3;
  b.b = 3;

  Registry r;
  registry_init(&r, 10, components, 2);

  Entity e1 = registry_entity_create(&r);
  Entity e2 = registry_entity_create(&r);
  Entity e3 = registry_entity_create(&r);

  registry_entity_add(&r, e1);
  registry_entity_add(&r, e2);
  registry_entity_add(&r, e3);

  registry_entity_commit_entities(&r);

  registry_entity_component_add(&r, e1, 1, &a);
  registry_entity_component_add(&r, e2, 2, &b);
  registry_entity_component_add(&r, e3, 1, &c);
  registry_entity_component_add(&r, e3, 2, &d);

  assert(registry_entity_has_component(&r, e1, 1));
  assert(registry_entity_has_component(&r, e2, 2));
  assert(registry_entity_has_component(&r, e3, 1));
  assert(registry_entity_has_component(&r, e3, 2));

  registry_entity_component_remove(&r, e1, 1);
  assert(!registry_entity_has_component(&r, e1, 1));

  registry_entity_component_remove(&r, e2, 2);
  assert(!registry_entity_has_component(&r, e2, 2));

  registry_entity_component_remove(&r, e3, 1);
  assert(!registry_entity_has_component(&r, e3, 1));

  registry_entity_component_remove(&r, e3, 2);
  assert(!registry_entity_has_component(&r, e3, 2));

  registry_entity_remove(&r, e1);
  registry_entity_remove(&r, e2);
  registry_entity_remove(&r, e3);

  registry_entity_commit_entities(&r);
}
#endif
