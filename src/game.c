#include "game.h"

#include "system.h"
#include "component.h"
#include "render_system.h"
#include "animation_system.h"
#include "input_system.h"

#include <core/arena.h>
#include <core/assetstore.h>
#include <core/log.h>
#include <core/math.h>
#include <core/os.h>
#include <core/ecs.h>
#include <core/util.h>
#include <core/systembase.h>

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>
#include <math.h>


#define MAX_ENTITIES 1024

typedef struct {
    Vec2u8 atlas_coord;
} MapTile;

struct Map_t {
    Vec2u8 map_size;
    Vec2u8 atlas_size;
    MapTile* tiles;
};
typedef struct Map_t Map;

struct Game_t {
    Registry registry;
    GLFWwindow* window;
    Assets assets;
    Map map;

    int main_loop_running;
    size_t frame_counter;
};

enum ReadlineResult {
    READLINE_DONE = 0,
    READLINE_CONTINUE = 1,
    READLINE_BUFFER_TOO_SMALL = -2,
    READLINE_READ_ERROR = -3
};
typedef enum ReadlineResult_t ReadlineResult;

static ReadlineResult readline(FILE* fp, char* buffer, int buf_size, int*line_len) {
    long int curr_pos = ftell(fp);
    int len = 0;
    char curr_char = 0;
    int more_lines = 0;
    int line_done = 0;
    do {
        char c = fgetc(fp);
        switch (c) {
        case '\n':
            more_lines = 1;
            line_done = 1;
            ++len;
            break;
        case EOF:
            more_lines = 0;
            line_done = 1;
            break;
        default:
            ++len;
        }
    } while (!line_done);

    fseek(fp, curr_pos, SEEK_SET);
    if (buf_size < len) {
        return READLINE_BUFFER_TOO_SMALL;
    }   

    if (fread(buffer, len, 1, fp) != 1) {
        return READLINE_READ_ERROR;
    }

    if (more_lines) {
        return READLINE_CONTINUE;
    }

    return READLINE_DONE;
}


MapTile map_get_tile(Map* map, int col, int row) {
    return map->tiles[row * 25 + col];
}

void map_set_tile(Map* map, int col, int row, int x, int y) {
    MapTile* tile = &map->tiles[row * 25 + col];
    tile->atlas_coord.x = (uint8_t)x;
    tile->atlas_coord.y = (uint8_t)y;
}

void map_print(Map* map) {
    for (int row = 0; row < 20; ++row) {
        for (int col = 0; col < 25; ++col) {
            MapTile tile = map_get_tile(map, col, row);
            printf("%d%d,", tile.atlas_coord.x, tile.atlas_coord.y);
        }
        printf("\n");
    }
}

static int load_tile_map_layout(const char* file, Map* map) {

    FILE* fp = fopen(file, "rb");
    if(!fp) {
        LOG_ERROR("Failed to open file '%s'", file);
        return 0;
    }

    char line_buf[1024];
    int map_row_index = 0;
    ReadlineResult read_result;
    int map_index_row = 0;
    int map_index_col = 0;

    do {
        int tile_row = 0;
        int tile_col = 0;
        int line_len = 0;
        memset(line_buf, 0x0, 1024);
        read_result = readline(fp, line_buf, 1024, &line_len);

        switch (read_result) {
        case READLINE_READ_ERROR:
            return 0;       
        case READLINE_BUFFER_TOO_SMALL:
            return 0;            
        case READLINE_CONTINUE:
        case READLINE_DONE: {
            map_index_col = 0;
            int offset = 0;
            for (int i = 0; i < 25; ++i) {
                sscanf(line_buf + offset, "%1d", &tile_row);
                sscanf(line_buf + offset + 1, "%1d", &tile_col);
//                LOG_INFO("map [%d, %d] = [%d, %d]", map_index_col, map_index_row, tile_col, tile_row);
                map_set_tile(map, map_index_col, 19 - map_index_row, tile_col, tile_row);
                map_index_col++;
                offset += 3; // skip comma
                ++tile_col;
            }
            map_index_row++;
            if(read_result == READLINE_DONE) {
                return 1;
            }
        }
            break;
        default:
            return 0;
        }
    } while(read_result != READLINE_DONE);

    return 1;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Game* game = glfwGetWindowUserPointer(window);

    SystemBase* base = registry_get_system(&game->registry, INPUT_SYSTEM_BIT); */
    struct InputSystem* input_system = base->system_impl;

    if(key == GLFW_KEY_ESCAPE) {
        game->main_loop_running = 0;
        return;
    }

    input_system_handle_keyboard(input_system, key, action);
    
}

