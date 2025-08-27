#include "projectile_emitter_system.h"

#include "components.h"
#include "core/os.h"
#include "entity_flags.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/ecs.h"
#include <math.h>

#define PROJECTILE_VELOCITY_SCALE 0.1f

static Vec2f random_direction() {
  Vec2f ret;

  float r = (float)rand() / (float)RAND_MAX * PI_MUL_2;
  (void)r;
  /* ret.x = cosf(r); */
  /* ret.y = sinf(r); */
  ret.x = -1.f;
  ret.y = 0.f;

  return ret;
}

void create_projectile(ProjectileEmitterComponent *component,
                       Registry *registry, Vec3f origin, TimeT spawn_time) {
  Vec2f dir = random_direction();
  Entity e = registry_entity_create(registry);

  TransformComponent tc = {};
  tc.pos.x = origin.x + dir.x;
  tc.pos.y = origin.y + dir.y;
  tc.pos.z = 1.f;
  tc.scale = 0.1f;
  tc.rotation = 0.f;

  PhysicsComponent pc = {};
  pc.velocity.x = dir.x * PROJECTILE_VELOCITY_SCALE;
  pc.velocity.y = dir.y * PROJECTILE_VELOCITY_SCALE;

  RenderComponent rc;
  rc.material_id = assets_make_id_str("bullet.mat");
  rc.pipeline_id = assets_make_id_str("unit.prog");
  rc.texture_atlas_index.x = 0;
  rc.texture_atlas_index.y = 0;
  rc.texture_atlas_size.x = 2;
  rc.texture_atlas_size.y = 4;
  rc.render_layer = RENDER_COMPONENT_RENDER_LAYER_AIR;

  TimeComponent ttc = {};
  ttc.created = spawn_time;
  ttc.expires = time_add(ttc.created, component->projectile_duration);

  CollisionComponent cc;
  cc.width = 0.1f;
  cc.height = 0.1f;

  ProjectileComponent projc;
  projc.damage = component->damage;

  registry_entity_component_add(registry, e, TIME_COMPONENT_BIT, &ttc);
  registry_entity_component_add(registry, e, RENDER_COMPONENT_BIT, &rc);
  registry_entity_component_add(registry, e, PHYSICS_COMPONENT_BIT, &pc);
  registry_entity_component_add(registry, e, TRANSFORM_COMPONENT_BIT, &tc);
  registry_entity_component_add(registry, e, COLLISION_COMPONENT_BIT, &cc);
  registry_entity_component_add(registry, e, PROJECTILE_COMPONENT_BIT, &projc);

  registry_entity_add(registry, e);

  registry_entity_group(registry, e, "projectile");
  registry_entity_set_flags(registry, e, ENTITY_PROJECTILE_HOSTILE);
}

void projectile_emitter_system_update(Registry *registry,
                                      struct SystemBase *sys, size_t frame_nr) {
  (void)frame_nr;
  Pool *transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);
  Pool *projectile_pool =
      registry_get_pool(registry, PROJECTILE_EMITTER_COMPONENT_BIT);
  TimeT now = time_now();

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity e = VEC_GET_T(&sys->entities, Entity, i);
    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, e.index);
    ProjectileEmitterComponent *pec =
        PoolGetComponent(projectile_pool, ProjectileEmitterComponent, e.index);

    TimeT elapsed = time_elapsed_now(pec->last_emitted);

    if (time_gte(elapsed, pec->emission_frequency)) {
      pec->last_emitted = now;
      (void)tc;
      create_projectile(pec, registry, tc->pos, now);
    }
  }
}

ProjectileEmitterSystem *projectile_emitter_system_create(Services *services) {
  ProjectileEmitterSystem *sys =
      ArenaAlloc<ProjectileEmitterSystem>(&global_static_allocator, 1);

  int component_flags = PROJECTILE_EMITTER_COMPONENT_BIT |
                        TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT;
  system_base_init((struct SystemBase *)sys, PROJECTILE_EMITTER_SYSTEM_BIT,
                   &projectile_emitter_system_update, component_flags, services,
                   "ProjectileEmitterSystem");
  return sys;
}
