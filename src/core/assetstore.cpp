#include "assetstore.h"

#include "keyvalueparser.h"
#include "log.h"
#include "util.h"

#include <stb_image.h>

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXTENSION_VERTEX_SHADER ".vert"
#define EXTENSION_FRAGMENT_SHADER ".frag"
#define EXTENSION_SHADER_PROGRAM ".prog"

#define MAKE_FILE_PATH_BUF_MAX 1024
#define ASSETS_ALLOCATOR_MAX_SIZE 1024 * 1024 * 64

AssetName assets_make_asset_name(const char *name, size_t len) {
  if (len > ASSET_NAME_MAX_LEN) {
    LOG_WARN("Truncating asset name '%' length %d exceeding max length %d",
             name, len, ASSET_NAME_MAX_LEN);
  }

  AssetName asset_name;
  memset(asset_name.name, 0x0, ASSET_NAME_MAX_LEN);
  memcpy(asset_name.name, name, len);
  asset_name.len = (uint8_t)len;

  return asset_name;
}

AssetName assets_make_asset_name_str(const char *name) {
  return assets_make_asset_name(name, strlen(name));
}

AssetFilePath assets_make_asset_file_path(const char *file_path) {
  size_t len = strlen(file_path);
  if (len >= ASSET_FILE_PATH_MAX) {
    LOG_EXIT("Asset file path '%s' too long (max %d)", file_path,
             ASSET_FILE_PATH_MAX);
  }

  AssetFilePath asset_path = {};
  strcpy(asset_path.path, file_path);
  asset_path.len = len;

  return asset_path;
}

int assets_asset_name_eq(AssetName *lhs, AssetName *rhs) {
  /* size_t len = (lhs->len > rhs->len ? (size_t)lhs->len : rhs->len); */
  /* LOG_INFO("lhs %.*s rhs %.*s", lhs->len, lhs->name, rhs->len, rhs->name); */
  /* return memcmp(lhs->name, rhs->name, len) == 0; */
  return strcmp(lhs->name, rhs->name) == 0;
}

typedef struct _TileInfo {
  uint16_t x;
  uint16_t y;
} TileInfo;

static void insert_asset_meta(struct Assets *assets, AssetId id, AssetType type,
                              AssetFilePath *file_path, AssetName *name) {
  AssetMeta meta;
  meta.id = id;
  meta.type = type;
  meta.name = *name;
  meta.file_path = *file_path;

  VEC_PUSH_T(&assets->asset_meta, AssetMeta, meta);
  /* hash_map_insert_copy(&assets->asset_meta, &id, sizeof(id), &meta, */
  /*                     sizeof(meta)); */
}

static int load_texture_data(const char *file_path, AssetTexture *texture,
                             void **texture_data) {
  stbi_set_flip_vertically_on_load(1);
  int x = 0;
  int y = 0;
  int comp = 0;

  *texture_data = stbi_load(file_path, &x, &y, &comp, 0);

  if (!*texture_data) {
    LOG_ERROR("Failed to load texture '%s'", file_path);
    return 0;
  }

  LOG_INFO("Loaded texture data '%s': pixel size (%d, %d). Image channels %d",
           file_path, x, y, comp);

  texture->meta.width = (uint16_t)x;
  texture->meta.height = (uint16_t)y;
  texture->meta.channels = comp;

  return 1;
}

static int enumerate_textures_callback(const FileSystemListResult *result,
                                       void *user_data) {
  if (!result) {
    LOG_EXIT("Invalid file system list result");
  }

  if (!result->file_path || !result->extension || !result->file_name) {
    LOG_WARN("Texture callback failed: path '%s' filename '%s' extension '%s'");
    return FILE_CALLBACK_RESULT_CONTINUE;
  }

  struct Assets *assets = (struct Assets *)user_data;

  AssetId id = assets_make_id_str(result->file_name);
  AssetName name = assets_make_asset_name_str(result->file_name);
  AssetFilePath file_path = assets_make_asset_file_path(result->file_path);

  insert_asset_meta(assets, id, AssetTypeTexture, &file_path, &name);

  return FILE_CALLBACK_RESULT_CONTINUE;
}

