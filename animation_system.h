#ifndef _ANIMATION_SYSTEM_H
#define _ANIMATION_SYSTEM_H

#include "ecs.h"

struct AnimationSystem;
struct AnimationResource {
    AssetId texture_id;
    uint8_t tile_width;
    uint8_t tile_height;
};
struct AnimationResources {
    struct AnimationResource* resources;
    size_t count;
};
struct AnimationSystem* animation_system_create(Assets* assets);

void animation_system_prepare(struct AnimationSystem* sys, struct AnimationResources* resources);


#endif // _ANIMATION_SYSTEM_H
