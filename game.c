#include "game.h"

#include "arena.h"
#include "assetstore.h"
#include "component.h"
#include "log.h"
#include "math.h"
#include "render_system.h"
#include "animation_system.h"
#include "os.h"
#include "ecs.h"
#include "util.h"

// Exclude rarely-used stuff from Windows headers
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stdio.h>


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
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        LOG_INFO("ESC pressed");
        Game* game = glfwGetWindowUserPointer(window);
        game->main_loop_running = 0;
    }
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

static void movement_update(Registry* reg, System* system, size_t frame_nr) {
    BeginScopedTimer(movement_time);

    Entity* entities = VEC_ITER_BEGIN_T(&system->entities, Entity);
    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
    struct Pool* physics_pool = registry_get_pool(reg, PHYSICS_COMPONENT_BIT);
    
    for (int i = 0; i < system->entities.size; ++i) {
        const int entity_index = entities[i].id;
        TransformComponent* tc = RegistryGetComponent(transform_pool, TransformComponent, entity_index);
        PhysicsComponent* pc = RegistryGetComponent(physics_pool, PhysicsComponent, entity_index);
        tc->pos.x += pc->velocity.x;
        tc->pos.y += pc->velocity.y;
    }
    
    AppendScopedTimer(movement_time);
    PrintScopedTimer(movement_time);
}

static void physics_update(Registry* reg, System* s, size_t frame_nr) {
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

static void render_update(Registry* reg, System* s, size_t frame_nr) {
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
        TransformComponent* tc = RegistryGetComponent(transform_pool, TransformComponent, entity.id);
        RenderComponent* rc = RegistryGetComponent(render_pool, RenderComponent, entity.id);

        RenderData* rd = VEC_GET_T_PTR(render_data, RenderData, entity.id);

        rd->render_layer = rc->render_layer;
        
        Vec3f scale;
        scale.x = tc->scale.x;
        scale.y = tc->scale.y;
        scale.z = 1.0f;
        
        Vec3f pos;
        pos.x = tc->pos.x / 25.f;
        pos.y = tc->pos.y / 20.f;
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

static void animation_update(Registry* reg, System* sys, size_t frame_nr) {
    BeginScopedTimer(animation_time);

    struct AnimationSystem* animation_system = sys->system_impl;
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);
    struct Pool* animation_pool = registry_get_pool(reg, ANIMATION_COMPONENT_BIT);
    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);
    
    for (int i = 0; i < sys->entities.size; ++i) {
        Entity entity = entities[i];
        RenderComponent* rc = RegistryGetComponent(render_pool, RenderComponent, entity.id);
        AnimationComponent* ac =
            RegistryGetComponent(animation_pool, AnimationComponent, entity.id);
        size_t animation_frame_nr = (frame_nr / ac->frames_per_animation_frame) % ac->num_animation_frames;
    }

    AppendScopedTimer(animation_time);
    PrintScopedTimer(animation_time);
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
    registry_init(&game->registry, MAX_ENTITIES, component_table, component_table_size);

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
            tc.pos.x = (float)(col - 25.f / 2.f);
            tc.pos.y = (float)(row - 20.f / 2.f);
            tc.pos.z = 0.0f;
            tc.scale.x = 1.0f / 51.0f;
            tc.scale.y = 1.0f / 41.0f;
            tc.rotation = 0.f;
            
            registry_add_component(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

            MapTile tile = map_get_tile(map, col, row);
            
            RenderComponent rc;
//            rc.flags = 0;
            rc.render_layer = 0;
            
            // In map space, tile starts at the upper left
            // In GL texture coord space, tile starts at lower left
            // Offset.y must be the bottom of the texture rect.
            rc.tex_coord_offset.x =  (float)tile.atlas_coord.x / atlas_cols_f; 
            rc.tex_coord_offset.y = (atlas_rows_f - 1.0f - (float)tile.atlas_coord.y) / atlas_rows_f;
            rc.tex_coord_scale.x = 1.0f / atlas_cols_f;
            rc.tex_coord_scale.y = 1.0f / atlas_rows_f;
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
        tc.scale.x = 1.0f / 51.0f;
        tc.scale.y = 1.0f / 41.f;
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

        registry_add_component(registry, truck, RENDER_COMPONENT_BIT, &rc);
        registry_add_component(registry, truck, TRANSFORM_COMPONENT_BIT, &tc);
        registry_add_component(registry, truck, PHYSICS_COMPONENT_BIT, &pc);

        registry_add_entity(registry, truck);
    }
}

void game_setup(Game* game) {
    LOG_INFO("Setup");

    assets_init(&game->assets);

    System* movement_system = system_create(&movement_update, TRANSFORM_COMPONENT_BIT | PHYSICS_COMPONENT_BIT);
    System* physics_system = system_create(&physics_update, PHYSICS_COMPONENT_BIT);
    System* render_system = system_create(&render_update, RENDER_COMPONENT_BIT);
    System* animation_system = system_create(&animation_update, ANIMATION_COMPONENT_BIT);
    
    RenderSystem* render_system_impl = render_system_create(&game->assets);
    render_system->system_impl = render_system_impl;
    
    struct AnimationSystem* animation_system_impl = animation_system_create(&game->assets);

    Registry* registry = &game->registry;
    
    registry_add_system(registry, movement_system);
    registry_add_system(registry, physics_system);
    registry_add_system(registry, render_system);
    registry_add_system(registry, animation_system);

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

    render_system_prepare_resources(render_system_impl, &prep);

    // Push our initial entities before entering main loop
    registry_commit_entities(registry);
}

void game_update(Game* game) {
    registry_update(&game->registry, game->frame_counter);
    /* RenderSystem* render_system = registry_get_system(&game->registry)->system_impl; */

    /* render_system_render(render_system); */
    

/*     #error "Maybe: run render_system_update after registry is done" */
/*     #error "is this enough to guarantee all systems that could?" */
/*     #error "affect what we render have ran before we render?" */
/*     #error "Is it better to create ordering of system update funcs?" */
/* //    registry_get_systme(&game->registry) */
/* //    render_system_update(render_sys); */

}

void game_run(Game* game) {
    LOG_INFO("Main loop");
    game->main_loop_running = 1;

    while (game->main_loop_running) {
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
    glViewport(0, 0, width, height);
}

void game_window_size_changed(Game* game, int width, int height) {
    LOG_INFO("Window size changed: (%d, %d)", width, height);    
}
