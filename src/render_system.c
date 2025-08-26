#include "render_system.h"

#include "components.h"
#include "events.h"

#include "systems.h"

#include "core/allocators.h"
#include "core/assetstore.h"
#include "core/hashmap.h"
#include "core/log.h"
#include "core/math.h"
#include "core/renderer.h"
#include "core/vec.h"
#include "core/worker.h"

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdint.h>

typedef struct {
  Mat4x4 model_matrix;
  AssetId material_id;
  AssetId program_id;
  AssetId mesh_id;
  Vec2f tex_coord_offset;
  Vec2f tex_coord_scale;
  uint8_t render_layer;
} RenderData;

void CALLING_CONVENTION gl_debug_callback(GLenum source, GLenum type, GLuint id,
                                          GLenum severity, GLsizei length,
                                          const GLchar *message,
                                          const void *userParam) {
  (void)source;
  (void)type;
  (void)id;
  (void)length;
  (void)userParam;
  switch (severity) {
  case GL_DEBUG_SEVERITY_HIGH:
    LOG_GL_HIGH(message);
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    LOG_GL_MEDIUM(message);
    break;
  case GL_DEBUG_SEVERITY_LOW:
    LOG_GL_LOW(message);
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    LOG_GL_NOTIFY(message);
    break;
  }
}

static GLuint compile_shader(const char *src, GLenum type) {
  GLint src_len = (GLint)strlen(src);
  GLuint handle = glCreateShader(type);
  glShaderSource(handle, 1, &src, &src_len);
  CHECK_GL_ERROR();
  glCompileShader(handle);
  CHECK_GL_ERROR();

  int status = 0;
  glGetShaderiv(handle, GL_COMPILE_STATUS, &status);
  if (status != GL_TRUE) {
    int log_len = 0;
    glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len) {
      GLchar *log_buf = malloc(log_len);
      GLsizei actual_len = 0;
      glGetShaderInfoLog(handle, log_len, &actual_len, log_buf);
      LOG_EXIT("Failed to compile shader: %s\n%s", log_buf, src);
    } else {
      LOG_EXIT("Failed to compile shader: Log len is 0");
    }

    CHECK_GL_ERROR();
  }

  return handle;
}

static GLuint create_program(GLuint *shaders, int count) {
  GLuint prog = glCreateProgram();

  CHECK_GL_ERROR();

  for (int i = 0; i < count; ++i) {
    glAttachShader(prog, *(shaders + i));
  }

  glLinkProgram(prog);

  CHECK_GL_ERROR();

  glValidateProgram(prog);
  int status = 0;
  glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);

  if (status != GL_TRUE) {
    int log_len = 0;
    // Includes null-terminator
    glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
    if (log_len) {
      GLchar *log_buf = malloc(log_len);
      GLsizei actual_len = 0;
      glGetProgramInfoLog(prog, log_len, &actual_len, log_buf);
      LOG_EXIT("Failed to validate program:\n%s", log_buf);
      free(log_buf);
      CHECK_GL_ERROR();
    } else {
      LOG_EXIT("Failed to validate program, but info log length is 0");
    }
  }

  return prog;
}

static GLuint get_program_handle_by_asset_id(RenderSystem *system, AssetId id) {
  GLuint *program = 0;
  if (!hash_map_get(&system->programs, &id, sizeof(AssetId),
                    (void **)&program)) {
    LOG_EXIT("Failed to find shader program with asset ID %zu", id);
  }
  return (GLuint)(uintptr_t)program;
}

static GLuint get_program_handle_by_name(RenderSystem *system,
                                         const char *name) {
  AssetId asset_id = assets_make_id_str(name);
  return get_program_handle_by_asset_id(system, asset_id);
}

static GLint get_uniform_location_checked(GLuint program, const char *name) {
  GLint loc = glGetUniformLocation(program, name);
  CHECK_GL_ERROR();

  if (loc == -1) {
    LOG_EXIT("Invalid uniform location '%s'", name);
  }

  return loc;
}

static int render_data_order_comp(const void *lhs, const void *rhs) {
  const RenderData *a = lhs;
  const RenderData *b = rhs;

  if (a->render_layer < b->render_layer) {
    return -1;
  } else if (a->render_layer == b->render_layer) {
    return 0;
  }

  return 1;
}

