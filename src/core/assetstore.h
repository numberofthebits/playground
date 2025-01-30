#ifndef ASSETSTORE_H
#define ASSETSTORE_H

#define ASSET_NAME_MAX_LEN 64
#define ASSET_FILE_PATH_MAX 256

#include "vec.h"
#include "math.h"
#include "types.h"
#include "hashmap.h"
#include <core/os.h>

#include <stdint.h>

typedef struct {
    char path[ASSET_FILE_PATH_MAX];
} AssetFilePath;

AssetId asset_hash_name(const char* name, size_t len);

typedef struct {
    char name[ASSET_NAME_MAX_LEN];
    uint8_t len;
} AssetName;

AssetName assets_make_asset_name(const char* name, size_t len);
AssetName assets_make_asset_name_str(const char* name);

AssetFilePath assets_make_asset_file_path(const char* file_path);

int assets_asset_name_eq(AssetName* lhs, AssetName* rhs);

// TODO: Nuke this. Type type should be implicit from its existence
// in container for a given asset type
typedef enum {
    AssetTypeTexture,
    AssetTypeTextureTiled,
    AssetTypeShaderProgram
} AssetType;

typedef struct {
    AssetType type;
    AssetName name;
    char file_path[ASSET_FILE_PATH_MAX];
} Asset;

typedef struct {
    AssetName name;
    ImageMeta meta;
} AssetTexture;

typedef struct {
    AssetTexture base;
    uint16_t tiles_x;
    uint16_t tiles_y;
} AssetTiledTexture;

typedef struct {
    AssetId id;
    Vec4u8 color;
    AssetId texture_id;
} AssetMaterial;

typedef struct {
    AssetId id;
    AssetName name;

    // Note shader owns this source ptr
    const char* shader_src;
    AssetFilePath file_path;
} AssetShader;

typedef enum {
    AssetShaderUnknown = 0x0,
    AssetShaderVertex = 0x1,
    AssetShaderFragment = 0x2
} AssetProgramShaderFlag;

typedef struct {
    AssetId id;
    AssetName name;

    // There are only 6 types of shaders in openGL.
    // Let's assume we're in OpenGL land
    AssetId shader_ids[6];
    int has_shader;
} AssetShaderProgram;

int assets_shader_program_has_shader(AssetShaderProgram* program, int index);

typedef struct {
    // Map of AssetId, Asset
    HashMap textures;
    Vec programs;
    Vec shaders;
    Vec materials;
    Vec tiled_textures;
} Assets;

void assets_init(Assets* assets);

AssetId assets_make_id(const char* name, size_t len);
AssetId assets_make_id_str(const char* name);

int assets_load_asset(Assets* assets, AssetId id, void** data, void* meta);

AssetMaterial* assets_get_material(Assets* assets, AssetId material_id);

void assets_load_texture(Assets* assets,
                         const char* name,
                         ImageMeta* meta);

void assets_load_tiled_texture(Assets* assets,
                               const char* name,
                               ImageMeta* meta,
                               uint16_t tiles_x,
                               uint16_t tiles_y);

AssetShaderProgram* assets_get_program(Assets* assets, AssetId program_id);

AssetShader* assets_get_shader(Assets* assets, AssetId shader_id);



#endif // ASSETSTORE_H
