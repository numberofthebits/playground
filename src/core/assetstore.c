#include "assetstore.h"

#include <core/log.h>
#include <core/util.h>

#include <stb_image.h>

#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>

#define EXTENSION_VERTEX_SHADER ".vert"
#define EXTENSION_FRAGMENT_SHADER ".frag"
#define MAKE_FILE_PATH_BUF_MAX 1024

AssetName assets_make_asset_name(const char* name, size_t len) {
    if (len > ASSET_NAME_MAX_LEN) {
        LOG_WARN("Truncating asset name '%' length %d exceeding max length %d", name, len, ASSET_NAME_MAX_LEN);
    }

    AssetName asset_name;
    memset(asset_name.name, 0x0, ASSET_NAME_MAX_LEN);
    memcpy(asset_name.name, name, len);
    asset_name.len = (uint8_t)len;

    return asset_name;
}

AssetName assets_make_asset_name_str(const char* name) {
    return assets_make_asset_name(name, strlen(name));
}

AssetFilePath assets_make_asset_file_path(const char* file_path) {
    size_t len = strlen(file_path);
    if(len >= ASSET_FILE_PATH_MAX) {
        LOG_EXIT("Asset file path '%s' too long (max %d)", file_path, ASSET_FILE_PATH_MAX);
    }

    AssetFilePath asset_path = {0};
    strcpy(asset_path.path, file_path);

    return asset_path;
}

int assets_asset_name_eq(AssetName* lhs, AssetName* rhs) {
    size_t len = (lhs->len > rhs->len ? (size_t)lhs->len : rhs->len);
    LOG_INFO("lhs %.*s rhs %.*s", lhs->len, lhs->name, rhs->len, rhs->name);
    return memcmp(lhs->name, rhs->name, len);
}

typedef struct _TileInfo {
    uint16_t x;
    uint16_t y;
} TileInfo;

static int load_texture_data(const char* file_path, ImageMeta* meta, void** data) {
    stbi_set_flip_vertically_on_load(1);
    int x = 0;
    int y = 0;
    int comp = 0;

    *data = stbi_load(file_path, &x, &y, &comp, 0);
    
    if (!*data) {
        LOG_ERROR("Failed to load texture '%s'", file_path);
        return 0;
    }
    
    LOG_INFO("Loaded texture data '%s': pixel size (%d, %d). Image channels %d", file_path, x, y, comp);

    meta->width = (uint16_t)x;
    meta->height = (uint16_t)y;
    meta->channels = comp;

    return 1;
}

static int handle_texture(const FileSystemListResult* result, void* user_data) {
    if(!result) {
        LOG_EXIT("Invalid file system list result");
    }

    if (!result->file_path || !result->extension || !result->file_name) {
        LOG_WARN("Texture callback failed: path '%s' filename '%s' extension '%s'");
        return FILE_CALLBACK_RESULT_CONTINUE;
    }
    
    Assets* assets = user_data;

    AssetId id = assets_make_id_str(result->file_name);
    Asset* asset = malloc(sizeof(Asset));
    asset->type = AssetTypeTexture; 
    asset->name = assets_make_asset_name_str(result->file_name);
    memset(asset->file_path, 0x0, sizeof(asset->file_path));
    strcpy(asset->file_path, result->file_path);

    hash_map_insert(&assets->textures, &id, sizeof(id), asset);

    return FILE_CALLBACK_RESULT_CONTINUE;
}

static void load_textures(Assets* assets) {
    LOG_INFO("Loading textures...");    
    file_system_list("./assets/images/", "*.png", &handle_texture, assets);
    file_system_list("./assets/tilemaps/", "jungle.png", &handle_texture, assets);
    
}

typedef struct {
    Assets* assets;
    AssetShaderProgram* program;
} LoadShaderSourceContext;

static int extension_to_shader_type(const char* extension) {
    if (!extension) {
        return AssetShaderUnknown;
    }
    
    if (strcmp(extension, EXTENSION_VERTEX_SHADER) == 0) {
        return AssetShaderVertex;
    } else if (strcmp(extension, EXTENSION_FRAGMENT_SHADER) == 0) {
        return AssetShaderFragment;
    }
    
    LOG_INFO("Unhandled shader extension '%s'", extension);
    return AssetShaderUnknown;
}