static void sort_render_data(RenderData *data, size_t count) {
  qsort(data, count, sizeof(RenderData), &render_data_order_comp);
}

static void render_entities(RenderSystem *system, RenderData *render_data,
                            size_t render_data_size) {
  sort_render_data(render_data, render_data_size);

  CHECK_GL_ERROR();

  glClearColor(0.2f, 0.2f, 0.2f, 1.f);
  glDisable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT);
  glDisable(GL_CULL_FACE);

  glEnable(GL_BLEND);
  glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (render_data_size == 0) {
    LOG_WARN("Render called with render data size 0");
    return;
  }

  renderer_use(system->tile_renderer);
  glUseProgram(system->program_handle);
  CHECK_GL_ERROR();

  system->loc_view_projection =
      get_uniform_location_checked(system->program_handle, "ViewProj");

  glUniformMatrix4fv(system->loc_view_projection, 1, GL_FALSE,
                     system->camera.view_projection.data);

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER,
               system->tile_renderer->multi_draw_indirect_buffer);
  CHECK_GL_ERROR();

  unsigned int count_in_batch = 0;

  for (size_t i = 0; i < render_data_size; ++i) {
    RenderData *rd = &render_data[i];

    DrawElementsIndirectCommand *command =
        &system->draw_commands_elements[count_in_batch];
    command->count = 6;
    command->instance_count = 1;
    command->first_index = 0;
    command->base_vertex = 0;
    command->base_instance = 0;

    DrawCommandDataTiled *draw_data =
        &system->draw_command_data[count_in_batch];
    draw_data->model = rd->model_matrix;
    draw_data->tex_coord_offset = rd->tex_coord_offset;
    draw_data->tex_coord_scale = rd->tex_coord_scale;

    void *value = 0;
    int found = hash_map_get(&system->material_asset_index_mapping,
                             &rd->material_id, sizeof(AssetId), &value);

    // We only have to do more work if haven't seen this material id
    // before.
    if (found) {
      draw_data->material_index = (unsigned int)(uintptr_t)value;
    } else {
      LOG_EXIT("Failed to map material id to material index");
    }

    count_in_batch++;
  }

  CHECK_GL_ERROR();
  glBeginQuery(GL_TIME_ELAPSED, system->query_time_elapsed);

  glNamedBufferSubData(system->tile_renderer->multi_draw_indirect_buffer, 0,
                       sizeof(DrawElementsIndirectCommand) * count_in_batch,
                       system->draw_commands_elements);
  CHECK_GL_ERROR();

  glNamedBufferSubData(
      system->tile_renderer->shader_storage_buffer_objects.buffer_object[0], 0,
      sizeof(DrawCommandDataTiled) * count_in_batch, system->draw_command_data);
  CHECK_GL_ERROR();

  glNamedBufferSubData(
      system->tile_renderer->shader_storage_buffer_objects.buffer_object[1], 0,
      sizeof(Material) * system->materials.size,
      VEC_ITER_BEGIN_T(&system->materials, Material));

  CHECK_GL_ERROR();

  glEndQuery(GL_TIME_ELAPSED);
  GLint time_elapsed_ns = -1;
  glGetQueryObjectiv(system->query_time_elapsed, GL_QUERY_RESULT,
                     &time_elapsed_ns);

  LOG_INFO("named buffer sub data elapsed ns %d", time_elapsed_ns);

  renderer_multi_draw_elements_indirect(system->tile_renderer, count_in_batch);
}

typedef struct {
  Entity *entities;
  Pool *transform_pool;
  Pool *render_pool;
  Pool *tile_pool;
  RenderData *render_data;
  int begin;
  int end;
} RenderUpdateRangeArgs;

