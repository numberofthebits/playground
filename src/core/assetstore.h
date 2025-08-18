#ifndef ASSETSTORE_H
#define ASSETSTORE_H

/* Why 62? Leave room for \0 terminator and uint8_t len
   in the same cacheline*/
#define ASSET_NAME_MAX_LEN 62
#define ASSET_NAME_ARRAY_MAX_LEN ASSET_NAME_MAX_LEN + 1
#define ASSET_FILE_PATH_MAX 251

#include "allocators.h"
#include "hashmap.h"
#include "math.h"
#include "os.h"
#include "types.h"
#include "util.h"
#include "vec.h"

#include <stdalign.h>
#include <stdint.h>

typedef struct {
  char path[ASSET_FILE_PATH_MAX];
  uint32_t len;
} AssetFilePath;

typedef struct AssetName {
  char name[ASSET_NAME_ARRAY_MAX_LEN];
  uint8_t len;
} AssetName;

AssetName assets_make_asset_name(const char *name, size_t len);
AssetName assets_make_asset_name_str(const char *name);

AssetId assets_make_id(const char *name, size_t len);
AssetId assets_make_id_str(const char *name);

AssetFilePath assets_make_asset_file_path(const char *file_path);

int assets_asset_name_eq(AssetName *lhs, AssetName *rhs);

// TODO: Nuke this. Type type should be implicit from its existence
// in container for a given asset type
typedef enum AssetType {
  AssetTypeTexture,
  AssetTypeTextureTiled,
  AssetTypeShader,
  AssetTypeShaderProgram,
  AssetTypeMaterial,
  AssetTypeMap,
  AssetTypeMesh,
  AssetTypeFont
} AssetType;

static const char *AssetTypeNames[] = {
    "Texture",  "TextureTiled", "Shader", "ShaderProgram",
    "Material", "Map",          "Mesh"};

typedef struct Asset {
  AssetType type;
  AssetId id;
} Asset;

typedef struct AssetMeta {
  AssetId id;
  AssetType type;
  AssetName name;
  AssetFilePath file_path;
  size_t file_size;
} AssetMeta;

typedef struct {
  AssetId id;
  ImageMeta meta;
} AssetTexture;

typedef struct {
  AssetId id;
  Vec4u8 color;
  AssetId texture_id;
} AssetMaterial;

typedef enum AssetShaderType {
  AssetShaderTypeVertex = 0,
  AssetShaderTypeTessCtrl = 1,
  AssetShaderTypeTessEval = 2,
  AssetShaderTypeGeometry = 3,
  AssetShaderTypeFragment = 4,
  AssetShaderTypeTypeCount = 5,
  AssetShaderTypeProgram = 16,

  AssetShaderTypeUnknown = 0xFF,
  AssetShaderTypeNone = 0xFF
} AssetShaderType;

typedef struct {
  AssetId id;
  AssetShaderType shader_type;
  Buffer source_buffer;
} AssetShader;

typedef struct AssetShaderProgram {
  AssetId id;

  // There are only 6 types of shaders in openGL.
  // Let's assume we're in OpenGL land for now
  AssetId shader_ids[AssetShaderTypeTypeCount];
  int has_shader;
} AssetShaderProgram;

static inline void assets_shader_program_set_shader(AssetShaderProgram *prog,
                                                    AssetShaderType type,
                                                    AssetId id) {
  prog->shader_ids[type] = id;
  prog->has_shader |= (0x1 << (int)type);
}

typedef struct AssetMesh {
  AssetId id;
  Mesh mesh;
} AssetMesh;

typedef struct AssetMap {
  AssetId id;
  Vec2u32 size_world;
  Vec2u32 size_tilemap;
  AssetId tilemap_texture_id;
  size_t num_indices;
  Vec2u8 *indices; // Size world x*y encodes how big this should be
} AssetMap;

typedef struct AssetFont {
  AssetId id;
  Buffer font_data;
} AssetFont;

int assets_shader_program_has_shader(AssetShaderProgram *program, size_t index);

typedef struct Assets {
  Vec textures;
  Vec programs;
  Vec shaders;
  Vec materials;
  Vec meshes;
  Vec maps;
  Vec asset_meta;

  // For assets with dynamic size. User must know
  // its data and call 'assets_clear_temp_asset_data'
  SubArenaAllocator temp_data;
} Assets;

void assets_init(struct Assets *assets);

void assets_clear_temp_data(struct Assets *assets);

AssetName *assets_asset_name_get_by_id(struct Assets *assets, AssetId id);

int assets_asset_id_get_by_name(struct Assets *assets, const char *name,
                                AssetId *id);

AssetMeta *assets_asset_meta_get(struct Assets *assets, AssetId id);

int assets_load_texture(struct Assets *assets, AssetId id,
                        AssetTexture *texture, void **texture_data);

int assets_load_shader(struct Assets *assets, AssetId id, AssetShader *shader);

int assets_load_shader_program(struct Assets *assets, AssetId asset_id,
                               AssetShaderProgram *program);

int assets_load_material(struct Assets *assets, AssetId id,
                         AssetMaterial *material);

int assets_load_map(struct Assets *assets, AssetId id, AssetMap *map);

int assets_load_font(struct Assets *assets, AssetId id, AssetFont *font);

AssetMaterial *assets_get_material(struct Assets *assets, AssetId material_id);

void assets_add_mesh(struct Assets *assets, Mesh *mesh, const char *name);

Mesh *assets_get_mesh(struct Assets *assets, AssetId mesh_id);

AssetShaderProgram *assets_get_program(struct Assets *assets,
                                       AssetId program_id);

AssetShader *assets_get_shader(struct Assets *assets, AssetId shader_id);

#ifdef BUILD_TESTS
void assetstore_test();
#endif

#endif // ASSETSTORE_H
