#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include <core/ecs.h>
#include <core/types.h>
#include <core/assetstore.h>
#include <core/vec.h>
#include <core/systembase.h>

#include <glad/glad.h>

#include <stdint.h>

#define MAX_DRAW_INDIRECT_DRAW_COMMANDS 10000

typedef struct {
    unsigned int count;
    unsigned int instance_count;
    unsigned int first_index;
    int base_vertex;
    unsigned int base_instance;
} DrawElementsIndirectCommand;

typedef struct {
    Mat4x4 model; // 64 bytes
    Vec2f tex_coord_offset; // 8 bytes
    Vec2f tex_coord_scale; // 8 bytes
    unsigned int material_index; // 4 bytes
    char padding[12];
} DrawCommandDataTiled;

typedef struct {
    GLuint64 handle;
    Vec4u8 color;
} Material;

struct Framebuffer {
    int width;
    int height;
};

typedef struct {
    Mat4x4 model_matrix;
    AssetId material_id;
    AssetId program_id;
    // unsigned int col;
    // unsigned int row;
    Vec2f tex_coord_offset;
    Vec2f tex_coord_scale;
    int8_t render_layer;
} RenderData;

typedef struct {
    Vec program_ids;
    Vec material_ids;
} PreparedResources;

struct RenderSystem {
    struct SystemBase base;
    DrawElementsIndirectCommand draw_commands[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
    DrawCommandDataTiled draw_command_data[MAX_DRAW_INDIRECT_DRAW_COMMANDS];
    HashMap material_asset_index_mapping;
    HashMap textures;
    // Program asset ID => GL program handle
    HashMap programs;
    // Keep track of which materials we've seen as
    // Key: AssetId, Value: Material SSBO vector index
    Vec materials;
    GLuint buffer_objects[32];
    struct Assets* assets;
    GLuint tilemap;
    GLuint vao;
    struct Framebuffer main_framebuffer;
};
typedef struct RenderSystem RenderSystem;

RenderSystem* render_system_create(struct Services* services, int intitial_width, int initial_height);

void render_system_prepare_resources(RenderSystem* system, PreparedResources* resources);

uint64_t render_system_create_texture(RenderSystem* system, void* data, ImageMeta* meta);

void render_system_frame_buffer_size_changed(RenderSystem *render_system,
                                             int width, int height);


/* struct PipelineLayout { */
/*     size_t num_attributes; */
    
/* }; */

/* struct Pipeline { */
  
/* }; */

/* Pipeline* render_system_create_pipeline(const char* name, PipelineLayout* layout); */


#endif
