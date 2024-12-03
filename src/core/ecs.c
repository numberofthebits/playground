#include "ecs.h"

#include "vec.h"
#include "component.h"
#include "log.h"
#include "arena.h"
#include "systembase.h"

#define REGISTRY_ENTITY_COMMIT_BUFFER_CAPACITY 1024

#define REGISTRY_ARENA_SIZE 1024*1024*64


//static EntityId entity_id_counter;
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


static void entity_id_pool_init(struct EntityIdPool* entity_ids, size_t max_entity_count) {
    if(!max_entity_count) {
        LOG_EXIT("At least 1 entity is required");
    }

    entity_ids->pool = ArenaAlloc(&allocator, max_entity_count, EntityId);
    entity_ids->size = max_entity_count;
    entity_ids->used = 0;

    for (int i = 0; i < max_entity_count; ++i) {
        entity_ids->pool[i] = i;
    }
}

static size_t entity_id_pool_reserve_index(struct EntityIdPool* entity_ids) {
    if (entity_ids->used == entity_ids->size) {
        LOG_EXIT("Failed to find entity index. ID pool used %zu == size %zu",
                 entity_ids->used, entity_ids->size);
    }

    size_t index = entity_ids->pool[entity_ids->used];
    entity_ids->used++;

    return index;
}

static void entity_id_pool_remove_entity(struct EntityIdPool* entity_ids, Entity entity) {

    size_t* to_remove = 0; //entity_ids->pool[entity.index];
    
    for (int i = 0; i < entity_ids->used; ++i) {
        if (entity.index == entity_ids->pool[i]) {
            to_remove = &entity_ids->pool[i];
            break;
        }
    }

    if (!to_remove) {
        LOG_WARN("Failed to remove entity with id %d @ index %zu. Not found", entity.id, entity.index);
        return;
    }
    
    entity_ids->used -= 1;

    *to_remove = entity_ids->pool[entity_ids->used];
}

static Entity create_entity(struct EntityIdPool* entity_id_pool) {
    size_t index = entity_id_pool_reserve_index(entity_id_pool);

    // TODO: In order to make entity ID really mean something
    //       we need to use entity.index in our code referring to
    //       pools, not id.
    //       The notion of a separate entity ID for an entity
    //       reusing the same index could be helpful for
    //       debugging errors like already removed entities
    //       showing up again
    int entity_id = (int)index; //entity_id_counter++;

    Entity entity;
    entity.id = entity_id;
    entity.index = index;

    return entity;
}

static void print_entity_id_pool(struct EntityIdPool* entity_id_pool) {
    LOG_INFO("Entity ID pool used %zu / %zu", entity_id_pool->used, entity_id_pool->size);
    for(int i = 0; i < entity_id_pool->used; ++i) {
        LOG_INFO("Pool index %i: entity index %zu", i, entity_id_pool->pool[i]);
    }
}

static void zero_system_pointers(SystemBase** systems, int count) {
    for (int i = 0; i < count; ++i) {
        *systems = 0;
        systems++;
    }
}

void registry_init(Registry* reg,
                   size_t max_entity_count,
                   const Component* components,
                   size_t component_count)
{
    printf("%zu\n", component_count);
    entity_id_pool_init(&reg->entity_id_pool, max_entity_count);

    // Initialize all pools to null size null pointer data and
    // null pointer 
    memset(&reg->pools[0], 0x0, sizeof(reg->pools));
    for (int i = 0; i < component_count; ++i) {
        const Component* component = &components[i];
        struct Pool pool;
        pool.descriptor = component;
        pool.count = max_entity_count;
        pool.data = arena_alloc(&allocator,
                                component->size,
                                max_entity_count,
                                component->alignment);
        reg->pools[i] = pool;
    }
    
    zero_system_pointers(&reg->systems[0], SYSTEMS_MAX);

    reg->to_add = ArenaAlloc(&allocator, max_entity_count, Entity);
    reg->count_to_add = 0;
    
    reg->to_remove = ArenaAlloc(&allocator, max_entity_count, Entity);
    reg->count_to_remove = 0;
    
    reg->entity_component_signatures = ArenaAlloc(&allocator, max_entity_count, SignatureT);
}

