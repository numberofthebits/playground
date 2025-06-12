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
#define STATIC_ARENA_SIZE 1024 * 1024 * 64
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

enum GameState {
  GAME_RUNNING = 0x1,
  GAME_DEBUG_ENABLED = 0x2,
};

struct Game_t {
  Registry registry;
  GLFWwindow *window;
  struct Assets assets;
  Map map;
  struct EventBus event_bus;
  struct WorkQueue work_queue;

  int state;
  size_t frame_counter;
  Services services;

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

enum ReadlineResult {
  READLINE_DONE = 0,
  READLINE_CONTINUE = 1,
  READLINE_BUFFER_TOO_SMALL = -2,
  READLINE_READ_ERROR = -3
};
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
  game_frame_buffer_size_changed(game, width, height);
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
  Game *game = ArenaAlloc(&global_static_allocator, 1, Game);

  work_queue_init(&game->work_queue);

  game->state = 0;
  game->frame_counter = 0;

  if (!glfwInit()) {
    LOG_ERROR("GLFW init failed");
    return 0;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
  glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

  GLFWwindow *window = glfwCreateWindow(1280, 720, "1,2,3 techno", 0, 0);

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

  GLFWmonitor *monitor = glfwGetPrimaryMonitor();
  const GLFWvidmode *mode = glfwGetVideoMode(monitor);
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
  Vec2f camera_area = {16.f, 16.f};
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
                      (struct SystemBase *)game->render_system);

#ifdef BUILD_TESTS
  stack_test();
  pool_test();
  registry_test();
  work_queue_test();
#endif
  return game;
}

void map_init(Map *map) {
  // Hard code this for now, since we haven't defined a meta data
  // structure for maps yet
  map->world_size.x = 10 * 25;
  map->world_size.y = 10 * 20;
  map->map_size.x = 25;
  map->map_size.y = 20;
  map->atlas_size.x = 10;
  map->atlas_size.y = 3;
  map->tiles = ArenaAlloc(&global_static_allocator,
                          map->map_size.x * map->map_size.y, MapTile);
}

void map_load(Map *map, Registry *registry, struct Assets *assets) {
  (void)assets;
  const float atlas_cols_f = (float)map->atlas_size.x;
  const float atlas_rows_f = (float)map->atlas_size.y;

  for (uint32_t row = 0; row < map->world_size.y; ++row) {
    for (uint32_t col = 0; col < map->world_size.x; ++col) {
      Entity e = registry_entity_create(registry);

      TransformComponent tc = {0};
      tc.scale.x = 1.f;
      tc.scale.y = 1.f;
      tc.pos.x = col;
      tc.pos.y = row;
      tc.pos.z = 0.0f;
      tc.rotation = 0.0f;

      registry_entity_component_add(registry, e, TRANSFORM_COMPONENT_BIT, &tc);

      MapTile tile =
          map_get_tile(map, col % map->map_size.x, row % map->map_size.y);

      RenderComponent rc;
      rc.render_layer = 0;

      // In map space, tile starts at the upper left
      // In GL texture coord space, tile starts at lower left
      // Offset.y must be the bottom of the texture rect.
      rc.tex_coord_offset.x = ((float)tile.atlas_coord.x) / atlas_cols_f;
      rc.tex_coord_offset.y =
          (atlas_rows_f - 1.f - (float)tile.atlas_coord.y) / atlas_rows_f;
      rc.tex_coord_scale.x = 1.f / (atlas_cols_f);
      rc.tex_coord_scale.y = 1.f / (atlas_rows_f);
      rc.material_id = assets_make_id_str("jungle-mat");
      rc.pipeline_id = assets_make_id_str("tilemap");

      registry_entity_component_add(registry, e, RENDER_COMPONENT_BIT, &rc);

      registry_entity_add(registry, e);

      //      registry_entity_group(registry, e, "tiles");
    }
  }
}

