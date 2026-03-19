#ifndef RENDERER_H
#define RENDERER_H

#include <windows.h>
#include <glad/glad.h>

#include "assetstore.h"
#include "log.h"
#include "math.h"

#include <stdint.h>

#define MAX_MATERIALS 10000
#define MAX_DRAW_INDIRECT_DRAW_COMMANDS 100000

#define BO_INDEX_POSITION 0
#define BO_INDEX_COLOR 1
#define BO_INDEX_UV 2

#define BO_INDEX_ELEMENT_ARRAY 16     // IBO
#define BO_INDEX_DRAW_INDIRECT 17     // Command buffer
#define BO_INDEX_DRAW_COMMAND_DATA 18 // gl_DrawID indexed array

#define BO_INDEX_MATERIALS 20
#define BO_INDEX_USER_0 21
#define BO_INDEX_USER_1 BO_INDEX_USER_0 + 1

#define BO_INDEX_MAX 32
#define VBO_MAX 16
#define SSBO_MAX 2

#define CHECK_GL_ERROR()                                                       \
  if (glGetError() != GL_NO_ERROR) {                                           \
    LOG_ERROR("GL error: %s:%d", __FUNCTION__, __LINE__);                      \
    exit(1);                                                                   \
  }

typedef struct {
  unsigned int count;
  unsigned int instance_count;
  unsigned int first;
  unsigned int base_instance;
} DrawArraysIndirectCommand;

typedef struct {
  unsigned int count;
  unsigned int instance_count;
  unsigned int first_index;
  int base_vertex;
  unsigned int base_instance;
} DrawElementsIndirectCommand;

typedef struct {
  GLuint64 handle;
  Vec4u8 color;
} Material;

/* typedef struct DrawCommandData { */
/*   Mat4x4 model; */
/*   Vec2u32 tile_tex_index; */
/*   Vec2u32 tile_tex_size; */
/*   unsigned int material_index; */
/* } DrawCommandData; */

struct Framebuffer {
  uint16_t width;
  uint16_t height;
};

struct VertexDataSource {
  void *src;
  size_t vertex_size_bytes;
};

struct Interleave {
  unsigned int num_attribs;
  struct VertexDataSource *attribs;

  void *dst;
  size_t element_count;
};