static AssetShaderType extension_to_shader_type(const char *extension) {
  if (!extension) {
    return AssetShaderTypeUnknown;
  }

  if (strcmp(extension, EXTENSION_VERTEX_SHADER) == 0) {
    return AssetShaderTypeVertex;
  } else if (strcmp(extension, EXTENSION_FRAGMENT_SHADER) == 0) {
    return AssetShaderTypeFragment;
  } else if (strcmp(extension, EXTENSION_SHADER_PROGRAM) == 0) {
    return AssetShaderTypeProgram;
  }

  return AssetShaderTypeUnknown;
}

AssetName *assets_asset_name_get_by_id(struct Assets *assets, AssetId id) {

  AssetMeta *meta = 0;
  for (int i = 0; i < assets->asset_meta.size; ++i) {
    meta = VEC_GET_T_PTR(&assets->asset_meta, AssetMeta, i);
    if (meta->id == id) {
      return &meta->name;
    }
  }

  return 0;
}

int assets_asset_id_get_by_name(struct Assets *assets, const char *name,
                                AssetId *id) {
  AssetMeta *meta = 0;
  AssetName asset_name = assets_make_asset_name_str(name);

  for (int i = 0; i < assets->asset_meta.size; ++i) {
    meta = VEC_GET_T_PTR(&assets->asset_meta, AssetMeta, i);
    if (assets_asset_name_eq(&meta->name, &asset_name)) {
      *id = meta->id;
      return 1;
    }
  }

  LOG_ERROR("Failed to get asset ID for '%s'", name);

  return 0;
}

int assets_shader_program_has_shader(AssetShaderProgram *program,
                                     size_t index) {
  return program->has_shader & (0x1 << index);
}

void load_materials(struct Assets *assets) {
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
  jungle_map.color.a = 0;   // a
  jungle_map.texture_id = assets_make_id_str("jungle.png");

  AssetMaterial chopper_udlr;
  chopper_udlr.id = assets_make_id_str("chopper-udlr");
  chopper_udlr.color.r = 255;
  chopper_udlr.color.g = 255;
  chopper_udlr.color.b = 255;
  chopper_udlr.color.a = 255;
  chopper_udlr.texture_id = assets_make_id_str("chopper-spritesheet.png");

  AssetMaterial plain_red;
  plain_red.id = assets_make_id_str("plain-red");
  plain_red.color.r = 255;
  plain_red.color.g = 0;
  plain_red.color.b = 0;
  plain_red.texture_id = assets_make_id_str("white-pixel.png");

  AssetMaterial bullet;
  bullet.id = assets_make_id_str("bullet-mat");
  bullet.color.r = 255;
  bullet.color.g = 255;
  bullet.color.b = 255;
  bullet.color.a = 0;
  bullet.texture_id = assets_make_id_str("bullet.png");

  VEC_PUSH_T(&assets->materials, AssetMaterial, truck_red);
  VEC_PUSH_T(&assets->materials, AssetMaterial, truck_blue);
  VEC_PUSH_T(&assets->materials, AssetMaterial, jungle_map);
  VEC_PUSH_T(&assets->materials, AssetMaterial, chopper_udlr);
  VEC_PUSH_T(&assets->materials, AssetMaterial, plain_red);
  VEC_PUSH_T(&assets->materials, AssetMaterial, bullet);
}

static void parse_shader_program_entry(AssetShaderProgram *program,
                                       unsigned char *begin, unsigned char *end,
                                       unsigned char *ext_begin) {
  const size_t ext_buffer_size = 256;
  char ext_buffer[ext_buffer_size];
  memset(ext_buffer, 0x0, ext_buffer_size);
  size_t ext_len = end - ext_begin;
  memcpy(ext_buffer, ext_begin, ext_len);
  int shader_type = extension_to_shader_type(ext_buffer);

  program->shader_ids[shader_type] =
      assets_make_id((const char *)begin, end - begin);
  program->has_shader |= (0x1 << shader_type);
}

