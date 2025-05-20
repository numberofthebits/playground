#include "arena.h"
#include "log.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

struct ArenaAllocator global_static_allocator;

struct ArenaAllocator frame_allocator;

struct StackAllocator global_stack_allocator;

static size_t calc_alignment_bump(size_t s, size_t alignment) {
  size_t alignment_bump = 0;
  if (alignment != 1) {
    alignment_bump = (alignment - s) % alignment;
  }
  return alignment_bump;
}

void arena_init(struct ArenaAllocator *allocator, size_t capacity) {
  if (!allocator) {
    LOG_EXIT("Null pointer allocator");
    return;
  }

  if (allocator->base != 0) {
    LOG_EXIT("Can't initialize same arena twice");
    return;
  }

  if (!capacity) {
    LOG_EXIT("Allocating size 0 is forbidden");
    return;
  }

  allocator->base = malloc(capacity);
  allocator->capacity = capacity;
  allocator->used = 0;
}

void *arena_alloc(struct ArenaAllocator *allocator, size_t element_size,
                  size_t element_count, size_t alignment) {
  if (!allocator) {
    LOG_ERROR("Null pointer allocator");
    return 0;
  }

  // Example/reasoning:
  // Arena is 8 bytes.
  // We allocated 1 byte previously.
  // Next allocation is an int and should be aligned to 4
  // [ 0, 1, 2, 3, 4, 5, 7, 8 ] total
  // [ x,  ,  ,   ]
  // Let's say base address startted at 0x0, and is now at 0x1
  // base addr % alignment == 1
  // increment to align should be 3 bytes. alignment - base % alignment = 3

  const size_t count_bytes = element_count * element_size;

  size_t alignment_bump = calc_alignment_bump(alignment, count_bytes);

  void *return_ptr = allocator->base + allocator->used + alignment_bump;
  const size_t total_bytes = count_bytes + alignment_bump;

  if (allocator->used + total_bytes >= allocator->capacity) {
    LOG_ERROR("Allocator used %lu + new allocation %lu bytes exceeds allocator "
              "capacity %lu",
              allocator->used, total_bytes, allocator->capacity);
    return 0;
  }

  allocator->used += total_bytes;

  /* LOG_INFO("Allocated %lu bytes with alignment %lu (%lu bytes total with
   * alignment bump %lu) at %p. %lu / %lu bytes used", */
  /*          count_bytes, */
  /*          alignment, */
  /*          total_bytes, */
  /*          alignment_bump, */
  /*          return_ptr, */
  /*          allocator->used, */
  /*          allocator->capacity); */

  return return_ptr;
}

void arena_dealloc_all(struct ArenaAllocator *allocator) {
  allocator->used = 0;
}

void stack_init(struct StackAllocator *stack, struct ArenaAllocator *arena,
                size_t s) {
  // NOTE: Ask for 16 Mi times 1 byte, but align this to max_align_t
  // since we don't know what type of alignments the client might
  // ask for down the road. Maybe this is bad. We will see?
  void *base_ptr =
      arena_alloc(arena, 1, 1024 * 1024 * 16, alignof(max_align_t));
  if (!base_ptr) {
    LOG_EXIT("Failed to allocate %zu bytes for stack", s);
    return;
  }

  stack->base = base_ptr;
  stack->capacity = s;
  stack->used = 0;
}

void *stack_alloc(struct StackAllocator *allocator, size_t s,
                  size_t alignment) {
  size_t alignment_bump = calc_alignment_bump(alignment, s);
  size_t actual_usage = s + alignment_bump;

  if (actual_usage >= allocator->capacity - allocator->used) {
    LOG_EXIT("Stack allocation of %zu exceeds capacity %zu", actual_usage,
             allocator->capacity);
    return 0;
  }

  // This is the starting point of our allocation of size 's'
  void *allocation = allocator->base + allocator->used + alignment;

  // But we include the alignment in the actual usage of the allocator
  allocator->used += actual_usage;

  return allocation;
}

void stack_dealloc(struct StackAllocator *allocator, void *ptr,
                   size_t alignment, size_t s) {
  size_t alignment_bump = calc_alignment_bump(alignment, s);
  if ((ptr + alignment_bump + s) == allocator->base + allocator->used) {
    allocator->used -= alignment_bump - s;
  } else {
    LOG_EXIT("Failed to deallocate %x of size %zu with alignment %zu", ptr, s,
             alignment);
  }
}

void stack_dealloc_all(struct StackAllocator *allocator) {
  allocator->used = 0;
}
