#ifndef DEBUG_RENDERER_H
#define DEBUG_RENDERER_H

#include "render_system.h"
#include <core/log.h>
#include <core/math.h>

#include <glad/glad.h>

#include <stdint.h>

#define MAX_MATERIALS 10000
#define MAX_DRAW_INDIRECT_DRAW_COMMANDS 10000

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

typedef struct {
  Mat4x4 model;                // 64 bytes
  Vec2f tex_coord_offset;      // 8 bytes
  Vec2f tex_coord_scale;       // 8 bytes
  unsigned int material_index; // 4 bytes
  char padding[12];
} DrawCommandDataTiled;

struct Framebuffer {
  int width;
  int height;
};

// When interleaving vertex attributes data,
// all we really care about is bytes. Size, stride, alignment.
struct VertexDataSource {
    void* src;
    size_t vertex_size_bytes;
};

struct Interleave {
    unsigned int num_attribs;
    struct VertexDataSource* attribs;
    
    void* dst;
    size_t element_count;
};

struct Interleave3 {
    void* src_0;
    size_t vertex_size_0;

    void* src_1;
    size_t vertex_size_1;

    void* src_2;
    size_t vertex_size_2;

    void* dst;
    size_t element_count;
};

inline static void interleave_attributes(struct Interleave* interleave) {
    ptrdiff_t write_offset = 0;
    for (size_t i = 0; i < interleave->element_count; ++i) {
	for (size_t j = 0; j < interleave->num_attribs; ++j) {
	    memcpy(interleave->dst + write_offset,
		   interleave->attribs[j].src + i * interleave->attribs[j].vertex_size_bytes,
		   interleave->attribs[j].vertex_size_bytes);

	    write_offset += interleave->attribs[j].vertex_size_bytes;
	}
    }
}

inline static void interleave_attributes_3(struct Interleave3* params ) {

    ptrdiff_t write_offset = 0;
    for (size_t i = 0; i < params->element_count; ++i) {
	memcpy(params->dst + write_offset,
	       params->src_0 + i * params->vertex_size_0,
	       params->vertex_size_0);

	write_offset += params->vertex_size_0;
	
	memcpy(params->dst + write_offset,
	       params->src_1 + i * params->vertex_size_1,
	       params->vertex_size_1);

	write_offset += params->vertex_size_1;

	memcpy(params->dst + write_offset,
	       params->src_2 + i * params->vertex_size_2,
	       params->vertex_size_2);

	write_offset += params->vertex_size_2;
    }
}

// Describing multiple attributes in same buffer object
// - Are they sequentially packed or interleaved
//   - Is there any other way we would want to do this? Padding?
//   - Sequentially defined vertex array attributes
//     - BufferObject Handle
//     - Array of {offset, stride,
//     - Format specifier {number of elements, type of elements, normalized,
//     *RELATIVE OFFSET??*}
//         (relativeoffset is the offset, measured in basic machine units of the
//         first element
//          relative to the start of the vertex buffer binding this attribute
//          fetches from.)
//

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
  | 0 |      0      |       0       |     0        | vao_name | sizeof(Attrib0) | 0
  | 1 |      1      |       1       |     0        | vao_name | sizeof(Attrib1) | NumVertex * sizeof(Attrib0)
  | 2 |      2      |       2       |     0        | vao_name | sizeof(Attrib2) | NumVertex * sizeof(Attrib0) + NumVertex*sizeof(attrib1)
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

struct BindingPointDescriptor {
  GLuint binding_point_index;
  GLuint stride;
  GLintptr offset;
  unsigned int num_attrib_descriptors;
  struct VertexAttributeDescriptor *attrib_descriptors;
};

struct VertexBufferDescriptor {
  struct BindingPointDescriptor *binding_descriptors;
  unsigned int binding_point_count;
};

struct RendererParameters {
  unsigned int num_vertices;
  int index_buffer_size;

  unsigned int num_buffer_descriptors;
  struct VertexBufferDescriptor *buffer_descriptors;
};

struct Renderer {
  GLuint vertex_array_object;
  GLuint element_array_buffer;

  unsigned int num_vertex_buffers;
  GLuint vertex_buffer_objects[VBO_MAX];

  GLuint multi_draw_indirect_buffer;
  GLuint shader_storage_buffer_objects[SSBO_MAX];
  /* GLuint draw_data; */

  /* GLuint material_buffer; */
};

// GL_BYTE, GL_SHORT, GL_INT, GL_FIXED, GL_FLOAT, GL_HALF_FLOAT, and GL_DOUBLE
// GLbyte,  GLshort,  GLint,  GLfixed,  GLfloat,  GLhalf,        and GLdouble
// GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, and GL_UNSIGNED_INT
// GLubyte,          GLushort,          and GLuint
// TODO: Packed types if needed
inline static unsigned int gl_type_byte_size(GLenum type) {
  unsigned int element_size = 0;
  switch (type) {
  case GL_BYTE:
    element_size = sizeof(GLbyte);
    break;
  case GL_SHORT:
    element_size = sizeof(GLshort);
    break;
  case GL_INT:
    element_size = sizeof(GLint);
    break;
  case GL_FIXED:
    element_size = sizeof(GLfixed);
    break;
  case GL_FLOAT:
    element_size = sizeof(GLfloat);
    break;
  case GL_HALF_FLOAT:
    element_size = sizeof(GLhalf);
    break;
  case GL_DOUBLE:
    element_size = sizeof(GLdouble);
    break;
  case GL_UNSIGNED_BYTE:
    element_size = sizeof(GLubyte);
    break;
  case GL_UNSIGNED_SHORT:
    element_size = sizeof(GLushort);
    break;
  case GL_UNSIGNED_INT:
    element_size = sizeof(GLuint);
    break;
  default:
    LOG_EXIT("Unhandled vertex array element type");
  }

  return element_size;
}

