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

  pool->indices =
      ArenaAlloc(&global_static_allocator, max_entities, EntityIndex);

  pool->entities =
      ArenaAlloc(&global_static_allocator, max_entities, EntityIndex);

  for (EntityIndex i = 0; i < max_entities; ++i) {
    pool->entities[i] = ENTITY_INVALID_INDEX;
  }

  pool->max_entities = max_entities;
}

void pool_insert(Pool *pool, Entity e, void *data) {
  size_t current_idx = pool->count;

  pool->entities[e.index] = current_idx;

  ptrdiff_t ptr_offset = current_idx * pool->descriptor->size;
  memcpy((char *)pool->data + ptr_offset, data, pool->descriptor->size);

  pool->indices[current_idx] = e.index;

  pool->count++;
}

void *pool_get(Pool *pool, EntityIndex index) {
  if (pool->entities[index] == ENTITY_INVALID_INDEX) {
    return 0;
  }

  int packed_index = pool->entities[index];

  return &pool->data[packed_index * pool->descriptor->size];
}

// void pool_remove(Pool *pool, Entity e) {}
#ifdef BUILD_TESTS
void pool_test() {
  const int max_entities = 1024;
  struct Component c;
  c.alignment = 8;
  c.name = "test_component";
  c.size = 8;

  Pool p;

  pool_init(&p, &c, max_entities);

  for (int i = 0; i < max_entities; ++i) {
    assert(p.entities[i] == ENTITY_INVALID_INDEX);
  }

  Entity e;
  e.id = 0;
  e.index = 512;

  int entity_data[2] = {123, 987};

  pool_insert(&p, e, entity_data);

  assert(p.count == 1);
  assert(p.entities[e.index] == 0);
  assert(p.indices[0] == e.index);

  e.id = 0;
  e.index = 0;

  pool_insert(&p, e, entity_data);

  assert(p.count == 2);
  assert(p.entities[e.index] == 1);
  assert(p.indices[1] == e.index);
}
#endif
