#include "arena.h"
#include "log.h"

#include <assert.h>
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

static ptrdiff_t offset_to_aligned(void *ptr, size_t s, size_t alignment) {
  uintptr_t res = (uintptr_t)(ptr + s) % alignment;

  if (res != 0) {
    // We want the remaining number of bytes required to align to
    // 'alignment'
    res = alignment - res;
  }

  return res;
}

/* static ptrdiff_t offset_to_aligned_down(void *ptr, size_t alignment) { */
/*   return (uintptr_t)ptr % alignment; */
/* } */

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

  size_t alignment_bump = calc_alignment_bump(count_bytes, alignment);

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

void arena_free(struct ArenaAllocator *allocator) {
  free(allocator->base);
  allocator->capacity = 0;
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
  (void)alignment;
  if (allocator->used + s >= allocator->capacity) {
    LOG_EXIT("Stack allocation of %zu exceeds capacity %zu", s,
             allocator->capacity);
    return 0;
  }

  void *ptr = allocator->base + allocator->used;

  if ((uintptr_t)ptr % alignment == 0) {
    allocator->used += s;
    return ptr;
  }

  ptrdiff_t offset = offset_to_aligned(ptr, s, alignof(max_align_t));
  ptr += offset;

  allocator->used += offset + s;

  LOG_INFO("allocated %zu bytes + %zu alignment @ %p", s, offset, ptr);
  return ptr;
}

int stack_is_most_recent_allocation(struct StackAllocator *allocator, void *ptr,
                                    size_t s,

                                    ptrdiff_t offset) {
  if (ptr + s + offset == allocator->base + allocator->used) {
    return 1;
  }

  return 0;
}

void stack_dealloc(struct StackAllocator *allocator, void *ptr, size_t s,
                   size_t alignment) {
  ptrdiff_t offset = offset_to_aligned(ptr, s, alignof(max_align_t));

  if (stack_is_most_recent_allocation(allocator, ptr, s, offset)) {

    allocator->used -= s + offset;
  } else {
    LOG_EXIT("Failed to deallocate %x of size %zu with alignment %zu", ptr, s,
             alignment);
  }
}

int stack_dealloc_checked(struct StackAllocator *allocator, void *ptr, size_t s,
                          size_t alignment) {
  ptrdiff_t offset = offset_to_aligned(ptr, s, alignof(max_align_t));

  if (stack_is_most_recent_allocation(allocator, ptr, s, offset)) {
    allocator->used -= s + offset;
  } else {
    LOG_ERROR("Failed to deallocate %x of size %zu with alignment %zu", ptr, s,
              alignment);
    return 0;
  }

  return 1;
}

void stack_dealloc_all(struct StackAllocator *allocator) {
  allocator->used = 0;
}

#ifdef BUILD_TESTS
typedef struct {
  double d;
  char c;
} TestStructPadded;

typedef struct {
  char array[1024];
} TestStructLarge;

static void test_offset_to_align() {
  size_t alignment = alignof(max_align_t);
  char *ptr = 0;

  for (size_t i = 0; i < alignment; ++i) {
    uintptr_t offset = (uintptr_t)offset_to_aligned(ptr, i, alignment);
    uintptr_t expected = (uintptr_t)((alignment - i) % alignment);
    assert(offset == expected);
  }
}

void stack_test() {
  test_offset_to_align();

  int test_int = 0xf0e0d0c0;

  struct ArenaAllocator arena = {0};
  arena_init(&arena, 1024 * 1024 * 32);

  struct StackAllocator stack = {0};
  stack_init(&stack, &arena, 1024 * 1024 * 16);

  int *int_ptr = stack_alloc(&stack, sizeof(int), alignof(int));
  assert((uintptr_t)int_ptr % alignof(int) == 0);
  assert(stack.used == 4);

  *int_ptr = 0xf0e0d0c0;
  TestStructPadded *ts =
      stack_alloc(&stack, sizeof(TestStructPadded), alignof(TestStructPadded));

  uintptr_t ts_value = (uintptr_t)ts;
  int alignof_ts = alignof(TestStructPadded);
  int res = ts_value % alignof_ts;
  (void)res;
  assert(ts_value % alignof_ts == 0);
  // should be aligned to eight and we lose 4 bytes because of previous
  // allocation of int when allocating TestStructPadded
  assert(stack.used == 4 + 4 + sizeof(TestStructPadded));
  assert(stack_dealloc_checked(&stack, int_ptr, sizeof(int), alignof(int)) ==
         0);
  assert(stack_dealloc_checked(&stack, ts, sizeof(*ts),
                               alignof(TestStructPadded)) == 1);
  assert(*int_ptr == test_int);

  assert(stack_dealloc_checked(&stack, int_ptr, sizeof(int), alignof(int)) ==
         1);
  assert(stack.used == 0);

  int *int_ptr_2 = stack_alloc(&stack, sizeof(int), alignof(int));
  TestStructPadded *ts2 =
      stack_alloc(&stack, sizeof(TestStructPadded), alignof(TestStructPadded));
  TestStructLarge *tsl =
      stack_alloc(&stack, sizeof(TestStructLarge), alignof(TestStructLarge));
  assert((uintptr_t)int_ptr_2 % alignof(int) == 0);
  *int_ptr_2 = 1;

  assert(*int_ptr == *int_ptr_2);

  assert(stack_dealloc_checked(&stack, tsl, sizeof(TestStructLarge),
                               alignof(TestStructLarge)) == 1);
  assert(stack_dealloc_checked(&stack, ts2, sizeof(TestStructPadded),
                               alignof(TestStructPadded)) == 1);
  assert(stack_dealloc_checked(&stack, int_ptr_2, sizeof(int), alignof(int)) ==
         1);

  assert(stack.used == 0);

  arena_free(&arena);
}

#endif
