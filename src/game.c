#include "game.h"

#include "components.h"
#include "events.h"

#include "entity_flags.h"
#include "systems.h"

#include "core/allocators.h"
#include "core/assetstore.h"
#include "core/ecs.h"
#include "core/log.h"
#include "core/math.h"
#include "core/os.h"
#include "core/systembase.h"
#include "core/worker.h"

#include <GLFW/glfw3.h>
// #include <glad/glad.h>
#include <stdio.h>

#define MAX_ENTITIES 100000

// Try to define all the memory we will ever need up front.
//
// NOTE: There's quite a few dynamic arrays and hash maps
//       left to remove.
#define STATIC_ARENA_SIZE 1024ULL * 1024ULL * 256ULL
#define FRAME_ARENA_SIZE 1024 * 1024 * 16

/* extern const struct Component component_table[]; */
/* extern const size_t component_table_len; */

typedef struct {
  Vec2u8 atlas_coord;
} MapTile;

struct Map_t {
  Vec2u32 world_size;
  Vec2u8 map_size;
  Vec2u8 atlas_size;
  MapTile *tiles;
};
typedef struct Map_t Map;

enum GameState { GAME_RUNNING = 0x1, GAME_DEBUG_ENABLED = 0x2 };

struct Game_t {
  Registry registry;
  GLFWwindow *window;
  struct Assets assets;
  struct EventBus event_bus;
  struct WorkQueue work_queue;

  int state;
  size_t frame_counter;
  Services services;

  Map map;

  // Systems live here, but we register them in the
  // registry to wire them up to entity related things
  struct InputSystem *input_system;
  struct PlayerSystem *player_system;
  struct MovementSystem *movement_system;
  struct CollisionSystem *collision_system;
  struct AnimationSystem *animation_system;
  struct TimeSystem *time_system;
  HitDetectionSystem *hit_detection_system;
  CameraMovementSystem *camera_movement_system;
  ProjectileEmitterSystem *projectile_emitter_system;
  struct RenderSystem *render_system;
  DamageSystem *damage_system;
};

/* VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("tilemap")); */
/* VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("unit")); */
/* VEC_PUSH_T(&prep.program_ids, AssetId,
 * assets_make_id_str("collision_debug"));

VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("truck-blue-mat"));
VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("jungle-mat"));
VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("chopper-udlr"));
VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("bullet-mat"));
VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("plain-red"));

*/

const char *level_0_programs[] = {"tilemap.prog", "unit.prog",
                                  "collision_debug.prog"};

const char *level_0_materials[] = {"truck.mat", "jungle.mat", "chopper.mat",
                                   "plain-red.mat", "bullet.mat"};

const char *level_0_textures[] = {"bullet.png", "chopper-spritesheet.png",
                                  "jungle.png", "white-pixel.png",
                                  "truck-ford-right.png"};

const char *level_0_maps[] = {"jungle.meta"};

typedef struct LevelAssets {
  const char **textures;
  size_t textures_count;

  const char **materials;
  size_t materials_count;

  const char **programs;
  size_t programs_count;

  const char **maps;
  size_t maps_count;
} LevelAssets;

typedef enum ReadlineResult {
  READLINE_DONE = 0,
  READLINE_CONTINUE = 1,
  READLINE_BUFFER_TOO_SMALL = -2,
  READLINE_READ_ERROR = -3
} ReadlineResult;
// typedef enum ReadlineResult_t ReadlineResult;

static enum ReadlineResult readline(FILE *fp, char *buffer, int buf_size) {
  long int curr_pos = ftell(fp);
  int len = 0;
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

MapTile map_get_tile(Map *map, int col, int row) {
  return map->tiles[row * 25 + col];
}

void map_set_tile(Map *map, int col, int row, int x, int y) {
  MapTile *tile = &map->tiles[row * 25 + col];
  tile->atlas_coord.x = (uint8_t)x;
  tile->atlas_coord.y = (uint8_t)y;
}

void map_print(Map *map) {
  for (int row = 0; row < 20; ++row) {
    for (int col = 0; col < 25; ++col) {
      MapTile tile = map_get_tile(map, col, row);
      printf("%d%d,", tile.atlas_coord.x, tile.atlas_coord.y);
    }
    printf("\n");
  }
}

int load_tile_map_layout(const char *file, Map *map) {

  FILE *fp = fopen(file, "rb");
  if (!fp) {
    LOG_ERROR("Failed to open file '%s'", file);
    return 0;
  }

  char line_buf[1024];
  enum ReadlineResult read_result;
  int map_index_row = 0;
  int map_index_col = 0;

  do {
    int tile_row = 0;
    int tile_col = 0;
    memset(line_buf, 0x0, 1024);
    read_result = readline(fp, line_buf, 1024);

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
        //                LOG_INFO("map [%d, %d] = [%d, %d]", map_index_col,
        //                map_index_row, tile_col, tile_row);
        map_set_tile(map, map_index_col, 19 - map_index_row, tile_col,
                     tile_row);
        map_index_col++;
        offset += 3; // skip comma
        ++tile_col;
      }
      map_index_row++;
      if (read_result == READLINE_DONE) {
        return 1;
      }
    } break;
    default:
      return 0;
    }
  } while (read_result != READLINE_DONE);

  return 1;
}

