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

  return;
  (void)frame_nr;

  TextSystem *system = (TextSystem *)sys;

  renderer_use(&system->text_renderer);
  glUseProgram(system->program_id);
  glUniformMatrix4fv(system->loc_proj, 1, GL_FALSE,
                     system->view_projection.data);

  Pool *text_pool = registry_get_pool(registry, TEXT_COMPONENT_BIT);
  Pool *transform_pool = registry_get_pool(registry, TRANSFORM_COMPONENT_BIT);

  Vec3f *vbo_pos = renderer_map_vertex_buffer(&system->text_renderer, 0);
  Vec2f *vbo_uv = renderer_map_vertex_buffer(&system->text_renderer, 1);
  Vec3f *vbo_col = renderer_map_vertex_buffer(&system->text_renderer, 2);

  uint32_t num_glyphs = 0;

  float sf = 1.f / 512.f;
  Vec3f scale_factor = {sf, sf, 1.f};
  (void)scale_factor;

  for (int i = 0; i < sys->entities.size; ++i) {
    Entity entity = VEC_GET_T(&sys->entities, Entity, i);
    TextComponent *tc =
        PoolGetComponent(text_pool, TextComponent, entity.index);
    TransformComponent *xform =
        PoolGetComponent(transform_pool, TransformComponent, entity.index);
    (void)xform;

    Mat4x4 translate;
    Mat4x4 scale;
    (void)translate;
    (void)scale;
    mat4_identity(&translate);
    mat4_identity(&scale);
    Vec3f pos_neg = {xform->pos.x, xform->pos.y, 0.f};
    (void)pos_neg;
    mat4_translate(&translate, &pos_neg);
    mat4_scale(&scale, &scale_factor);

    Mat4x4 model = mat4_mul(&scale, &translate);
    mat4_identity(&model);

    glUniformMatrix4fv(system->loc_model, 1, GL_FALSE, model.data);

    // Quad uses top left and bottom right convention
    stbtt_aligned_quad quad = {0};

    // Make quads. 4 vertices per quad. 6 indices to render them
    // as GL_TRIANGLES

    float xpos = 0.f;
    float ypos = 0.f;

    for (uint32_t c = 0; c < tc->len; ++c) {
      stbtt_GetPackedQuad(packed_chars, TEXT_SYSTEM_ATLAS_PX_WIDTH,
                          TEXT_SYSTEM_ATLAS_PX_HEIGHT, tc->text[c], &xpos,
                          &ypos, &quad, 0);

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

      /* Vec4f a = {quad.x0, quad.y1, 0.f, 1.f}; */
      /* Vec4f res = mat4_mul_vec(&model, &a); */
      /* (void)res; */

      vbo_uv[0].x = quad.s0;
      vbo_uv[0].y = quad.t1;

      vbo_col[0].x = 1.f;
      vbo_col[0].y = 1.f;
      vbo_col[0].z = 1.f;

      // Upper left
      vbo_pos[1].x = quad.x0;
      vbo_pos[1].y = quad.y0;
      vbo_pos[1].z = 0.f;

      vbo_uv[1].x = quad.s0;
      vbo_uv[1].y = quad.t0;

      vbo_col[1].x = 1.f;
      vbo_col[1].y = 1.f;
      vbo_col[1].z = 1.f;

      // Lower Right
      vbo_pos[2].x = quad.x1;
      vbo_pos[2].y = quad.y0;
      vbo_pos[2].z = 0.f;

      vbo_uv[2].x = quad.s1;
      vbo_uv[2].y = quad.t0;

      vbo_col[2].x = 1.f;
      vbo_col[2].y = 1.f;
      vbo_col[2].z = 1.f;

      // Upper Right
      vbo_pos[3].x = quad.x1;
      vbo_pos[3].y = quad.y1;
      vbo_pos[3].z = 0.f;

      vbo_uv[3].x = quad.s1;
      vbo_uv[3].y = quad.t1;

      vbo_col[3].x = 1.f;
      vbo_col[3].y = 1.f;
      vbo_col[3].z = 1.f;

      vbo_pos += 4;
      vbo_uv += 4;
      vbo_col += 4;
      ++num_glyphs;
    }
  }

  renderer_unmap_vertex_buffer(&system->text_renderer, 0);
  renderer_unmap_vertex_buffer(&system->text_renderer, 1);
  renderer_unmap_vertex_buffer(&system->text_renderer, 2);

  /* glDisable(GL_CULL_FACE); */
  /* glDisable(GL_DEPTH_TEST); */
  /* glEnable(GL_BLEND); */
  /* glBlendFunci(0, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); */

  // DrawElements takes number of indices, NOT number of primitives.
  //  renderer_dispatch_indexed(&system->text_renderer, 0, num_glyphs * 6);
  (void)num_glyphs;
}

TextSystem *text_system_create(Services *services) {
  TextSystem *system =
      ArenaAlloc(&global_static_allocator, 1, struct TextSystem);

  system_base_init(
      (struct SystemBase *)system, TEXT_SYSTEM_BIT, &text_system_update,
      TEXT_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, services, "TextSystem");

  system->base.services = *services;

  struct VertexAttributeDescriptor attrib_desc[3];
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

  attrib_desc[2].vertex_attribute = 2;
  attrib_desc[2].element_count = 3;
  attrib_desc[2].element_type = GL_FLOAT;
  attrib_desc[2].normalize = GL_FALSE;
  attrib_desc[2].relative_offset = 0;

  struct BindingPointDescriptor bp_desc[3];
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

  bp_desc[2].attrib_descriptors = &attrib_desc[2];
  bp_desc[2].num_attrib_descriptors = 1;
  bp_desc[2].binding_point_index = 2;
  bp_desc[2].offset = 0;
  bp_desc[2].stride = sizeof(float) * 3;

  struct VertexBufferDescriptor vb_desc[3];
  vb_desc[0].binding_descriptors = &bp_desc[0];
  vb_desc[0].binding_point_count = 1;

  vb_desc[1].binding_descriptors = &bp_desc[1];
  vb_desc[1].binding_point_count = 1;

  vb_desc[2].binding_descriptors = &bp_desc[2];
  vb_desc[2].binding_point_count = 1;

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

  system->loc_proj = glGetUniformLocation(system->program_id, "ViewProj");
  system->loc_model = glGetUniformLocation(system->program_id, "Model");
  /* Mat4x4 proj = ortho(1.f, -1.f, 400, -400, -400, 400); */

  /* Mat4x4 scale; */
  /* mat4_identity(&scale); */

  /* Vec3f scale_factor = {.x = 1.f / 800.f, .y = 1.f / 800.f, .z = 0.f}; */
  /* mat4_scale(&scale, &scale_factor); */

  /* Mat4x4 lol = mat4_mul(&proj, &scale); */
  /* (void)lol; */

  CHECK_GL_ERROR();
  return system;
}

void text_system_handle_camera_position_changed(struct SystemBase *system,
                                                struct Event e) {
  (void)e;
  TextSystem *text_sys = (TextSystem *)system;
  CameraUpdatedEventData *pos_changed_event = e.event_data;

  text_sys->view_projection = pos_changed_event->projection;
}