static AssetShaderProgram parse_shader_program(const char *file_path) {
  AssetShaderProgram program = {};
  size_t buffer_size = 1024 * 1024;

  Buffer buffer = {.data = StackAlloc<uint8_t>(&stack_allocator, buffer_size),
                   .capacity = buffer_size,
                   .used = 0};

  // The returned file size should never be greater than buffer size
  size_t file_size = file_read_all_buffer_text(file_path, &buffer);

  if (!file_size) {
    LOG_ERROR("Failed to read file: %s", file_path);
    return program;
  }

  size_t pos_begin = 0;
  size_t ext_begin = 0;
  size_t i = 0;

  while (i < file_size) {
    switch (buffer.data[i]) {
    case '.':
      ext_begin = i;
      ++i;
      break;
    case '\r': {
      parse_shader_program_entry(&program, buffer.data + pos_begin,
                                 buffer.data + i, buffer.data + ext_begin);
      i += 2;
      pos_begin = i; // skip CRLF '\r\n'
    } break;
    case '\n': {
      parse_shader_program_entry(&program, buffer.data + pos_begin,
                                 buffer.data + i, buffer.data + ext_begin);
      i += 1;
      pos_begin = i; // skip LF '\n'
    } break;
    default:;
      ++i;
    }
  }

  // In case there's no newline, parse the remaining line.
  parse_shader_program_entry(&program, buffer.data + pos_begin, buffer.data + i,
                             buffer.data + ext_begin);

  stack_dealloc(&stack_allocator, buffer.data, buffer.capacity);
  return program;
}

static int enumerate_shaders_callback(const FileSystemListResult *result,
                                      void *context) {
  if (!result) {
    LOG_EXIT("Invalid file system list result");
  }

  if (strcmp(result->extension, ".") == 0 ||
      strcmp(result->extension, "..") == 0) {
    LOG_INFO("Skipping result '%s'", result->extension);
    return FILE_CALLBACK_RESULT_CONTINUE;
  }

  struct Assets *assets = (struct Assets *)context;

  AssetName name = assets_make_asset_name_str(result->file_name);
  AssetId id = assets_make_id_str(result->file_name);
  AssetFilePath file_path = assets_make_asset_file_path(result->file_path);
  int shader_type = extension_to_shader_type(result->extension);

  switch (shader_type) {
  case AssetShaderTypeVertex:
  case AssetShaderTypeTessCtrl:
  case AssetShaderTypeTessEval:
  case AssetShaderTypeGeometry:
  case AssetShaderTypeFragment: {
    insert_asset_meta(assets, id, AssetTypeShader, &file_path, &name);
  } break;
  case AssetShaderTypeProgram: {
    insert_asset_meta(assets, id, AssetTypeShaderProgram, &file_path, &name);
  } break;
  default:
    LOG_WARN("Unknown shader asset type: %s", result->extension);
    return FILE_CALLBACK_RESULT_CONTINUE;
  }

  return FILE_CALLBACK_RESULT_CONTINUE;
}

static int enumerate_tilemaps_callback(const FileSystemListResult *result,
                                       void *context) {
  if (!result) {
    LOG_EXIT("Invalid file system list result");
  }

  struct Assets *assets = (struct Assets *)context;

  if (strcmp(result->extension, ".meta") != 0) {
    LOG_INFO("Ignoring tilemap file '%s'", result->file_path);
    return FILE_CALLBACK_RESULT_CONTINUE;
  }

  AssetFilePath file_path = assets_make_asset_file_path(result->file_path);
  AssetName name = assets_make_asset_name_str(result->file_name);
  AssetId id = assets_make_id_str(result->file_name);

  insert_asset_meta(assets, id, AssetTypeMap, &file_path, &name);

  return FILE_CALLBACK_RESULT_CONTINUE;
}

