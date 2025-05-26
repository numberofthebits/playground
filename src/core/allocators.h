#ifndef ARENAALLOCATOR_H
#define ARENAALLOCATOR_H

#include <stdalign.h>
#include <stdint.h>
#include <stdlib.h>

struct ArenaAllocator {
  uint8_t *base;
  size_t capacity;
  size_t used;
};

void arena_init(struct ArenaAllocator *allocator, size_t s);

void *arena_alloc(struct ArenaAllocator *allocator, size_t element_size,
                  size_t element_count, size_t alignment);

void arena_dealloc_all(struct ArenaAllocator *allocator);

void arena_free(struct ArenaAllocator *allocator);

// Allocate memory for for 'count' objects of size sizeof('type')
#define ArenaAlloc(allocator, count, type)                                     \
  arena_alloc(allocator, sizeof(type), count, alignof(type));

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
                size_t s);

void *stack_alloc(struct StackAllocator *allocator, size_t s, size_t alignment);

// Returns non-null if true
int stack_is_most_recent_allocation(struct StackAllocator *allocator, void *ptr,
                                    size_t s, ptrdiff_t offset);

void stack_dealloc(struct StackAllocator *allocator, void *ptr, size_t s,
                   size_t alignment);

int stack_dealloc_checked(struct StackAllocator *allocator, void *ptr, size_t s,
                          size_t alignment);

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

#endif