static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  LOG_INFO("key %d scancode %d action %d mods %d", key, scancode, action, mods);
  (void)scancode;
  (void)mods;
  Game *game = glfwGetWindowUserPointer(window);

  struct InputSystem *input_system = (struct InputSystem *)registry_get_system(
      &game->registry, INPUT_SYSTEM_BIT);

  switch (key) {
  case GLFW_KEY_ESCAPE:
    game->state &= ~GAME_RUNNING;
    return;
  case GLFW_KEY_F4: {
    if (action == GLFW_RELEASE) {
      if (game->state & GAME_DEBUG_ENABLED) {
        game->state &= ~GAME_DEBUG_ENABLED;
      } else {
        game->state |= GAME_DEBUG_ENABLED;
      }
    }
  }
  default:
    break;
  }

  input_system_handle_keyboard_input(input_system, key, action);
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  if (!window) {
    LOG_ERROR(
        "Nullptr window in frame buffer size callback. Width %d, height %d",
        width, height);
    return;
  }

  Game *game = glfwGetWindowUserPointer(window);

  if (width <= 0 || height <= 0 || width >= 0xffff || height > 0xffff) {
    LOG_ERROR("Invalid framebuffer size %d %d", width, height);
    return;
  }

  OSFramebufferSizeChanged event_data = {
      .size = {(uint16_t)width, (uint16_t)height}};

  Event e = {.id = OS_FramebufferSizeChanged,
             .event_data = &event_data,
             .event_data_size = sizeof(OSFramebufferSizeChanged)};

  LOG_INFO("Framebuffer resize event: width %d height %d", event_data.size.x,
           event_data.size.y);
  event_bus_defer(&game->event_bus, &e);
}

static void window_size_callback(GLFWwindow *window, int width, int height) {
  if (!window) {
    LOG_ERROR("Nullptr window in window size callback. Width %d, height %d",
              width, height);
    return;
  }

  Game *game = glfwGetWindowUserPointer(window);
  game_window_size_changed(game, width, height);
}

