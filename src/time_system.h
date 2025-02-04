#ifndef TIME_SYSTEM_H
#define TIME_SYSTEM_H

#include <core/systembase.h>

struct TimeSystem {
    struct SystemBase base;
};

struct TimeSystem* time_system_create(struct Services* services);

#endif