static void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    if (!window) {
        LOG_ERROR("Nullptr window in frame buffer size callback. Width %d, height %d", width, height);
        return;
    }

    Game* game = glfwGetWindowUserPointer(window);
    game_frame_buffer_size_changed(game, width, height);
}

static void window_size_callback(GLFWwindow* window, int width, int height) {
    if (!window) {
        LOG_ERROR("Nullptr window in window size callback. Width %d, height %d", width, height);
        return;
    }

    Game* game = glfwGetWindowUserPointer(window);
    game_window_size_changed(game, width, height);
}


static void print_transform_component(TransformComponent* tc) {
    LOG_INFO("\tTransform pos (%f, %f) ", tc->pos.x, tc->pos.y);
}

static void movement_update(Registry* reg, SystemBase* system, size_t frame_nr) {
    BeginScopedTimer(movement_time);

    Entity* entities = VEC_ITER_BEGIN_T(&system->entities, Entity);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
    struct Pool* physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);
    
    for (int i = 0; i < system->entities.size; ++i) {
        const int entity_index = entities[i].id;
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity_index);
        PhysicsComponent* pc = PoolGetComponent(physics_pool, PhysicsComponent, entity_index);
        tc->pos.x += pc->velocity.x;
        tc->pos.y += pc->velocity.y;

        if (tc->pos.x < 0.f) {
            pc->velocity.x = -pc->velocity.x;
        }
        if (tc->pos.y < 0.f) {
            pc->velocity.y = -pc->velocity.y;
        }
    }
    
    AppendScopedTimer(movement_time);
    PrintScopedTimer(movement_time);
}

static void physics_update(Registry* reg, SystemBase* s, size_t frame_nr) {
    BeginScopedTimer(physics_time);

    Entity* entities = VEC_ITER_BEGIN_T(&s->entities, Entity);
    struct Pool* physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);

    for (int i = 0; i < s->entities.size; ++i) {
        ; //Entity entity = entities[i];
    }
    
    AppendScopedTimer(physics_time);
    PrintScopedTimer(physics_time);
}

static void render_update(Registry* reg, SystemBase* s, size_t frame_nr) {
    BeginScopedTimer(render_time);

    RenderSystem* render_sys = (RenderSystem*)s->system_impl;
    Entity* entities = VEC_ITER_BEGIN_T(&s->entities, Entity);

    // This assumes knowledge of which pools the system needs.
    // That seems okay, since the implementation of a system can't
    // not need to know about the components. But we already have
    // this information in the system's signature
    //
    // PS: Write better explanations of your thinking when you
    // have a ideas or concerns. The notes above are not exactly clear.
    // WHY is was this knowledge of the pools needed considered bad?
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
    Vec* render_data = render_system_get_render_data(render_sys, s->entities.size);
    
    for (int i = 0; i < s->entities.size; ++i) {
        Entity entity = entities[i];
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity.id);
        RenderComponent* rc = PoolGetComponent(render_pool, RenderComponent, entity.id);

        RenderData* rd = VEC_GET_T_PTR(render_data, RenderData, entity.id);

        rd->render_layer = rc->render_layer;
        
        Vec3f scale;
        scale.x = tc->scale.x;
        scale.y = tc->scale.y;
        scale.z = 1.0f;
        
        Vec3f pos;
        pos.x = tc->pos.x;
        pos.y = tc->pos.y;
        pos.z = tc->pos.z;

        rd->tex_coord_offset = rc->tex_coord_offset;
        rd->tex_coord_scale = rc->tex_coord_scale;
        rd->model_matrix = identity();

        scale_mat4(&rd->model_matrix, &scale);
        translate(&rd->model_matrix, &pos);

        rd->material_id = rc->material_id;
        rd->program_id = rc->pipeline_id;
    }

    render_system_update(render_sys);

    AppendScopedTimer(render_time);
    PrintScopedTimer(render_time);
}

