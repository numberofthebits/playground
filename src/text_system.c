#include "text_system.h"

#include "components.h"
#include "core/allocators.h"
#include "core/assetstore.h"
#include "core/math.h"
#include "core/renderer.h"
#include "core/sparsecomponentpool.h"
#include "core/util.h"
#include "events.h"
#include "systems.h"

#include <core/allocators.h>
#include <core/assetstore.h>
#include <core/renderer.h>
#include <core/services.h>

// Clang format cannot rearrange these includes alphabetically
// clang-format off
#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
// clang-format on

#define TEXT_SYSTEM_ATLAS_PX_WIDTH 512
#define TEXT_SYSTEM_ATLAS_PX_HEIGHT 512
#define TEXT_SYSTEM_ATLAS_FONT_SIZE 40
#define TEXT_SYSTEM_FONT_FILE_NAME "arial.ttf"
#define TEXT_SYSTEM_ASCII_TABLE_SIZE 128
#define TEXT_SYSTEM_GLYPH_MAX_PER_DRAW_CALL 4096
#define TEXT_SYSTEM_GLYPH_VERTEX_COUNT 4

static stbtt_packedchar packed_chars[TEXT_SYSTEM_ASCII_TABLE_SIZE];

static GLuint64 text_system_create_font_atlas(TextSystem *system,
                                              AssetId asset_id) {
  AssetFont font = {0};
  if (!assets_load_font(system->base.services.assets, asset_id, &font)) {
    LOG_EXIT("Failed to load font");
  }

  stbtt_pack_context pack_ctx;
  size_t bitmap_data_size =
      TEXT_SYSTEM_ATLAS_PX_WIDTH * TEXT_SYSTEM_ATLAS_PX_HEIGHT;
  unsigned char *bitmap_data = stack_alloc(&stack_allocator, bitmap_data_size);

  stbtt_PackBegin(&pack_ctx, bitmap_data, TEXT_SYSTEM_ATLAS_PX_WIDTH,
                  TEXT_SYSTEM_ATLAS_PX_HEIGHT, 0, 1, 0);

  int font_index = 0;

  size_t size_ascii_table = TEXT_SYSTEM_ASCII_TABLE_SIZE * sizeof(int);
  int *ascii_table = stack_alloc(&stack_allocator, size_ascii_table);
  for (int i = 0; i < TEXT_SYSTEM_ASCII_TABLE_SIZE; ++i) {
    ascii_table[i] = i;
  }

  stbtt_pack_range range[1] = {0};
  range[0].font_size = 20.f;
  range[0].first_unicode_codepoint_in_range = 0;
  range[0].array_of_unicode_codepoints = ascii_table;
  range[0].num_chars = TEXT_SYSTEM_ASCII_TABLE_SIZE;
  range[0].chardata_for_range = packed_chars;

  stbtt_PackSetOversampling(&pack_ctx, 4, 4);
  stbtt_PackFontRanges(&pack_ctx, font.font_data.data, font_index, range,
                       ArrayLen(range, stbtt_pack_range));
  stbtt_PackEnd(&pack_ctx);

  GLuint64 handle_font_atlas_texture = renderer_create_texture_bindless(
      &system->text_renderer, GL_TEXTURE_2D, GL_RED, TEXT_SYSTEM_ATLAS_PX_WIDTH,
      TEXT_SYSTEM_ATLAS_PX_HEIGHT, 0, bitmap_data);

  // Clear the raw font data from temp storage
  assets_clear_temp_data(system->base.services.assets);

  // Clear the ascii table tmp data
  stack_dealloc(&stack_allocator, ascii_table, size_ascii_table);

  // Clear the texture bitmap after uploading to GPU
  stack_dealloc(&stack_allocator, bitmap_data, bitmap_data_size);

  return handle_font_atlas_texture;
}