static void render_update_range(void *job_params) {
  RenderUpdateRangeArgs *range_args = job_params;

  LOG_INFO("Processing range [%d,%d>", range_args->begin, range_args->end);
  for (int i = range_args->begin; i < range_args->end; ++i) {
    Entity entity = range_args->entities[i];

    TransformComponent *tc = PoolGetComponent(range_args->transform_pool,
                                              TransformComponent, entity.index);
    RenderComponent *rc = PoolGetComponent(range_args->render_pool,
                                           RenderComponent, entity.index);

    RenderData *rd = &range_args->render_data[i];

    Vec3f scale;

    // Scale <1.f to see individual map tiles
    scale.x = tc->scale;
    scale.y = tc->scale;
    scale.z = 1.0f;

    Vec3f pos;
    pos.x = tc->pos.x;
    pos.y = tc->pos.y;
    pos.z = tc->pos.z;

    const float atlas_cols_f = (float)rc->texture_atlas_size.x;
    const float atlas_rows_f = (float)rc->texture_atlas_size.y;

    /*
     rc.tex_coord_offset.x = ((float)tile.atlas_coord.x) / atlas_cols_f;
     rc.tex_coord_offset.y =
         (atlas_rows_f - 1.f - (float)tile.atlas_coord.y) /
           atlas_rows_f;
     */
    rd->tex_coord_offset.x = ((float)rc->texture_atlas_index.x) / atlas_cols_f;
    rd->tex_coord_offset.y =
        (atlas_rows_f - 1.f - (float)rc->texture_atlas_index.y) / atlas_rows_f;
    rd->tex_coord_scale.x = 1.f / atlas_cols_f;
    rd->tex_coord_scale.y = 1.f / atlas_rows_f;

    rd->material_id = rc->material_id;
    rd->program_id = rc->pipeline_id;

    rd->render_layer = rc->render_layer;

    Vec3f axis = {0.f, 0.f, 1.f};

    Mat4x4 matrix_translate;
    mat4_identity(&matrix_translate);

    Mat4x4 matrix_scale;
    mat4_identity(&matrix_scale);

    Mat4x4 matrix_rotate;
    mat4_identity(&matrix_rotate);

    mat4_rotate(&matrix_rotate, &axis, tc->rotation);
    mat4_scale(&matrix_scale, &scale);
    mat4_translate(&matrix_translate, &pos);

    Mat4x4 m;
    mat4_identity(&m);
    m = mat4_mul(&m, &matrix_translate);
    m = mat4_mul(&m, &matrix_rotate);
    m = mat4_mul(&m, &matrix_scale);

    rd->model_matrix = m;
  }
}

static void render_update(Registry *reg, struct SystemBase *sys,
                          size_t frame_nr) {
  BeginScopedTimer(render_update);
  (void)frame_nr;

  RenderSystem *render_sys = (RenderSystem *)sys;
  Entity *entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

  Pool *transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
  Pool *render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);

  RenderData *render_data =
      ArenaAlloc(&frame_allocator, sys->entities.size, RenderData);

  // 'NUM_THREADS' + 1 because we also need to deal with the remainder
  // after evenly distributing work
  RenderUpdateRangeArgs *job_args =
      ArenaAlloc(&frame_allocator, NUM_THREADS + 1, RenderUpdateRangeArgs);

  int batch_size = sys->entities.size / NUM_THREADS;

  for (int i = 0; i < NUM_THREADS; ++i) {

    job_args[i].entities = entities;
    job_args[i].transform_pool = transform_pool;
    job_args[i].render_data = render_data;
    job_args[i].render_pool = render_pool;
    int begin = i * batch_size;
    int end = begin + batch_size;
    job_args[i].begin = begin;
    job_args[i].end = end;

    work_queue_push(sys->services.work_queue, &render_update_range,
                    &job_args[i]);
  }

  // Dispatch the leftover work
  int job_batch_remainder = sys->entities.size % NUM_THREADS;
  if (job_batch_remainder) {
    job_args[NUM_THREADS].entities = entities;
    job_args[NUM_THREADS].transform_pool = transform_pool;
    job_args[NUM_THREADS].render_data = render_data;
    job_args[NUM_THREADS].render_pool = render_pool;
    job_args[NUM_THREADS].begin = NUM_THREADS * batch_size;
    job_args[NUM_THREADS].end =
        job_args[NUM_THREADS].begin + job_batch_remainder;

    work_queue_push(sys->services.work_queue, &render_update_range,
                    &job_args[NUM_THREADS]);
  }

  work_queue_sync(sys->services.work_queue);

  AppendScopedTimer(render_update);
  PrintScopedTimer(render_update);

  BeginScopedTimer(render_entities);
  render_entities(render_sys, render_data, sys->entities.size);
  AppendScopedTimer(render_entities);
  PrintScopedTimer(render_entities);
}

