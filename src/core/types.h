#ifndef TYPES_H
#define TYPES_H

#include <stdlib.h>
#include <stdint.h>

#define ENTITY_INVALID_INDEX -1

typedef struct {
    int width;
    int height;
    int channels;
} ImageMeta;

typedef uint32_t AssetId;
typedef uint8_t SignatureT;
typedef int EntityId;
typedef size_t EntityIndex;

struct Entity_t {
    EntityIndex index;
    EntityId id;
};
typedef struct Entity_t Entity;

#endif // TYPES_H
