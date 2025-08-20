#include "renderer.h"

void interleave_attributes(struct Interleave *interleave) {
  ptrdiff_t write_offset = 0;
  for (size_t i = 0; i < interleave->element_count; ++i) {
    for (size_t j = 0; j < interleave->num_attribs; ++j) {
      memcpy((char *)interleave->dst + write_offset,
             (char *)interleave->attribs[j].src +
                 i * interleave->attribs[j].vertex_size_bytes,
             interleave->attribs[j].vertex_size_bytes);

      write_offset += interleave->attribs[j].vertex_size_bytes;
    }
  }
}

// GL_BYTE, GL_SHORT, GL_INT, GL_FIXED, GL_FLOAT, GL_HALF_FLOAT, and GL_DOUBLE
// GLbyte,  GLshort,  GLint,  GLfixed,  GLfloat,  GLhalf,        and GLdouble
// GL_UNSIGNED_BYTE, GL_UNSIGNED_SHORT, and GL_UNSIGNED_INT
// GLubyte,          GLushort,          and GLuint
// TODO: Packed types if needed
unsigned int gl_type_byte_size(GLenum type) {
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

GLuint calc_stride(struct VertexAttributeDescriptor *descriptors, int count) {
  GLuint stride = 0;
  for (int i = 0; i < count; ++i) {
    stride += descriptors[i].element_count *
              gl_type_byte_size(descriptors[i].element_type);
  }

  return stride;
}

void renderer_init(struct Renderer *renderer,
                   struct RendererParameters *params) {
  memset(renderer, 0x0, sizeof(struct Renderer));

  glCreateVertexArrays(1, &renderer->vertex_array_object);
  glBindVertexArray(renderer->vertex_array_object);

  glCreateBuffers(params->num_buffer_descriptors,
                  renderer->vertex_buffer_objects);
  renderer->num_vertex_buffers = params->num_buffer_descriptors;

  // TODO: Clean this up. We can ask for all the buffer objects in one call,
  // then put them into named variables for syntactical separation of vertex
  // buffer objects and other types of buffer objects.
  glCreateBuffers(2, renderer->shader_storage_buffer_objects.buffer_object);
  glCreateBuffers(1, &renderer->multi_draw_indirect_buffer);
  glCreateBuffers(1, &renderer->element_array_buffer);

  for (unsigned int i = 0; i < renderer->num_vertex_buffers; ++i) {
    struct VertexBufferDescriptor buffer_desc = params->buffer_descriptors[i];
    unsigned int num_binding_points = buffer_desc.binding_point_count;

    GLsizeiptr vertex_size_bytes = 0;

    for (unsigned int j = 0; j < num_binding_points; ++j) {
      struct BindingPointDescriptor binding_point_desc =
          buffer_desc.binding_descriptors[j];
      unsigned int num_attribs = binding_point_desc.num_attrib_descriptors;

      glVertexArrayVertexBuffer(
          renderer->vertex_array_object, binding_point_desc.binding_point_index,
          renderer->vertex_buffer_objects[i], binding_point_desc.offset,
          binding_point_desc.stride);

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
    }

    glNamedBufferStorage(renderer->vertex_buffer_objects[i],
                         vertex_size_bytes * params->num_vertices, NULL,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);

    renderer->vertex_buffer_vertex_sizes[i] = vertex_size_bytes;
  }

  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, renderer->multi_draw_indirect_buffer);

  if (params->index_buffer_size) {
    GLuint buffer_size =
        sizeof(DrawElementsIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS;
    glNamedBufferStorage(renderer->multi_draw_indirect_buffer, buffer_size,
                         NULL, GL_DYNAMIC_STORAGE_BIT);

    // Binding should not be necessary

    glVertexArrayElementBuffer(renderer->vertex_array_object,
                               renderer->element_array_buffer);

    glNamedBufferStorage(renderer->element_array_buffer,
                         params->index_buffer_size, 0,
                         GL_DYNAMIC_STORAGE_BIT | GL_MAP_WRITE_BIT);
  } else {
    GLuint buffer_size =
        sizeof(DrawArraysIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS;
    glNamedBufferStorage(renderer->multi_draw_indirect_buffer,
                         buffer_size * MAX_DRAW_INDIRECT_DRAW_COMMANDS, NULL,
                         GL_DYNAMIC_STORAGE_BIT);
  }

  CHECK_GL_ERROR();

  renderer->parameters = *params;
}

void renderer_use(struct Renderer *renderer) {
  glBindVertexArray(renderer->vertex_array_object);

  for (int i = 0; i < SSBO_MAX; ++i) {
    glBindBufferBase(
        GL_SHADER_STORAGE_BUFFER,
        renderer->shader_storage_buffer_objects.binding_point_index[i],
        renderer->shader_storage_buffer_objects.buffer_object[i]);
  }

  // Note: There's probably other relevant state that isn't implicitly bound via
  // vertex array object.
}

void renderer_dispatch_indexed(struct Renderer *renderer, uint32_t offset,
                               uint32_t count) {
  (void)renderer;
  uint32_t min = offset;
  uint32_t max = offset + count;
  uint32_t num_elements = count - offset;

  glDrawRangeElements(GL_TRIANGLES, min, max, num_elements, GL_UNSIGNED_SHORT,
                      0); // use bound index buffer
}

void renderer_write_element_array_buffer(struct Renderer *renderer,
                                         size_t offset, size_t size,
                                         void *data) {
  glNamedBufferSubData(renderer->element_array_buffer, offset, size, data);
  CHECK_GL_ERROR();
}

void renderer_ssbo_create(struct Renderer *renderer, int index,
                          int binding_point, GLsizeiptr buffer_size) {
  renderer->shader_storage_buffer_objects.binding_point_index[index] =
      binding_point;

  glNamedBufferStorage(
      renderer->shader_storage_buffer_objects.buffer_object[index], buffer_size,
      NULL, GL_DYNAMIC_STORAGE_BIT);
  CHECK_GL_ERROR();
}

void renderer_ssbo_write(struct Renderer *renderer, int index, GLintptr offset,
                         void *data, size_t size) {
  if (index >= SSBO_MAX) {
    LOG_EXIT("SSBO index exceeds max SSBOs {d}", SSBO_MAX);
  }

  glNamedBufferSubData(
      renderer->shader_storage_buffer_objects.buffer_object[index], offset,
      size, data);
  CHECK_GL_ERROR();
}

void renderer_log_state() {
  int be_rgb;
  glGetIntegerv(GL_BLEND_EQUATION_RGB, &be_rgb);

  int be_alpha;
  glGetIntegerv(GL_BLEND_EQUATION_ALPHA, &be_alpha);

  if (be_rgb == GL_FUNC_ADD) {
    LOG_INFO("ADD");
  }
  if (be_alpha == GL_FUNC_ADD) {
    LOG_INFO("ADD");
  }

  /* GLint arr = 99; */
  /* glGetNamedBufferParameteriv(system->text_renderer.vertex_buffer_objects[0],
   */
  /*                             GL_BUFFER_MAPPED, &arr); */
  /* arr = 99; */
  /* glGetNamedBufferParameteriv(system->text_renderer.vertex_buffer_objects[1],
   */
  /*                             GL_BUFFER_MAPPED, &arr); */
}

GLuint64 renderer_create_texture_bindless(struct Renderer *renderer,
                                          GLenum texture_type,
                                          GLenum texture_format,
                                          uint16_t width_px, uint16_t height_px,
                                          uint16_t depth_px,
                                          unsigned char *texture_data) {
  (void)depth_px;
  (void)renderer;

  GLuint tex_handle;
  glCreateTextures(texture_type, 1, &tex_handle);
  CHECK_GL_ERROR();

  GLenum internal_fmt = 0;
  switch (texture_format) {
  case GL_RED:
    internal_fmt = GL_R8;
    break;
  case GL_RGB:
    internal_fmt = GL_RGB8;
    break;
  case GL_RGBA:
    internal_fmt = GL_RGBA8;
    break;
  default:
    LOG_EXIT("Unknown GL texture format %d", texture_format);
  }

  int levels = 1;
  int level = 0;
  int xoffset = 0;
  int yoffset = 0;
  GLenum data_type = GL_UNSIGNED_BYTE;

  switch (texture_type) {
  case GL_TEXTURE_2D:
    glTextureStorage2D(tex_handle, levels, internal_fmt, width_px, height_px);
    CHECK_GL_ERROR();

    glTextureSubImage2D(tex_handle, level, xoffset, yoffset, width_px,
                        height_px, texture_format, data_type, texture_data);

    break;
  default:
    LOG_EXIT("Unhandled texture dimensions: Type %d", texture_type);
  }
  CHECK_GL_ERROR();

  // Hack set up a texture unit

  glTextureParameteri(tex_handle, GL_TEXTURE_MIN_FILTER,
                      GL_LINEAR_MIPMAP_LINEAR);
  glTextureParameteri(tex_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTextureParameteri(tex_handle, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTextureParameteri(tex_handle, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glBindTexture(GL_TEXTURE_2D, tex_handle);
  glGenerateTextureMipmap(tex_handle);

  CHECK_GL_ERROR();

  GLuint64 bindless_handle = glGetTextureHandleARB(tex_handle);
  glMakeTextureHandleResidentARB(bindless_handle);

  CHECK_GL_ERROR();

  glBindTexture(GL_TEXTURE_2D, 0);
  CHECK_GL_ERROR();

  return bindless_handle;
}

void *renderer_map_vertex_buffer(struct Renderer *renderer, uint32_t index) {
  GLint min_align = 0;
  glGetIntegerv(GL_MIN_MAP_BUFFER_ALIGNMENT, &min_align);
  size_t num_bytes = renderer->parameters.num_vertices *
                     renderer->vertex_buffer_vertex_sizes[index];
  void *mapped_buffer_ptr = glMapNamedBufferRange(
      renderer->vertex_buffer_objects[index], 0, num_bytes,
      GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  CHECK_GL_ERROR();

  return mapped_buffer_ptr;
}

void renderer_unmap_vertex_buffer(struct Renderer *renderer, uint32_t index) {
  if (glUnmapNamedBuffer(renderer->vertex_buffer_objects[index]) != GL_TRUE) {
    LOG_EXIT("Failed to unmap vertex array buffer");
  }
  CHECK_GL_ERROR();
}

void *renderer_map_element_array_buffer(struct Renderer *renderer) {
  size_t num_bytes = renderer->parameters.index_buffer_size;
  void *mapped_buffer_ptr = glMapNamedBufferRange(
      renderer->element_array_buffer, 0, num_bytes, GL_MAP_WRITE_BIT);

  CHECK_GL_ERROR();
  return mapped_buffer_ptr;
}

void renderer_unmap_element_array_buffer(struct Renderer *renderer) {
  if (glUnmapNamedBuffer(renderer->element_array_buffer) != GL_TRUE) {
    LOG_EXIT("Failed to unmap element array buffer");
  }
  CHECK_GL_ERROR();
}

static GLuint renderer_create_program_impl(GLuint *shaders, int count) {
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

GLuint renderer_compile_shader(const char *src, GLenum type) {
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

GLuint renderer_create_program(struct Renderer *renderer, struct Assets *assets,
                               AssetId program_id) {
  (void)renderer;
  AssetShaderProgram program;
  if (!assets_load_shader_program(assets, program_id, &program)) {
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

      if (!assets_load_shader(assets, program.shader_ids[i], &shader_asset)) {
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

      shader_handles[num_shaders_in_program++] = renderer_compile_shader(
          (const char *)shader_asset.source_buffer.data, gl_shader_type);

      stack_dealloc(&stack_allocator, (void *)shader_asset.source_buffer.data,
                    shader_asset.source_buffer.capacity);
    }
  }

  GLuint program_handle =
      renderer_create_program_impl(shader_handles, num_shaders_in_program);

  return program_handle;
}
