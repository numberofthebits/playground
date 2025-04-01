#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdlib.h>

#define ENTITY_INVALID_INDEX (EntityIndex)(-1)

#define TRUE 1
#define FALSE 0

typedef struct {
  int width;
  int height;
  int channels;
} ImageMeta;

typedef uint32_t AssetId;
typedef uint32_t SignatureT;
typedef int EntityId;
typedef size_t EntityIndex;
typedef uint64_t EntityFlags;

struct Entity_t {
  // The index of the entity data
  EntityIndex index;

  // The id of the entity, so we can tell entities that reuse indices
  // apart
  EntityId id;
};
typedef struct Entity_t Entity;

#endif // TYPES_H