void text_system_update(Registry *registry, struct SystemBase *sys,
                        size_t frame_nr) {
  (void)frame_nr;

  TextSystem *system = (TextSystem *)sys;

  renderer_use(&system->text_renderer);

  glUseProgram(system->program_id);

  /* glUniformMatrix4fv(system->loc_viewproj, 1, GL_FALSE, */
  /*                    system->view_projection_matrix.data); */

  /* glUniformMatrix4fv(system->loc_view, 1, GL_FALSE,
   * system->view_matrix.data); */

  glUniformMatrix4fv(system->loc_proj, 1, GL_FALSE,
                     system->projection_matrix.data);

  Pool *text_pool = registry_get_pool(registry, TEXT_COMPONENT_BIT);
  Pool *transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);

  Vec3f *vbo_pos = renderer_map_vertex_buffer(&system->text_renderer, 0);
  Vec2f *vbo_uv = renderer_map_vertex_buffer(&system->text_renderer, 1);

  float sf = 1.f / 50.f;
  Vec3f scale_factor = {sf, sf, 1.f};
  uint32_t num_glyphs_total = 0;

  for (int i = 0; i < sys->entities.size; ++i) {

    DrawElementsIndirectCommand *draw_command = &system->draw_commands[i];
    DrawCommandDataText *draw_command_data = &system->draw_command_data[i];

    Entity entity = VEC_GET_T(&sys->entities, Entity, i);

    TextComponent *tc =
        PoolGetComponent(text_pool, TextComponent, entity.index);
    TransformComponent *xform =
        PoolGetComponent(transform_pool, TransformComponent, entity.index);

    Mat4x4 translate;
    Mat4x4 scale;
    mat4_identity(&translate);
    mat4_identity(&scale);

    Vec3f pos = {0};

    if (tc->flags & TEXT_COMPONENT_FLAG_SCREEN_SPACE) {
      pos = (Vec3f){.x = 0.f, .y = 3.f, .z = 0.f};
      mat4_identity(&draw_command_data->view_matrix);
    } else {
      pos = (Vec3f){.x = xform->pos.x, .y = xform->pos.y, 0.f};
      draw_command_data->view_matrix = system->view_matrix;
    }

    mat4_translate(&translate, &pos);
    mat4_scale(&scale, &scale_factor);

    draw_command_data->model_matrix = mat4_mul(&translate, &scale);
    draw_command_data->color = tc->color;

    // Quad uses top left and bottom right convention
    stbtt_aligned_quad quad = {0};

    float xpos = 0.f;
    float ypos = 0.f;

    // Make quads. 4 vertices per quad. 6 indices to render them
    // as GL_TRIANGLES. GL_QUADS doesn't exist in 4.6 core.
    // NOTE: Could use triangle strip or fan to reduce number of indices to 4
    uint32_t num_glyphs = 0;

    for (; num_glyphs < tc->len; ++num_glyphs) {
      stbtt_GetPackedQuad(packed_chars, TEXT_SYSTEM_ATLAS_PX_WIDTH,
                          TEXT_SYSTEM_ATLAS_PX_HEIGHT, tc->text[num_glyphs],
                          &xpos, &ypos, &quad, 0);

      /* quad.x0 *= 1.f / 800.f; */
      /* quad.y0 *= 1.f / 800.f; */
      /* quad.x1 *= 1.f / 800.f; */
      /* quad.y1 *= 1.f / 800.f; */

      quad.y0 *= -1.f;
      quad.y1 *= -1.f;

      // Flip y-axis
      //      quad.y0 = (quad.y1 - quad.y0);

      // NOTE: Corners must be mapped to match indices in index buffer
      //       assuming: LL, LR, UL, UR
      //    uint16_t index_data[] = [  0,  1,  2,  0,  2,  3 ]
      //                            [ UL, LL, LR, UL, LR, UR ]
      //                            [ LL, UL, UR

      // Lower left
      vbo_pos[0].x = quad.x0;
      vbo_pos[0].y = quad.y1;
      vbo_pos[0].z = 0.f;

      vbo_uv[0].x = quad.s0;
      vbo_uv[0].y = quad.t1;

      // Upper left
      vbo_pos[1].x = quad.x0;
      vbo_pos[1].y = quad.y0;
      vbo_pos[1].z = 0.f;

      vbo_uv[1].x = quad.s0;
      vbo_uv[1].y = quad.t0;

      // Lower Right
      vbo_pos[2].x = quad.x1;
      vbo_pos[2].y = quad.y0;
      vbo_pos[2].z = 0.f;

      vbo_uv[2].x = quad.s1;
      vbo_uv[2].y = quad.t0;

      // Upper Right
      vbo_pos[3].x = quad.x1;
      vbo_pos[3].y = quad.y1;
      vbo_pos[3].z = 0.f;

      vbo_uv[3].x = quad.s1;
      vbo_uv[3].y = quad.t1;

      vbo_pos += 4;
      vbo_uv += 4;
    }

    draw_command->base_instance = 0;
    draw_command->base_vertex = 0;
    draw_command->instance_count = 1;
    draw_command->count = num_glyphs * 6;
    draw_command->first_index = num_glyphs_total * 6;

    num_glyphs_total += num_glyphs;
  }

  renderer_unmap_vertex_buffer(&system->text_renderer, 0);
  renderer_unmap_vertex_buffer(&system->text_renderer, 1);

  /* glDisable(GL_CULL_FACE); */
  /* glDisable(GL_DEPTH_TEST); */
  glEnable(GL_BLEND);
  glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  CHECK_GL_ERROR();
  glNamedBufferSubData(system->text_renderer.multi_draw_indirect_buffer, 0,
                       sizeof(DrawElementsIndirectCommand) * sys->entities.size,
                       system->draw_commands);
  CHECK_GL_ERROR();

  glNamedBufferSubData(
      system->text_renderer.shader_storage_buffer_objects.buffer_object[0], 0,
      sizeof(DrawCommandDataText) * sys->entities.size,
      system->draw_command_data);
  CHECK_GL_ERROR();

  // DrawElements takes number of indices, NOT number of primitives.
  //  renderer_dispatch_indexed(&system->text_renderer, 0, sys->entities.size);
  glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_SHORT, 0, // *indirect *
                              sys->entities.size,
                              sizeof(DrawElementsIndirectCommand));
  CHECK_GL_ERROR();
}

