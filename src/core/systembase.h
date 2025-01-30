#ifndef SYSTEMBASE_H
#define SYSTEMBASE_H

#include <core/types.h>
#include <core/vec.h>
#include <core/assetstore.h>
#include <core/eventbus.h>
#include <core/ecs.h>

typedef struct Registry_t Registry;
typedef void(*pfnSystemUpdate)(Registry*, struct SystemBase*, size_t);

struct SystemBase {
    int id;
    SignatureT signature;
    Vec entities;
    pfnSystemUpdate update_fn;
    Assets* assets;
    struct EventBus* event_bus;
};

void system_base_init(struct SystemBase* system,
                      int system_id,
                      pfnSystemUpdate update_fn,
                      int required_component_flags,
                      Assets* assets,
                      struct EventBus* event_bus);

void system_add_entity(struct SystemBase* system, Entity entity);

void system_remove_entity(struct SystemBase* system, Entity entity);

void system_require_component(struct SystemBase* system, int bit);

#endif // SYSTEMBASE_H
