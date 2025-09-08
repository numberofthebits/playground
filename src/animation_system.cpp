#include "animation_system.h"

#include "components.h"
#include "core/allocators.h"
#include "core/math.h"
#include "core/systembase.h"
#include "systems.h"

static void animation_update(Registry *reg, struct SystemBase *system,
                             size_t frame_nr, TimeT frame_time_now) {
  (void)frame_nr;
  (void)system;
  (void)frame_time_now;

  Pool *render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
  Pool *animation_pool = registry_get_pool(reg, ANIMATION_COMPONENT_BIT);
  //  Pool *transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  Pool *physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);

  Entity *entities = VEC_ITER_BEGIN_T(&system->entities, Entity);

  for (int i = 0; i < system->entities.size; ++i) {
    Entity entity = entities[i];
    RenderComponent *rc =
        PoolGetComponent(render_pool, RenderComponent, entity.index);

    AnimationComponent *ac =
        PoolGetComponent(animation_pool, AnimationComponent, entity.index);

    PhysicsComponent *pc =
        PoolGetComponent(physics_pool, PhysicsComponent, entity.index);

    size_t animation_frame_nr = (frame_nr / ac->frames_per_animation_frame) %
                                ac->num_animation_frames * ac->is_playing;

    // X is our animation playing in time
    rc->texture_atlas_index.x = animation_frame_nr;

    // Y is WHICH animation is playing. I.e. where is our unit
    // headed
    if (fabsf(pc->velocity.x) > fabsf(pc->velocity.y)) {
      if (pc->velocity.x >= 0) {
        // right
        rc->texture_atlas_index.y = 1;
      } else {
        // left
        rc->texture_atlas_index.y = 3;
      }
    } else {
      if (pc->velocity.y >= 0) {
        // up
        rc->texture_atlas_index.y = 0;
      } else {
        // down
        rc->texture_atlas_index.y = 2;
      }
    }
  }
}

struct AnimationSystem *animation_system_create(Services *services) {
  struct AnimationSystem *system =
      ArenaAlloc<AnimationSystem>(&global_static_allocator, 1);

  RequiredComponents components = {
      .signature = ANIMATION_COMPONENT_BIT | PHYSICS_COMPONENT_BIT |
                   RENDER_COMPONENT_BIT,
      .read_access_flags = ANIMATION_COMPONENT_BIT | PHYSICS_COMPONENT_BIT,
      .write_access_flags = RENDER_COMPONENT_BIT};

  system_base_init((struct SystemBase *)system, ANIMATION_SYSTEM_BIT,
                   &animation_update, components, services, "AnimationSystem");

  return system;
}
