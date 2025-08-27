#include "vec.h"

#include "log.h"

#include <stdlib.h>
#include <string.h>

void chunk_print_hex(Chunk *c) {
  LOG_INFO("%p:", (char *)c->ptr);
  for (size_t i = 0; i < c->size; ++i) {
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
    LOG_EXIT("malloc() %d bytes failed", s);
  }

  Chunk c;
  c.ptr = ptr;
  c.size = s;

  LOG_INFO("chunk alloc: %d bytes @ %p", s, c.ptr);
  return c;
}

Chunk chunk_realloc(Chunk *c, size_t s) {
  if (!c->ptr) {
    LOG_EXIT("Can't realloc null pointer)");
  }

  void *prev = c->ptr;
  void *ptr = realloc(c->ptr, s);
  LOG_INFO("Realloc %zu bytes: prev ptr %p new ptr %p", s, prev, ptr);

  if (!ptr) {
    LOG_EXIT("realloc() failed");
  }

  Chunk new_chunk = {.ptr = ptr, .size = s};

  return new_chunk;
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

void vec_init(Vec *v) {
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
  if (chunk_is_allocated(&v->storage)) {
    v->storage = chunk_realloc(&v->storage, s);
  } else {
    v->storage = chunk_alloc(s);
  }
}

void vec_resize(Vec *v, int s) {
  LOG_INFO("Resize: current %d, new %d", v->storage.size, s);
  if (chunk_is_allocated(&v->storage)) {
    v->storage = chunk_realloc(&v->storage, s);
  } else {
    v->storage = chunk_alloc(s);
  }
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
