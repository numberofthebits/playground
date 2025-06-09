#include "sparsecomponentpool.h"

#include "allocators.h"
#include "componentbase.h"
#include "log.h"
#include "types.h"
#include "util.h"

#include <stdio.h>
#include <string.h>

void pool_print(Pool *p) {
  printf("Sparse:\n");
  for (size_t i = 0; i < p->max_entities; ++i) {
    printf("#%zu=", i);
    if (p->sparse[i] == ENTITY_INVALID_INDEX) {
      printf("X");
    } else {
      printf("%zu", p->sparse[i]);
    }
    printf("\n");
  }

  printf("\nPacked:\n");

  for (size_t i = 0; i < p->count; ++i) {
    printf("%zu=%zu\n", i, p->packed[i]);
  }
}

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
  //  pool_print(pool);
}

void pool_remove(Pool *pool, Entity e) {
  if (!pool_has_entity(pool, e)) {
    return;
  }

  EntityIndex to_remove = pool->sparse[e.index];
  pool->sparse[e.index] = ENTITY_INVALID_INDEX;

  // swap packed_to_remove and last
  EntityIndex last = pool->count - 1;
  if (to_remove == last) {
    pool->count--;
    return;
  }

  EntityIndex sparse_to_update = pool->packed[last];
  pool->packed[to_remove] = pool->packed[last];
  pool->sparse[sparse_to_update] = to_remove;

  size_t count_bytes = pool->descriptor->size;
  ptrdiff_t ptr_offset_last = last * count_bytes;
  ptrdiff_t ptr_offset_removed = to_remove * count_bytes;
  memcpy((char *)pool->data + ptr_offset_removed,
         (char *)pool->data + ptr_offset_last, count_bytes);

  pool->count--;

  // pool_print(pool);
}

#ifdef BUILD_TESTS
typedef struct {
  int a;
} PoolTestComponentData;

void pool_test() {
  const int max_entities = 10;
  struct Component c;
  c.alignment = alignof(PoolTestComponentData);
  c.name = "test_component";
  c.size = sizeof(PoolTestComponentData);

  Pool p;

  pool_init(&p, &c, max_entities);

  for (int i = 0; i < max_entities; ++i) {
    Assert(p.sparse[i] == ENTITY_INVALID_INDEX);
  }

  Entity e1;
  e1.id = 0;
  e1.index = 0;

  Entity e2;
  e2.id = 1;
  e2.index = 1;

  Entity e3;
  e3.id = 2;
  e3.index = 2;

  size_t s = sizeof(PoolTestComponentData);
  (void)s;
  PoolTestComponentData entity_1_data = {111};
  PoolTestComponentData entity_2_data = {222};
  PoolTestComponentData entity_3_data = {333};
  pool_insert(&p, e1, &entity_1_data);
  pool_insert(&p, e2, &entity_2_data);
  pool_insert(&p, e3, &entity_3_data);

  Assert(p.count == 3);
  Assert(p.sparse[e1.index] == 0);
  Assert(p.sparse[e2.index] == 1);
  Assert(p.sparse[e3.index] == 2);

  Assert(p.packed[0] == e1.index);
  Assert(p.packed[1] == e2.index);
  Assert(p.packed[2] == e3.index);

  PoolTestComponentData *e1_data =
      PoolGetComponent(&p, PoolTestComponentData, e1.index);
  PoolTestComponentData *e2_data =
      PoolGetComponent(&p, PoolTestComponentData, e2.index);
  PoolTestComponentData *e3_data =
      PoolGetComponent(&p, PoolTestComponentData, e3.index);

  Assert(e1_data->a == entity_1_data.a);
  Assert(e2_data->a == entity_2_data.a);
  Assert(e3_data->a == entity_3_data.a);

  pool_remove(&p, e1);

  Assert(p.count == 2);
  Assert(p.sparse[e1.index] == ENTITY_INVALID_INDEX);
  Assert(p.sparse[e2.index] == 1);
  Assert(p.sparse[e3.index] == 0);
  Assert(p.packed[0] == e3.index);
  Assert(p.packed[1] == e2.index);

  e1_data = PoolGetComponent(&p, PoolTestComponentData, e1.index);
  Assert(e1_data == 0);

  e2_data = PoolGetComponent(&p, PoolTestComponentData, e2.index);
  Assert(e2_data->a == entity_2_data.a);

  e3_data = PoolGetComponent(&p, PoolTestComponentData, e3.index);
  Assert(e3_data->a == entity_3_data.a);

  pool_insert(&p, e1, &entity_1_data);
  Assert(p.count == 3);
  Assert(p.sparse[e1.index] == 2);
  Assert(p.sparse[e2.index] == 1);
  Assert(p.sparse[e3.index] == 0);

  Assert(p.packed[0] == e3.index);
  Assert(p.packed[1] == e2.index);
  Assert(p.packed[2] == e1.index);

  e1_data = PoolGetComponent(&p, PoolTestComponentData, e1.index);
  Assert(e1_data->a == entity_1_data.a);

  e2_data = PoolGetComponent(&p, PoolTestComponentData, e2.index);
  Assert(e2_data->a == entity_2_data.a);

  e3_data = PoolGetComponent(&p, PoolTestComponentData, e3.index);
  Assert(e3_data->a == entity_3_data.a);
}
#endif