void render_system_create_program(RenderSystem *system, AssetId program_id) {

  void *prog_ptr = 0;
  if (hash_map_get(&system->programs, &program_id, sizeof(program_id),
                   prog_ptr)) {
    return;
  }

  AssetShaderProgram program;
  if (!assets_load_shader_program(system->base.services.assets, program_id,
                                  &program)) {
    LOG_EXIT("Failed to create program with id '%d': Program asset not found",
             program_id);
  }

  int num_shaders_in_program = 0;
  GLuint shader_handles[6];

  for (size_t i = 0; i < sizeof(program.shader_ids) / sizeof(AssetId); ++i) {
    if (assets_shader_program_has_shader(&program, i)) {

      size_t buffer_size = 1024 * 1024;

      AssetShader shader_asset;
      shader_asset.source_buffer =
          (Buffer){.data = stack_alloc(&stack_allocator, buffer_size),
                   .capacity = buffer_size,
                   .used = 0};

      if (!assets_load_shader(system->base.services.assets,
                              program.shader_ids[i], &shader_asset)) {
        LOG_EXIT(
            "Failed to create program with id '%d': Program asset not found",
            program.shader_ids[i]);
      }

      GLenum gl_shader_type = 0;
      switch (shader_asset.shader_type) {
      case AssetShaderTypeVertex:
        gl_shader_type = GL_VERTEX_SHADER;
        break;
      case AssetShaderTypeFragment:
        gl_shader_type = GL_FRAGMENT_SHADER;
        break;
      default:
        LOG_EXIT("Unhandled shader type");
      }

      LOG_INFO("Compiling shader '%s'", shader_asset.source_buffer.data);

      shader_handles[num_shaders_in_program++] = compile_shader(
          (const char *)shader_asset.source_buffer.data, gl_shader_type);

      stack_dealloc(&stack_allocator, (void *)shader_asset.source_buffer.data,
                    shader_asset.source_buffer.capacity);
    }
  }

  GLuint program_handle =
      create_program(shader_handles, num_shaders_in_program);

  hash_map_insert(&system->programs, &program_id, sizeof(program_id),
                  (void *)(uintptr_t)program_handle);
}

void render_system_global_init() {
  gladLoadGL();
  glDebugMessageCallback(&gl_debug_callback, 0);
  glEnable(GL_DEBUG_OUTPUT);
}

static struct Renderer *create_tile_renderer() {
  struct Renderer *tile_renderer = (struct Renderer *)ArenaAlloc(
      &global_static_allocator, 1, struct Renderer);

  struct VertexAttributeDescriptor attrib_desc[2];
  attrib_desc[0].vertex_attribute = 0;
  attrib_desc[0].element_count = 3;
  attrib_desc[0].element_type = GL_FLOAT;
  attrib_desc[0].normalize = GL_FALSE;
  attrib_desc[0].relative_offset = 0;

  attrib_desc[1].vertex_attribute = 1;
  attrib_desc[1].element_count = 2;
  attrib_desc[1].element_type = GL_FLOAT;
  attrib_desc[1].normalize = GL_FALSE;
  attrib_desc[1].relative_offset = sizeof(float) * 3;

  struct BindingPointDescriptor bp_desc[1];
  bp_desc[0].attrib_descriptors = &attrib_desc[0];
  bp_desc[0].num_attrib_descriptors = 2;
  bp_desc[0].binding_point_index = 0;
  bp_desc[0].offset = 0;
  bp_desc[0].stride = sizeof(float) * 5;

  struct VertexBufferDescriptor vb_desc;
  vb_desc.binding_descriptors = bp_desc;
  vb_desc.binding_point_count = 1;

  uint16_t index_data[] = {0, 1, 2, 0, 2, 3};

  struct RendererParameters tile_params;
  tile_params.index_buffer_size = sizeof(index_data);
  tile_params.num_vertices = 4;
  tile_params.num_buffer_descriptors = 1;
  tile_params.buffer_descriptors = &vb_desc;

  renderer_init(tile_renderer, &tile_params);