Game *game_create() {
  LOG_INFO("max_align_t == %d", alignof(max_align_t));

  arena_init(&global_static_allocator, STATIC_ARENA_SIZE);
  arena_init(&frame_allocator, FRAME_ARENA_SIZE);

  stack_init(&stack_allocator, &global_static_allocator,
             STACK_ALLOCATOR_DEFAULT_THREAD_LOCAL_STACK_SIZE);

  Game *game = ArenaAlloc(&global_static_allocator, 1, Game);

  work_queue_init(&game->work_queue);

  game->state = 0;
  game->frame_counter = 0;

  if (!glfwInit()) {
    LOG_ERROR("GLFW init failed");
    return 0;
  }

  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(monitor);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
  glfwWindowHint(GLFW_RED_BITS, mode->redBits);
  glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
  glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
  glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

  GLFWwindow *window = glfwCreateWindow(800, 800, "1,2,3 techno", 0, 0);

  // This creates borderless fullscreen but completely screws up remedybg
  /* GLFWwindow *window =
  glfwCreateWindow(mode->width, mode->height, "1,2,3 techno", monitor, 0); */

  if (!window) {
    const char *error = 0;
    glfwGetError(&error);
    LOG_ERROR("Window creation failed: %s", error);
    return 0;
  }

  glfwMakeContextCurrent(window);

  render_system_global_init();

  glfwSwapInterval(1);
  glfwSetKeyCallback(window, &key_callback);
  glfwSetFramebufferSizeCallback(window, &framebuffer_size_callback);
  glfwSetWindowSizeCallback(window, &window_size_callback);

  const char *error_ptr = 0;
  if (glfwGetError(&error_ptr) != GLFW_NO_ERROR) {
    LOG_ERROR("GLFW error: %s");
    return 0;
  }

  glfwSetWindowUserPointer(window, game);
  game->window = window;

  registry_init(&game->registry, MAX_ENTITIES, component_table,
                sizeof(component_table) / sizeof(component_table[0]));
  assets_init(&game->assets);
  event_bus_init(&game->event_bus);
  time_init();

  game->services.assets = &game->assets;
  game->services.registry = &game->registry;
  game->services.event_bus = &game->event_bus;
  game->services.work_queue = &game->work_queue;

  LOG_INFO("Primary monitor mode: width=%d, height=%d,redbits=%d, "
           "greenbits=%d, bluebits=%d,refresh rate=%d",
           mode->width, mode->height, mode->redBits, mode->greenBits,
           mode->blueBits, mode->refreshRate);

  int window_width, window_height;
  glfwGetWindowSize(game->window, &window_width, &window_height);

  game->input_system = input_system_create(&game->services);
  game->hit_detection_system = hit_detection_system_create(&game->services);
  game->player_system = player_system_create(&game->services);
  game->movement_system = movement_system_create(&game->services);
  game->collision_system = collision_system_create(&game->services);
  game->animation_system = animation_system_create(&game->services);
  game->time_system = time_system_create(&game->services);

  // TODO: What's the point of this camera_area?
  //       Doesn't adjust ortho projection currently, which it has to?
  Vec2f camera_area = {8.f, 8.f};
  game->camera_movement_system =
      camera_movement_system_create(&game->services, &camera_area);
  game->projectile_emitter_system =
      projectile_emitter_system_create(&game->services);
  game->render_system = render_system_create(
      &game->services, window_width, window_height, mode->width, mode->height);
  game->damage_system = damage_system_create(&game->services);

  registry_add_system(&game->registry, (struct SystemBase *)game->input_system);
  registry_add_system(&game->registry, (struct SystemBase *)game->time_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->player_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->movement_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->collision_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->animation_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->projectile_emitter_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->damage_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->camera_movement_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->hit_detection_system);
  registry_add_system(&game->registry,
                      (struct SystemBase *)game->render_system);

  return game;
}

void map_write_meta(Map *map, const char *filepath) {
  FILE *handle = fopen(filepath, "w+b");
  int bytes_to_write = offsetof(Map, tiles);

  int res = fwrite(map, 1, bytes_to_write, handle);
  if (res != bytes_to_write) {
    LOG_EXIT("Failed to write map meta");
  }
  fclose(handle);
}

void map_read_meta(Map *map, const char *filepath) {
  FILE *handle = fopen(filepath, "r+b");
  int bytes_to_read = offsetof(Map, tiles);
  fread(map, 1, bytes_to_read, handle);
  fclose(handle);
}

void map_init(Map *map) {
  // Hard code this for now, since we haven't defined a meta data
  // structure for maps yet
  map->world_size.x = 25;
  map->world_size.y = 20;
  map->atlas_size.x = 10;
  map->atlas_size.y = 3;
  map->tiles = 0;
}

