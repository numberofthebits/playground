#include "ecs.h"

#include "arena.h"
#include "componentbase.h"
#include "log.h"
#include "systembase.h"
#include "types.h"

#include <stddef.h>

#define REGISTRY_ENTITY_COMMIT_BUFFER_CAPACITY 1024

#define REGISTRY_ARENA_SIZE 1024 * 1024 * 64

// static EntityId entity_id_counter;
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
      ArenaAlloc(&global_static_allocator, max_entity_count, EntityIndex);
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
  LOG_INFO("Reserve entity index %zu (in use total %zu)", index,
           entity_ids->used + 1);
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

  LOG_INFO("Create entity with ID %d and index %zd", entity.id, entity.index);

  return entity;
}

static void zero_system_pointers(struct SystemBase **systems, size_t count) {
  for (size_t i = 0; i < count; ++i) {
    *systems = 0;
    systems++;
  }
}

void registry_init(Registry *reg, size_t max_entity_count,
                   const struct Component *components, size_t component_count) {
  entity_id_pool_init(&reg->entity_id_pool, max_entity_count);

  // Initialize all pools to null size null pointer data and
  // null pointer
  memset(&reg->pools[0], 0x0, sizeof(reg->pools));
  for (size_t i = 0; i < component_count; ++i) {
    const struct Component *component = &components[i];
    struct Pool pool;
    pool.descriptor = component;
    pool.count = max_entity_count;
    pool.data = arena_alloc(&global_static_allocator, component->size,
                            max_entity_count, component->alignment);
    reg->pools[i] = pool;
  }

  hash_map_init(&reg->entity_tags.entity_tag_map, 10);
  hash_map_init(&reg->entity_tags.tag_entity_map, 10);
  hash_map_init(&reg->entity_groups.group_entity_map, 10);
  hash_map_init(&reg->entity_groups.entity_group_map, 10);

  zero_system_pointers(&reg->systems[0], SYSTEMS_MAX);

  reg->to_add = ArenaAlloc(&global_static_allocator, max_entity_count, Entity);
  reg->count_to_add = 0;

  reg->to_remove =
      ArenaAlloc(&global_static_allocator, max_entity_count, Entity);
  reg->count_to_remove = 0;

  reg->entity_component_signatures =
      ArenaAlloc(&global_static_allocator, max_entity_count, SignatureT);
  reg->entity_flags =
      ArenaAlloc(&global_static_allocator, max_entity_count, EntityFlags);

  reg->num_systems = 0;

  reg->components.num_components = component_count;
  reg->components.components = components;
}

struct Pool *registry_get_pool(Registry *reg, int component_bit) {
  const int index = component_index(&reg->components, component_bit);
  return &reg->pools[index];
}

Entity registry_entity_create(Registry *reg) {

  Entity entity = create_entity(&reg->entity_id_pool);

  reg->entity_component_signatures[entity.index] = 0;
  reg->entity_flags[entity.index] = 0;

  return entity;
}

static void registry_add_entity_to_systems(Registry *registry, Entity entity) {
  size_t entity_index = entity.index;
  SignatureT entity_signature =
      registry->entity_component_signatures[entity_index];

  for (size_t i = 0; i < registry->num_systems; ++i) {
    struct SystemBase *system = registry->systems[i];
    if (!system) {
      continue;
    }

    SignatureT system_signature = system->signature;
    if ((entity_signature & system_signature) == system_signature) {

      LOG_INFO("Add entity %d (index %d) to system %s", entity.id, entity.index,
               system->name);
      system_add_entity(system, entity);
    }
  }
}

static void remove_entity_from_systems(Registry *registry, Entity entity) {
  size_t entity_index = entity.index;
  SignatureT entity_signature =
      registry->entity_component_signatures[entity_index];

  for (size_t i = 0; i < registry->num_systems; ++i) {
    struct SystemBase *system = registry->systems[i];
    if (!system) {
      continue;
    }

    SignatureT system_signature = system->signature;
    if ((entity_signature & system_signature) == system_signature) {
      system_remove_entity(system, entity);
    }
  }
}

void registry_entity_add(Registry *reg, Entity e) {
  LOG_INFO("Add entity ID %d index %zu", e.id, e.index);
  reg->to_add[reg->count_to_add++] = e;
}

void registry_entity_remove(Registry *reg, Entity e) {
  LOG_INFO("Remove entity ID %d index %zu", e.id, e.index);
  reg->to_remove[reg->count_to_remove++] = e;
}

