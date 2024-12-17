#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#include <core/assetstore.h>
#include <core/systembase.h>

struct TimeSystem {
    struct SystemBase base;
};

struct TimeSystem* time_system_create(Assets* assets, struct EventBus* event_bus);

#endif
