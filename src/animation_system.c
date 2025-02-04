#include "animation_system.h"

#include "system.h"

#include <core/assetstore.h>
#include <core/systembase.h>
#include <core/arena.h>
#include <core/os.h>
#include <core/math.h>

static void animation_update(Registry* reg, struct SystemBase* system, size_t frame_nr) {
    (void)frame_nr;
    (void)system;
    BeginScopedTimer(animation_time);

    //struct AnimationSystem* animation_system = (struct AnimationSystem*)system;
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
    struct Pool* animation_pool = registry_get_pool(reg, ANIMATION_COMPONENT_BIT);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);

    Entity* entities = VEC_ITER_BEGIN_T(&system->entities, Entity);
    
    for (int i = 0; i < system->entities.size; ++i) {
        Entity entity = entities[i];
        RenderComponent* rc = PoolGetComponent(render_pool, RenderComponent, entity.index);
        AnimationComponent* ac =
            PoolGetComponent(animation_pool, AnimationComponent, entity.index);
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity.index);
        
        size_t animation_frame_nr = (frame_nr / ac->frames_per_animation_frame) % ac->num_animation_frames * ac->is_playing;

        float offset = 0.f;
        float radians = tc->rotation;

        // Pick from our sprite sheet based on angle
        if (radians < PI_DIV_4) {
            offset = 1.f;
        } else if (radians < PI_3_DIV_4) {
            // up
            offset = 0.f;
        } else if (radians < (PI + PI_DIV_4)) {
            // left
            offset = 3.f;
        } else if (radians < PI_7_DIV_4) {
            // down
            offset = 2.f;
        } else {
            offset = 1.f;
            // right again, different quadrant
        }

        // The rotations within each sprite are always in +/- PI/4 (45 degrees),
        // which is PI/2 total. Adding PI/4 maps our rotation to +/- PI/4 radians.
        // Making it wrap correctly at PI_DIV_4. To map inversely, subtract PI/4 radians
        // again.
        tc->rotation = (float)fmod((tc->rotation + PI_DIV_4), PI_DIV_2) - PI_DIV_4;

        ac->last_offset = offset;

        rc->tex_coord_scale.x = 1.f / ac->num_frames_width;
        rc->tex_coord_scale.y = 1.f / ac->num_frames_height;

        rc->tex_coord_offset.x = 1.f / ac->num_frames_width * animation_frame_nr;
        rc->tex_coord_offset.y = ((float)ac->num_frames_height - 1.f - ac->last_offset) / (float)ac->num_frames_height;
    }

    AppendScopedTimer(animation_time);
    PrintScopedTimer(animation_time);
}


struct AnimationSystem* animation_system_create(struct Services* services) {
    struct AnimationSystem* system =
        ArenaAlloc(&allocator, 1, struct AnimationSystem);
    
    system_base_init((struct SystemBase*)system,
                     ANIMATION_SYSTEM_BIT,
                     &animation_update,
                     ANIMATION_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT | RENDER_COMPONENT_BIT,
                     services);
     
    return system;
}

void animation_system_prepare(struct AnimationSystem* sys, struct AnimationResources* resources) {
  (void)sys;
  (void)resources;
}