static int enumerate_materials_callback(const FileSystemListResult *result,
                                        void *context) {
  if (!result) {
    LOG_EXIT("Invalid file system list result");
  }

  struct Assets *assets = (struct Assets *)context;

  AssetId id = assets_make_id_str(result->file_name);
  AssetName name = assets_make_asset_name_str(result->file_name);
  AssetFilePath file_path = assets_make_asset_file_path(result->file_path);

  insert_asset_meta(assets, id, AssetTypeMaterial, &file_path, &name);

  return FILE_CALLBACK_RESULT_CONTINUE;
}

static int enumerate_fonts_callback(const FileSystemListResult *result,
                                    void *context) {

  if (!result) {
    LOG_EXIT("Invalid file system list result");
  }

  struct Assets *assets = (struct Assets *)context;

  AssetId id = assets_make_id_str(result->file_name);
  AssetName name = assets_make_asset_name_str(result->file_name);
  AssetFilePath file_path = assets_make_asset_file_path(result->file_path);

  insert_asset_meta(assets, id, AssetTypeMaterial, &file_path, &name);

  return FILE_CALLBACK_RESULT_CONTINUE;
}

void assets_print_asset_meta(struct Assets *assets) {
  printf("*********** ASSET META *************\n");
  for (int i = 0; i < assets->asset_meta.size; ++i) {
    AssetMeta *meta = VEC_GET_T_PTR(&assets->asset_meta, AssetMeta, i);
    (void)meta;
    printf("%d Id: %zu\tType: %s\tName: %s\tPath: %s\n", i, meta->id,
           AssetTypeNames[meta->type], meta->name.name, meta->file_path.path);
  }
}

void assets_enumerate(struct Assets *assets) {
  (void)assets;

  file_system_list("./assets/shaders/", "*.*", &enumerate_shaders_callback,
                   assets);
  file_system_list("./assets/tilemaps/", "*.meta*",
                   &enumerate_tilemaps_callback, assets);

  file_system_list("./assets/images/", "*.png", &enumerate_textures_callback,
                   assets);

  file_system_list("./assets/materials/", "*.mat",
                   &enumerate_materials_callback, assets);

  file_system_list("./assets/fonts/", "*.ttf", &enumerate_fonts_callback,
                   assets);

  assets_print_asset_meta(assets);
}

void assets_init(struct Assets *assets) {
  assets->temp_data = arena_subarena_create(&global_static_allocator,
                                            ASSETS_ALLOCATOR_MAX_SIZE);

  assets->textures = vec_create();
  assets->shaders = vec_create();
  assets->materials = vec_create();
  assets->programs = vec_create();
  assets->maps = vec_create();
  assets->meshes = vec_create();
  assets->asset_meta = vec_create();

  assets_enumerate(assets);
}

void assets_clear_temp_data(struct Assets *assets) {
  arena_subarena_dealloc_all(&assets->temp_data);
}

AssetId assets_make_id(const char *name, size_t len) { return hash(name, len); }

AssetId assets_make_id_str(const char *name) {
  return assets_make_id(name, strlen(name));
}

// TODO: Why was this suddenly unused? Bad refactoring?
//       Removed static to get rid of warning. Might not need it
//       but keep around unless I forgot why this is a thing.
int assets_make_file_path(const char *path, const char *file_name, char *buf,
                          unsigned int buf_len) {
  // +2 add one for newline and one for path separator
  if (strlen(path) + strlen(file_name) + 2 >= buf_len) {
    return 0;
  }
  strcat(buf, path);
  strcat(buf, "/");
  strcat(buf, file_name);

  return 1;
}

AssetMeta *assets_asset_meta_get(struct Assets *assets, AssetId id) {
  for (int i = 0; i < assets->asset_meta.size; ++i) {
    AssetMeta *meta = VEC_GET_T_PTR(&assets->asset_meta, AssetMeta, i);
    AssetMeta deref = *meta;
    if (deref.id == id) {
      return meta;
    }
  }

  LOG_WARN("Asset meta not found: %u", id);

  return 0;
}

AssetMaterial *assets_get_material(struct Assets *assets, AssetId material_id) {

  for (int i = 0; i < assets->materials.size; ++i) {
    AssetMaterial *mat = VEC_GET_T_PTR(&assets->materials, AssetMaterial, i);
    if (mat->id == material_id) {
      return mat;
    }
  }

  return 0;
}