void registry_entity_commit_entities(Registry *reg) {
  for (size_t j = 0; j < reg->count_to_remove; ++j) {
    Entity e = reg->to_remove[j];
    remove_entity_from_systems(reg, e);
    entity_id_pool_remove_entity(&reg->entity_id_pool, e);
    registry_entity_untag(reg, e);
    registry_entity_ungroup(reg, e);
  }
  reg->count_to_remove = 0;

  for (size_t i = 0; i < reg->count_to_add; ++i) {
    registry_add_entity_to_systems(reg, reg->to_add[i]);
  }

  reg->count_to_add = 0;
}

void registry_entity_add_component(Registry *reg, Entity e, int component_bit,
                                   void *data) {
  int index = component_index(&reg->components, component_bit);
  if (index < 0 || index >= COMPONENT_POOLS_MAX) {
    LOG_EXIT("DED");
  }

  LOG_INFO("Entity ID %d index %zu: add component %s)", e.id, e.index,
           component_name(&reg->components, index));

  struct Pool *pool = registry_get_pool(reg, component_bit);
  if (!pool) {
    LOG_EXIT("Could not find pool for component %d", component_bit);
    return;
  }

  ptrdiff_t ptr_offset = e.index * pool->descriptor->size;
  memcpy((char *)pool->data + ptr_offset, data, pool->descriptor->size);

  SignatureT *entity_signature = &reg->entity_component_signatures[e.index];
  *entity_signature |= component_bit;
}

void registry_add_system(Registry *registry, struct SystemBase *systembase) {
  if (registry->num_systems >= SYSTEMS_MAX) {
    LOG_EXIT("%zu would exceed SYSTEMS_MAX %zu", registry->num_systems,
             SYSTEMS_MAX);
  }
  LOG_INFO("Add system %s ID %d", systembase->name, systembase->flag);
  registry->systems[registry->num_systems++] = systembase;
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
  for (size_t i = 0; i < reg->num_systems; ++i) {
    struct SystemBase *system = reg->systems[i];
    if (system) {
#ifdef ENABLE_DEBUG_TIMERS
      TimeT t0 = time_now();
#endif
      system->update_fn(reg, system, frame_index);
#ifdef ENABLE_DEBUG_TIMERS
      TimeT elapsed = time_elapsed_now(t0);
      LOG_INFO("System '%s' update time: %lu microsecs", system->name,
               time_to_microsecs(elapsed));
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

void registry_entity_tag(Registry *reg, Entity entity, char *tag) {
  unsigned long long len = strlen(tag);
  if (registry_entity_has_tag(reg, entity, tag)) {
    LOG_WARN("Entity ID %d (index %d) already has tag %s", entity.id,
             entity.index, tag);
    return;
  }

  hash_map_insert_value(&reg->entity_tags.entity_tag_map, &entity,
                        sizeof(entity), tag);
  // It's nasty, but whatever
  Entity *e = malloc(sizeof(Entity));
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
  char *value = hash_map_remove(&reg->entity_tags.entity_tag_map, &entity,
                                sizeof(entity));
  if (value) {
    Entity *e = hash_map_remove(&reg->entity_tags.tag_entity_map, (char *)value,
                                strlen(value));
    if (e) {
      free(e);
    } else {
      LOG_WARN("Tag entity mapping mismatch for entity id %d (index) %d",
               entity.id, entity.index);
    }
  }
}

void registry_entity_group(Registry *reg, Entity entity, char *group) {

  if (registry_entity_in_group(reg, entity, group)) {
    LOG_WARN("Entity ID %d (index %d) already in group %s", entity.id,
             entity.index, group);
    return;
  }

  unsigned long long len = strlen(group);
  hash_map_insert_value(&reg->entity_groups.entity_group_map, &entity,
                        sizeof(entity), group);

  void *entity_vec_ptr;
  // Check if we already have a group with this name
  if (hash_map_get(&reg->entity_groups.group_entity_map, group, len,
                   &entity_vec_ptr)) {
    // If we do, get the vector and insert our value

    Vec *vec = (Vec *)entity_vec_ptr;
    VEC_PUSH_T(vec, Entity, entity);
  } else {
    // Create a vector and stuff it in the map
    Vec *v = malloc(sizeof(Vec));
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
  if (hash_map_get(&reg->entity_groups.group_entity_map, group, strlen(group),
                   &vptr)) {
    Vec *v = vptr;
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

int registry_entity_has_tag(Registry *reg, Entity entity, char *tag) {
  void *found_tag = 0;
  if (!hash_map_get(&reg->entity_tags.entity_tag_map, &entity, sizeof(entity),
                    &found_tag)) {
    return 0;
  }

  return strcmp((char *)found_tag, tag) == 0;
}

int registry_entity_in_group(Registry *reg, Entity entity, char *group) {
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
