#include "vec.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

void chunk_print_hex(Chunk *c) {
  LOG_INFO("%p:", (char *)c->ptr);
  for (int i = 0; i < c->size; ++i) {
    if (i % 8 == 0) {
      LOG_INFO("");
    }
    LOG_INFO("0x%x ", *((unsigned char *)(c->ptr) + i) & 0xff);
  }
  LOG_INFO("");
}

Chunk chunk_create() {
  Chunk c;
  c.ptr = 0;
  c.size = 0;

  return c;
}

Chunk chunk_alloc(int s) {
  void *ptr = malloc(s);
  if (!ptr) {
    LOG_INFO("malloc() %d bytes failed", s);
  }

  Chunk c;
  c.ptr = ptr;
  c.size = s;

  LOG_INFO("chunk alloc: %d bytes @ %p", s, c.ptr);
  return c;
}

int chunk_is_allocated(Chunk *c) { return c->ptr != 0; }

void chunk_dealloc(Chunk *c) {
  if (!c) {
    LOG_INFO("chunk is null");
    return;
  }
  c->size = 0;
  if (!c->ptr) {
    LOG_INFO("chunk ptr is null");
    return;
  }

  free(c->ptr);
  c->ptr = 0;
}

void vec_init(Vec* v) {
  v->size = 0;
  v->capacity = 0;
  v->storage = chunk_create();  
}

Vec vec_create(void) {
  Vec v;
  vec_init(&v);
  return v;
}

void vec_reserve(Vec *v, int s) {
  LOG_INFO("Reserve: current %d, new %d", v->storage.size, s);
  if (s > v->storage.size) {
    Chunk new_chunk = chunk_alloc(s);
    if (chunk_is_allocated(&v->storage)) {
      LOG_INFO("Dealloc old chunk (%d bytes @ %p to %p", v->storage.size,
               v->storage.ptr, new_chunk.ptr);
      Chunk old_chunk = v->storage;
      memcpy(new_chunk.ptr, v->storage.ptr, v->storage.size);
      chunk_dealloc(&old_chunk);
    }
    v->storage = new_chunk;
  }
}

void vec_resize(Vec *v, int s) {
  LOG_INFO("Resize: current %d, new %d", v->storage.size, s);
  Chunk new_chunk = chunk_alloc(s);
  if (chunk_is_allocated(&v->storage)) {
    LOG_INFO("Dealloc old chunk (%d bytes @ %p to %p", v->storage.size,
             v->storage.ptr, new_chunk.ptr);
    Chunk old_chunk = v->storage;
    memcpy(new_chunk.ptr, v->storage.ptr, v->storage.size);
    chunk_dealloc(&old_chunk);
  }
  v->storage = new_chunk;
}

void vec_destroy(Vec *v) {
  if (!v) {
    return;
  }

  v->size = 0;
  if (chunk_is_allocated(&v->storage)) {
    chunk_dealloc(&v->storage);
  }
}
