#include "systembase.h"

#include "types.h"
#include <core/arena.h>

#include <stdlib.h>

#define SYSTEM_ENTITIES_DEFAULT_CAPACITY 1024

static int find_entity_index(Vec* entities, Entity e) {
    Entity* iter = VEC_ITER_BEGIN_T(entities, Entity);
    for (int i = 0; i < entities->size; ++i, ++iter) {
        if (iter->id == e.id) {
            return i;
        }
    }
    return ENTITY_INVALID_INDEX;
}

void system_base_init(SystemBase* system, int system_id, pfnSystemUpdate update_fn, int required_component_flags, Assets* assets) {
    system->id = system_id;
    system->signature = required_component_flags;
    system->update_fn = update_fn;
    system->entities = vec_create();
    system->assets = assets;
    VEC_RESERVE_T(&system->entities, Entity, SYSTEM_ENTITIES_DEFAULT_CAPACITY);
}

void system_add_entity(SystemBase* system, Entity e) {
    int index = find_entity_index(&system->entities, e);
    if (index != ENTITY_INVALID_INDEX) {
        LOG_WARN("Entity %d already in System %d", e.id, system->id);
        return;
    }

    VEC_PUSH_T(&system->entities, Entity, e);
}

void system_remove_entity(SystemBase* system, Entity e) {
    int index = find_entity_index(&system->entities, e);
    if (index == ENTITY_INVALID_INDEX) {
        LOG_WARN("Entity %d not found in System %d", e.id, system->id);
        return;
    }

    LOG_INFO("Remove entity ID %d with index from system %d", e.id, e.index, system->id);
    VEC_ERASE_T(&system->entities, Entity, index);
}

void system_require_component(SystemBase* sys, int bit) {
    sys->signature |= bit;
}

void system_subscribe_to_events(SystemBase* sys, int event_id, EventCallback callback) {
    event_bus_subscribe(sys->event_bus, sys->id, event_id, callback);
}

void system_emit_event(SystemBase* sys, int event_id, void* event_data) {
    struct Event event;
    event.id = event_id;
    event.event_data = event_data;
    
    event_bus_publish(sys->event_bus, sys->id, event);
}
