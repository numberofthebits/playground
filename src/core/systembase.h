#ifndef _SYSTEMBASE_H
#define _SYSTEMBASE_H

#include "types.h"
#include "vec.h"
#include "assetstore.h"

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


System* system_create(pfnSystemUpdate update_fn, int required_component_flags);

void system_add_entity(System* system, Entity e);

void system_remove_entity(System* sys, Entity e);

void system_require_component(System* sys, int bit);


#endif _SYSTEM_H