static void animation_update(Registry* reg, SystemBase* sys, size_t frame_nr) {
    BeginScopedTimer(animation_time);

    struct AnimationSystem* animation_system = sys->system_impl;
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
    struct Pool* animation_pool = registry_get_pool(reg, ANIMATION_COMPONENT_BIT);
    struct Pool* physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);

    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for (int i = 0; i < sys->entities.size; ++i) {
        Entity entity = entities[i];
        PhysicsComponent* pc = PoolGetComponent(physics_pool, PhysicsComponent, entity.id);
        RenderComponent* rc = PoolGetComponent(render_pool, RenderComponent, entity.id);
        AnimationComponent* ac =
            PoolGetComponent(animation_pool, AnimationComponent, entity.id);
        size_t animation_frame_nr = (frame_nr / ac->frames_per_animation_frame) % ac->num_animation_frames * ac->is_playing;

        float offset = 0;            
        float len = length_vec2f(&pc->velocity);
 
        if (len) {
            Vec2f normalized_velocity = normalize_with_len_vec2f(&pc->velocity, len);
            Vec2f x_axis = {
                .x = 1.f,
                .y = 0.f
            };
            Vec2f y_axis = {
                .x = 0.f,
                .y = 1.f
            };
            float dp_x = dot_vec2f(&normalized_velocity, &x_axis);
            float dp_y = dot_vec2f(&normalized_velocity, &y_axis);

            // Pick which contributes the most to our direction
            if (fabs(dp_x) > fabs(dp_y)) {
                if (dp_x > 0) {
                    // We're going right ish
                    offset = 1.f;
                } else {
                    // left ish
                    offset = 3.f;
                }                
            } else {
                if (dp_y > 0) {
                    // up
                    offset = 0.f;
                } else {
                    // down
                    offset = 2.f;
                }
            }

            ac->last_offset = offset;

        }
        
        rc->tex_coord_scale.x = 1.f / ac->num_frames_width;
        rc->tex_coord_scale.y = 1.f / ac->num_frames_height;

        rc->tex_coord_offset.x = 1.f / ac->num_frames_width * animation_frame_nr;
        rc->tex_coord_offset.y = ((float)ac->num_frames_height - 1.f - ac->last_offset) / (float)ac->num_frames_height;
    }

    AppendScopedTimer(animation_time);
    PrintScopedTimer(animation_time);
}

static void collision_update(Registry* reg, SystemBase* sys, size_t frame_nr) {
    BeginScopedTimer(collision_time);

    struct Pool* collision_pool = registry_get_pool(reg, COLLISION_COMPONENT_BIT);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);

    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for (int i = 0; i < sys->entities.size; ++i) {
        Entity self = entities[i];
        CollisionComponent* self_collision = PoolGetComponent(collision_pool, CollisionComponent, self.id);
        TransformComponent* self_transform = PoolGetComponent(transform_pool, TransformComponent, self.id);
            
        for (int j = i; i < sys->entities.size; ++j) {
            Entity other = entities[j];

            if(other.id == self.id) {
                continue;
            }
            
            TransformComponent* other_transform = PoolGetComponent(transform_pool, TransformComponent, other.id);
            CollisionComponent* other_collision = PoolGetComponent(collision_pool, CollisionComponent, other.id);

            if (intersect_rectf(&self_collision->bounding_rect, &other_collision->bounding_rect)) {
                LOG_INFO("YOU HAVE TAKEN THE LEAD");
            }
        }
    }

    AppendScopedTimer(collision_time);
    PrintScopedTimer(collision_time);
}