static int load_shader_source(const FileSystemListResult* result, void* user_data) {
    if(!result) {
        LOG_EXIT("Invalid file system list result");
    }
    LoadShaderSourceContext* context = user_data;
    int shader_type = extension_to_shader_type(result->extension);

    if(!shader_type) {
        return FILE_CALLBACK_RESULT_CONTINUE;
    }

    AssetShader shader_asset;
    shader_asset.id = assets_make_id_str(result->file_name);
    shader_asset.name = assets_make_asset_name_str(result->file_name);
    shader_asset.file_path = assets_make_asset_file_path(result->file_path);

    const char* shader_src = (const char*)file_read_all(result->file_path);
    if(!shader_src) {
        LOG_EXIT("Failed to load shader from '%s'", result->file_path);
    }

    LOG_INFO("Loaded shader source:\n%s", shader_src);
    shader_asset.shader_src = shader_src;

    int shader_index = get_msb_set(shader_type);
    if (shader_index < 0) {
        LOG_EXIT("Failed to find shader index from shader type '%d', file ext '%s'", shader_type, result->extension);
    }
    context->program->has_shader |= shader_type;
    context->program->shader_ids[shader_index] = shader_asset.id;
    VEC_PUSH_T(&context->assets->shaders, AssetShader, shader_asset);    
    
    return FILE_CALLBACK_RESULT_CONTINUE;
}

static void load_shaders(Assets* assets) {
    LoadShaderSourceContext context = {0};
    context.assets = assets;
    
    AssetShaderProgram tilemap = {0};
    tilemap.id = assets_make_id_str("tilemap");
    tilemap.name = assets_make_asset_name_str("tilemap");
    context.program = &tilemap;
    file_system_list("./assets/shaders/", "tilemap.*", &load_shader_source, &context);

    VEC_PUSH_T(&assets->programs, AssetShaderProgram, tilemap);

    AssetShaderProgram unit = {0};
    unit.id = assets_make_id_str("unit");
    unit.name = assets_make_asset_name_str("unit");
    context.program = &unit;
    file_system_list("./assets/shaders/", "unit.*", &load_shader_source, &context);

    VEC_PUSH_T(&assets->programs, AssetShaderProgram, unit);   
}

int assets_shader_program_has_shader(AssetShaderProgram* program, int index) {
    return program->has_shader & (0x1 << index);
}

static void load_materials(Assets* assets) {
    AssetId truck_tex = assets_make_id_str("truck-ford-right.png");
    
    AssetMaterial truck_red;
    truck_red.id = assets_make_id_str("truck-red-mat");
    truck_red.color.r = 255;
    truck_red.color.g = 0;
    truck_red.color.b = 0;
    truck_red.color.a = 255;
    truck_red.texture_id = truck_tex;

    AssetMaterial truck_blue;
    truck_blue.id = assets_make_id_str("truck-blue-mat");
    truck_blue.color.r = 255;
    truck_blue.color.g = 255;
    truck_blue.color.b = 255;
    truck_blue.color.a = 255;
    truck_blue.texture_id = truck_tex;

    AssetMaterial jungle_map;
    jungle_map.id = assets_make_id_str("jungle-mat");
    jungle_map.color.r = 255; // r
    jungle_map.color.g = 255; // b
    jungle_map.color.b = 255; // g 
    jungle_map.color.a = 0; // a
    jungle_map.texture_id = assets_make_id_str("jungle.png");

    AssetMaterial chopper_udlr;
    chopper_udlr.id = assets_make_id_str("chopper-udlr");
    chopper_udlr.color.r = 255;
    chopper_udlr.color.g = 255;
    chopper_udlr.color.b = 255;
    chopper_udlr.color.a = 255;
    chopper_udlr.texture_id = assets_make_id_str("chopper-spritesheet.png");

    VEC_PUSH_T(&assets->materials, AssetMaterial, truck_red);
    VEC_PUSH_T(&assets->materials, AssetMaterial, truck_blue);
    VEC_PUSH_T(&assets->materials, AssetMaterial, jungle_map);
    VEC_PUSH_T(&assets->materials, AssetMaterial, chopper_udlr);

    AssetMaterial bullet;
    bullet.id = assets_make_id_str("bullet-mat");
    bullet.color.r = 255;
    bullet.color.g = 255;
    bullet.color.b = 255;
    bullet.color.a = 0;
    bullet.texture_id = assets_make_id_str("bullet.png");
    VEC_PUSH_T(&assets->materials, AssetMaterial, bullet);
}

