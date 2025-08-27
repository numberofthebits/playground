#include "damage_system.h"

#include "core/allocators.h"
#include "core/log.h"
#include "entity_flags.h"

void damage_system_update(Registry *registry, struct SystemBase *sys,
                          size_t frame_nr) {
  (void)registry;
  (void)sys;
  (void)frame_nr;
  Pool *health_pool = registry_get_pool(registry, HEALTH_COMPONENT_BIT);

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity e = VEC_GET_T(&sys->entities, Entity, i);
    HealthComponent *hc =
        PoolGetComponent(health_pool, HealthComponent, e.index);
    if (hc->health <= 0) {
      LOG_INFO("Removing Entity id %d index %d reached due to 0 health ", e.id,
               e.index);
      registry_entity_remove(registry, e);
    }
  }
}

static void
damage_system_handle_collision_detected(DamageSystem *sys,
                                        struct CollisionDetectedEvent *ev) {
  Registry *reg = sys->base.services.registry;

  EntityFlags flags_a = registry_entity_get_flags(reg, ev->entity_a);
  EntityFlags flags_b = registry_entity_get_flags(reg, ev->entity_b);
  if (is_same_faction(flags_a, flags_b)) {
    // For now, friendly fire isn't a thing
    return;
  }

  // Now we know the two entities are hostile towards each other
  int is_a_projectile = is_projectile(flags_a);
  int is_b_projectile = is_projectile(flags_b);

  // One should be a projectile and one should be a unit
  if (!(is_a_projectile ^ is_b_projectile)) {
    return;
  }

  Pool *health_pool = registry_get_pool(reg, HEALTH_COMPONENT_BIT);
  Pool *projectile_pool = registry_get_pool(reg, PROJECTILE_COMPONENT_BIT);

  HealthComponent *hc = 0;
  ProjectileComponent *pc = 0;
  Entity damager;
  Entity damagee;

  if (is_a_projectile && !is_b_projectile) {
    // A damages B
    damager = ev->entity_a;
    damagee = ev->entity_b;
  } else if (!is_a_projectile && is_b_projectile) {
    // B damages A
    damager = ev->entity_b;
    damagee = ev->entity_a;
  }

  hc = PoolGetComponent(health_pool, HealthComponent, damagee.index);
  pc = PoolGetComponent(projectile_pool, ProjectileComponent, damager.index);

  registry_entity_remove(sys->base.services.registry, damager);

  hc->health -= pc->damage;
}

void damage_system_handle_event(struct SystemBase *sys, struct Event e) {
  switch (e.id) {
  case CollisionSystem_Detected:
    damage_system_handle_collision_detected(
        (DamageSystem *)sys, (struct CollisionDetectedEvent *)e.event_data);
    break;
  default:
    LOG_WARN("Unhandled event ID %d", e.id);
  }
}

DamageSystem *damage_system_create(Services *services) {
  DamageSystem *sys = ArenaAlloc<DamageSystem>(&global_static_allocator, 1);
  system_base_init((struct SystemBase *)sys, DAMAGE_SYSTEM_BIT,
                   &damage_system_update, HEALTH_COMPONENT_BIT, services,
                   "DamageSystem");

  return sys;
}