int assets_load_texture(struct Assets *assets, AssetId asset_id,
                        AssetTexture *texture, void **texture_data) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, asset_id);
  if (!asset_meta) {
    return 0;
  }

  texture->id = asset_id;

  if (!load_texture_data(asset_meta->file_path.path, texture, texture_data)) {
    LOG_WARN("Material %s id %u not found", asset_meta->name, asset_id);
    return 0;
  }

  return 1;
}

int assets_load_shader(struct Assets *assets, AssetId asset_id,
                       AssetShader *shader) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, asset_id);
  if (!asset_meta) {
    return 0;
  }

  shader->id = asset_id;

  int ret = file_read_all_buffer_text(asset_meta->file_path.path,
                                      &shader->source_buffer);

  // TODO: Extract this to util.*?
  int len_path = strlen(asset_meta->file_path.path);
  const char *extension_ptr = asset_meta->file_path.path + len_path;

  while ((*extension_ptr != '.') &&
         (extension_ptr != asset_meta->file_path.path)) {
    --extension_ptr;
  }

  shader->shader_type = extension_to_shader_type(extension_ptr);

  return ret;
}

static int load_shader_program(const char *file_path,
                               AssetShaderProgram *program) {
  *program = parse_shader_program(file_path);
  if (program->has_shader == 0) {
    LOG_WARN("Program has no shaders: '%s'", file_path);
    return 0;
  }
  return 1;
}

int assets_load_shader_program(struct Assets *assets, AssetId asset_id,
                               AssetShaderProgram *program) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, asset_id);
  if (!asset_meta) {
    return 0;
  }

  program->id = asset_id;

  return load_shader_program(asset_meta->file_path.path, program);
}

#define PARSE_MATERIAL_BUFFER_SIZE 128
static int parse_material(Buffer *buffer, AssetMaterial *material) {
  int ret = 0;
  ParseState state = {.buffer = buffer, .offset = 0};
  while (!parse_is_done(&state)) {
    KeyValueRaw raw = {};

    int status = get_next_key_value_raw(&state, &raw);
    switch (status) {
    case PARSER_ERROR:
      return 0;
    case PARSER_DONE:
      return 1;
    case PARSER_OK:
      if (strncmp(raw.key_begin, "color", strlen("color")) == 0) {
        if (!parse_array_u8(raw.value_begin, raw.value_len, 4,
                            &material->color.r)) {
          return 0;
        }

        ++ret;

      } else if (strncmp(raw.key_begin, "texture", strlen("texture")) == 0) {
        material->texture_id = assets_make_id(raw.value_begin, raw.value_len);
        ++ret;
      }

      break;
    default:
      LOG_ERROR("Unhandled key value raw status %d", status);
      break;
    }
  }
  return ret == 2;
}

static int load_material(const char *file_path, AssetMaterial *material) {
  size_t buffer_size = 1024 * 1024;
  Buffer buffer = {};
  buffer.data = StackAlloc<uint8_t>(&stack_allocator, buffer_size);
  buffer.capacity = 1024 * 1024;

  if (!file_read_all_buffer_text(file_path, &buffer)) {
    return 0;
  }

  int ret = parse_material(&buffer, material);
  stack_dealloc(&stack_allocator, buffer.data, buffer_size);
  return ret;
}

int assets_load_material(struct Assets *assets, AssetId asset_id,
                         AssetMaterial *material) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, asset_id);
  if (!asset_meta) {
    return 0;
  }

  if (!load_material(asset_meta->file_path.path, material)) {
    LOG_EXIT("Failed to find material with id %u: '%s", asset_id,
             asset_meta->name);
    return 0;
  }

  return 1;
}

