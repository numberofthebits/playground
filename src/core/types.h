#ifndef TYPES_H
#define TYPES_H

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

struct Entity_t {
    EntityId id;
    size_t index;
};
typedef struct Entity_t Entity;

#endif