  float pos_data[] = {
      -0.5f, -0.5f, 0.0f, // lower left
      0.5f,  -0.5f, 0.0f, // lower right
      0.5f,  0.5f,  0.0f, // upper right
      -0.5f, 0.5f,  0.0f, // upper left
  };

  float uv_data[] = {
      0.0f, 0.0f, // lower left,
      1.0f, 0.0f, // lower right,
      1.0f, 1.0f, // upper right,
      0.0f, 1.0f, // upper left
  };

  char interleaved[sizeof(pos_data) + sizeof(uv_data)];
  memset(interleaved, 0x0, sizeof(pos_data) + sizeof(uv_data));
  struct Interleave args;
  args.dst = interleaved;
  args.element_count = 4;

  struct VertexDataSource attribs[2];
  attribs[0].src = pos_data;
  attribs[0].vertex_size_bytes = 3 * sizeof(float);
  attribs[1].src = uv_data;
  attribs[1].vertex_size_bytes = 2 * sizeof(float);

  args.num_attribs = 2;
  args.attribs = attribs;

  interleave_attributes(&args);

  glNamedBufferSubData(tile_renderer->vertex_buffer_objects[0], 0,
                       sizeof(interleaved), interleaved);

  // Draw command aux data
  renderer_ssbo_create(tile_renderer, 0, BO_INDEX_DRAW_COMMAND_DATA,
                       MAX_DRAW_INDIRECT_DRAW_COMMANDS *
                           sizeof(DrawCommandDataTiled));

  // Materials
  renderer_ssbo_create(tile_renderer, 1, BO_INDEX_MATERIALS,
                       MAX_DRAW_INDIRECT_DRAW_COMMANDS * sizeof(Material));

  // Indexed quad
  renderer_write_element_array_buffer(tile_renderer, 0, sizeof(index_data),
                                      index_data);
  CHECK_GL_ERROR();

  return tile_renderer;
}

struct Renderer *create_debug_renderer() {
  struct Renderer *debug_renderer = (struct Renderer *)ArenaAlloc(
      &global_static_allocator, 1, struct Renderer);

  struct VertexAttributeDescriptor attrib_desc[2];
  attrib_desc[0].vertex_attribute = 0;
  attrib_desc[0].element_count = 3;
  attrib_desc[0].element_type = GL_FLOAT;
  attrib_desc[0].normalize = GL_FALSE;
  attrib_desc[0].relative_offset = 0;

  struct BindingPointDescriptor bp_desc[1];
  bp_desc[0].attrib_descriptors = &attrib_desc[0];
  bp_desc[0].num_attrib_descriptors = 1;
  bp_desc[0].binding_point_index = 0;
  bp_desc[0].offset = 0;
  bp_desc[0].stride = sizeof(float) * 3;

  struct VertexBufferDescriptor vb_desc;
  vb_desc.binding_descriptors = bp_desc;
  vb_desc.binding_point_count = 1;

  struct RendererParameters params;
  params.index_buffer_size = 0;
  params.num_vertices = 65535;
  params.num_buffer_descriptors = 1;
  params.buffer_descriptors = &vb_desc;

  renderer_init(debug_renderer, &params);

  CHECK_GL_ERROR();

  return debug_renderer;
}

/* // TODO: Make it work without a position/center argument first. */
/* // I haven't tested if a straight translation is actually the right thing */
/* void recalc_camera(struct OrthoCamera *camera, int width, int height, */
/*                    float scale, Vec3f *center) { */
/*   (void)scale; */
/*   (void)center; */
/*   float aspect_ratio = (float)width / (float)height; */
/*   camera->aspect_ratio = aspect_ratio; */

/*   camera->projection = ortho( */
/*       1.f, -1.f, camera->rect.pos.x + camera->rect.width, camera->rect.pos.x,
 */
/*       camera->rect.pos.y + camera->rect.height, camera->rect.pos.y); */

/*   mat4_identity(&camera->view); */
/*   //  mat4_translate(&camera->view, center); */
/* } */

