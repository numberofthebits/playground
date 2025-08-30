#ifndef ARENAALLOCATOR_H
#define ARENAALLOCATOR_H

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

#define STACK_ALLOCATOR_DEFAULT_THREAD_LOCAL_STACK_SIZE 1024ULL * 1024ULL * 8ULL

typedef void *(*Allocate)(void *allocator, size_t element_size,
                          size_t element_count, size_t alignment);
typedef void (*Deallocate)(void *allocator, void *ptr, size_t s);
typedef void (*DeallocateAll)(void *allocator);

struct IAllocator {
  Allocate allocate;
  Deallocate deallocate;
  DeallocateAll deallocate_all;
  void *instance;
};

struct ArenaAllocator {
  uint8_t *base;
  size_t capacity;
  size_t used;
};

typedef struct SubArenaAllocator {
  uint8_t *base;
  size_t capacity;
  size_t used;
} SubArenaAllocator;

void arena_init(struct ArenaAllocator *allocator, size_t s);

void *arena_alloc(struct ArenaAllocator *allocator, size_t element_size,
                  size_t element_count, size_t alignment);

void arena_dealloc_all(struct ArenaAllocator *allocator);

void arena_free(struct ArenaAllocator *allocator);

/* // Allocate memory for for 'count' objects of size sizeof('type') */
/* #define ArenaAlloc(allocator, count, type) \ */
/*   arena_alloc(allocator, sizeof(type), count, alignof(type)); */

// Did this just to avoid having to explicitly cast every usage of ArenaAlloc
// when moving from C to C++.
template <typename T> T *ArenaAlloc(ArenaAllocator *allocator, size_t count) {
  return (T *)arena_alloc(allocator, sizeof(T), count, alignof(T));
}

SubArenaAllocator arena_subarena_create(struct ArenaAllocator *allocator,
                                        size_t s);

void *arena_subarena_alloc(struct SubArenaAllocator *allocator,
                           size_t element_size, size_t element_count,
                           size_t alignment);

// #define SubArenaAlloc(allocator, count, type)                                  \
//   arena_subarena_alloc(allocator, sizeof(type), count, alignof(type));

template <typename T>
T *SubArenaAlloc(SubArenaAllocator *allocator, size_t count) {
  return (T *)arena_subarena_alloc(allocator, sizeof(T), count, alignof(T));
}

void arena_subarena_dealloc_all(struct SubArenaAllocator *allocator);

// NOTE: We lose potential bytes used to align the allocation here
// because we do not know what alignment the allocation required
// when deallocating again.
//
// We could make this a member of a potential Block struct
// typedef struct {
//   void* ptr;
//   size_t s;
//   size_t aligned_by;
// };
// ... But ew?

struct StackAllocator {
  void *base;
  size_t used;
  size_t capacity;
};

void stack_init(struct StackAllocator *stack, struct ArenaAllocator *allocator,
                size_t capacity);

void *stack_alloc(struct StackAllocator *allocator, size_t s);

template <typename T, typename Allocator>
T *StackAlloc(Allocator *allocator, size_t s) {
  return (T *)stack_alloc(allocator, s);
}

// Returns non-null if true
int stack_is_most_recent_allocation(struct StackAllocator *allocator, void *ptr,
                                    size_t s);

void stack_dealloc(struct StackAllocator *allocator, void *ptr, size_t s);

int stack_dealloc_checked(struct StackAllocator *allocator, void *ptr,
                          size_t s);

// This is the very definition of dangling pointers. You'd
// better be sure none of them are in use before allocating
// again
void stack_dealloc_all(struct StackAllocator *allocator);

#ifdef BUILD_TESTS
void stack_test();
#endif

// Static life time allocations only
extern struct ArenaAllocator global_static_allocator;

// Things put things here that have "frame" lifetime
// i.e. can die each frame
extern struct ArenaAllocator frame_allocator;

extern thread_local struct StackAllocator stack_allocator;
#endif
