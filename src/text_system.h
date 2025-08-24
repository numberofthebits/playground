#ifndef TEXT_SYSTEM_H
#define TEXT_SYSTEM_H

#include <core/renderer.h>
#include <core/systembase.h>

typedef struct DrawCommandDataText {
  Mat4x4 view_matrix;
  Mat4x4 model_matrix;
  Vec4u8 color;
} DrawCommandDataText;

typedef struct TextSystem {
  SystemBase base;
  Renderer text_renderer;
  GLuint program_id;
  Mat4x4 view_projection_matrix;
  Mat4x4 view_matrix;
  Mat4x4 projection_matrix;
  GLint loc_viewproj;
  GLint loc_view;
  GLint loc_proj;
  DrawElementsIndirectCommand draw_commands[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
  DrawCommandDataText draw_command_data[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
} TextSystem;

TextSystem *text_system_create(Services *services);

void text_system_update(Registry *registry, struct SystemBase *sys,
                        size_t frame_nr);

void text_system_handle_camera_position_changed(struct SystemBase *system,
                                                struct Event e);

#endif
