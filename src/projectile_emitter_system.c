#include "projectile_emitter_system.h"

#include "components.h"
#include "core/os.h"
#include "systems.h"

#include "core/ecs.h"
#include <math.h>

#define PROJECTILE_VELOCITY_SCALE 0.1f

static Vec2f random_direction() {
  Vec2f ret;

  float r = (float)rand() / (float)RAND_MAX * PI_MUL_2;
  ret.x = cosf(r);
  ret.y = sinf(r);

  return ret;
}

void create_projectile(ProjectileEmitterComponent *component,
                       Registry *registry, Vec3f origin, TimeT spawn_time) {
  Vec2f dir = random_direction();
  dir.x *= PROJECTILE_VELOCITY_SCALE;
  dir.y *= PROJECTILE_VELOCITY_SCALE;

  Entity e = registry_entity_create(registry);

  TransformComponent tc = {0};
  tc.pos = origin;
  tc.scale.x = 0.1f;
  tc.scale.y = 0.1f;
  tc.rotation = 0.f;

  PhysicsComponent pc = {0};
  pc.velocity.x = dir.x;
  pc.velocity.y = dir.y;

  RenderComponent rc;
  rc.render_layer = 1;

  rc.tex_coord_offset.x = 0.f;
  rc.tex_coord_offset.y = 0.f;
  rc.tex_coord_scale.x = 1.0f;
  rc.tex_coord_scale.y = 1.0f;
  rc.material_id = assets_make_id_str("bullet-mat");
  rc.pipeline_id = assets_make_id_str("tilemap");

  TimeComponent ttc = {0};
  ttc.created = spawn_time;
  ttc.expires = time_add(ttc.created, component->projectile_duration);

  CollisionComponent cc;
  cc.aabr.pos.x = tc.pos.x;
  cc.aabr.pos.y = tc.pos.y;
  cc.aabr.width = 0.1f;
  cc.aabr.height = 0.1f;

  ProjectileComponent projc;
  projc.damage = component->damage;
  projc.flags = component->flags;

  registry_entity_add_component(registry, e, TIME_COMPONENT_BIT, &ttc);
  registry_entity_add_component(registry, e, RENDER_COMPONENT_BIT, &rc);
  registry_entity_add_component(registry, e, PHYSICS_COMPONENT_BIT, &pc);
  registry_entity_add_component(registry, e, TRANSFORM_COMPONENT_BIT, &tc);
  registry_entity_add_component(registry, e, PROJECTILE_COMPONENT_BIT, &projc);

  registry_entity_add(registry, e);

  registry_entity_group(registry, e, "projectile");
}

void projectile_emitter_system_update(Registry *registry,
                                      struct SystemBase *sys, size_t frame_nr) {
  (void)frame_nr;
  struct Pool *transform_pool =
      registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);
  struct Pool *projectile_pool =
      registry_get_pool(registry, PROJECTILE_EMITTER_COMPONENT_BIT);
  TimeT now = time_now();

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity e = VEC_GET_T(&sys->entities, Entity, i);
    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, e.id);
    ProjectileEmitterComponent *pec =
        PoolGetComponent(projectile_pool, ProjectileEmitterComponent, e.id);

    TimeT elapsed = time_elapsed_now(pec->last_emitted);

    // TODO: Win API specific. Needs to use os time abstraction
    if (elapsed.QuadPart >= pec->emission_frequency.QuadPart) {
      pec->last_emitted = now;
      (void)tc;
      create_projectile(pec, registry, tc->pos, now);
    }
  }
}

ProjectileEmitterSystem *
projectile_emitter_system_create(struct Services *services) {
  ProjectileEmitterSystem *sys =
      ArenaAlloc(&global_static_allocator, 1, ProjectileEmitterSystem);

  int component_flags = PROJECTILE_EMITTER_COMPONENT_BIT |
                        TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT;
  system_base_init((struct SystemBase *)sys, PROJECTILE_EMITTER_SYSTEM_BIT,
                   &projectile_emitter_system_update, component_flags, services,
                   "ProjectileEmitterSystem");
  return sys;
}
