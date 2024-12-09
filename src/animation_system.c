#include "animation_system.h"

#include "system.h"

#include <core/assetstore.h>
#include <core/systembase.h>
#include <core/arena.h>

struct AnimationSystem* animation_system_create(pfnSystemUpdate update_callback, Assets* assets) {
    struct AnimationSystem* system =
        ArenaAlloc(&allocator, 1, struct AnimationSystem);
    system_base_init((SystemBase*)system, ANIMATION_SYSTEM_BIT, update_callback, ANIMATION_COMPONENT_BIT, assets);
     
    return system;
}

void animation_system_prepare(struct AnimationSystem* sys, struct AnimationResources* resources) {
    ;
}
