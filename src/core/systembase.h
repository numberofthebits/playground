#ifndef _SYSTEMBASE_H
#define _SYSTEMBASE_H

#include "types.h"
#include "vec.h"
#include "assetstore.h"
#include "eventbus.h"

typedef void(*pfnSystemUpdate)(struct Registry_t*, struct SystemBase_t*, size_t);

struct SystemBase_t {
    int id;
    SignatureT signature;
    Vec entities;
    pfnSystemUpdate update_fn;
    void* system_impl;
    Assets* assets;
    struct EventBus* event_bus;
};
typedef struct SystemBase_t SystemBase;

SystemBase* system_create(int system_id, pfnSystemUpdate update_fn, int required_component_flags);

void system_add_entity(SystemBase* system, Entity e);

void system_remove_entity(SystemBase* sys, Entity e);

void system_require_component(SystemBase* sys, int bit);

void system_subscribe_to_events(SystemBase* sys, int event_id, EventCallback callback);

void system_emit_event(SystemBase* sys, int event_id, void* event_data);

#endif _SYSTEM_H