// void map_load(Map *map, Registry *registry, struct Assets *assets) {
void map_load(Game *game, AssetId asset_id) {
  Assets *assets = &game->assets;
  Registry *registry = &game->registry;

  AssetMap map_asset = {0};
  if (!assets_load_map(assets, asset_id, &map_asset)) {
    LOG_EXIT("Failed to load map asset");
  }

  game->map.world_size.x = map_asset.size_world.x;
  game->map.world_size.y = map_asset.size_world.y;

  /* const float atlas_cols_f = (float)map->atlas_size.x; */
  /* const float atlas_rows_f = (float)map->atlas_size.y; */

  for (uint32_t row = 0; row < map_asset.size_world.y; ++row) {
    for (uint32_t col = 0; col < map_asset.size_world.x; ++col) {
      Entity e = registry_entity_create(registry);

      TransformComponent tc = {0};
      tc.scale = 0.95f;
      tc.pos.x = col;
      tc.pos.y = row;
      tc.pos.z = 0.0f;
      tc.rotation = 0.0f;

      registry_entity_component_add(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

      /* MapTile tile = */
      /*     map_get_tile(map, col % map->map_size.x, row % map->map_size.y); */

      RenderComponent rc;

      // In map space, tile starts at the upper left
      // In GL texture coord space, tile starts at lower left
      // Offset.y must be the bottom of the texture rect.
      rc.texture_atlas_size.x = map_asset.size_tilemap.x;
      rc.texture_atlas_size.y = map_asset.size_tilemap.y;

      size_t tile_index_index =
          (map_asset.size_world.y - 1 - row) * map_asset.size_world.x + col;
      Vec2u8 tile_index = map_asset.indices[tile_index_index];
      rc.texture_atlas_index = tile_index;
      /* rc.tex_coord_offset.x = ((float)tile.atlas_coord.x) / atlas_cols_f;
       */
      /* rc.tex_coord_offset.y = */
      /*     (atlas_rows_f - 1.f - (float)tile.atlas_coord.y) /
  atlas_rows_f; */
      /* rc.tex_coord_scale.x = 1.f / (atlas_cols_f); */
      /* rc.tex_coord_scale.y = 1.f / (atlas_rows_f); */
      rc.material_id = assets_make_id_str("jungle.mat");
      rc.pipeline_id = assets_make_id_str("tilemap.prog");
      rc.render_layer = RENDER_COMPONENT_RENDER_LAYER_TERRAIN;

      registry_entity_component_add(registry, e, RENDER_COMPONENT_BIT, &rc);

      registry_entity_add(registry, e);
    }
  }
}

/* void map_load(Map *map, Registry *registry, struct Assets *assets) { */
/*   (void)registry; */
/*   (void)assets; */
/*   const float atlas_cols_f = (float)map->atlas_size.x; */
/*   const float atlas_rows_f = (float)map->atlas_size.y; */

/*   Entity e = registry_entity_create(registry); */

/*   const uint32_t total_number_of_vertices = */
/*       map->world_size.x * map->world_size.y * 6; */

/*   Vec2f tex_coord_scale = {.x = 1.f / atlas_cols_f, .y = 1.f / atlas_rows_f};
 */

/*   MeshComponent mc = {0}; */
/*   mc.mesh.vertices = (Vertex3f *)ArenaAlloc(&global_static_allocator, */
/*                                             total_number_of_vertices,
 * Vertex3f); */
/*   mc.mesh.num_vertices = total_number_of_vertices; */
/*   mc.mesh.size_vertices = total_number_of_vertices; */

/*   // Skip edges for now */
/*   const uint32_t total_number_of_triangles = */
/*       map->world_size.x * map->world_size.y * 2; */
/*   mc.mesh.triangles = (Triangle *)ArenaAlloc( */
/*       &global_static_allocator, total_number_of_triangles, Triangle); */
/*   mc.mesh.num_triangles = total_number_of_triangles; */
/*   mc.mesh.size_triangles = total_number_of_triangles; */

/*   mc.mesh.uv_coords = (Vec2f *)ArenaAlloc(&global_static_allocator, */
/*                                           total_number_of_vertices, Vec2f);
 */
/*   mc.mesh.num_uv_coords = total_number_of_vertices; */
/*   mc.mesh.size_uv_coords = total_number_of_vertices; */

/*   uint32_t vertices_counter = 0; */
/*   uint32_t triangle_counter = 0; */

/*   /\* */
/*     The UV coord problem: */

/*     Problem statement: */
/*     UV coordinates as vertex attributes cannot be shared in instanced */
/*     rendering.. Unless... No. It can't. Why? Because the same vertex */
/*     will have more than one UV coordinate when reused. */

/*     Conclusions: - Indexed rendering will not work. */
/*                  - glMultiDrawELEMENTSIndirect will not work. */
/*                    Use glMultiDrawArraysIndirect instead. */
/*                  - Tile renderer is special because it is not continous */
/*                    in UV space? */
/*                  - Cannot iterate across a single row first to generate */
/*                    vertices. Must do two rows at a time to form triangles? */
/*                  - Let renderer test for null pointer in indices to know
 * which */
/*                    gl draw call to use. */
/*    *\/ */

/*   Vertex3f quad[4] = {0}; */

/*   // These are constant for each triangle */
/*   Vec2f uv[4] = {{0.f, 0.f}, {1.f, 0.f}, {0.f, 1.f}, {1.f, 1.f}}; */

/*   for (uint32_t row = 0; row < map->world_size.y; ++row) { */
/*     for (uint32_t col = 0; col < map->world_size.x; ++col) { */

/*       MapTile tile = */
/*           map_get_tile(map, col % map->map_size.x, row % map->map_size.y); */
/*       quad[0].x = (float)col; */
/*       quad[0].y = (float)row; */

/*       quad[1].x = (float)(col + 1); */
/*       quad[1].y = (float)row; */

/*       quad[2].x = (float)col; */
/*       quad[2].y = (float)(row + 1); */

/*       quad[3].x = (float)(col + 1); */
/*       quad[3].y = (float)(row + 1); */

/*       /\* */
/*        |\ */
/*        | \ */
/*        --- */
/*        *\/ */
/*       mc.mesh.vertices[vertices_counter + 0] = quad[0]; */
/*       mc.mesh.vertices[vertices_counter + 1] = quad[1]; */
/*       mc.mesh.vertices[vertices_counter + 2] = quad[2]; */

/*       /\* */
/*        \--- */
/*         \ | */
/*          \| */
/*       *\/ */
/*       mc.mesh.vertices[vertices_counter + 3] = quad[2]; */
/*       mc.mesh.vertices[vertices_counter + 4] = quad[3]; */
/*       mc.mesh.vertices[vertices_counter + 5] = quad[1]; */

/*       // In map space, tile starts at the upper left */
/*       // In GL texture coord space, tile starts at lower left */
/*       // Offset.y must be the bottom of the texture rect. */
/*       Vec2f tex_coord_offset; */

/*       tex_coord_offset.x = ((float)tile.atlas_coord.x) / atlas_cols_f; */
/*       tex_coord_offset.y = */
/*           (atlas_rows_f - 1.f - (float)tile.atlas_coord.y) / atlas_rows_f; */

/*       mc.mesh.uv_coords[vertices_counter + 0] = */
/*           vec2f_add(vec2f_mul(uv[0], tex_coord_scale), tex_coord_offset); */

/*       mc.mesh.uv_coords[vertices_counter + 1] = */
/*           vec2f_add(vec2f_mul(uv[1], tex_coord_scale), tex_coord_offset); */

/*       mc.mesh.uv_coords[vertices_counter + 2] = */
/*           vec2f_add(vec2f_mul(uv[2], tex_coord_scale), tex_coord_offset); */

/*       mc.mesh.uv_coords[vertices_counter + 2] = */
/*           vec2f_add(vec2f_mul(uv[2], tex_coord_scale), tex_coord_offset); */

/*       mc.mesh.uv_coords[vertices_counter + 3] = */
/*           vec2f_add(vec2f_mul(uv[3], tex_coord_scale), tex_coord_offset); */

/*       mc.mesh.uv_coords[vertices_counter + 1] = */
/*           vec2f_add(vec2f_mul(uv[2], tex_coord_scale), tex_coord_offset); */

/*       Triangle *ta = &mc.mesh.triangles[triangle_counter++]; */
/*       ta->index_v0 = vertices_counter + 0; */
/*       ta->index_v1 = vertices_counter + 1; */
/*       ta->index_v2 = vertices_counter + 2; */

/*       Triangle *tb = &mc.mesh.triangles[triangle_counter++]; */
/*       tb->index_v0 = vertices_counter + 3; */
/*       tb->index_v1 = vertices_counter + 4; */
/*       tb->index_v2 = vertices_counter + 5; */

/*       vertices_counter += 6; */
/*     } */
/*   } */

/*   registry_entity_component_add(registry, e, MESH_COMPONENT_BIT, &mc); */

/*   RenderComponent rc; */
/*   rc.render_layer = 0; */
/*   rc.material_id = assets_make_id_str("jungle-mat"); */
/*   rc.pipeline_id = assets_make_id_str("tilemap"); */

/*   registry_entity_component_add(registry, e, RENDER_COMPONENT_BIT, &rc); */

/*   registry_entity_add(registry, e); */
/* } */

void load_units(Registry *registry, struct Assets *assets) {
  (void)assets;

  // const char *unit_shader_name = "unit.prog";
  const char *unit_shader_name = "tilemap.prog";
  AssetId unit_pipeline_id =
      assets_make_id(unit_shader_name, strlen(unit_shader_name));
  {
    Entity truck = registry_entity_create(registry);
    registry_entity_group(registry, truck, "enemies");

    TransformComponent tc = {0};
    tc.pos.x = 0.5;
    tc.pos.y = 4.0;
    tc.pos.z = 1.0f;
    tc.scale = 1.0f;
    tc.rotation = 0.f;

    RenderComponent rc;
    rc.material_id = assets_make_id_str("truck.mat");
    rc.pipeline_id = unit_pipeline_id;
    rc.texture_atlas_index.x = 0;
    rc.texture_atlas_index.y = 0;
    rc.texture_atlas_size.x = 1;
    rc.texture_atlas_size.y = 1;
    rc.render_layer = RENDER_COMPONENT_RENDER_LAYER_GROUND;

    PhysicsComponent pc;
    pc.velocity.x = 0.0001f;
    pc.velocity.y = 0.f;

    CollisionComponent cc;
    cc.aabr.pos.x = 0.f;
    cc.aabr.pos.y = 0.f;
    cc.aabr.width = 1.f;
    cc.aabr.height = 1.f;

    ProjectileEmitterComponent pec;
    pec.emission_frequency = time_from_secs(1);
    pec.last_emitted = time_now();
    pec.damage = 50;
    pec.projectile_duration = time_from_secs(3);

    HealthComponent hc;
    hc.health = 100;

    registry_entity_component_add(registry, truck, RENDER_COMPONENT_BIT, &rc);
    registry_entity_component_add(registry, truck, TRANSFORM_COMPONENT_BIT,
                                  &tc);
    registry_entity_component_add(registry, truck, PHYSICS_COMPONENT_BIT, &pc);
    registry_entity_component_add(registry, truck, COLLISION_COMPONENT_BIT,
                                  &cc);
    /* registry_entity_component_add(registry, truck, */
    /*                               PROJECTILE_EMITTER_COMPONENT_BIT, &pec); */
    registry_entity_component_add(registry, truck, HEALTH_COMPONENT_BIT, &hc);

    registry_entity_add(registry, truck);
    registry_entity_set_flags(registry, truck, ENTITY_HOSTILE);

    // Add a second truck for collision tests
    truck = registry_entity_create(registry);
    registry_entity_group(registry, truck, "enemies");

    tc.pos.x = 10.f;
    pc.velocity.x = -0.01f;
    registry_entity_component_add(registry, truck, RENDER_COMPONENT_BIT, &rc);
    registry_entity_component_add(registry, truck, TRANSFORM_COMPONENT_BIT,
                                  &tc);
    registry_entity_component_add(registry, truck, PHYSICS_COMPONENT_BIT, &pc);
    registry_entity_component_add(registry, truck, COLLISION_COMPONENT_BIT,
                                  &cc);
    registry_entity_component_add(registry, truck, HEALTH_COMPONENT_BIT, &hc);
    /* registry_entity_component_add(registry, truck, */
    /*                               PROJECTILE_EMITTER_COMPONENT_BIT, &pec); */
    registry_entity_add(registry, truck);
  }

  {
    Entity chopper = registry_entity_create(registry);
    registry_entity_set_flags(registry, chopper, ENTITY_FRIENDLY);

    TransformComponent tc = {0};
    tc.pos.x = 0.0f;
    tc.pos.y = 0.0f;
    tc.pos.z = 1.0f;
    tc.scale = 1.f;
    tc.rotation = 0.f;

    RenderComponent rc;
    rc.material_id = assets_make_id_str("chopper.mat");
    rc.pipeline_id = unit_pipeline_id;
    rc.render_layer = 1;
    rc.texture_atlas_index.x = 0;
    rc.texture_atlas_index.y = 0;
    rc.texture_atlas_size.x = 2;
    rc.texture_atlas_size.y = 4;
    rc.render_layer = RENDER_COMPONENT_RENDER_LAYER_AIR;

    PhysicsComponent pc;
    pc.velocity.x = 0.01f;
    pc.velocity.y = 0.01f;

    AnimationComponent ac = {0};
    ac.frames_per_animation_frame = 5;
    ac.num_animation_frames = 2;
    ac.num_frames_width = 2;
    ac.num_frames_height = 4;
    ac.is_playing = 1;
    ac.last_offset = 0.f;

    InputComponent ic = {0};

    CollisionComponent cc;
    cc.aabr.pos.x = 0.f;
    cc.aabr.pos.y = 0.f;
    cc.aabr.width = 1.f;
    cc.aabr.height = 1.f;

    CameraMovementComponent cmc = {0};

    HealthComponent hc;
    hc.health = 100;

    registry_entity_component_add(registry, chopper, RENDER_COMPONENT_BIT, &rc);
    registry_entity_component_add(registry, chopper, TRANSFORM_COMPONENT_BIT,
                                  &tc);
    registry_entity_component_add(registry, chopper, PHYSICS_COMPONENT_BIT,
                                  &pc);
    registry_entity_component_add(registry, chopper, ANIMATION_COMPONENT_BIT,
                                  &ac);
    registry_entity_component_add(registry, chopper, INPUT_COMPONENT_BIT, &ic);
    registry_entity_component_add(registry, chopper, COLLISION_COMPONENT_BIT,
                                  &cc);
    registry_entity_component_add(registry, chopper,
                                  CAMERA_MOVEMENT_COMPONENT_BIT, &cmc);
    registry_entity_component_add(registry, chopper, HEALTH_COMPONENT_BIT, &hc);

    registry_entity_add(registry, chopper);

    registry_entity_tag(registry, chopper, "player");
  }
}

static void game_load_level_assets(Game *game, LevelAssets *level_assets) {
  size_t total_asset_count =
      level_assets->textures_count + level_assets->programs_count +
      level_assets->materials_count; // + level_assets->maps_count;
  size_t total_asset_bytes = total_asset_count * sizeof(Asset);
  Asset *assets = stack_alloc(&stack_allocator, total_asset_bytes);

  size_t asset_index = 0;
  for (size_t i = 0; i < level_assets->textures_count; ++i) {
    assets[asset_index].id = assets_make_id_str(level_assets->textures[i]);
    assets[asset_index].type = AssetTypeTexture;
    ++asset_index;
  }

  for (size_t i = 0; i < level_assets->programs_count; ++i) {
    assets[asset_index].id = assets_make_id_str(level_assets->programs[i]);
    assets[asset_index].type = AssetTypeShaderProgram;
    ++asset_index;
  }

  for (size_t i = 0; i < level_assets->materials_count; ++i) {
    assets[asset_index].id = assets_make_id_str(level_assets->materials[i]);
    assets[asset_index].type = AssetTypeMaterial;
    ++asset_index;
  }

  for (size_t i = 0; i < level_assets->maps_count; ++i) {
    AssetId map_id = assets_make_id_str(level_assets->maps[i]);
    map_load(game, map_id);

    /* assets[asset_index].id = assets_make_id_str(level_assets->maps[i]); */
    /* assets[asset_index].type = AssetTypeMap; */
    /* ++asset_index; */
  }

  render_system_load_assets(game->render_system, assets, total_asset_count);

  stack_dealloc(&stack_allocator, assets, total_asset_bytes);
}

// Level consists of a map and units.
// Map => tile entities, whatever they are. RenderComponent ++
// Units are either friendly or enemy, player controller
// or non player controlled
/*
Unit: - TransformComponent / Position
      -
Level
- Map: map asset
- Units: Type {truck, tank, chopper}, map tile position to world position
         Flags: Friendly/unfriendly, Playable, Nonplayable


*/

void game_setup(Game *game) {
  LOG_INFO("Setup");

  LevelAssets level_assets = {
      .textures = level_0_textures,
      .textures_count = sizeof(level_0_textures) / sizeof(const char *),
      .materials = level_0_materials,
      .materials_count = sizeof(level_0_materials) / sizeof(const char *),
      .programs = level_0_programs,
      .programs_count = sizeof(level_0_programs) / sizeof(const char *),
      .maps = level_0_maps,
      .maps_count = sizeof(level_0_maps) / sizeof(const char *)};

  game_load_level_assets(game, &level_assets);

  /* map_init(&game->map); */
  /* map_load(&game->map, &game->registry, &game->assets); */

  /* /\* map_write_meta(&game->map, "./assets/tilemaps/jungle.meta"); *\/ */
  /* /\* Map lol = {0}; *\/ */
  /* /\* map_read_meta(&lol, "./assets/tilemaps/jungle.meta"); *\/ */

  /* load_tile_map_layout("./assets/tilemaps/jungle.map", &game->map); */

  load_units(&game->registry, &game->assets);

  CameraMovementSystem *camera_movement_system =
      (CameraMovementSystem *)registry_get_system(&game->registry,
                                                  CAMERA_MOVEMENT_SYSTEM_BIT);
  camera_movement_system_set_world_size(camera_movement_system,
                                        game->map.world_size);

  // Push our initial entities before entering main loop
  registry_entity_commit_entities(&game->registry);

  /* PreparedResources prep; */
  /* prep.program_ids = vec_create(); */
  /* prep.material_ids = vec_create(); */

  /* VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("tilemap")); */
  /* VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("unit")); */
  /* VEC_PUSH_T(&prep.program_ids, AssetId,
   * assets_make_id_str("collision_debug")); */

  /* VEC_PUSH_T(&prep.material_ids, AssetId,
   * assets_make_id_str("truck-blue-mat")); */
  /* VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("jungle-mat"));
   */
  /* VEC_PUSH_T(&prep.material_ids, AssetId,
   * assets_make_id_str("chopper-udlr")); */
  /* VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("bullet-mat"));
   */
  /* VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("plain-red"));
   */

  /* render_system_prepare_resources(game->render_system, &prep); */
}

void game_update(Game *game) {
  arena_dealloc_all(&frame_allocator);

  double x, y;
  int fb_width, fb_height;
  glfwGetCursorPos(game->window, &x, &y);
  glfwGetFramebufferSize(game->window, &fb_width, &fb_height);

  // NOTE: Apparently one of these can be < 0 in some cases. Can't reproduce
  // but it is UB, so don't do anything in that case
  if (fb_width > 0 && fb_height > 0 && x > 0 && y > 0) {
    input_system_set_cursor_pos(game->input_system, (uint16_t)x, (uint16_t)y,
                                (uint16_t)fb_width, (uint16_t)fb_height);
  } else {
    LOG_WARN("fb.x %d fb.y %d mouse.x %f mouse.y %f", fb_width, fb_height, x,
             y);
  }

  event_bus_subscribe(
      &game->event_bus, (struct SystemBase *)game->render_system,
      OS_FramebufferSizeChanged, render_system_handle_framebuffer_size_changed);

  event_bus_subscribe(&game->event_bus,
                      (struct SystemBase *)game->player_system,
                      KeyboardInput_Update, &player_system_handle_event);

  event_bus_subscribe(&game->event_bus,
                      (struct SystemBase *)game->damage_system,
                      CollisionSystem_Detected, &damage_system_handle_event);

  event_bus_subscribe(
      &game->event_bus, (struct SystemBase *)game->render_system,
      CameraSystem_CameraChanged, render_system_handle_camera_position_changed);

  event_bus_subscribe(
      &game->event_bus, (struct SystemBase *)game->hit_detection_system,
      InputSystem_CursorMoved, hit_detection_system_handle_cursor_moved);

  event_bus_subscribe(
      &game->event_bus, (struct SystemBase *)game->render_system,
      HitDetectionSystem_MeshHit, render_system_handle_hit_detection);

  LOG_INFO("************ BEGIN DEFERRED EVENTS ***************");
  for (size_t i = 0; i < game->event_bus.num_deferred_events; ++i) {
    LOG_INFO("%zu Type: %s", i,
             event_type_name(game->event_bus.deferred_events[i].id));
  }

  LOG_INFO("************ END DEFFERED EVENTS ***************");
  event_bus_process_deferred(&game->event_bus);

  registry_update(&game->registry, game->frame_counter);

  event_bus_reset(&game->event_bus);
}

void game_run(Game *game) {

  LOG_INFO("Main loop");
  game->state |= GAME_RUNNING;

  while ((game->state & GAME_RUNNING) != 0) {
    LOG_INFO("###### FRAME %zu #####", game->frame_counter);
    BeginScopedTimer(frame_time);

    glfwPollEvents();

    BeginScopedTimer(update_time);
    game_update(game);

    AppendScopedTimer(update_time);
    PrintScopedTimer(update_time);

    if (game->state & GAME_DEBUG_ENABLED) {
      RenderSystem *render_system = (RenderSystem *)registry_get_system(
          &game->registry, RENDER_SYSTEM_BIT);
      render_system_debug(render_system, &game->registry);
    }

    BeginScopedTimer(swap_buffers);
    glfwSwapBuffers(game->window);
    AppendScopedTimer(swap_buffers);
    PrintScopedTimer(swap_buffers);

    game->frame_counter++;

    AppendScopedTimer(frame_time);
    PrintScopedTimer(frame_time);

    // Last time checked, each logged line takes 20microsecs.
    // That's alot.
    //    LOG_INFO("Time spent logging %llu microsecs", time_logging);
    time_logging = 0;
  }
}

void game_destroy(Game *game) {
  if (game->window) {
    glfwDestroyWindow(game->window);
  }
  glfwTerminate();
}

void game_window_size_changed(Game *game, int width, int height) {
  (void)game;
  LOG_INFO("Window size changed: (%d, %d)", width, height);
}
