#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "core/assetstore.h"
#include "core/ecs.h"
#include "core/math.h"
#include "core/renderer.h"
#include "core/systembase.h"
#include "core/types.h"
#include "core/vec.h"

#include <glad/glad.h>

#include <stdint.h>

typedef struct {
  Vec program_ids;
  Vec material_ids;
} PreparedResources;

struct OrthoCamera {
  Mat4x4 projection;
  Mat4x4 view;
  Rectf rect;
  float aspect_ratio;
  float scale;
};

struct RenderSystem {
  struct SystemBase base;
  DrawElementsIndirectCommand
      draw_commands_elements[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
  DrawArraysIndirectCommand draw_commands_arrays;
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
  struct OrthoCamera camera;
};
typedef struct RenderSystem RenderSystem;

void render_system_global_init();

RenderSystem *render_system_create(Services *services, int window_w,
                                   int window_h, int screen_w, int screen_h);

void render_system_prepare_resources(RenderSystem *system,
                                     PreparedResources *resources);

uint64_t render_system_create_texture(RenderSystem *system, void *data,
                                      ImageMeta *meta);

void render_system_frame_buffer_size_changed(RenderSystem *render_system,
                                             int width, int height);

void render_system_handle_camera_position_changed(struct SystemBase *system,
                                                  struct Event e);

void render_system_debug(struct RenderSystem *system, Registry *registry);

#endif