static inline GLuint calc_stride(struct VertexAttributeDescriptor *descriptors,
                                 int count) {
  GLuint stride = 0;
  for (int i = 0; i < count; ++i) {
    stride += descriptors[i].element_count *
              gl_type_byte_size(descriptors[i].element_type);
  }

  return stride;
}

static inline void renderer_init(struct Renderer *renderer,
                                 struct RendererParameters *params) {
  memset(renderer, 0x0, sizeof(struct Renderer));

  glCreateVertexArrays(1, &renderer->vertex_array_object);
  glBindVertexArray(renderer->vertex_array_object);

  glCreateBuffers(params->num_buffer_descriptors,
                  renderer->vertex_buffer_objects);
  renderer->num_vertex_buffers = params->num_buffer_descriptors;

  // TODO: Clean this up. We can ask for all the buffer objects in one call,
  // then then
  //       put them into named variables for syntactical separation of vertex
  //       buffer objects and other types of buffer objects.
  glCreateBuffers(2, renderer->shader_storage_buffer_objects);
  glCreateBuffers(1, &renderer->multi_draw_indirect_buffer);
  glCreateBuffers(1, &renderer->element_array_buffer);

  for (unsigned int i = 0; i < renderer->num_vertex_buffers; ++i) {
    struct VertexBufferDescriptor buffer_desc = params->buffer_descriptors[i];
    unsigned int num_binding_points = buffer_desc.binding_point_count;

    for (unsigned int j = 0; j < num_binding_points; ++j) {
      struct BindingPointDescriptor binding_point_desc =
          buffer_desc.binding_descriptors[j];
      unsigned int num_attribs = binding_point_desc.num_attrib_descriptors;

      glVertexArrayVertexBuffer(
          renderer->vertex_array_object, binding_point_desc.binding_point_index,
          renderer->vertex_buffer_objects[i], binding_point_desc.offset,
          binding_point_desc.stride);

      GLsizeiptr vertex_size_bytes = 0;

      for (unsigned int k = 0; k < num_attribs; ++k) {
        struct VertexAttributeDescriptor attrib_desc =
            binding_point_desc.attrib_descriptors[k];

        glEnableVertexArrayAttrib(renderer->vertex_array_object,
                                  attrib_desc.vertex_attribute);

        glVertexArrayAttribBinding(renderer->vertex_array_object,
                                   attrib_desc.vertex_attribute,
                                   binding_point_desc.binding_point_index);

        glVertexArrayAttribFormat(
            renderer->vertex_array_object, attrib_desc.vertex_attribute,
            attrib_desc.element_count, attrib_desc.element_type,
            attrib_desc.normalize, attrib_desc.relative_offset);

        vertex_size_bytes += attrib_desc.element_count *
                             gl_type_byte_size(attrib_desc.element_type);
      }

      glNamedBufferStorage(renderer->vertex_buffer_objects[i],
                           vertex_size_bytes * params->num_vertices, NULL,
                           GL_DYNAMIC_STORAGE_BIT);
    }
  }

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, renderer->multi_draw_indirect_buffer);

  if (params->index_buffer_size) {
      GLuint buffer_size = sizeof(DrawElementsIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS;
      glNamedBufferStorage(renderer->multi_draw_indirect_buffer,
			   buffer_size,
			   NULL,
			   GL_DYNAMIC_STORAGE_BIT);

      // Binding should not be necessary

      glVertexArrayElementBuffer(renderer->vertex_array_object,
				 renderer->element_array_buffer);

      glNamedBufferStorage(renderer->element_array_buffer, params->index_buffer_size,
			   0,
			   GL_DYNAMIC_STORAGE_BIT);
  } else {
      GLuint buffer_size = sizeof(DrawArraysIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS;
      glNamedBufferStorage(renderer->multi_draw_indirect_buffer,
			   buffer_size * MAX_DRAW_INDIRECT_DRAW_COMMANDS,
			   NULL,
			   GL_DYNAMIC_STORAGE_BIT);
  }

  CHECK_GL_ERROR();
}

static inline void
renderer_write_element_array_buffer(struct Renderer *renderer,
				    size_t offset,
                                    size_t size,
				    void *data) {
  glNamedBufferSubData(renderer->element_array_buffer, offset, size, data);
  CHECK_GL_ERROR();
}

static inline void renderer_ssbo_create(struct Renderer *renderer,
					int index,
                                        GLsizeiptr buffer_size) {
  if (index >= SSBO_MAX) {
    LOG_EXIT("SSBO index exceeds max SSBOs {d}", SSBO_MAX);
  }

  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, BO_INDEX_USER_0 + index,
                   renderer->shader_storage_buffer_objects[index]);

  glNamedBufferStorage(renderer->shader_storage_buffer_objects[index],
                       buffer_size, NULL,
		       GL_DYNAMIC_STORAGE_BIT);
  CHECK_GL_ERROR();
}

static inline void renderer_ssbo_write(struct Renderer *renderer,
				       int index,
                                       GLintptr offset,
				       void *data,
                                       size_t size) {
  if (index >= SSBO_MAX || renderer->shader_storage_buffer_objects[0]) {
    LOG_EXIT("SSBO index exceeds max SSBOs {d}", SSBO_MAX);
  }

  glNamedBufferSubData(renderer->shader_storage_buffer_objects[index], offset,
                       size, data);
  CHECK_GL_ERROR();
}

#endif
