#ifndef SPARSECOMPONENTPOOL_H
#define SPARSECOMPONENTPOOL_H

#include "types.h"
#include "vec.h"

#include <stdint.h>

typedef struct {

  // Number of actual entities in this pool, i.e. the packed components
  // and entity list
  size_t count;
  void *data;
  const struct Component *descriptor;

  // Packed indices. Allocated for worst case.
  // Size is the same as 'count' for data
  EntityIndex *indices;

  EntityIndex *entities;
  size_t max_entities;

  size_t num_indices;

} Pool;

void pool_init(Pool *scp, const struct Component *descriptor,
               size_t max_entities);

void pool_insert(Pool *pool, Entity e, void *data);

void *pool_get(Pool *pool, EntityIndex index);

#define PoolGetComponent(pool, type, index) (type *)pool_get(pool, index);

#ifdef BUILD_TESTS
void pool_test();
#endif

#endif