RenderSystem *render_system_create(Services *services, int window_w,
                                   int window_h, int screen_w, int screen_h) {
  LOG_INFO("Create render system implementation...");
  (void)screen_w;
  (void)screen_h;

  // TODO: This is per renderer data. Should be set up when invoking each
  // individual renderer

  RenderSystem *system = ArenaAlloc(&global_static_allocator, 1, RenderSystem);
  /* RenderSystem *system = */
  /*     arena_alloc(&global_static_allocator, sizeof(RenderSystem), 1, 4); */
  system_base_init(
      (struct SystemBase *)system, RENDER_SYSTEM_BIT, &render_update,
      RENDER_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, services, "RenderSystem");

  system->base.services = *services;
  system->materials = vec_create();
  system->main_framebuffer.width = window_w;
  system->main_framebuffer.height = window_h;

  hash_map_init(&system->programs, 100);
  hash_map_init(&system->textures, 1000);
  hash_map_init(&system->material_asset_index_mapping, 1000);

  system->tile_renderer = create_tile_renderer();
  CHECK_GL_ERROR();

  system->debug_renderer = create_debug_renderer();
  CHECK_GL_ERROR();

  //  Vec3f center = {0.f, 0.f, 0.f};
  /* system->camera.rect.pos.x = -system->main_framebuffer.width / 2; */
  /* system->camera.rect.pos.y = -system->main_framebuffer.height / 2; */
  /* system->camera.rect.width = system->main_framebuffer.width; */
  /* system->camera.rect.height = system->main_framebuffer.height; */

  /* system->camera.rect.pos.x = -4.f; */
  /* system->camera.rect.pos.y = -4.f; */
  /* system->camera.rect.width = 8.f; */
  /* system->camera.rect.height = 8.f; */

  /* recalc_camera(&system->camera, system->main_framebuffer.width, */
  /*               system->main_framebuffer.height, 25.f, &center); */

  render_system_framebuffer_size_changed(system, system->main_framebuffer.width,
                                         system->main_framebuffer.height);

  glCreateQueries(GL_TIME_ELAPSED, 1, &system->query_time_elapsed);

  LOG_INFO("Created tile renderer");

  return system;
}

static void render_system_create_material(RenderSystem *system,
                                          AssetId material_id) {
  Material new_material;
  AssetMaterial material_asset;

  if (!assets_load_material(system->base.services.assets, material_id,
                            &material_asset)) {
    LOG_EXIT("Material id %u not found: '%s", material_id);
  }

  new_material.color = material_asset.color;

  void *texture_handle_ptr = 0;
  int found_texture_handle = (GLuint64)(uintptr_t)hash_map_get(
      &system->textures, &material_asset.texture_id, sizeof(AssetId),
      &texture_handle_ptr);
  // Careful. Don't know if it's valid before testing hash_map_get return
  // value
  GLuint64 texture_handle = (GLuint64)texture_handle_ptr;
  if (!found_texture_handle) {

    void *data = 0;

    AssetTexture texture = {0};
    if (!assets_load_texture(system->base.services.assets,
                             material_asset.texture_id, &texture, &data)) {
      LOG_EXIT("Failed to load texture asset '%d'", material_asset.texture_id);
    }

    // Store the new handle as a void* in the hash map to avoid allocating
    // just for a pointer sized type
    GLuint64 new_handle =
        render_system_create_texture(system, data, &texture.meta);
    uintptr_t new_handle_ptr = (uintptr_t)new_handle;

    hash_map_insert(&system->textures, &material_asset.texture_id,
                    sizeof(AssetId), (void *)(uintptr_t)new_handle_ptr);

    if (data) {
      free(data);
    }

    new_material.handle = new_handle;

  } else {
    new_material.handle = texture_handle;
  }

  unsigned int new_material_index = system->materials.size;
  hash_map_insert(&system->material_asset_index_mapping, &material_id,
                  sizeof(AssetId), (void *)(uintptr_t)new_material_index);

  VEC_PUSH_T(&system->materials, Material, new_material);
}