static void input_update(Registry* registry, SystemBase* sys, size_t frame_nr) {
    struct InputSystem* input_system = sys->system_impl;
    struct Pool* physics_pool = registry_get_pool(registry, PHYSICS_COMPONENT_BIT);
    if (input_system_is_key_pressed(input_system, GLFW_KEY_SPACE)) {
        Entity e = registry_create_entity(registry);

        TransformComponent tc = {0};
        tc.pos.x = 0.f;
        tc.pos.y = 0.f;
        tc.pos.z = 0.01f;
        tc.scale.x = 0.2f;
        tc.scale.y = 0.2f;
        tc.rotation = 0.f;
            
        registry_add_component(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

        PhysicsComponent pc = {0};
        pc.velocity.x = 0.1f;
        pc.velocity.y = 0.1f;

        registry_add_component(registry, e, PHYSICS_COMPONENT_BIT, &pc);

        RenderComponent rc;
        rc.render_layer = 0;
            
        rc.tex_coord_offset.x = 0.f;
        rc.tex_coord_offset.y = 0.f;
        rc.tex_coord_scale.x = 1.0f;
        rc.tex_coord_scale.y = 1.0f;
        rc.material_id = assets_make_id_str("bullet-mat");
        rc.pipeline_id = assets_make_id_str("tilemap");
            
        registry_add_component(registry, e, RENDER_COMPONENT_BIT, &rc);

        TimeComponent ttc = {0};
        ttc.created = time_now();
        uint64_t expires = time_from_secs(1);
        ttc.expires = ttc.created + expires;

        registry_add_component(registry, e, TIME_COMPONENT_BIT, &ttc);

        registry_add_entity(registry, e);

    }
    
    static float key_to_velocity_factor = 0.011f;
    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for(int i = 0; i < sys->entities.size; ++i) {
        Entity entity = entities[i];
        PhysicsComponent* pc = PoolGetComponent(physics_pool, PhysicsComponent, entity.id);

        if (input_system->key_state[GLFW_KEY_UP] == GLFW_PRESS) {
            pc->velocity.y += key_to_velocity_factor;
        }

        int lol = GLFW_PRESS;
        if (input_system->key_state[GLFW_KEY_DOWN] == GLFW_PRESS) {
            pc->velocity.y -= key_to_velocity_factor;
        }

        if(input_system->key_state[GLFW_KEY_LEFT] == GLFW_PRESS) {
            pc->velocity.x -= key_to_velocity_factor;
        }

        if(input_system->key_state[GLFW_KEY_RIGHT] == GLFW_PRESS) {
            pc->velocity.x += key_to_velocity_factor;
        }
    }

    input_system_reset(input_system);
}

void time_update(Registry* reg, SystemBase* sys, size_t frame_nr) {
    struct Pool* time_pool = registry_get_pool(reg, TIME_COMPONENT_BIT);

    static float key_to_velocity_factor = 0.011f;
    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for(int i = 0; i < sys->entities.size; ++i) {
        Entity e = entities[i];
        TimeComponent* tc = PoolGetComponent(time_pool, TimeComponent, e.id);
        
        if (time_expired(tc->created, tc->expires)) {
            registry_remove_entity(reg, e);
        }
    }
}

Game* game_create() {
    timers_init();
    
    if(!glfwInit()) {
        LOG_ERROR("GLFW init failed");
        return 0;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_CONTEXT_DEBUG, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "1,2,3 techno", 0, 0);

    if (!window) {
        LOG_ERROR("Window creation failed");
        return 0;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glfwSetKeyCallback(window, &key_callback);
    glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
    glfwSetWindowSizeCallback(window, &window_size_callback);
    
    const char* error_ptr = 0;
    if (glfwGetError(&error_ptr) != GLFW_NO_ERROR) {
        LOG_ERROR("GLFW error: %s");
        return 0;
    }

    Game* game = ArenaAlloc(&allocator, 1, Game);

    game->window = window;

    registry_init(&game->registry, MAX_ENTITIES, component_table, component_table_size());

    game->frame_counter = 0;

    glfwSetWindowUserPointer(window, game);

    return game;
}

static void map_init(Map* map) {
    // Hard code this for now, since we haven't defined a meta data
    // structure for maps yet
    map->map_size.x = 25;
    map->map_size.y = 20;
    map->atlas_size.x = 10;
    map->atlas_size.y = 3;
    map->tiles = ArenaAlloc(&allocator, map->map_size.x * map->map_size.y, MapTile);
}

static void map_load(Map* map, Registry* registry, Assets* assets) {
    const float fcols = (float)map->map_size.x;
    const float frows = (float)map->map_size.y;
    const float atlas_cols_f = (float)map->atlas_size.x;
    const float atlas_rows_f = (float)map->atlas_size.y;
    
    for (int row = 0; row < map->map_size.y; ++row) {
        for (int col = 0; col < map->map_size.x; ++col) {
            Entity e = registry_create_entity(registry);

            TransformComponent tc = {0};
            /* tc.pos.x = (float)(col - 24.f / 2.f); */
            /* tc.pos.y = (float)(row - 19.f / 2.f); */
            tc.pos.x = (float)col;
            tc.pos.y = (float)row;
            tc.pos.z = 0.0f;
            tc.scale.x = 0.5f;// * 9.f/16.f;
            tc.scale.y = 0.5f;// * 16.f/9.f;
            /* tc.scale.x_= 1.0f / 50.0f; */
            /* tc.scale.y = 1.0f / 40.0f; */
            tc.rotation = 0.f;
            
            registry_add_component(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

            MapTile tile = map_get_tile(map, col, row);
            
            RenderComponent rc;
            rc.render_layer = 0;
            
            // In map space, tile starts at the upper left
            // In GL texture coord space, tile starts at lower left
            // Offset.y must be the bottom of the texture rect.
            rc.tex_coord_offset.x = ((float)tile.atlas_coord.x) / atlas_cols_f; 
            rc.tex_coord_offset.y = (atlas_rows_f - 1.f - (float)tile.atlas_coord.y) / atlas_rows_f;
            rc.tex_coord_scale.x = 1.0f / (atlas_cols_f);
            rc.tex_coord_scale.y = 1.0f / (atlas_rows_f);
            rc.material_id = assets_make_id_str("jungle-mat");
            rc.pipeline_id = assets_make_id_str("tilemap");
            
            registry_add_component(registry, e, RENDER_COMPONENT_BIT, &rc);

            registry_add_entity(registry, e);
        }
    }
}

static void load_units(Registry* registry, Assets* assets) {
//    const char* unit_shader_name = "unit";
    const char* unit_shader_name = "tilemap";
    AssetId unit_shader_id = assets_make_id(unit_shader_name, strlen(unit_shader_name));

    {
        Entity truck = registry_create_entity(registry);

        TransformComponent tc = {0};
        tc.pos.x = 0.5;
        tc.pos.y = 0.5;
        tc.pos.z = 0.1f;
        tc.scale.x = 1.0f;
        tc.scale.y = 1.0f;
        tc.rotation = 0.f;

        RenderComponent rc;
        rc.render_layer = 0;
        rc.tex_coord_offset.x = 0.f;
        rc.tex_coord_offset.y = 0.f;
        rc.tex_coord_scale.x = 1.f;
        rc.tex_coord_scale.y = 1.f;
        rc.material_id = assets_make_id_str("truck-blue-mat");
        rc.pipeline_id = unit_shader_id;

        PhysicsComponent pc;
        pc.velocity.x = 0.01f;
        pc.velocity.y = -0.01f;

        InputComponent ic = {0};
        
        registry_add_component(registry, truck, RENDER_COMPONENT_BIT, &rc);
        registry_add_component(registry, truck, TRANSFORM_COMPONENT_BIT, &tc);
        registry_add_component(registry, truck, PHYSICS_COMPONENT_BIT, &pc);

        registry_add_entity(registry, truck);
    }

    {
        Entity chopper = registry_create_entity(registry);

        TransformComponent tc = {0};
        tc.pos.x = 0.2f;
        tc.pos.y = 0.2f;
        tc.pos.z = 0.1f;
        tc.scale.x = 1.f;
        tc.scale.y = 1.f;
        /* tc.scale.x = 1.0f / 51.0f * 2.f; */
        /* tc.scale.y = 1.0f / 41.f * 2.f; */
        tc.rotation = 0.f;

        RenderComponent rc;
        rc.render_layer = 0;
        rc.tex_coord_offset.x = 0.f;
        rc.tex_coord_offset.y = 0.f;
        rc.tex_coord_scale.x = 1.f;
        rc.tex_coord_scale.y = 1.f;
        rc.material_id = assets_make_id_str("chopper-udlr");
        rc.pipeline_id = unit_shader_id;

        PhysicsComponent pc;
        pc.velocity.x = 0.01f;
        pc.velocity.y = 0.00f;

        AnimationComponent ac = {0};
        ac.frames_per_animation_frame = 5;
        ac.num_animation_frames = 2;
        ac.num_frames_width = 2;
        ac.num_frames_height = 4;
        ac.is_playing = 1;
        ac.last_offset = 0.f;

        InputComponent ic = {0};
        
        registry_add_component(registry, chopper, RENDER_COMPONENT_BIT, &rc);
        registry_add_component(registry, chopper, TRANSFORM_COMPONENT_BIT, &tc);
        registry_add_component(registry, chopper, PHYSICS_COMPONENT_BIT, &pc);
        registry_add_component(registry, chopper, ANIMATION_COMPONENT_BIT, &ac);
        registry_add_component(registry, chopper, INPUT_COMPONENT_BIT, &ic);

        registry_add_entity(registry, chopper);

        /* Entity chopper2 = registry_create_entity(registry); */

        /* pc.velocity.x = -0.01f; */
        /* pc.velocity.y = 0.00f; */

        /* registry_add_component(registry, chopper2, RENDER_COMPONENT_BIT, &rc); */
        /* registry_add_component(registry, chopper2, TRANSFORM_COMPONENT_BIT, &tc); */
        /* registry_add_component(registry, chopper2, PHYSICS_COMPONENT_BIT, &pc); */
        /* registry_add_component(registry, chopper2, ANIMATION_COMPONENT_BIT, &ac); */

        /* registry_add_entity(registry, chopper2); */


        /* Entity chopper3 = registry_create_entity(registry); */

        /* pc.velocity.x = 0.00f; */
        /* pc.velocity.y = 0.01f; */

        /* registry_add_component(registry, chopper3, RENDER_COMPONENT_BIT, &rc); */
        /* registry_add_component(registry, chopper3, TRANSFORM_COMPONENT_BIT, &tc); */
        /* registry_add_component(registry, chopper3, PHYSICS_COMPONENT_BIT, &pc); */
        /* registry_add_component(registry, chopper3, ANIMATION_COMPONENT_BIT, &ac); */

        /* registry_add_entity(registry, chopper3); */


        /* Entity chopper4 = registry_create_entity(registry); */

        /* pc.velocity.x = 0.00f; */
        /* pc.velocity.y = -0.01f; */

        /* registry_add_component(registry, chopper4, RENDER_COMPONENT_BIT, &rc); */
        /* registry_add_component(registry, chopper4, TRANSFORM_COMPONENT_BIT, &tc); */
        /* registry_add_component(registry, chopper4, PHYSICS_COMPONENT_BIT, &pc); */
        /* registry_add_component(registry, chopper4, ANIMATION_COMPONENT_BIT, &ac); */

        /* registry_add_entity(registry, chopper4); */

    }

}

void game_setup(Game* game) {
    LOG_INFO("Setup");

    assets_init(&game->assets);

    /*     MOVEMENT_SYSTEM_BIT = (1U << 0), */
    /* RENDER_SYSTEM_BIT = (1U << 1), */
    /* COLLISION_SYSTEM_BIT = (1U << 2), */
    /* ANIMATION_SYSTEM_BIT = (1U << 3), */

    /* SystemBase* physics_system = system_create( */
        
    /*     &physics_update, */
    /*     PHYSICS_COMPONENT_BIT); */

    SystemBase* input_system = system_create(
        INPUT_SYSTEM_BIT,
        &input_update,
        INPUT_COMPONENT_BIT | PHYSICS_COMPONENT_BIT);

    SystemBase* movement_system = system_create(
        MOVEMENT_SYSTEM_BIT,
        &movement_update,
        TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT);
    
    SystemBase* collision_system = system_create(
        COLLISION_SYSTEM_BIT,
        &collision_update,
        COLLISION_COMPONENT_BIT);

    SystemBase* animation_system = system_create(
        ANIMATION_SYSTEM_BIT,
        &animation_update,
        ANIMATION_COMPONENT_BIT);

    SystemBase* time_system = system_create(
        TIME_SYSTEM_BIT,
        &time_update,
        TIME_COMPONENT_BIT
        );

    SystemBase* render_system = system_create(
        RENDER_SYSTEM_BIT,
        &render_update,
        RENDER_COMPONENT_BIT);
 

    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    LOG_INFO("Primary monitor mode: width=%d, height=%d,redbits=%d, greenbits=%d, bluebits=%d,refresh rate=%d",
             mode->width, mode->height, mode->redBits, mode->greenBits, mode->blueBits, mode->refreshRate);

    RenderSystem* render_system_impl = render_system_create(&game->assets, mode->width, mode->height);
    render_system->system_impl = render_system_impl;
    animation_system->system_impl = animation_system_create(&game->assets);
    input_system->system_impl = input_system_create();
    
    Registry* registry = &game->registry;
    
    registry_add_system(registry, movement_system);
    /* registry_add_system(registry, physics_system); */
    registry_add_system(registry, animation_system);
    registry_add_system(registry, collision_system);
    registry_add_system(registry, input_system);
    registry_add_system(registry, time_system);
    registry_add_system(registry, render_system);
 

    map_init(&game->map);
    load_tile_map_layout("./assets/tilemaps/jungle.map", &game->map);
    map_load(&game->map, registry, &game->assets);
    map_print(&game->map);
    
    load_units(registry, &game->assets);
    
    PreparedResources prep;
    prep.program_ids = vec_create();
    prep.material_ids = vec_create();
    VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("tilemap"));
    VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("unit"));

    VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("truck-blue-mat"));
    VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("jungle-mat"));
    VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("chopper-udlr"));
    VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("bullet-mat"));

    render_system_prepare_resources(render_system_impl, &prep);

    // Push our initial entities before entering main loop
    registry_commit_entities(registry);
}