void assets_init(Assets* assets) {
    hash_map_init(&assets->textures, 1000);
    assets->shaders = vec_create();
    assets->tiled_textures = vec_create();
    assets->materials = vec_create();
    assets->programs = vec_create();

    load_textures(assets);
    load_materials(assets);
    load_shaders(assets);
}

AssetId assets_make_id(const char* name, size_t len) {
    return hash(name, len);
}

AssetId assets_make_id_str(const char* name) {
    return assets_make_id(name, strlen(name));
}

static int assets_make_file_path(const char* path, const char* file_name, char* buf, unsigned int buf_len) {
    // +2 add one for newline and one for path separator
    if(strlen(path) + strlen(file_name) + 2 >= buf_len) {
        return 0;
    }
    strcat(buf, path);
    strcat(buf, "/");
    strcat(buf, file_name);

    return 1;
}

AssetMaterial* assets_get_material(Assets* assets, AssetId material_id) {
    
    for (int i = 0; i < assets->materials.size; ++i) {
        AssetMaterial* mat = VEC_GET_T_PTR(&assets->materials, AssetMaterial, i);
        if (mat->id == material_id) {
            return mat;
        }
    }

    return 0;
}

int assets_load_asset(Assets* assets, AssetId id, void** data, void* meta) {
    void* asset_ptr = 0;
    int found_asset_id  = hash_map_get(&assets->textures, &id, sizeof(id), &asset_ptr);
    if (!found_asset_id) {
        return 0;
    }

    Asset* asset = (Asset*)asset_ptr;

    char buf[MAKE_FILE_PATH_BUF_MAX] = {0};
    if(!assets_make_file_path(asset->file_path, asset->name.name, buf, sizeof(buf))) {
        LOG_ERROR("Failed to make file path for path '%s' file '%s': Concat buffer size=%d", asset->file_path, asset->name, MAKE_FILE_PATH_BUF_MAX);
        return 0;
    }
    
    int ret = 0;
    switch (asset->type) {
    case AssetTypeTexture:
        ret = load_texture_data(asset->file_path, meta, data );
        break;
    case AssetTypeTextureTiled:
        ret = load_texture_data(asset->file_path, meta, data);        
        break;
    case AssetTypeShaderProgram:
        break;
    default:
        LOG_ERROR("Unknown asset type: '%d'", asset->type);
        return 0;
    }
    
    return ret;
}
AssetShaderProgram* assets_get_program(Assets* assets, AssetId program_id) {
    for (int i = 0; i < assets->programs.size; ++i) {
        AssetShaderProgram* program = VEC_GET_T_PTR(&assets->programs, AssetShaderProgram, i);
        if (program->id == program_id) {
            return program;
        }
    }

    return 0;
}

AssetShader* assets_get_shader(Assets* assets, AssetId shader_id) {
    for (int i = 0; i < assets->shaders.size; ++i) {
        AssetShader* shader = VEC_GET_T_PTR(&assets->shaders, AssetShader, i);
        if (shader->id == shader_id) {
            return shader;
        }
    }

    return 0;
}
/* void assets_add_texture(Assets* assets, TextureHandle* handle, const char* asset_name, ImageMeta* meta) { */
/*     LOG_INFO("Add texture '%s'", asset_name); */
/*     AssetTexture texture; */
/*     assets_add_texture_init_shared(&texture, handle, asset_name, meta); */
/*     VEC_PUSH_T(&assets->textures, AssetTexture, texture); */
/*     LOG_INFO("Done"); */
/* } */

/* void assets_add_tiled_texture(Assets* assets, TextureHandle* handle, const char* asset_name, ImageMeta* meta, uint16_t tiles_x, uint16_t tiles_y) { */
/*     LOG_INFO("Add tiled texture '%s'", asset_name); */
/*     AssetTiledTexture texture; */
/*     assets_add_texture_init_shared((AssetTexture*)&texture, handle, asset_name, meta); */
/*     texture.tiles_x = tiles_x; */
/*     texture.tiles_y = tiles_y; */

/*     VEC_PUSH_T(&assets->tiled_textures, AssetTiledTexture, texture); */
/* } */

