#include "projectile_emitter_system.h"

#include "components.h"
#include "systems.h"

void projectile_emitter_system_update(Registry *registry,
                                      struct SystemBase *sys, size_t frame_nr) {
  Entity* ptr = VEC_GET_T_PTR(&sys->entities, Entity, 0);
  for (int i = 0; i < sys->entities.size; ++i) {
    
  }
}

ProjectileEmitterSystem *projectile_emitter_system_create(struct Services *services) {
  ProjectileEmitterSystem *sys =
      ArenaAlloc(&global_static_allocator, 1, ProjectileEmitterSystem);

  int component_flags = PROJECTILE_EMITTER_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT;
  system_base_init((struct SystemBase*)sys,
                   PROJECTILE_EMITTER_SYSTEM_BIT,
                   &projectile_emitter_system_update,
                   component_flags, services);
  
  return sys;
}
