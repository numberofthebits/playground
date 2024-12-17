#ifndef _ECS_H
#define _ECS_H

#include "vec.h"
#include "component.h"
#include "assetstore.h"
#include "types.h"
#include "systembase.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define COMPONENT_POOLS_MAX 32
#define SYSTEMS_MAX 32

struct Pool {
    void* data;
    size_t count;
    const Component* descriptor;
};

//Track our entity indexes so we can reuse destroyed entities
// Note this is not a component pool
struct EntityIdPool {
    size_t size;
    size_t used;
    size_t* pool;
};
// static struct EntityIdPool entity_id_pool;


struct Registry_t {
    struct Pool pools[COMPONENT_POOLS_MAX];

    struct SystemBase* systems[SYSTEMS_MAX];
    size_t num_systems;

    struct EntityIdPool entity_id_pool;

    size_t num_entities;
    size_t max_entities;

    Entity* to_add;
    size_t count_to_add;

    Entity* to_remove;
    size_t count_to_remove;

    // Dynamically allocated signatures array sized. Must
    // be 'num_entities' size always
    SignatureT* entity_component_signatures;
};
typedef struct Registry_t Registry;

/*
 Registry API
 */
void registry_init(Registry* registry, size_t max_entity_count, const Component* components, size_t component_count);

struct Pool* registry_get_pool(Registry* reg, enum component_bit bit);

#define PoolGetComponent(pool, type, index) \
    ((type*)pool->data) + index;

Entity registry_create_entity(Registry* reg);

void registry_add_system(Registry* reg, struct SystemBase* s);

struct SystemBase* registry_get_system(Registry* reg, int system_id);
// Process 'to_add' and 'to_remove' lists

void registry_update(Registry* reg, size_t frame_index);

/*
  Entity API
*/
void registry_add_entity(Registry* reg, Entity entity);

void registry_remove_entity(Registry* reg, Entity e);

void registry_commit_entities(Registry* reg);

void registry_add_component(Registry* reg, Entity e, enum component_bit, void* data);
// TODO: add registry_remove_component(...). Remember to clear bit from entity


#endif _ECS_H
