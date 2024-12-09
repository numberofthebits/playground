#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#include <core/assetstore.h>
#include <core/systembase.h>

struct TimeSystem {
    SystemBase base;
};

struct TimeSystem* time_system_create(pfnSystemUpdate callback, Assets* assets);

#endif
