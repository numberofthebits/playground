#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/assetstore.h"
#include "core/ecs.h"
#include "core/math.h"
#include "core/renderer.h"
#include "core/systembase.h"
#include "core/types.h"
#include "core/vec.h"

extern "C" {
#include <glad/glad.h>
}

#include <stdint.h>

typedef struct OrthoCamera {
  Mat4x4 projection;
  Mat4x4 view;
  Mat4x4 view_projection;
  Rectf rect;
  float aspect_ratio;
  float scale;
} OrthoCamera;

typedef struct {
  Mat4x4 model;                // 64 bytes
  Vec2f tex_coord_offset;      // 8 bytes
  Vec2f tex_coord_scale;       // 8 bytes
  unsigned int material_index; // 4 bytes
  char padding[4];
} DrawCommandDataTiled;

struct RenderSystem {
  struct SystemBase base;
  DrawElementsIndirectCommand
      draw_commands_elements[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
  DrawCommandDataTiled draw_command_data[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
  HashMap material_asset_index_mapping;
  HashMap textures;
  // Program asset ID => GL program handle
  HashMap programs;
  // Keep track of which materials we've seen as
  // Key: AssetId, Value: Material SSBO vector index
  Vec materials;
  GLuint buffer_objects[32];
  struct Assets *assets;
  struct Framebuffer main_framebuffer;
  struct Renderer *tile_renderer;
  struct Renderer *debug_renderer;
  GLint loc_view_projection;
  GLint loc_view;
  GLint loc_projection;
  GLuint program_handle;
  OrthoCamera camera;
  GLuint query_time_elapsed;
};
typedef struct RenderSystem RenderSystem;

RenderSystem *render_system_create(Services *services, int window_w,
                                   int window_h, int screen_w, int screen_h);

void render_system_load_assets(RenderSystem *system, Asset *assets,
                               size_t asset_count);

uint64_t render_system_create_texture(RenderSystem *system, void *data,
                                      ImageMeta *meta);

void render_system_framebuffer_size_changed(RenderSystem *render_system,
                                            uint16_t width_px,
                                            uint16_t height_px);

void render_system_handle_framebuffer_size_changed(struct SystemBase *base,
                                                   struct Event e);

void render_system_handle_camera_position_changed(struct SystemBase *system,
                                                  struct Event e);

void render_system_handle_hit_detection(struct SystemBase *base,
                                        struct Event e);

void render_system_debug(struct RenderSystem *system, Registry *registry);

#endif