// Read this: Sepecifically the section 'Separate attribute format'
// https://www.khronos.org/opengl/wiki/Vertex_Specification
/*
  --------------------------------------------------
  | Standalone everything. Trivial.
  |-------------------------------------------------
  | VertexAttribute | Binding Point | VertexBuffer |
  --------------------------------------------------
  |      0          |       0       |     0        |
  --------------------------------------------------
  // Implementation example
  GLuint attribindex = 0;
  GLuint bindingindex = 0;
  GLintptr offset = 0;
  GLsizei stride = sizeof(AttributePosition);
  GLuint vbo;
  GLuint relative_offset = 0;

  glCreateBuffers(GL_VERTEX_BUFFER, &vbo);
  glEnableVertexArrayAttrib(vao, 0);
  glVertexArrayAttribBinding(vao, attribindex, bindingindex);
  glVertexArrayVertexBuffer(vao, bindingindex, vbo, stride);
  glVertexArrayAttribFormat(vao, attribindex, vec{1,2,3,4}, {GL_FLOAT,
  GL_BYTE...}, NORMALIZED?, relative_offset);

  --------------------------------------------------
  | 3 vertex attributes interleaved in the same vertex buffer
  | This is how I BELIEVE it works. Need to actually test this
  | with a shader that uses the 3 attributes in a test
  | Here, Vertex buffer stride is essentially sizeof(Vertex)
  | ABCABCABC
  |-------------------------------------------------
  | VertexAttribute | Binding Point | VertexBuffer |
  --------------------------------------------------
  |      0          |       0       |     0        |
  |      1          |       0       |     0        |
  |      2          |       0       |     0        |
  --------------------------------------------------
  // Implementation example

  GLsizei stride = sizeof(Attrib1) + sizeof(Attrib2) + sizeof(Attrib3);
  GLuint vbo;

  glCreateBuffers(GL_VERTEX_BUFFER, &vbo);
  glEnableVertexArrayAttrib(vao, 0);
  glEnableVertexArrayAttrib(vao, 1);
  glEnableVertexArrayAttrib(vao, 2);

  glVertexArrayAttribBinding(vao, 0, 0);
  glVertexArrayAttribBinding(vao, 1, 0);
  glVertexArrayAttribBinding(vao, 2, 0);

  GLintptr offset_1 = 0;
  GLintptr offset_2 = sizeof(Attrib1);
  GLintptr offset_3 = sizeof(Attrib2);

  // Stride here must be what exactly?
  glVertexArrayVertexBuffer(vao, bindingindex, vbo, offset_1, stride);
  glVertexArrayVertexBuffer(vao, bindingindex, vbo, offset_2, stride);
  glVertexArrayVertexBuffer(vao, bindingindex, vbo, offset_3, stride);

  glVertexArrayAttribFormat(vao, 0, vec{1,2,3,4}, {GL_FLOAT, GL_BYTE...},
  NORMALIZED?, 0); glVertexArrayAttribFormat(vao, 1, vec{1,2,3,4}, {GL_FLOAT,
  GL_BYTE...}, NORMALIZED?, 0); glVertexArrayAttribFormat(vao, 2, vec{1,2,3,4},
  {GL_FLOAT, GL_BYTE...}, NORMALIZED?, 0);


  |---------------------------------------------------------------
  | 3 vertex attributes sequentially in the same vertex buffer
  | AAABBBCCC.
  | Here, it is necessary to know vertex count to know offsets of each attribute
  |--------------------------------------------------------------------------------------
  | VertexAttribute | Binding Point | VertexBuffer | VAO      | stride | offset
  |----------------------------------------------------------------|--------------------------
  | 0 |      0      |       0       |     0        | vao_name | sizeof(Attrib0)
  | 0 | 1 |      1      |       1       |     0        | vao_name |
  sizeof(Attrib1) | NumVertex * sizeof(Attrib0) | 2 |      2      |       2 | 0
  | vao_name | sizeof(Attrib2) | NumVertex * sizeof(Attrib0) +
  NumVertex*sizeof(attrib1)
  |----------------------------------------------------------------------------------

  // Implementation example
  GLuint VERTEX_COUNT 1000
  GLuint attribindex = 0;
  GLuint bindingindex_1 = 0;
  GLuint bindingindex_2 = 1;
  GLuint bindingindex_3 = 2;

  GLuint attribindex = 0;
  GLuint bindingindex = 0;
  GLintptr offset = 0;
  GLsizei stride_1 = sizeof(Attrib1);
  GLsizei stride_2 = sizeof(Attrib2);
  GLsizei stride_3 = sizeof(Attrib3);

  GLsizei offset_1 = 0;
  GLsizei offset_2 = VERTEX_COUNT * sizeof(attrib2);
  GLsizei offset_3 = VERTEX_COUNT * sizeof(attrib3);

  GLuint vbo;

  glCreateBuffers(GL_VERTEX_BUFFER, &vbo);

  glEnableVertexArrayAttrib(vao, 0);
  glEnableVertexArrayAttrib(vao, 1);
  glEnableVertexArrayAttrib(vao, 2);

  glVertexArrayAttribBinding(vao, 0, bindingindex_1);
  glVertexArrayAttribBinding(vao, 1, bindingindex_2);
  glVertexArrayAttribBinding(vao, 2, bindingindex_3);

  glVertexArrayVertexBuffer(vao, bindingindex_1, vbo, offset_1, stride_1);
  glVertexArrayVertexBuffer(vao, bindingindex_2, vbo, offset_2, stride_2);
  glVertexArrayVertexBuffer(vao, bindingindex_3, vbo, offset_3, stride_3);

  glVertexArrayAttribFormat(vao, 0, vec{1,2,3,4}, {GL_FLOAT, GL_BYTE...},
  NORMALIZED?, 0); glVertexArrayAttribFormat(vao, 1, vec{1,2,3,4}, {GL_FLOAT,
  GL_BYTE...}, NORMALIZED?, 0); glVertexArrayAttribFormat(vao, 2, vec{1,2,3,4},
  {GL_FLOAT, GL_BYTE...}, NORMALIZED?, 0);
*/