void game_update(Game* game) {
    registry_update(&game->registry, game->frame_counter);
}

void game_run(Game* game) {
    LOG_INFO("Main loop");
    game->main_loop_running = 1;

    LARGE_INTEGER fps;
    QueryPerformanceCounter(&fps);

    while (game->main_loop_running) {
        if (game->frame_counter % 60 == 0) {
            LARGE_INTEGER fps2;
            QueryPerformanceCounter(&fps2);
            LARGE_INTEGER elapsed;
            elapsed.QuadPart = fps2.QuadPart - fps.QuadPart;
            elapsed.QuadPart *= 1000000;
            elapsed.QuadPart /= performance_counter_frequency.QuadPart;
            LOG_INFO("Time 60 frames %ld", elapsed.QuadPart);

            QueryPerformanceCounter(&fps);
        }
        
        BeginScopedTimer(frame_time);

        LOG_INFO("###### FRAME %zu #####", game->frame_counter);
        glfwPollEvents();

        BeginScopedTimer(update_time);
        game_update(game);
        AppendScopedTimer(update_time);
        PrintScopedTimer(update_time);

        BeginScopedTimer(swap_buffers_time);
        glfwSwapBuffers(game->window);
        AppendScopedTimer(swap_buffers_time);
        PrintScopedTimer(swap_buffers_time);
        
        game->frame_counter++;
        
        AppendScopedTimer(frame_time);
        PrintScopedTimer(frame_time);
    }
}

void game_destroy(Game* game) {
    if(game->window) {
        glfwDestroyWindow(game->window);
    }
    glfwTerminate();
}

void game_frame_buffer_size_changed(Game* game, int width, int height) {
    LOG_INFO("Framebuffer size changed: (%d, %d)", width, height);
    SystemBase* system = registry_get_system(&game->registry, RENDER_SYSTEM_BIT);
    if (!system) {
        LOG_ERROR("Failed to resize frame buffer: System not found");
    }

    RenderSystem* render_system = system->system_impl;
    render_system_frame_buffer_size_changed(render_system, width, height);
}

void game_window_size_changed(Game* game, int width, int height) {
    LOG_INFO("Window size changed: (%d, %d)", width, height);    
}
