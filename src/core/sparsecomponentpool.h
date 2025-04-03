#ifndef SPARSECOMPONENTPOOL_H
#define SPARSECOMPONENTPOOL_H

#include "types.h"
#include "vec.h"

#include <stdint.h>

typedef struct {
  size_t max_entities;

  // Number of actual entities in this pool, i.e. the packed components
  // and entity list
  size_t count;
  void *data;
  const struct Component *descriptor;

  // Packed indices. Allocated for worst case.
  // Size is the same as 'count' for data
  EntityIndex *packed;

  EntityIndex *sparse;

} Pool;

void pool_init(Pool *scp, const struct Component *descriptor,
               size_t max_entities);

Bool pool_has_entity(Pool *p, Entity e);

void pool_insert(Pool *pool, Entity e, void *data);

void pool_remove(Pool *pool, Entity e);

static inline void *pool_get(Pool *pool, EntityIndex index,
                             size_t element_size) {
  if (pool->sparse[index] == ENTITY_INVALID_INDEX) {
    return 0;
  }

  int packed_index = pool->sparse[index];

  return &pool->data[packed_index * element_size];
}

#define PoolGetComponent(pool, type, index)                                    \
  (type *)pool_get(pool, index, sizeof(type));

#ifdef BUILD_TESTS
void pool_test();
#endif

#endif
