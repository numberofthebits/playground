#include "damage_system.h"

#include "core/log.h"
#include "core/arena.h"

void damage_system_update(Registry *registry,
                          struct SystemBase *sys,
                          size_t frame_nr)
{
  (void)registry;
  (void)sys;
  (void)frame_nr;
  struct Pool* health_pool = registry_get_pool(registry, HEALTH_COMPONENT_BIT);
  
  for (int i = 0; i < sys->entities.size; ++i) {
    Entity e = VEC_GET_T(&sys->entities, Entity, i);
    HealthComponent* hc = PoolGetComponent(health_pool, HealthComponent, e.index);
    if (hc->health <= 0) {
      LOG_INFO("Removing Entity id %d index %d reached due to 0 health ", e.id, e.index);
      registry_entity_remove(registry, e);
    }
  }
}

static void damage_system_handle_collision_detected(DamageSystem* sys, struct CollisionDetectedEvent* ev) {
  (void)sys; (void)ev;
}

void damage_system_handle_event(struct SystemBase *sys,
                                struct Event e) {
  switch (e.id) {
  case CollisionSystem_Detected:
    damage_system_handle_collision_detected((DamageSystem*)sys, (struct CollisionDetectedEvent*)e.event_data);
    break;
  default:
    LOG_WARN("Unhandled event ID %d", e.id);
  }
}

DamageSystem* damage_system_create(struct Services* services) {
  DamageSystem* sys = ArenaAlloc(&global_static_allocator, 1, DamageSystem);
  system_base_init((struct SystemBase*)sys,
                   DAMAGE_SYSTEM_BIT,
                   &damage_system_update,
                   HEALTH_COMPONENT_BIT,
                   services, "DamageSystem");

  return sys;
}