static int parse_map_meta(Buffer *buffer, struct SubArenaAllocator *sub_arena,
                          AssetMap *map) {
  ParseState parser = {.buffer = buffer, .offset = 0};
  size_t num_parsed = 0;
  while (!parse_is_done(&parser)) {
    KeyValueRaw raw = {};
    int status = get_next_key_value_raw(&parser, &raw);
    switch (status) {
    case PARSER_OK:
      if (strncmp(raw.key_begin, "size_world", strlen("size_world")) == 0) {
        if (!parse_array_u32(raw.value_begin, raw.value_len, 2,
                             &map->size_world.x)) {
          return 0;
        }
        ++num_parsed;
      } else if (strncmp(raw.key_begin, "size_tile_map",
                         strlen("size_tile_map")) == 0) {
        if (!parse_array_u32(raw.value_begin, raw.value_len, 2UL,
                             &map->size_tilemap.x)) {
          return 0;
        }
        ++num_parsed;
      } else if (strncmp(raw.key_begin, "tilemap_texture",
                         strlen("tilemap_texture")) == 0) {
        map->tilemap_texture_id =
            assets_make_id(raw.value_begin, raw.value_len);
        ++num_parsed;
      } else if (strncmp(raw.key_begin, "tilemap_indices",
                         strlen("tilemap_indices")) == 0) {
        map->num_indices = map->size_world.x * map->size_world.y;
        map->indices = SubArenaAlloc<Vec2u8>(sub_arena, map->num_indices);

        uint8_t *packed_tilemap_coords =
            StackAlloc<uint8_t>(&stack_allocator, map->num_indices);
        if (!parse_array_u8(raw.value_begin, raw.value_len, map->num_indices,
                            packed_tilemap_coords)) {
          return 0;
        }

        for (size_t i = 0; i < map->num_indices; ++i) {
          uint8_t x = packed_tilemap_coords[i] % 10;
          uint8_t y = packed_tilemap_coords[i] / 10;

          map->indices[i].x = x;
          map->indices[i].y = y;
        }

        stack_dealloc(&stack_allocator, packed_tilemap_coords,
                      map->num_indices);

        ++num_parsed;
      }
      break;
    case PARSER_DONE:
      return num_parsed == 4;
    case PARSER_ERROR:
      return 0;
    default:
      return num_parsed == 4;
    }
  }

  return num_parsed == 4;
}

int assets_load_map(struct Assets *assets, AssetId asset_id, AssetMap *map) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, asset_id);
  if (!asset_meta) {
    return 0;
  }

  size_t buffer_size = 1024 * 1024;
  Buffer buffer = {};
  buffer.data = StackAlloc<uint8_t>(&stack_allocator, buffer_size);
  buffer.capacity = buffer_size;

  buffer.used = file_read_all_buffer_text(asset_meta->file_path.path, &buffer);

  if (!buffer.used) {
    LOG_WARN("Failed to read %s", asset_meta->file_path.path);
    return 0;
  }

  if (!parse_map_meta(&buffer, &assets->temp_data, map)) {
    return 0;
  }

  stack_dealloc(&stack_allocator, buffer.data, buffer.capacity);

  // Release dynamically allocated data used by AssetMap
  assets_clear_temp_data(assets);

  return 1;
}

int assets_load_font(struct Assets *assets, AssetId id, AssetFont *font) {
  AssetMeta *asset_meta = assets_asset_meta_get(assets, id);
  if (!asset_meta) {
    return 0;
  }

  int file_size = file_get_size(asset_meta->file_path.path);
  if (file_size < 0) {
    LOG_EXIT("No file size for '%s'", asset_meta->file_path.path);
  }

  uint8_t *file_buf =
      SubArenaAlloc<uint8_t>(&assets->temp_data, (size_t)file_size);

  font->id = id;
  font->font_data = {
      .data = file_buf, .capacity = (size_t)file_size, .used = 0};
  if (!file_read_all_buffer_binary(asset_meta->file_path.path,
                                   font->font_data.data,
                                   font->font_data.capacity)) {
    assets_clear_temp_data(assets);
    return 0;
  }

  return 1;
}

