#ifndef _SYSTEMBASE_H
#define _SYSTEMBASE_H

#include "types.h"
#include "vec.h"
#include "assetstore.h"
#include "eventbus.h"

typedef void(*pfnSystemUpdate)(struct Registry_t*, struct SystemBase*, size_t);

struct SystemBase {
    int id;
    SignatureT signature;
    Vec entities;
    pfnSystemUpdate update_fn;
    void* system_impl;
    Assets* assets;
    struct EventBus* event_bus;
};
//typedef struct SystemBase_t SystemBase;

void system_base_init(struct SystemBase* system,
                      int system_id,
                      pfnSystemUpdate update_fn,
                      int required_component_flags,
                      Assets* assets,
                      struct EventBus* event_bus);

void system_add_entity(struct SystemBase* system, Entity e);

void system_remove_entity(struct SystemBase* sys, Entity e);

void system_require_component(struct SystemBase* sys, int bit);

// void system_subscribe_to_event(SystemBase* sys, int event_id, EventCallback callback);

// void system_emit_event(SystemBase* sys, struct Event* event);

#endif _SYSTEM_H
