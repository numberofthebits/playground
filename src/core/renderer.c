#include <core/renderer.h>


void interleave_attributes(struct Interleave* interleave) {
    ptrdiff_t write_offset = 0;
    for (size_t i = 0; i < interleave->element_count; ++i) {
	for (size_t j = 0; j < interleave->num_attribs; ++j) {
	    memcpy((char*)interleave->dst + write_offset,
		   (char*)interleave->attribs[j].src + i * interleave->attribs[j].vertex_size_bytes,
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

GLuint calc_stride(struct VertexAttributeDescriptor *descriptors,
                                 int count) {
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

	    glVertexArrayVertexBuffer(renderer->vertex_array_object, binding_point_desc.binding_point_index,
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

		glVertexArrayAttribFormat(renderer->vertex_array_object,
					  attrib_desc.vertex_attribute,
					  attrib_desc.element_count,
					  attrib_desc.element_type,
					  attrib_desc.normalize,
					  attrib_desc.relative_offset);

		vertex_size_bytes += attrib_desc.element_count *
		    gl_type_byte_size(attrib_desc.element_type);
	    }
	}

	glNamedBufferStorage(renderer->vertex_buffer_objects[i],
			     vertex_size_bytes * params->num_vertices, NULL,
			     GL_DYNAMIC_STORAGE_BIT);
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

void renderer_use(struct Renderer* renderer) {
    glBindVertexArray(renderer->vertex_array_object);

    for(int i = 0; i < SSBO_MAX; ++i) {
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
			 renderer->shader_storage_buffer_objects.binding_point_index[i],
			 renderer->shader_storage_buffer_objects.buffer_object[i]);
    }

    // Note: There's probably other relevant state that isn't implicitly bound via
    // vertex array object.
}

void renderer_write_element_array_buffer(struct Renderer *renderer,
				    size_t offset,
                                    size_t size,
				    void *data) {
    glNamedBufferSubData(renderer->element_array_buffer, offset, size, data);
    CHECK_GL_ERROR();
}

void renderer_ssbo_create(struct Renderer *renderer,
			  int index,
			  int binding_point,
			  GLsizeiptr buffer_size) {
    renderer->shader_storage_buffer_objects.binding_point_index[index] = binding_point;

    glNamedBufferStorage(renderer->shader_storage_buffer_objects.buffer_object[index],
			 buffer_size,
			 NULL,
			 GL_DYNAMIC_STORAGE_BIT);
    CHECK_GL_ERROR();
}

void renderer_ssbo_write(struct Renderer *renderer,
			 int index,
			 GLintptr offset,
			 void *data,
			 size_t size) {
    if (index >= SSBO_MAX) {
	LOG_EXIT("SSBO index exceeds max SSBOs {d}", SSBO_MAX);
    }

    glNamedBufferSubData(renderer->shader_storage_buffer_objects.buffer_object[index], offset,
			 size, data);
    CHECK_GL_ERROR();
}