AssetShaderProgram *assets_get_program(struct Assets *assets,
                                       AssetId program_id) {
  for (int i = 0; i < assets->programs.size; ++i) {
    AssetShaderProgram *program =
        VEC_GET_T_PTR(&assets->programs, AssetShaderProgram, i);
    if (program->id == program_id) {
      return program;
    }
  }

  return 0;
}

AssetShader *assets_get_shader(struct Assets *assets, AssetId shader_id) {
  for (int i = 0; i < assets->shaders.size; ++i) {
    AssetShader *shader = VEC_GET_T_PTR(&assets->shaders, AssetShader, i);
    if (shader->id == shader_id) {
      return shader;
    }
  }

  return 0;
}

#ifdef BUILD_TESTS
void parse_material_test() {
  const char *material_str =
      "texture=truck-ford-right.png\ncolor=255,254,253,252,\n";
  Buffer buffer;
  buffer.data = (unsigned char *)material_str;
  AssetMaterial material = {};
  buffer.capacity = strlen(material_str);
  buffer.used = buffer.capacity;

  Assert(parse_material(&buffer, &material));
  AssetId expected_texture_id = assets_make_id_str("truck-ford-right.png");
  Assert(material.texture_id == expected_texture_id);
  Assert(material.color.r == 255);
  Assert(material.color.g == 254);
  Assert(material.color.b == 253);
  Assert(material.color.a == 252);
}

void assetstore_test() {
  // Bleh. The arena and stack must be
  // set up before we test things.

  /* parse_material_test(); */
  /* parse_shader_program("./assets/shaders/tilemap.prog"); */
  /* AssetShaderProgram tilemap = {}; */
  /* tilemap.id =
   * assets_make_id_str("tilemap.prog"); */

  /* assets_shader_program_set_shader(&tilemap,
   * AssetShaderTypeVertex, */
  /*                                  assets_make_id_str("tilemap.vert"));
   */

  /* assets_shader_program_set_shader(&tilemap,
   * AssetShaderTypeFragment, */
  /*                                  assets_make_id_str("tilemap.frag"));
   */

  /* FILE *fp =
   * fopen("./assets/shaders/tilemap.prog",
   * "w+b"); */
  /* int ret = fwrite(&tilemap, 1,
   * sizeof(tilemap), fp); */
  /* Assert(ret == sizeof(tilemap)); */
  /* fclose(fp); */

  /* fp = fopen("./assets/shaders/tilemap.prog",
   * "r+b"); */
  /* AssetShaderProgram prog2 = {}; */
  /* fread(&prog2, 1, sizeof(prog2), fp); */
  /* fclose(fp); */

  /* AssetShaderProgram unit = {}; */
  /* unit.id = assets_make_id_str("unit.prog");
   */
  /* assets_shader_program_set_shader(&unit,
   * AssetShaderTypeVertex, */
  /*                                  assets_make_id_str("unit.vert"));
   */
  /* assets_shader_program_set_shader(&unit,
   * AssetShaderTypeFragment, */
  /*                                  assets_make_id_str("unit.frag"));
   */

  /* fp = fopen("./assets/shaders/unit.prog",
   * "w+b"); */
  /* ret = fwrite(&unit, 1, sizeof(unit), fp); */
  /* fclose(fp); */
  /* Assert(ret == sizeof(unit)); */

  /* AssetShaderProgram collision_debug = {}; */
  /* collision_debug.id =
   * assets_make_id_str("collision_debug.prog");
   */
  /* assets_shader_program_set_shader(&collision_debug,
   * AssetShaderTypeVertex,
   */
  /*                                  assets_make_id_str("collision_debug.vert"));
   */
  /* assets_shader_program_set_shader(&collision_debug,
   * AssetShaderTypeFragment,
   */
  /*                                  assets_make_id_str("collision_debug.frag"));
   */

  /* fp =
   * fopen("./assets/shaders/collision_debug.prog",
   * "w+b"); */
  /* ret = fwrite(&collision_debug, 1,
   * sizeof(collision_debug), fp); */
  /* fclose(fp); */
  /* Assert(ret == sizeof(collision_debug)); */
}
#endif