uint64_t render_system_create_texture(RenderSystem *system, void *data,
                                      ImageMeta *meta) {
  (void)system;
  GLuint tex_handle;
  glCreateTextures(GL_TEXTURE_2D, 1, &tex_handle);
  CHECK_GL_ERROR();

  GLenum format = 0;
  GLenum internal_fmt = 0;
  switch (meta->channels) {
  case 3:
    format = GL_RGB;
    internal_fmt = GL_RGB8;
    break;
  case 4:
    format = GL_RGBA;
    internal_fmt = GL_RGBA8;
    break;
  default:
    LOG_EXIT("Unimplemented number of image channels (%d) to GL enum mapping",
             meta->channels);
  }

  int levels = 1;
  glTextureStorage2D(tex_handle, levels, internal_fmt, meta->width,
                     meta->height);
  CHECK_GL_ERROR();

  int level = 0;
  int xoffset = 0;
  int yoffset = 0;

  GLenum type = GL_UNSIGNED_BYTE;
  glTextureSubImage2D(tex_handle, level, xoffset, yoffset, meta->width,
                      meta->height, format, type, data);

  CHECK_GL_ERROR();

  // Hack set up a texture unit

  glTextureParameteri(tex_handle, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTextureParameteri(tex_handle, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTextureParameteri(tex_handle, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTextureParameteri(tex_handle, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glBindTexture(GL_TEXTURE_2D, tex_handle);
  glGenerateTextureMipmap(tex_handle);

  CHECK_GL_ERROR();

  GLuint64 bindless_handle = glGetTextureHandleARB(tex_handle);
  glMakeTextureHandleResidentARB(bindless_handle);

  CHECK_GL_ERROR();

  return bindless_handle;
}

void render_system_load_texture(RenderSystem *system, AssetId asset_id) {

  void *texture_handle_ptr = 0;
  int found_texture_handle = (GLuint64)(uintptr_t)hash_map_get(
      &system->textures, &asset_id, sizeof(AssetId), &texture_handle_ptr);
  if (found_texture_handle) {
    return;
  }

  void *texture_data = 0;
  AssetTexture texture;
  assets_load_texture(system->base.services.assets, asset_id, &texture,
                      &texture_data);

  // just for a pointer sized type
  GLuint64 new_handle =
      render_system_create_texture(system, texture_data, &texture.meta);
  uintptr_t new_handle_ptr = (uintptr_t)new_handle;

  hash_map_insert(&system->textures, &asset_id, sizeof(AssetId),
                  (void *)(uintptr_t)new_handle_ptr);

  if (texture_data) {
    free(texture_data);
  }
}

static void render_system_create_map_mesh(RenderSystem *system,
                                          AssetId asset_id) {
  AssetMap map_asset = {0};
  if (!assets_load_map(system->base.services.assets, asset_id, &map_asset)) {
    LOG_ERROR("Failed to create map mesh");
    return;
  }

  assets_clear_temp_data(system->base.services.assets);
}

void render_system_load_assets(RenderSystem *system, Asset *assets,
                               size_t asset_count) {
  for (size_t i = 0; i < asset_count; ++i) {
    AssetId asset_id = assets[i].id;
    switch (assets[i].type) {
    case AssetTypeTexture:
      render_system_load_texture(system, asset_id);
      break;
    case AssetTypeShaderProgram:
      render_system_create_program(system, asset_id);
      break;
    case AssetTypeMaterial:
      render_system_create_material(system, asset_id);
      break;
    case AssetTypeMap:
      render_system_create_map_mesh(system, asset_id);
      break;
    case AssetTypeMesh:
      break;
    default:
      continue;
    }
  }

  system->program_handle = get_program_handle_by_name(system, "tilemap.prog");
}

void render_system_framebuffer_size_changed(RenderSystem *system,
                                            uint16_t width_px,
                                            uint16_t height_px) {
  glClearColor(0.f, 0.0f, 0.0f, 1.f);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glDisable(GL_DEPTH_TEST);
  system->main_framebuffer.width = width_px;
  system->main_framebuffer.height = height_px;

  uint16_t w = width_px;
  uint16_t h = height_px;

  if (width_px >= height_px) {
    // height is the limiting factor
    w = height_px;
  } else {
    // width is the limiting factor
    h = width_px;
  }

  uint16_t offset_x = (uint16_t)(((float)width_px - (float)w) / 2.f);
  uint16_t offset_y = (uint16_t)(((float)height_px - (float)h) / 2.f);

  glViewport(offset_x, offset_y, w, h);
}

void render_system_handle_framebuffer_size_changed(struct SystemBase *base,
                                                   struct Event e) {
  if (!is_expected_event_id(OS_FramebufferSizeChanged, e.id)) {
    return;
  }

  OSFramebufferSizeChanged *ev = e.event_data;

  render_system_framebuffer_size_changed((RenderSystem *)base, ev->size.x,
                                         ev->size.y);
}

void render_system_handle_camera_position_changed(struct SystemBase *system,
                                                  struct Event e) {
  (void)e;
  struct RenderSystem *render_sys = (struct RenderSystem *)system;
  CameraUpdatedEventData *event_data = e.event_data;

  render_sys->camera.view_projection = event_data->view_projection;
  render_sys->camera.view = event_data->view;
  render_sys->camera.projection = event_data->projection;
}

void render_system_handle_hit_detection(struct SystemBase *base,
                                        struct Event e) {
  (void)base;
  (void)e;
  if (e.id != HitDetectionSystem_MeshHit) {
    LOG_ERROR("Incorrect event type %s expected %s", event_type_names[e.id],
              event_type_names[HitDetectionSystem_MeshHit]);
    return;
  }

  HitDetectionEvent *event = (HitDetectionEvent *)e.event_data;
  uint32_t material_index = event->intersection.triangle_index;

  LOG_INFO("Hit on triangle index %d", material_index);

  RenderSystem *system = (RenderSystem *)base;
  Material *material =
      VEC_GET_T_PTR(&system->materials, Material, material_index);

  material->color.r = 255;
  material->color.g = 0;
  material->color.b = 0;
  material->color.a = 255;

  /* glNamedBufferSubData( */
  /*     system->tile_renderer->shader_storage_buffer_objects.buffer_object[1],
   * 0, */
  /*     sizeof(Material) * system->materials.size, */
  /*     VEC_ITER_BEGIN_T(&system->materials, Material)); */
}

void render_system_debug(struct RenderSystem *system, Registry *registry) {
  CHECK_GL_ERROR();

  renderer_use(system->debug_renderer);
  CHECK_GL_ERROR();

  GLuint program_handle =
      get_program_handle_by_name(system, "collision_debug.prog");
  glUseProgram(program_handle);
  CHECK_GL_ERROR();

  GLint loc_view = get_uniform_location_checked(program_handle, "View");
  GLint loc_proj = get_uniform_location_checked(program_handle, "Proj");
  GLint loc_model = get_uniform_location_checked(program_handle, "Model");

  glUniformMatrix4fv(loc_view, 1, GL_FALSE, system->camera.view.data);
  glUniformMatrix4fv(loc_proj, 1, GL_FALSE, system->camera.projection.data);
  CHECK_GL_ERROR();

  Pool *collision_pool = registry_get_pool(registry, COLLISION_COMPONENT_BIT);
  Pool *transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);

  for (int i = 0; i < system->base.entities.size; ++i) {
    Entity e = VEC_GET_T(&system->base.entities, Entity, i);

    // Skip anything that doesn't have at least a collision component
    if (!registry_entity_has_component(registry, e, COLLISION_COMPONENT_BIT)) {
      continue;
    }

    TransformComponent *tc =
        PoolGetComponent(transform_pool, TransformComponent, e.index);
    CollisionComponent *cc =
        PoolGetComponent(collision_pool, CollisionComponent, e.index);
    Vec3f pos[4] = {0};
    float w = cc->width / 2.f;
    float h = cc->height / 2.f;

    pos[0].x = tc->pos.x - w;
    pos[0].y = tc->pos.y - h;
    pos[0].z = 0.f;

    pos[1].x = tc->pos.x + w;
    pos[1].y = tc->pos.y - h;
    pos[1].z = 0.f;

    pos[2].x = tc->pos.x + w;
    pos[2].y = tc->pos.y + h;
    pos[2].z = 0.f;

    pos[3].x = tc->pos.x - w;
    pos[3].y = tc->pos.y + h;
    pos[3].z = 0.f;

    Mat4x4 model;
    mat4_identity(&model);
    //    mat4_translate(&model, &tc->pos);

    glNamedBufferSubData(system->debug_renderer->vertex_buffer_objects[0], 0,
                         sizeof(pos), pos);

    glUniformMatrix4fv(loc_model, 1, GL_FALSE, model.data);

    glDrawArrays(GL_LINE_LOOP, 0, 4);
  }

  CHECK_GL_ERROR();
}