struct VertexAttributeDescriptor {
  GLuint vertex_attribute;
  uint8_t element_count;
  GLenum element_type;
  GLboolean normalize;
  GLuint relative_offset; // I have no clue when this is useful
};

typedef struct BufferObjectBindings {
  GLuint binding_point_index[SSBO_MAX];
  GLuint buffer_object[SSBO_MAX];
} BufferObjectBindings;

typedef struct BindingPointDescriptor {
  GLuint binding_point_index;
  GLuint stride;
  GLintptr offset;
  unsigned int num_attrib_descriptors;
  struct VertexAttributeDescriptor *attrib_descriptors;
} BindingPointDescriptor;

typedef struct VertexBufferDescriptor {
  struct BindingPointDescriptor *binding_descriptors;
  unsigned int binding_point_count;
} VertexBufferDescriptor;

typedef struct RendererParameters {
  unsigned int num_vertices;
  uint32_t num_indices;
  int index_buffer_size;

  unsigned int num_buffer_descriptors;
  struct VertexBufferDescriptor *buffer_descriptors;
} RendererParameters;

typedef struct Renderer {
  GLuint vertex_array_object;

  GLuint element_array_buffer;

  unsigned int num_vertex_buffers;
  GLuint vertex_buffer_objects[VBO_MAX];
  size_t vertex_buffer_vertex_sizes[VBO_MAX];

  GLuint multi_draw_indirect_buffer;
  struct BufferObjectBindings shader_storage_buffer_objects;

  RendererParameters parameters;
  GLuint query_multi_draw_elements_indirect_time_elapsed;
} Renderer;

// Utility function. Interleave N disparate vertex attributes into single buffer
void interleave_attributes(struct Interleave *interleave);

// GL_BYTE, GL_SHORT, GL_INT, GL_FIXED, GL_FLOAT, GL_HALF_FLOAT, and GL_DOUBLE
// GLbyte,  GLshort,  GLint,  GLfixed,  GLfloat,  GLhalf,        and GLdouble
// GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, and GL_UNSIGNED_INT
// GLubyte,          GLushort,          and GLuint
// TODO: Packed types if needed
unsigned int gl_type_byte_size(GLenum type);

GLuint calc_stride(struct VertexAttributeDescriptor *descriptors, int count);

void renderer_global_init();

void renderer_init(struct Renderer *renderer,
                   struct RendererParameters *params);

// Try to gather up all the state changes needed to use the renderer in one
// place
void renderer_use(struct Renderer *renderer);

void renderer_multi_draw_elements_indirect(struct Renderer *renderer,
                                           uint32_t draw_command_count);

void renderer_dispatch_indexed(struct Renderer *renderer, uint32_t offset,
                               uint32_t count);

void renderer_write_element_array_buffer(struct Renderer *renderer,
                                         size_t offset, size_t size,
                                         void *data);

void renderer_ssbo_create(struct Renderer *renderer, int index,
                          int binding_point, GLsizeiptr buffer_size);

void renderer_ssbo_write(struct Renderer *renderer, int index, GLintptr offset,
                         void *data, size_t size);

// Create a bindless texture and make it resident. Returns the bindless
// texture handle.
GLuint64 renderer_create_texture_bindless(struct Renderer *renderer,
                                          GLenum texture_type,
                                          GLenum texture_format,
                                          uint16_t width_px, uint16_t height_px,
                                          uint16_t depth_px,
                                          unsigned char *texture_data);

void *renderer_map_vertex_buffer(struct Renderer *renderer, uint32_t index);

void renderer_unmap_vertex_buffer(struct Renderer *renderer, uint32_t index);

void *renderer_map_element_array_buffer(struct Renderer *renderer);

void renderer_unmap_element_array_buffer(struct Renderer *renderer);

void renderer_log_state();

GLuint renderer_create_program(Renderer *renderer, Assets *assets,
                               AssetId program_id);

#endif
