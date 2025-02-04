#ifndef PLAYER_SYSTEM_H
#define PLAYER_SYSTEM_H

#include <core/ecs.h>
#include <core/systembase.h>
#include <core/log.h>
#include <core/eventbus.h>

#define PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR (1.f / 100000000000000.f)
#define PLAYER_SYSTEM_TIME_TO_ROTATION_FACTOR (1.f / 1000000000000.f)
#define PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX 0
#define PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX 1
#define PLAYER_SYSTEM_DIRECTION_AXIS_X_INDEX 0
#define PLAYER_SYSTEM_DIRECTION_AXIS_Y_INDEX 1


struct PlayerSystem {
    struct SystemBase base;

    // 4 Axes of keyboard movement
    float movement[2];
    float angle;
    uint16_t bullets_spawned;
};

struct PlayerSystem* player_system_create(struct Services* services);

void player_system_update(Registry* registry, struct SystemBase* sys, size_t frame_nr);


void player_system_handle_event(struct SystemBase* system, struct Event e);

void player_system_reset(struct PlayerSystem* system);

#endif