void load_units(Registry *registry, struct Assets *assets) {
  (void)assets;

  const char *unit_shader_name = "tilemap";
  AssetId unit_shader_id =
      assets_make_id(unit_shader_name, strlen(unit_shader_name));
  {
    Entity truck = registry_entity_create(registry);
    registry_entity_group(registry, truck, "enemies");

    TransformComponent tc = {0};
    tc.pos.x = 0.5;
    tc.pos.y = 4.0;
    tc.pos.z = 0.0f;
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
    registry_entity_component_add(registry, truck,
                                  PROJECTILE_EMITTER_COMPONENT_BIT, &pec);
    registry_entity_component_add(registry, truck, HEALTH_COMPONENT_BIT, &hc);

    registry_entity_add(registry, truck);
    registry_entity_set_flags(registry, truck, ENTITY_HOSTILE);

    // Add a second truck for collision tests
    //  truck = registry_entity_create(registry);
    registry_entity_group(registry, truck, "enemies");

    tc.pos.x = 10.f;
    pc.velocity.x = -0.01f;
    registry_entity_component_add(registry, truck, RENDER_COMPONENT_BIT, &rc);
    registry_entity_component_add(registry, truck, TRANSFORM_COMPONENT_BIT,
                                  &tc);
    registry_entity_component_add(registry, truck, PHYSICS_COMPONENT_BIT, &pc);
    registry_entity_component_add(registry, truck, COLLISION_COMPONENT_BIT,
                                  &cc);
    registry_entity_component_add(registry, truck,
                                  PROJECTILE_EMITTER_COMPONENT_BIT, &pec);
    registry_entity_add(registry, truck);
  }

  {
    Entity chopper = registry_entity_create(registry);
    registry_entity_set_flags(registry, chopper, ENTITY_FRIENDLY);

    TransformComponent tc = {0};
    tc.pos.x = 0.0f;
    tc.pos.y = 5.0f;
    tc.pos.z = 0.1f;
    tc.scale.x = 1.f;
    tc.scale.y = 1.f;
    tc.rotation = 0.f;

    RenderComponent rc;
    rc.render_layer = 10;
    rc.tex_coord_offset.x = 0.f;
    rc.tex_coord_offset.y = 0.f;
    rc.tex_coord_scale.x = 1.f;
    rc.tex_coord_scale.y = 1.f;
    rc.material_id = assets_make_id_str("chopper-udlr");
    rc.pipeline_id = unit_shader_id;

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

    CameraMovementComponent cmc;
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

void game_setup(Game *game) {
  (void)game;
  LOG_INFO("Setup");

  map_init(&game->map);
  load_tile_map_layout("./assets/tilemaps/jungle.map", &game->map);
  map_load(&game->map, &game->registry, &game->assets);
  map_print(&game->map);

  load_units(&game->registry, &game->assets);

  PreparedResources prep;
  prep.program_ids = vec_create();
  prep.material_ids = vec_create();
  VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("tilemap"));
  VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("unit"));
  VEC_PUSH_T(&prep.program_ids, AssetId, assets_make_id_str("collision_debug"));

  VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("truck-blue-mat"));
  VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("jungle-mat"));
  VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("chopper-udlr"));
  VEC_PUSH_T(&prep.material_ids, AssetId, assets_make_id_str("bullet-mat"));

  RenderSystem *render_system =
      (RenderSystem *)registry_get_system(&game->registry, RENDER_SYSTEM_BIT);
  render_system_prepare_resources(render_system, &prep);

  CameraMovementSystem *camera_movement_system =
      (CameraMovementSystem *)registry_get_system(&game->registry,
                                                  CAMERA_MOVEMENT_SYSTEM_BIT);
  camera_movement_system_set_world_size(camera_movement_system,
                                        game->map.world_size);

  // Push our initial entities before entering main loop
  registry_entity_commit_entities(&game->registry);
}

void game_update(Game *game) {
  arena_dealloc_all(&frame_allocator);

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
      &game->event_bus, (struct SystemBase *)game->render_system,
      HitDetectionSystem_MeshHit, render_system_handle_hit_detection);

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

void game_frame_buffer_size_changed(Game *game, int width, int height) {
  LOG_INFO("Framebuffer size changed: (%d, %d)", width, height);
  RenderSystem *render_system =
      (RenderSystem *)registry_get_system(&game->registry, RENDER_SYSTEM_BIT);
  if (!render_system) {
    LOG_ERROR("Failed to resize frame buffer: System not found");
    return;
  }

  render_system_frame_buffer_size_changed(render_system, width, height);
}

void game_window_size_changed(Game *game, int width, int height) {
  (void)game;
  LOG_INFO("Window size changed: (%d, %d)", width, height);
}