TextSystem *text_system_create(Services *services) {
  TextSystem *system =
      ArenaAlloc(&global_static_allocator, 1, struct TextSystem);

  system_base_init((struct SystemBase *)system, TEXT_SYSTEM_BIT,
                   &text_system_update, TEXT_COMPONENT_BIT, services,
                   "TextSystem");

  system->base.services = *services;

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
  attrib_desc[1].relative_offset = 0;

  struct BindingPointDescriptor bp_desc[2];
  bp_desc[0].attrib_descriptors = &attrib_desc[0];
  bp_desc[0].num_attrib_descriptors = 1;
  bp_desc[0].binding_point_index = 0;
  bp_desc[0].offset = 0;
  bp_desc[0].stride = sizeof(float) * 3;

  bp_desc[1].attrib_descriptors = &attrib_desc[1];
  bp_desc[1].num_attrib_descriptors = 1;
  bp_desc[1].binding_point_index = 1;
  bp_desc[1].offset = 0;
  bp_desc[1].stride = sizeof(float) * 2;

  struct VertexBufferDescriptor vb_desc[2];
  vb_desc[0].binding_descriptors = &bp_desc[0];
  vb_desc[0].binding_point_count = 1;

  vb_desc[1].binding_descriptors = &bp_desc[1];
  vb_desc[1].binding_point_count = 1;

  const size_t num_vertices =
      TEXT_SYSTEM_GLYPH_MAX_PER_DRAW_CALL * TEXT_SYSTEM_GLYPH_VERTEX_COUNT;

  struct RendererParameters renderer_params;
  // 4 vertices make up one quad (glyph). 6 indices renders one quad.
  renderer_params.num_indices = num_vertices / 4 * 6;
  renderer_params.index_buffer_size =
      renderer_params.num_indices * sizeof(uint16_t);
  renderer_params.num_vertices = num_vertices;
  renderer_params.num_buffer_descriptors =
      ArrayLen(vb_desc, VertexBufferDescriptor);
  renderer_params.buffer_descriptors = vb_desc;

  renderer_init(&system->text_renderer, &renderer_params);

  // Draw command aux data
  renderer_ssbo_create(&system->text_renderer, 0, BO_INDEX_DRAW_COMMAND_DATA,
                       MAX_DRAW_INDIRECT_DRAW_COMMANDS *
                           sizeof(DrawCommandDataText));

  uint16_t *index_buffer =
      renderer_map_element_array_buffer(&system->text_renderer);

  // Indices are 6 at a time per quad, but the vertices they point
  // to are only 4, so we write 6 indices at a time, but the vertices
  // they reference at 4 at a time
  uint32_t vertex_index = 0;
  for (uint32_t i = 0; i < renderer_params.num_indices; i += 6) {
    index_buffer[i + 0] = vertex_index + 0;
    index_buffer[i + 1] = vertex_index + 1;
    index_buffer[i + 2] = vertex_index + 2;
    index_buffer[i + 3] = vertex_index + 0;
    index_buffer[i + 4] = vertex_index + 2;
    index_buffer[i + 5] = vertex_index + 3;

    vertex_index += 4;
  }

  renderer_unmap_element_array_buffer(&system->text_renderer);

  AssetId font_id;
  if (!assets_asset_id_get_by_name(services->assets, TEXT_SYSTEM_FONT_FILE_NAME,
                                   &font_id)) {
    LOG_EXIT("Failed to load font '%'s", TEXT_SYSTEM_FONT_FILE_NAME);
  }

  GLuint64 handle_font_atlas_texture =
      text_system_create_font_atlas(system, font_id);

  AssetId text_prog_id;
  if (!assets_asset_id_get_by_name(services->assets, "text.prog",
                                   &text_prog_id)) {
    LOG_EXIT("Failed to get text renderer program asset ID %zu", text_prog_id);
  }

  system->program_id = renderer_create_program(&system->text_renderer,
                                               services->assets, text_prog_id);

  if (!system->program_id) {
    LOG_EXIT("Failed to create text rendering shader program");
    return 0;
  }

  renderer_use(&system->text_renderer);
  CHECK_GL_ERROR();
  glUseProgram(system->program_id);
  CHECK_GL_ERROR();
  GLint loc_font_atlas = glGetUniformLocation(system->program_id, "font_atlas");
  CHECK_GL_ERROR();
  glUniformHandleui64ARB(loc_font_atlas, handle_font_atlas_texture);
  CHECK_GL_ERROR();

  system->loc_proj = glGetUniformLocation(system->program_id, "Proj");

  CHECK_GL_ERROR();
  return system;
}

void text_system_handle_camera_position_changed(struct SystemBase *system,
                                                struct Event e) {
  (void)e;
  TextSystem *text_sys = (TextSystem *)system;
  CameraUpdatedEventData *pos_changed_event = e.event_data;

  text_sys->view_projection_matrix = pos_changed_event->view_projection;
  text_sys->view_matrix = pos_changed_event->view;
  text_sys->projection_matrix = pos_changed_event->projection;
}