struct Pool* registry_get_pool(Registry* reg, enum component_bit bit) {
    const int index = component_index(bit);
    return &reg->pools[index];
}

Entity registry_create_entity(Registry* reg) {
    
    Entity entity = create_entity(&reg->entity_id_pool);

    reg->entity_component_signatures[entity.id] = 0;
    
    return entity;
}

static void registry_add_entity_to_systems(Registry* registry, Entity entity) {
    int entity_id = entity.id;
    /* int entity_signature = VEC_GET_T(&registry->entity_component_signatures, int, entity_id); */
    SignatureT entity_signature = registry->entity_component_signatures[entity_id];

    for (int i = 0; i < SYSTEMS_MAX; ++i) {
        SystemBase* system = registry_get_system(registry, i);
        if (!system) {
            continue;
        }

        int system_signature = system->signature;
        if ((entity_signature & system_signature) == system_signature) {
            system_add_entity(system, entity);
        }
    }
}

static void registry_remove_entity_from_systems(Registry* registry, Entity entity) {
    int entity_id = entity.id;
    /* int entity_signature = VEC_GET_T(&registry->entity_component_signatures, int, entity_id); */
    SignatureT entity_signature = registry->entity_component_signatures[entity_id];
    for (int i = 0; i < SYSTEMS_MAX; ++i) {
        SystemBase* system = registry_get_system(registry, i);
        if(!system) {
            continue;
        }

        int system_signature = system->signature;
        if ((entity_signature & system_signature) == system_signature) {
            system_remove_entity(system, entity);
        }
    }
}

void registry_add_entity(Registry* reg, Entity e) {
    LOG_INFO("Add entity %d", e.id);
    reg->to_add[reg->count_to_add++] = e;
}

void registry_remove_entity(Registry* reg, Entity e) {
    LOG_INFO("Remove entity %d", e.id);
    reg->to_remove[reg->count_to_remove++] = e;
}

void registry_commit_entities(Registry* reg) {
//    LOG_INFO("Add %d entities", reg->count_to_remove);
    for (int i = 0; i < reg->count_to_add; ++i) {
        registry_add_entity_to_systems(reg, reg->to_add[i]);
    }

    reg->count_to_add = 0;

//    LOG_INFO("Remove %d entities", reg->count_to_remove);
    for (int j = 0; j < reg->count_to_remove; ++j) {
        registry_remove_entity_from_systems(reg, reg->to_remove[j]);
    }

    reg->count_to_remove = 0;
}

void registry_add_component(Registry* reg, Entity e, enum component_bit component, void* data) {
    int index = component_index(component);
    if (index < 0 || index >= COMPONENT_POOLS_MAX) {
        LOG_EXIT("DED");
    }
    
    LOG_INFO("Entity %d: add %s component %d (pool index %d)", e.id, component_name(index), index);
   
    struct Pool* pool = registry_get_pool(reg, component);
    if (!pool) {
        LOG_EXIT("Could not find pool for component %d", component);
        return;
    }

    ptrdiff_t ptr_offset = e.id * pool->descriptor->size;
    memcpy((char*)pool->data + ptr_offset, data, pool->descriptor->size);

    /* int* entity_signature = VEC_GET_T_PTR(&reg->entity_component_signatures, int, e.id); */

    SignatureT* entity_signature = &reg->entity_component_signatures[e.id];
    *entity_signature |= component;    
}

void registry_add_system(Registry* reg, SystemBase* sys) {
    LOG_INFO("Add system...");
    reg->systems[sys->id] = sys;
}

SystemBase* registry_get_system(Registry* reg, int system_id) {
    return reg->systems[system_id];
}

void registry_update(Registry* reg, size_t frame_index) {
    for (int i = 0; i < SYSTEMS_MAX; ++i) {
        SystemBase* system = reg->systems[i];
        if (system) {
            LOG_INFO("Update system");
            system->update_fn(reg, system, frame_index);
        }
    }
    
    BeginScopedTimer(commit_entities);

    registry_commit_entities(reg);

    AppendScopedTimer(commit_entities);
    PrintScopedTimer(commit_entities);
}


