#include "sparsecomponentpool.h"

#include "arena.h"
#include "componentbase.h"
#include "log.h"
#include "types.h"

#include <assert.h>
#include <string.h>

void pool_init(Pool *pool, const struct Component *descriptor,
               size_t max_entities) {
  pool->descriptor = descriptor;
  pool->data = arena_alloc(&global_static_allocator, descriptor->size,
                           max_entities, descriptor->alignment);
  pool->count = 0;

  pool->packed =
      ArenaAlloc(&global_static_allocator, max_entities, EntityIndex);

  pool->sparse =
      ArenaAlloc(&global_static_allocator, max_entities, EntityIndex);

  for (EntityIndex i = 0; i < max_entities; ++i) {
    pool->sparse[i] = ENTITY_INVALID_INDEX;
  }

  pool->max_entities = max_entities;
}

Bool pool_has_entity(Pool *p, Entity e) {
  return p->sparse[e.index] != ENTITY_INVALID_INDEX;
}

void pool_insert(Pool *pool, Entity e, void *data) {
  size_t current_idx = pool->count;

  pool->sparse[e.index] = current_idx;

  ptrdiff_t ptr_offset = current_idx * pool->descriptor->size;
  memcpy((char *)pool->data + ptr_offset, data, pool->descriptor->size);

  pool->packed[current_idx] = e.index;

  pool->count++;
}

void pool_remove(Pool *pool, Entity e) {
  if (!pool_has_entity(pool, e)) {
    return;
  }

  EntityIndex to_remove = pool->sparse[e.index];
  // If we passed the invalid entity test, we must have a count
  // of at least one
  EntityIndex to_move = pool->count - 1;
  pool->sparse[e.index] = ENTITY_INVALID_INDEX;

  // This gives us entity ID used to index into pool->sparse
  // The value of pool->sparse must be updated for the entity
  // where we moved its data and index
  EntityIndex moved_entity_id = pool->packed[to_move];
  pool->packed[to_remove] = pool->packed[to_move];
  pool->sparse[moved_entity_id] = to_remove;

  size_t count_bytes = pool->descriptor->size;
  ptrdiff_t ptr_offset_last = to_move * count_bytes;
  ptrdiff_t ptr_offset_removed = to_remove * count_bytes;
  memcpy((char *)pool->data + ptr_offset_removed,
         (char *)pool->data + ptr_offset_last, count_bytes);

  pool->count--;
}

#ifdef BUILD_TESTS
typedef struct {
  int a;
  int b;
} PoolTestComponentData;

void pool_test() {
  const int max_entities = 1024;
  struct Component c;
  c.alignment = 8;
  c.name = "test_component";
  c.size = 8;

  Pool p;

  pool_init(&p, &c, max_entities);

  for (int i = 0; i < max_entities; ++i) {
    assert(p.sparse[i] == ENTITY_INVALID_INDEX);
  }

  Entity e1;
  e1.id = 0;
  e1.index = 512;

  Entity e2;
  e2.id = 1;
  e2.index = 0;

  PoolTestComponentData entity_1_data = {123, 987};
  PoolTestComponentData entity_2_data = {456, 654};
  pool_insert(&p, e1, &entity_1_data);

  assert(p.count == 1);
  assert(p.sparse[e1.index] == 0);
  assert(p.packed[0] == e1.index);

  PoolTestComponentData *e1_data =
      PoolGetComponent(&p, PoolTestComponentData, e1.index);

  assert(e1_data->a == entity_1_data.a);
  assert(e1_data->b == entity_1_data.b);

  pool_insert(&p, e2, &entity_2_data);

  assert(p.count == 2);
  assert(p.sparse[e1.index] == 0);
  assert(p.sparse[e2.index] == 1);
  assert(p.packed[0] == e1.index);
  assert(p.packed[1] == e2.index);

  PoolTestComponentData *e2_data =
      PoolGetComponent(&p, PoolTestComponentData, e2.index);
  assert(e2_data->a == entity_2_data.a);
  assert(e2_data->b == entity_2_data.b);

  pool_remove(&p, e1);

  assert(p.count == 1);
  assert(p.sparse[e1.index] == ENTITY_INVALID_INDEX);
  assert(p.sparse[e2.index] == 0);
  assert(p.packed[0] == e2.index);

  e2_data = PoolGetComponent(&p, PoolTestComponentData, e2.index);
  assert(e2_data->a == entity_2_data.a);
  assert(e2_data->b == entity_2_data.b);

  pool_insert(&p, e1, &entity_1_data);
  assert(p.count == 2);
  assert(p.sparse[e1.index] == 1);
  assert(p.sparse[e2.index] == 0);
  assert(p.packed[0] == e2.index);
  assert(p.packed[1] == e1.index);
}
#endif
