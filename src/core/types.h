#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdlib.h>

#define ENTITY_INVALID_INDEX (EntityIndex)(-1)

#define COMPONENT_ACCESS_READ 0x1
#define COMPONENT_ACCESS_WRITE 0x2
#define COMPONENT_ACCESS_READ_WRITE                                            \
  (COMPONENT_ACCESS_READ | COMPONENT_ACCESS_WRITE)

#define TRUE 1
#define FALSE 0

typedef struct {
  int width;
  int height;
  int channels;
} ImageMeta;

typedef size_t AssetId;
typedef uint32_t ComponentBitmaskT;
typedef ComponentBitmaskT ComponentSignatureT;
typedef ComponentBitmaskT ComponentReadAccessT;
typedef ComponentBitmaskT ComponentWriteAccessT;
typedef int EntityId;
typedef size_t EntityIndex;
typedef uint64_t EntityFlags;
typedef int Bool;

typedef struct Entity {
  // The index of the entity data
  EntityIndex index;

  // The id of the entity, so we can tell entities that reuse indices
  // apart
  EntityId id;
} Entity;

#endif // TYPES_H
