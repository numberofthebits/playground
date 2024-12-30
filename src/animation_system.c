#include "animation_system.h"

#include "system.h"

#include <core/assetstore.h>
#include <core/systembase.h>
#include <core/arena.h>

static void animation_update(Registry* reg, struct SystemBase* sys, size_t frame_nr) {
    BeginScopedTimer(animation_time);

    struct AnimationSystem* animation_system = (struct AnimationSystem*)sys;
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
    struct Pool* animation_pool = registry_get_pool(reg, ANIMATION_COMPONENT_BIT);
    struct Pool* physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);

    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for (int i = 0; i < sys->entities.size; ++i) {
        Entity entity = entities[i];
        PhysicsComponent* pc = PoolGetComponent(physics_pool, PhysicsComponent, entity.index);
        RenderComponent* rc = PoolGetComponent(render_pool, RenderComponent, entity.index);
        AnimationComponent* ac =
            PoolGetComponent(animation_pool, AnimationComponent, entity.index);
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity.index);
        
        size_t animation_frame_nr = (frame_nr / ac->frames_per_animation_frame) % ac->num_animation_frames * ac->is_playing;

        float offset = 0.f;
        float radians = tc->rotation;

        if(radians >= -PI_DIV_4 && radians < PI_DIV_4) {
            offset = 1.f;
        } else if (radians < -PI_DIV_4 && radians >= -PI_3_DIV_4) {
            offset = 3.f;
        } else if (radians > PI_DIV_4 && radians < PI_3_DIV_4) {
            offset = 0.f;
        } else {
            offset = 2.f;
        }

        tc->rotation = 0.f;//fmodf(tc->rotation - PI_DIV_4, PI_DIV_2);
              
        /* float offset = 0;             */
        /* float len = length_vec2f(&pc->velocity); */
 
        /* if (len) { */
        /*     Vec2f normalized_velocity = normalize_with_len_vec2f(&pc->velocity, len); */
        /*     Vec2f x_axis = { */
        /*         .x = 1.f, */
        /*         .y = 0.f */
        /*     }; */
        /*     Vec2f y_axis = { */
        /*         .x = 0.f, */
        /*         .y = 1.f */
        /*     }; */
        /*     float dp_x = dot_vec2f(&normalized_velocity, &x_axis); */
        /*     float dp_y = dot_vec2f(&normalized_velocity, &y_axis); */

        /*     // Pick which contributes the most to our direction */
        /*     if (fabs(dp_x) > fabs(dp_y)) { */
        /*         if (dp_x > 0) { */
        /*             // We're going right ish */
        /*             offset = 1.f; */
        /*         } else { */
        /*             // left ish */
        /*             offset = 3.f; */
        /*         }                 */
        /*     } else { */
        /*         if (dp_y > 0) { */
        /*             // up */
        /*             offset = 0.f; */
        /*         } else { */
        /*             // down */
        /*             offset = 2.f; */
        /*         } */
        /*     } */

        /*     ac->last_offset = offset; */

        /* } */

        ac->last_offset = offset;

        rc->tex_coord_scale.x = 1.f / ac->num_frames_width;
        rc->tex_coord_scale.y = 1.f / ac->num_frames_height;

        rc->tex_coord_offset.x = 1.f / ac->num_frames_width * animation_frame_nr;
        rc->tex_coord_offset.y = ((float)ac->num_frames_height - 1.f - ac->last_offset) / (float)ac->num_frames_height;
    }

    AppendScopedTimer(animation_time);
    PrintScopedTimer(animation_time);
}


struct AnimationSystem* animation_system_create(Assets* assets, struct EventBus* event_bus) {
    struct AnimationSystem* system =
        ArenaAlloc(&allocator, 1, struct AnimationSystem);
    system_base_init((struct SystemBase*)system, ANIMATION_SYSTEM_BIT, &animation_update, ANIMATION_COMPONENT_BIT, assets, event_bus);
     
    return system;
}

void animation_system_prepare(struct AnimationSystem* sys, struct AnimationResources* resources) {
    ;
}
