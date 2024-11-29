#ifndef _ECS_H
#define _ECS_H

#include "vec.h"
#include "component.h"
#include "assetstore.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_INVALID_INDEX -1
#define COMPONENT_POOLS_MAX 32
#define SYSTEMS_MAX 32

typedef uint8_t SignatureT;
typedef int EntityId;

struct System_t;

typedef void(*pfnSystemUpdate)(struct Registry_t*, struct System_t*, size_t);

struct System_t {
    int id;
    SignatureT signature;
    Vec entities;
    pfnSystemUpdate update_fn;
    void* system_impl;
    Assets* assets;
};
typedef struct System_t System;

struct Entity_t {
    EntityId id;
    size_t index;
};
typedef struct Entity_t Entity;

struct Pool {
    void* data;
    size_t count;
    Component* descriptor;
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

    System* systems[SYSTEMS_MAX];

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
void registry_init(Registry* registry, size_t max_entity_count, Component* components, size_t component_count);

struct Pool* registry_get_pool(Registry* reg, enum component_bit bit);

#define RegistryGetComponent(pool, type, index) \
    ((type*)pool->data) + index;

Entity registry_create_entity(Registry* reg);

void registry_add_system(Registry* reg, System* s);

System* registry_get_system(Registry* reg, int system_id);
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

/*
  System API
 */
System* system_create(pfnSystemUpdate update_fn, int required_component_flags);

void system_add_entity(System* system, Entity e);

void system_remove_entity(System* sys, Entity e);

void system_require_component(System* sys, int bit);

#endif _ECS_H
