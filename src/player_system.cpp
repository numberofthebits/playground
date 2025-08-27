#include "player_system.h"

#include "components.h"
#include "entity_flags.h"
#include "events.h"
#include "input_system.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/math.h"
#include "core/systembase.h"

#include <GLFW/glfw3.h>

#define PLAYER_ROTATE_ANGLE_DELTA PI_DIV_4 / 8.f;

static Vec2f PLAYER_SYSTEM_PLAYER_VELOCITY_MAX = {0.05f, 0.05f};
static Vec2f PLAYER_SYSTEM_BULLET_VELOCITY_DEFAULT = {0.1f, 0.1f};

static Vec2f clamp_velocity(Vec2f v, Vec2f max) {
  Vec2f clamped = v;
  if (v.x < -max.x)
    clamped.x = -max.x;
  if (v.y < -max.y)
    clamped.y = -max.y;
  if (v.x > max.x)
    clamped.x = max.x;
  if (v.y > max.y)
    clamped.y = max.y;

  return clamped;
}

void player_system_reset(struct PlayerSystem *system) {
  memset(system->movement, 0x0, sizeof(system->movement));
  system->bullets_spawned = 0;
}

struct PlayerSystem *player_system_create(Services *services) {
  struct PlayerSystem *system =
      ArenaAlloc<PlayerSystem>(&global_static_allocator, 1);

  int component_flags =
      INPUT_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT;
  system_base_init((struct SystemBase *)system, PLAYER_SYSTEM_BIT,
                   &player_system_update, component_flags, services,
                   "PlayerSystem");

  player_system_reset(system);
  return system;
}

static void player_system_spawn_projectile(Registry *registry, Vec3f player_pos,
                                           float angle) {
  Entity e = registry_entity_create(registry);

  registry_entity_group(registry, e, "projectile");

  Vec2f projectile_dir = {sinf(angle), cosf(angle)};

  TransformComponent tc = {};
  // Offset to avoid self collision
  tc.pos.x = player_pos.x + projectile_dir.x;
  tc.pos.y = player_pos.y + projectile_dir.y;
  tc.pos.z = 0.0f;
  tc.scale = 0.1f;
  tc.rotation = 0.f;

  PhysicsComponent pc = {};
  pc.velocity.x = projectile_dir.x * PLAYER_SYSTEM_BULLET_VELOCITY_DEFAULT.x;
  pc.velocity.y = projectile_dir.y * PLAYER_SYSTEM_BULLET_VELOCITY_DEFAULT.y;

  RenderComponent rc;
  rc.render_layer = 1;
  rc.material_id = assets_make_id_str("bullet.mat");
  rc.pipeline_id = assets_make_id_str("tilemap.prog");
  rc.texture_atlas_index.x = 0;
  rc.texture_atlas_index.y = 0;
  rc.texture_atlas_size.x = 1;
  rc.texture_atlas_size.y = 1;

  TimeComponent ttc = {};
  ttc.created = time_now();
  TimeT expires = time_from_secs(5);
  ttc.expires = time_add(ttc.created, expires);

  CollisionComponent cc;
  cc.width = 1.f * tc.scale;
  cc.height = 1.f * tc.scale;

  ProjectileComponent prc;
  prc.damage = 50;

  registry_entity_component_add(registry, e, TIME_COMPONENT_BIT, &ttc);
  registry_entity_component_add(registry, e, RENDER_COMPONENT_BIT, &rc);
  registry_entity_component_add(registry, e, PHYSICS_COMPONENT_BIT, &pc);
  registry_entity_component_add(registry, e, TRANSFORM_COMPONENT_BIT, &tc);
  registry_entity_component_add(registry, e, COLLISION_COMPONENT_BIT, &cc);
  registry_entity_component_add(registry, e, PROJECTILE_COMPONENT_BIT, &prc);
  registry_entity_add(registry, e);
  registry_entity_set_flags(registry, e, ENTITY_PROJECTILE_FRIENDLY);
}

void player_system_update(Registry *registry, struct SystemBase *sys,
                          size_t frame_nr) {
  (void)frame_nr;
  struct PlayerSystem *player_system = (struct PlayerSystem *)sys;
  Pool *physics_pool = registry_get_pool(registry, PHYSICS_COMPONENT_BIT);
  Pool *transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);

  Entity *entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

  for (int i = 0; i < player_system->base.entities.size; ++i) {
    Entity entity = entities[i];
    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, entity.index);

    PhysicsComponent *pc =
        PoolGetComponent(physics_pool, PhysicsComponent, entity.index);

    // NOTE: It would probably be fine to spawn 1 bullet per frame
    for (size_t j = 0; j < player_system->bullets_spawned; ++j) {
      player_system_spawn_projectile(registry, tc->pos, player_system->angle);
    }

    pc->velocity.x +=
        player_system->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX];
    pc->velocity.y +=
        player_system->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX];

    pc->velocity =
        clamp_velocity(pc->velocity, PLAYER_SYSTEM_PLAYER_VELOCITY_MAX);

    tc->rotation = player_system->angle;
  }

  player_system_reset(player_system);
}

static void
player_system_handle_keyboard_update(PlayerSystem *sys,
                                     AggregatedKeyboardEvents *event_data) {
  for (size_t i = 0; i < event_data->num_events; ++i) {

    struct KeyStateEventData key_state = event_data->events[i];
    switch (key_state.key) {
    case GLFW_KEY_SPACE:
      sys->bullets_spawned++;
      break;
    case GLFW_KEY_UP:
      sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX] +=
          PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
      break;
    case GLFW_KEY_DOWN:
      sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX] -=
          PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
      break;
    case GLFW_KEY_LEFT:
      sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX] -=
          PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
      break;
    case GLFW_KEY_RIGHT:
      sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX] +=
          PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
      break;
    /* case GLFW_KEY_A: */
    /*   sys->angle += PLAYER_ROTATE_ANGLE_DELTA; */
    /*   sys->angle = fmodf(sys->angle, PI_MUL_2); */
    /*   break; */
    /* case GLFW_KEY_D: */
    /*   sys->angle -= PLAYER_ROTATE_ANGLE_DELTA; */
    /*   sys->angle = fmodf(sys->angle, PI_MUL_2); */
    /*   if (sys->angle < 0.f) { */
    /*     sys->angle += PI_MUL_2; */
    /*   } */
    /*   break; */
    default:
      LOG_WARN("Unhandled key input: Key code %d", key_state.key);
    }
  }
}

void player_system_handle_event(struct SystemBase *base, struct Event e) {
  LOG_INFO("Player system handle event");
  struct PlayerSystem *system = (struct PlayerSystem *)base;
  switch (e.id) {
  case KeyboardInput_Update:
    player_system_handle_keyboard_update(
        system, (struct AggregatedKeyboardEvents *)e.event_data);
    break;
  default:
    LOG_WARN("Unhandled event %d", e.id);
  }
}
