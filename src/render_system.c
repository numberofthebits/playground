#include "render_system.h"

#include "components.h"
#include "systems.h"

#include <core/renderer.h>
#include <core/log.h>
#include <core/math.h>
#include <core/hashmap.h>
#include <core/arena.h>

#include <glad/glad.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <stdint.h>


typedef struct {
    Mat4x4 model_matrix;
    AssetId material_id;
    AssetId program_id;
    Vec2f tex_coord_offset;
    Vec2f tex_coord_scale;
    int8_t render_layer;
} RenderData;

void CALLING_CONVENTION gl_debug_callback(GLenum source,
					  GLenum type,
					  GLuint id,
					  GLenum severity,
					  GLsizei length,
					  const GLchar* message,
					  const void* userParam) {
  (void)source; (void)type; (void)id; (void)length; (void)userParam;
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

static GLuint compile_shader(const char* src, GLenum type) {
    GLint src_len = (GLint)strlen(src);
    GLuint handle = glCreateShader(type);
    glShaderSource(handle, 1, &src, &src_len);
    CHECK_GL_ERROR();
    glCompileShader(handle);
    CHECK_GL_ERROR();

    int status = 0;
    glGetShaderiv(handle,  GL_COMPILE_STATUS, &status);
    if(status != GL_TRUE) {
        int log_len = 0;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &log_len);
        if (log_len) {
            GLchar* log_buf = malloc(log_len);
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

static GLuint create_program(GLuint* shaders, int count) {
    GLuint prog = glCreateProgram();

    CHECK_GL_ERROR();

    for(int i = 0; i < count; ++i) {
        glAttachShader(prog, *(shaders+i));
    }

    glLinkProgram(prog);

    CHECK_GL_ERROR();

    glValidateProgram(prog);
    int status = 0;
    glGetProgramiv(prog, GL_VALIDATE_STATUS, &status);

    if(status != GL_TRUE) {
        int log_len = 0;
        // Includes null-terminator
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &log_len);
        if(log_len) {
            GLchar* log_buf = malloc(log_len);
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


static int render_data_order_comp(const void* lhs, const void* rhs) {
    const RenderData* a = lhs;
    const RenderData* b = rhs;

    return a->render_layer - b->render_layer;
}

static void sort_render_data(RenderData* data, size_t count) {
    qsort(data, count, sizeof(RenderData), &render_data_order_comp);
}

static void render_batch(RenderSystem* system, unsigned int batch_size) {
    if (!batch_size) {
	return;
    }

    float* vbo_data = malloc(80);   
    glGetNamedBufferSubData(system->tile_renderer->vertex_buffer_objects[0], 0, 80, vbo_data);
    free(vbo_data);
    
    (void)system;
    glMultiDrawElementsIndirect(GL_TRIANGLES,
                                GL_UNSIGNED_SHORT,
                                0, // *indirect *
                                batch_size,
                                sizeof(DrawElementsIndirectCommand));
    
    
    CHECK_GL_ERROR();
}

static GLint get_uniform_location_checked(GLuint program, const char* name) {
    GLint loc = glGetUniformLocation(program, name);
    CHECK_GL_ERROR();
    
    if(loc == -1) {
        LOG_EXIT("Invalid uniform location '%s'", name);
    }

    return loc;
}

static void render_system_update(RenderSystem* system, RenderData* data, size_t render_data_size ) {
    CHECK_GL_ERROR();

    glClearColor(0.2f, 0.2f, 0.2f, 255.f);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (render_data_size == 0) {
        return;
    }

    glBindVertexArray(system->tile_renderer->vertex_array_object);
    GLint loc_view;
    GLint loc_proj;

    CHECK_GL_ERROR();

    Vec3f cam_pos = { 10.f, 10.0f, 40.f };
    Vec3f cam_target = { 10.0f, 10.0f, 0.0f};
    Vec3f cam_up = { 0.0f, 1.0f, 0.0 };
    Mat4x4 view_mat = look_at(&cam_pos, &cam_target, &cam_up);
    
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, system->tile_renderer->multi_draw_indirect_buffer);
    //    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, system->tile_renderer->element_array_buffer);
    CHECK_GL_ERROR();

    sort_render_data(data, render_data_size);

    unsigned int count_in_batch = 0;
    AssetId current_program = 0;

    for (size_t i = 0; i < render_data_size; ++i) {
        RenderData* render_data = data + i;

        if(render_data->program_id != current_program) {
            // We we encounter a new program/pipeline
            // Render whatever we have so far, and then continue
            // building a new batch with the new program/pipeline
            render_batch(system, count_in_batch);
            count_in_batch = 0;
            
            GLuint* program = 0;
            if (!hash_map_get(&system->programs, &render_data->program_id, sizeof(render_data->program_id), (void**)&program)) {
                LOG_EXIT("Failed to find shader program");
            }
            GLuint program_handle = (GLuint)(uintptr_t)program;
            glUseProgram(program_handle);
            CHECK_GL_ERROR();

            current_program = render_data->program_id;

            loc_view = get_uniform_location_checked(program_handle, "View");
            loc_proj = get_uniform_location_checked(program_handle, "Proj");

            glUniformMatrix4fv(loc_view, 1, GL_FALSE, view_mat.data);
            CHECK_GL_ERROR();

            /* Mat4x4 proj_mat = perspective(0.1f, */
            /*                               10.0f, */
            /*                               65.f, */
            /*                               (float)system->main_framebuffer.width / */
            /*                               (float)system->main_framebuffer.height); */
            float scale = 1.0f;
            float aspect_ratio = (float)system->main_framebuffer.width / (float)system->main_framebuffer.height;
            Mat4x4 proj_mat = ortho(0.01f, 100.f, aspect_ratio * scale, -aspect_ratio * scale, 1.f * scale, -1.f * scale);
            glUniformMatrix4fv(loc_proj, 1, GL_FALSE, proj_mat.data);
            CHECK_GL_ERROR();
        }
       
        DrawElementsIndirectCommand* command = &system->draw_commands[count_in_batch];
        command->count = 6;
        command->instance_count = 1;
        command->first_index = 0;
        command->base_vertex = 0;
        command->base_instance = 0;
        
        DrawCommandDataTiled* draw_data = &system->draw_command_data[count_in_batch];
        draw_data->model = render_data->model_matrix;
        draw_data->tex_coord_offset = render_data->tex_coord_offset;
        draw_data->tex_coord_scale = render_data->tex_coord_scale;

        void* value = 0;
        int found = hash_map_get(&system->material_asset_index_mapping, &render_data->material_id, sizeof(AssetId), &value);

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
    glNamedBufferSubData(system->tile_renderer->multi_draw_indirect_buffer,
                         0,
                         sizeof(DrawElementsIndirectCommand) * count_in_batch,
                         system->draw_commands);
    CHECK_GL_ERROR();

    glNamedBufferSubData(system->tile_renderer->shader_storage_buffer_objects[0],
                         0,
                         sizeof(DrawCommandDataTiled) * count_in_batch,
                         system->draw_command_data);
    CHECK_GL_ERROR();

    glNamedBufferSubData(system->tile_renderer->shader_storage_buffer_objects[1],
                         0,
                         sizeof(Material) * system->materials.size,
                         VEC_ITER_BEGIN_T(&system->materials, Material));

    CHECK_GL_ERROR();

    // Dispatch the final batch
    render_batch(system, count_in_batch);
}


static void render_update(Registry* reg, struct SystemBase* sys, size_t frame_nr) {
  (void)frame_nr;
    BeginScopedTimer(render_time);

    RenderSystem* render_sys = (RenderSystem*)sys;
    Entity* entities = VEC_ITER_BEGIN_T(&sys->entities, Entity);

    struct Pool* transform_pool = registry_get_pool(reg, TRANSFORM_COMPONENT_BIT);
    struct Pool* render_pool = registry_get_pool(reg, RENDER_COMPONENT_BIT);

    RenderData* render_data = ArenaAlloc(&frame_allocator, sys->entities.size, RenderData);
    
    for (int i = 0; i < sys->entities.size; ++i) {
        Entity entity = entities[i];
        TransformComponent* tc = PoolGetComponent(transform_pool, TransformComponent, entity.index);
        RenderComponent* rc = PoolGetComponent(render_pool, RenderComponent, entity.index);

        RenderData* rd = &render_data[i];

        rd->render_layer = rc->render_layer;
        
        Vec3f scale;
        scale.x = tc->scale.x;
        scale.y = tc->scale.y;
        scale.z = 1.0f;
        
        Vec3f pos;
        pos.x = tc->pos.x;
        pos.y = tc->pos.y;
        pos.z = tc->pos.z;

        rd->tex_coord_offset = rc->tex_coord_offset;
        rd->tex_coord_scale = rc->tex_coord_scale;

        Vec3f axis = {0.f, 0.f, 1.f};
        Mat4x4 matrix_scale = identity();
        Mat4x4 matrix_translate = identity();
        Mat4x4 matrix_rotate = identity();

        mat4_rotate(&matrix_rotate, &axis, tc->rotation);        
        scale_mat4(&matrix_scale, &scale);
        translate(&matrix_translate, &pos);


        Mat4x4 m = identity();
        m = mul(&m, &matrix_rotate);
        m = mul(&m, &matrix_scale);
        m = mul(&m, &matrix_translate);
        
        rd->model_matrix = m;
        rd->material_id = rc->material_id;
        rd->program_id = rc->pipeline_id;
    }

    render_system_update(render_sys, render_data, sys->entities.size);

    AppendScopedTimer(render_time);
    PrintScopedTimer(render_time);
}


void render_system_create_program(RenderSystem* system, AssetId program_id) {
    AssetShaderProgram* program_asset = assets_get_program(system->assets, program_id);
    if(!program_asset) {
        LOG_EXIT("Failed to create program with id '%d'", program_id);
    }
    
    int num_shaders_in_program = 0;
    GLuint shader_handles[6];

    for (size_t i = 0; i < sizeof(program_asset->shader_ids) / sizeof(AssetId); ++i) {
        if(assets_shader_program_has_shader(program_asset, i)) {
            AssetShader* shader_asset = assets_get_shader(system->assets, program_asset->shader_ids[i]);

            GLenum gl_shader_type = 0;
            switch (0x1 << i) {
            case AssetShaderVertex:
                gl_shader_type = GL_VERTEX_SHADER;
                break;
            case AssetShaderFragment:
                gl_shader_type = GL_FRAGMENT_SHADER;
                break;
            default:
                LOG_EXIT("Unhandled shader type");
            }

            LOG_INFO("Compiling shader '%s'", shader_asset->file_path.path);
            shader_handles[num_shaders_in_program++] = compile_shader(shader_asset->shader_src, gl_shader_type);
        }
    }
    
    GLuint program_handle = create_program(shader_handles, num_shaders_in_program);
    hash_map_insert_value(&system->programs,
                          &program_asset->id,
                          sizeof(program_asset->id),
                          (void*)(uintptr_t)program_handle);
}

void render_system_global_init() {
    gladLoadGL();
    glDebugMessageCallback(&gl_debug_callback, 0);
    glEnable(GL_DEBUG_OUTPUT);
}

RenderSystem* render_system_create(struct Services* services, int initial_width, int initial_height ) {
    LOG_INFO("Create render system implementation...");

    // TODO: This is per renderer data. Should be set up when invoking each individual renderer
    glClearColor(0.f, 0.0f, 0.0f, 255.f);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSystem* system = ArenaAlloc(&allocator, 1, RenderSystem);
    system_base_init((struct SystemBase*)system,
		     RENDER_SYSTEM_BIT,
		     &render_update,
		     RENDER_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, services);

    system->assets = services->assets;
    system->materials = vec_create();
    system->main_framebuffer.width = initial_width;
    system->main_framebuffer.height = initial_height;

    hash_map_init(&system->programs, 100);
    hash_map_init(&system->textures, 1000);
    hash_map_init(&system->material_asset_index_mapping, 1000);

    system->tile_renderer = (struct Renderer*)ArenaAlloc(&allocator, 1, struct Renderer);
   
    struct VertexAttributeDescriptor attrib_desc[2];
    attrib_desc[0].vertex_attribute = 0;
    attrib_desc[0].element_count = 3;
    attrib_desc[0].element_type = GL_FLOAT;
    attrib_desc[0].normalize  = GL_FALSE;
    attrib_desc[0].relative_offset = 0;
   
    attrib_desc[1].vertex_attribute = 1;
    attrib_desc[1].element_count = 2;
    attrib_desc[1].element_type = GL_FLOAT;
    attrib_desc[1].normalize  = GL_FALSE;
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

    uint16_t index_data[] = {
	0, 1, 2,
	0, 2, 3
    };
    
    struct RendererParameters tile_params;
    tile_params.index_buffer_size = sizeof(index_data);
    tile_params.num_vertices = 4;
    tile_params.num_buffer_descriptors = 1;
    tile_params.buffer_descriptors = &vb_desc;

    renderer_init(system->tile_renderer, &tile_params);

    float pos_data[] = {
        -0.48f, -0.48f, 0.0f, // lower left
        0.48f, -0.48f, 0.0f, // lower right
        0.48f, 0.48f, 0.0f, // upper right
        -0.48f, 0.48f, 0.0f, // upper left
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
    attribs[0].src =  pos_data;
    attribs[0].vertex_size_bytes = 3 * sizeof(float);
    attribs[1].src = uv_data;
    attribs[1].vertex_size_bytes = 2 * sizeof(float);

    args.num_attribs = 2;
    args.attribs = attribs;

    interleave_attributes(&args);

    glNamedBufferSubData(system->tile_renderer->vertex_buffer_objects[0], 0, sizeof(interleaved), interleaved);

    // Draw command aux data
    renderer_ssbo_create(system->tile_renderer,
			 0,
			 BO_INDEX_DRAW_COMMAND_DATA,
			 MAX_DRAW_INDIRECT_DRAW_COMMANDS * sizeof(DrawCommandDataTiled));

    // Materials
    renderer_ssbo_create(system->tile_renderer,
			 1,
			 BO_INDEX_MATERIALS,
			 MAX_DRAW_INDIRECT_DRAW_COMMANDS * sizeof(Material));



    renderer_write_element_array_buffer(system->tile_renderer, 0, sizeof(index_data), index_data);
    CHECK_GL_ERROR();
    
    LOG_INFO("Created render system implementation");

    return system;
}

/*
RenderSystem* render_system_create(struct Services* services, int initial_width, int initial_height ) {
    LOG_INFO("Create render system implementation...");
    // init function pointer loader 

    glEnable(GL_DEBUG_OUTPUT);

    glClearColor(0.f, 0.0f, 0.0f, 255.f);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    RenderSystem* system = ArenaAlloc(&allocator, 1, RenderSystem);
    system_base_init((struct SystemBase*)system,
		     RENDER_SYSTEM_BIT,
		     &render_update,
		     RENDER_COMPONENT_BIT | TRANSFORM_COMPONENT_BIT, services);

    system->assets = services->assets;
    system->materials = vec_create();
    system->main_framebuffer.width = initial_width;
    system->main_framebuffer.height = initial_height;

    hash_map_init(&system->programs, 100);
    hash_map_init(&system->textures, 1000);
    hash_map_init(&system->material_asset_index_mapping, 1000);
    
    memset(&system->draw_commands, 0x0, sizeof(DrawElementsIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS);
    memset(&system->draw_command_data, 0x0, sizeof(DrawCommandDataTiled) * MAX_DRAW_INDIRECT_DRAW_COMMANDS);

//    glEnable(GL_BLEND);
    glCreateVertexArrays(1, &system->vao);
    glCreateBuffers(BO_INDEX_MAX, system->buffer_objects);

    CHECK_GL_ERROR();
    
    glEnableVertexArrayAttrib(system->vao, 0);
    glEnableVertexArrayAttrib(system->vao, 1);
    glEnableVertexArrayAttrib(system->vao, 2);
    
    glVertexArrayAttribBinding(system->vao, 0, 0);
    glVertexArrayAttribBinding(system->vao, 1, 1);
    glVertexArrayAttribBinding(system->vao, 2, 2);

    glVertexArrayVertexBuffer(system->vao, 0, system->buffer_objects[BO_INDEX_POSITION], 0, sizeof(float) * 3);
    glVertexArrayVertexBuffer(system->vao, 1, system->buffer_objects[BO_INDEX_COLOR], 0, sizeof(uint8_t)* 3);
    glVertexArrayVertexBuffer(system->vao, 2, system->buffer_objects[BO_INDEX_UV], 0, sizeof(float)* 2);
    
    glVertexArrayAttribFormat(system->vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribIFormat(system->vao, 1, 3, GL_UNSIGNED_BYTE, 0);
    glVertexArrayAttribFormat(system->vao, 2, 2, GL_FLOAT, GL_FALSE, 0);
    
    float pos_data[] = {
        -0.48f, -0.48f, 0.0f, // lower left
        0.48f, -0.48f, 0.0f, // lower right
        0.48f, 0.48f, 0.0f, // upper right
        -0.48f, 0.48f, 0.0f, // upper left
    };
    glNamedBufferData(system->buffer_objects[BO_INDEX_POSITION], sizeof(pos_data), &pos_data, GL_STATIC_DRAW);
    
    uint8_t color_data[] = {
        255,   0,   0,
        0, 255,   0,
        0,   0, 255,
        255, 255, 255
    };
    glNamedBufferData(system->buffer_objects[BO_INDEX_COLOR], sizeof(color_data), &color_data, GL_STATIC_DRAW);
    
    float uv_data[] = {
        0.0f, 0.0f, // lower left,
        1.0f, 0.0f, // lower right,
        1.0f, 1.0f, // upper right,
        0.0f, 1.0f, // upper left
    };
    glNamedBufferData(system->buffer_objects[BO_INDEX_UV], sizeof(uv_data), &uv_data, GL_STATIC_DRAW);

    uint16_t index_data[] = {
        0, 1, 2,
        0, 2, 3
    };
    glNamedBufferData(system->buffer_objects[BO_INDEX_ELEMENT_ARRAY],
                      sizeof(index_data),
                      index_data,
                      GL_STATIC_DRAW);

    glVertexArrayElementBuffer(system->vao, system->buffer_objects[BO_INDEX_ELEMENT_ARRAY]);

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, system->buffer_objects[BO_INDEX_DRAW_INDIRECT]);
    glNamedBufferData(system->buffer_objects[BO_INDEX_DRAW_INDIRECT],
                      sizeof(DrawElementsIndirectCommand) * MAX_DRAW_INDIRECT_DRAW_COMMANDS,
                      0,
                      GL_DYNAMIC_DRAW);

    glBindBufferBase( GL_SHADER_STORAGE_BUFFER,
                      BO_INDEX_DRAW_COMMAND_DATA,
                      system->buffer_objects[BO_INDEX_DRAW_COMMAND_DATA] );
    glNamedBufferData(system->buffer_objects[BO_INDEX_DRAW_COMMAND_DATA],
                      sizeof(DrawCommandDataTiled) * MAX_DRAW_INDIRECT_DRAW_COMMANDS,
                      0,
                      GL_DYNAMIC_DRAW);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER,
                     BO_INDEX_MATERIALS,
                     system->buffer_objects[BO_INDEX_MATERIALS]);
    glNamedBufferData(system->buffer_objects[BO_INDEX_MATERIALS],
                      sizeof(Material) * MAX_MATERIALS,
                      0,
                      GL_DYNAMIC_DRAW);
    
    LOG_INFO("Created render system implementation");

    return system;
}
*/
static void render_system_create_material(RenderSystem* system, AssetId material_id) {
    Material new_material;
    AssetMaterial* mat = assets_get_material(system->assets, material_id);
    if (!mat) {
        LOG_EXIT("Failed to find material with id %u", material_id);
    }
    new_material.color = mat->color;

    void* texture_handle_ptr = 0; 
    int found_texture_handle = (GLuint64)(uintptr_t)hash_map_get(&system->textures, &mat->texture_id, sizeof(AssetId), &texture_handle_ptr );
    // Careful. Don't know if it's valid before testing hash_map_get return value
    GLuint64 texture_handle = (GLuint64)texture_handle_ptr;
    if (!found_texture_handle) {

        ImageMeta meta;
        void* data = 0;
        if (!assets_load_asset(system->assets, mat->texture_id, &data, &meta)) {
            LOG_EXIT("Failed to load texture asset '%d'", mat->texture_id);
        }

        // Store the new handle as a void* in the hash map to avoid allocating just for
        // a pointer sized type
        GLuint64 new_handle = render_system_create_texture(system, data, &meta);
        uintptr_t new_handle_ptr = (uintptr_t)new_handle;

        hash_map_insert_value(&system->textures, &mat->texture_id, sizeof(AssetId), (void*)new_handle_ptr);

        if (data) {
            free(data);
        }

        new_material.handle = new_handle;
                
    } else {
        new_material.handle = texture_handle;
    }

    unsigned int new_material_index = system->materials.size;
    hash_map_insert_value(&system->material_asset_index_mapping,
                          &material_id,
                          sizeof(AssetId),
                          (void*)(uintptr_t)new_material_index);

    VEC_PUSH_T(&system->materials, Material, new_material);
}


void render_system_prepare_resources(RenderSystem* system, PreparedResources* resources) {
    for (int i = 0; i < resources->program_ids.size; ++i) {
        AssetId program_id = VEC_GET_T(&resources->program_ids, AssetId, i);
        render_system_create_program(system, program_id);        
    }

    for (int i = 0; i < resources->material_ids.size; ++i) {
        AssetId material_id =  VEC_GET_T(&resources->material_ids, AssetId, i);
        render_system_create_material(system, material_id);
    }
}


uint64_t render_system_create_texture(RenderSystem* system, void* data, ImageMeta* meta) {
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
        LOG_EXIT("Unimplemented number of image channels (%d) to GL enum mapping", meta->channels);
    }
    
    int levels = 1;
    glTextureStorage2D(tex_handle, levels, internal_fmt, meta->width, meta->height);
    CHECK_GL_ERROR();

    int level = 0;
    int xoffset = 0;
    int yoffset = 0;

    GLenum type = GL_UNSIGNED_BYTE;
    glTextureSubImage2D(tex_handle, level, xoffset, yoffset, meta->width, meta->height, format, type, data);

    CHECK_GL_ERROR();

    // Hack set up a texture unit

    glTextureParameteri(tex_handle, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(tex_handle, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
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

void render_system_frame_buffer_size_changed(RenderSystem* render_system , int width, int height) {
    render_system->main_framebuffer.width = width;
    render_system->main_framebuffer.height = height;
    glViewport(0, 0, width, height);
}
