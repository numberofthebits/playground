#include "animation_system.h"

#include "arena.h"

struct AnimationSystem {
    Assets* assets;
};

struct AnimationSystem* animation_system_create(Assets* assets) {
    struct AnimationSystem* impl =
        ArenaAlloc(&allocator, 1, struct AnimationSystem);
    impl->assets = assets;

    return impl;
}

void animation_system_prepare(struct AnimationSystem* sys, struct AnimationResources* resources) {
    /* for (int i = 0; i < resources->count; ++i) { */
        
    /* } */
}
