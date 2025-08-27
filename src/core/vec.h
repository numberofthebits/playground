#ifndef _VEC_H
#define _VEC_H

#include <stdint.h>

struct Chunk_t {
  void *ptr;
  size_t size;
};
typedef struct Chunk_t Chunk;

struct Vec_t {
  Chunk storage;
  // Count elements
  int size;

  //
  int capacity;
};
typedef struct Vec_t Vec;

Chunk chunk_alloc(int s);

int chunk_is_allocated(Chunk *c);

void chunk_dealloc(Chunk *c);

void chunk_print_hex(Chunk *c);

Vec vec_create(void);

void vec_init(Vec *v);

void vec_reserve(Vec *v, int s);

void vec_resize(Vec *v, int s);

void vec_destroy(Vec *v);

#define VEC_SET_T(vec, type, index, value)                                     \
  *((type *)((vec)->storage.ptr) + index) = value

// Cast to 'type', dereference and copy into vector storage
// Copy because the whole point of arrays is cache locality
// so it would not make sense to store the pointer to the data.
// Could alternatly get a pointer to "next index" and write
// directly to that. However, that leaves the job of casting
// correctly to the API client
#define VEC_SET_T_VOID(vec, type, index, void_ptr)                             \
  *((type *)((vec)->storage.ptr) + index) = *(type *)void_ptr

// This feels lazy
#define VEC_PUSH_T(vec, type, value)                                           \
  {                                                                            \
    if ((vec)->size > (vec)->capacity) {                                       \
      LOG_EXIT("What??");                                                      \
    } else if ((vec)->size == (vec)->capacity) {                               \
      int new_capacity_t = ((vec)->capacity + 1) * 2;                          \
      vec_reserve((vec), new_capacity_t * sizeof(type));                       \
      (vec)->capacity = new_capacity_t;                                        \
    }                                                                          \
    type *ptr = (type *)((vec)->storage.ptr);                                  \
    ptr += (vec)->size;                                                        \
    *ptr = (value);                                                            \
    (vec)->size += 1;                                                          \
  }

#define VEC_PUSH_T_VOID(vec, type, value_ptr)                                  \
  VEC_PUSH_T((vec), type, *((type *)value_ptr));

#define VEC_POP_T(vec) (vec)->size -= 1;

#define VEC_GET_T_PTR(vec, type, index) (((type *)(vec)->storage.ptr) + index)

#define VEC_GET_T(vec, type, index) *(((type *)(vec)->storage.ptr) + index)

#define VEC_RESERVE(vec, size_t_bytes, count)                                  \
  vec_reserve((vec), size_t_bytes *count);                                     \
  (vec)->capacity = (count);

#define VEC_RESERVE_T(vec, type, count) VEC_RESERVE((vec), sizeof(type), count);

#define VEC_RESIZE(vec, size_bytes, count)                                     \
  vec_resize((vec), (size_bytes) * count);                                     \
  (vec)->size = (count);                                                       \
  (vec)->capacity = (count);

#define VEC_RESIZE_T(vec, type, count) VEC_RESIZE((vec), sizeof(type), (count));

#define VEC_ITER_BEGIN_T(vec, type) (type *)((vec)->storage.ptr)

#define VEC_ERASE_T(vec, type, index)                                          \
  int end = (vec)->size - 1;                                                   \
  for (int i_ = index; i_ < end; ++i_) {                                       \
    VEC_SET_T(vec, type, i_, VEC_GET_T(vec, type, i_ + 1));                    \
  }                                                                            \
  (vec)->size -= 1;

#endif
