#include "player_system.h"

#include "system.h"
#include "component.h"
#include "input_system.h"
#include "events.h"

#include <core/math.h>
#include <core/arena.h>
#include <core/systembase.h>

#include <GLFW/glfw3.h>

#define PLAYER_ROTATE_ANGLE_DELTA PI_DIV_4 / 8.f;

static Vec2f BULLET_MIN_VELOCITY = {0.1f, 0.1f};

void player_system_reset(struct PlayerSystem* system) {
    memset(system->movement, 0x0, sizeof(system->movement));
    system->bullets_spawned = 0;
}

struct PlayerSystem* player_system_create(struct Services* services) {
    struct PlayerSystem* system =
        ArenaAlloc(&allocator, 1, struct PlayerSystem);

    int component_flags = INPUT_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT;
    system_base_init((struct SystemBase*)system, PLAYER_SYSTEM_BIT, &player_system_update, component_flags, services);

    player_system_reset(system);
    return system;
}

static void player_system_spawn_bullet(Registry* registry, Vec3f player_pos, Vec2f player_velocity) {
    Entity e = registry_create_entity(registry);

    TransformComponent tc = {0};
    tc.pos = player_pos;
    tc.pos.z = 0.0f;
    tc.scale.x = 0.1f;
    tc.scale.y = 0.1f;
    tc.rotation = 0.f;
            
    PhysicsComponent pc = {0};
    pc.velocity.x = Max(player_velocity.x, player_velocity.x + BULLET_MIN_VELOCITY.x);
    pc.velocity.y = Max(player_velocity.y, player_velocity.y + BULLET_MIN_VELOCITY.y);
    
    RenderComponent rc;
    rc.render_layer = 1;
            
    rc.tex_coord_offset.x = 0.f;
    rc.tex_coord_offset.y = 0.f;
    rc.tex_coord_scale.x = 1.0f;
    rc.tex_coord_scale.y = 1.0f;
    rc.material_id = assets_make_id_str("bullet-mat");
    rc.pipeline_id = assets_make_id_str("tilemap");
            
    TimeComponent ttc = {0};
    ttc.created = time_now();
    TimeT expires = time_from_secs(5);
    ttc.expires = time_add(ttc.created, expires);

    registry_add_component(registry, e, TIME_COMPONENT_BIT, &ttc);
    registry_add_component(registry, e, RENDER_COMPONENT_BIT, &rc);
    registry_add_component(registry, e, PHYSICS_COMPONENT_BIT, &pc);
    registry_add_component(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

    registry_add_entity(registry, e);
}

void player_system_update(Registry* registry, struct SystemBase* sys, size_t frame_nr) {
    BeginScopedTimer(player_system_update);
    (void)frame_nr;
    struct PlayerSystem* player_system = (struct PlayerSystem*)sys;
    struct Pool* physics_pool = registry_get_pool(registry, PHYSICS_COMPONENT_BIT);
    struct Pool* transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);
    
    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

    for(int i = 0; i < player_system->base.entities.size; ++i) {
        Entity entity = entities[i];
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity.index);
        
        PhysicsComponent* pc = PoolGetComponent(physics_pool, PhysicsComponent, entity.index);

        // NOTE: It would probably be fine to spawn 1 bullet per frame
        for (size_t j = 0; j < player_system->bullets_spawned; ++j) {
            player_system_spawn_bullet(registry, tc->pos, pc->velocity);
        }

        pc->velocity.x +=
            player_system->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX];
        pc->velocity.y +=
            player_system->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX];

        tc->rotation = player_system->angle;
    }

    player_system_reset(player_system);

    AppendScopedTimer(player_system_update);
    PrintScopedTimer(player_system_update);
}


static void player_system_handle_keyboard_update(struct PlayerSystem* sys, struct AggregatedKeyboardEvents* event_data) {
    for(size_t i = 0; i < event_data->num_events; ++i) {
        struct KeyStateEventData key_state = event_data->events[i];
        switch (key_state.key) {
        case GLFW_KEY_SPACE:
            sys->bullets_spawned++;
            break;
        case GLFW_KEY_UP:
            sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX] +=
	      (float)time_to_nanosecs(key_state.elapsed) * PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
            break;
        case GLFW_KEY_DOWN:
            sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_Y_INDEX] -=
                (float)time_to_nanosecs(key_state.elapsed) * PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
            break;
        case GLFW_KEY_LEFT:
            sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX] -=
                (float)time_to_nanosecs(key_state.elapsed) * PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
            break;
        case GLFW_KEY_RIGHT:
            sys->movement[PLAYER_SYSTEM_MOVEMENT_AXIS_X_INDEX] +=
                (float)time_to_nanosecs(key_state.elapsed) * PLAYER_SYSTEM_TIME_TO_MOVEMENT_FACTOR;
            break;
        case GLFW_KEY_A:
            sys->angle += PLAYER_ROTATE_ANGLE_DELTA;
            sys->angle = fmodf(sys->angle, PI_MUL_2);
            break;
        case GLFW_KEY_D:
            sys->angle -= PLAYER_ROTATE_ANGLE_DELTA;
            sys->angle = fmodf(sys->angle, PI_MUL_2);
            if(sys->angle < 0.f) {
                sys->angle += PI_MUL_2;
            }
            break;
        default:
            LOG_WARN("Unhandled key input: Key code %d", key_state.key);
        }
    }
}

void player_system_handle_event(struct SystemBase* base, struct Event e) {
    struct PlayerSystem* system = (struct PlayerSystem*)base;
    switch (e.id) {
    case KeyboardInput_Update:
        player_system_handle_keyboard_update(system, (struct AggregatedKeyboardEvents*)e.event_data);
        break;
    default:
        LOG_WARN("Unhandled event %d", e.id);
    }
}
