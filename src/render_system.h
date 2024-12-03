#ifndef _RENDER_SYSTEM_H
#define _RENDER_SYSTEM_H

#include <core/ecs.h>
#include <core/types.h>
#include <core/assetstore.h>
#include <core/vec.h>

#include <stdint.h>

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

typedef struct RenderSystem_t RenderSystem;

RenderSystem* render_system_create(Assets* assets);

void render_system_prepare_resources(RenderSystem* system, PreparedResources* resources);

void render_system_update(RenderSystem* system);

uint64_t render_system_create_texture(RenderSystem* system, void* data, ImageMeta* meta);

// Get the render jobs vector and ensure it has space for at least
// num_entities. No shrink logic implemented.
Vec* render_system_get_render_data(RenderSystem* system, int num_entities);

#endif
