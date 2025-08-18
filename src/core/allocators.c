#include "allocators.h"
#include "log.h"
#include "util.h"

#include <memory.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

typedef struct AllocImplResult {
  void *ptr;
  size_t used;
} AllocImplResult;

struct ArenaAllocator global_static_allocator;

struct ArenaAllocator frame_allocator;

thread_local struct StackAllocator stack_allocator;

static inline ptrdiff_t offset_to_aligned(void *ptr, size_t s,
                                          size_t alignment) {
  uintptr_t res = (uintptr_t)(ptr + s) % alignment;

  if (!res) {
    return res;
  }

  return alignment - res;
}

static inline size_t round_to_aligned(size_t s, size_t alignment) {
  size_t a = alignment - 1;
  return (s + a) & ~a;
}

static inline char *round_to_aligned_ptr(unsigned char *ptr, size_t alignment) {
  const size_t a = alignment - 1;
  const uintptr_t b = (uintptr_t)(ptr + a);
  const uintptr_t c = b & ~a;
  return (void *)c;
}

/* static inline AllocImplResult alloc_impl(void *ptr, size_t s, */
/*                                          size_t alignment) { */
/*   AllocImplResult res = {.ptr = 0, .used = s}; */

/*   if ((uintptr_t)ptr % alignment == 0) { */
/*     res.ptr = ptr; */
/*     return res; */
/*   } */

/*   ptrdiff_t offset = offset_to_aligned(ptr, s, alignment); */
/*   res.ptr += offset; */
/*   res.used += offset; */

/*   return res; */
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

  if (allocator->used != 0) {
    LOG_EXIT("Can't (re-)initialize arena with non-null usage");
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

  //  size_t size_rounded_up = round_to_aligned(count_bytes, alignment);
  // void *return_ptr = allocator->base + allocator->used + alignment_bump;

  void *aligned_ptr =
      round_to_aligned_ptr(allocator->base + allocator->used, alignment);
  void *unaligned_ptr = allocator->base + allocator->used;
  ptrdiff_t alignment_bump = aligned_ptr - unaligned_ptr;

  const size_t total_bytes = count_bytes + alignment_bump;

  if (allocator->used + total_bytes >= allocator->capacity) {
    LOG_ERROR("Allocator used %lu + new allocation %lu bytes exceeds allocator "
              "capacity %lu",
              allocator->used, total_bytes, allocator->capacity);
    return 0;
  }

  allocator->used += total_bytes;

  LOG_INFO("Allocated %lu bytes with alignment %lu (%lu bytes total with "
           "alignment bump %lu) at %p. %lu / %lu bytes used",
           count_bytes, alignment, total_bytes, alignment_bump, aligned_ptr,
           allocator->used, allocator->capacity);

  // TODO: Make this an optional flag if it ends up taking more
  // time than we want.
  // NOTE: Total bytes INCLUDES alignment. Set from unalignedptr to clear
  //       all of it
  memset(unaligned_ptr, 0x0, total_bytes);

  return aligned_ptr;
}

void arena_dealloc_all(struct ArenaAllocator *allocator) {
  allocator->used = 0;
}

void arena_free(struct ArenaAllocator *allocator) {
  free(allocator->base);
  allocator->base = 0;
  allocator->capacity = 0;
  allocator->used = 0;
}

SubArenaAllocator arena_subarena_create(struct ArenaAllocator *allocator,
                                        size_t s) {
  if (s > allocator->capacity - allocator->used) {
    LOG_EXIT("Failed to allocate sub arena: asked for %zu. Parent allocator "
             "%zu / %zu used",
             s, allocator->capacity - allocator->used, allocator->capacity);
  }

  SubArenaAllocator sub_arena;
  sub_arena.base = arena_alloc(allocator, 1, s, alignof(max_align_t));
  sub_arena.capacity = s;
  sub_arena.used = 0;

  return sub_arena;
}

void *arena_subarena_alloc(struct SubArenaAllocator *allocator,
                           size_t element_size, size_t element_count,
                           size_t alignment) {
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

  //  size_t size_rounded_up = round_to_aligned(count_bytes, alignment);
  // void *return_ptr = allocator->base + allocator->used + alignment_bump;

  void *aligned_ptr =
      round_to_aligned_ptr(allocator->base + allocator->used, alignment);
  void *unaligned_ptr = allocator->base + allocator->used;
  ptrdiff_t alignment_bump = aligned_ptr - unaligned_ptr;

  const size_t total_bytes = count_bytes + alignment_bump;

  if (allocator->used + total_bytes >= allocator->capacity) {
    LOG_ERROR("Allocator used %lu + new allocation %lu bytes exceeds allocator "
              "capacity %lu",
              allocator->used, total_bytes, allocator->capacity);
    return 0;
  }

  allocator->used += total_bytes;

  LOG_INFO("Allocated %lu bytes with alignment %lu (%lu bytes total with "
           "alignment bump %lu) at %p. %lu / %lu bytes used",
           count_bytes, alignment, total_bytes, alignment_bump, aligned_ptr,
           allocator->used, allocator->capacity);

  // TODO: Make this an optional flag if it ends up taking more
  // time than we want.
  // NOTE: Total bytes INCLUDES alignment. Set from unalignedptr to clear
  //       all of it
  memset(unaligned_ptr, 0x0, total_bytes);

  return aligned_ptr;

  /* AllocImplResult res = */
  /*     alloc_impl(allocator->base + allocator->used, s, alignment); */

  /* size_t total_used = allocator->used + res.used; */
  /* if (total_used > allocator->capacity) { */
  /*   LOG_EXIT("Failed to allocate sub arena: asked for %zu, available %zu", s,
   */
  /*            total_used); */
  /* } */

  /* LOG_INFO("Sub arena alloc from %p at %p capacity / used %zu / %zu", */
  /*          allocator->base, res.ptr, allocator->used, allocator->capacity);
   */

  /* allocator->used += res.used; */

  /* return res.ptr; */
}

void arena_subarena_dealloc_all(struct SubArenaAllocator *allocator) {
  LOG_INFO("Sub arena dealloc all at %p used/capacity %zu / %zu",
           allocator->base, allocator->used, allocator->capacity);
  allocator->used = 0;
}

void stack_init(struct StackAllocator *stack, struct ArenaAllocator *arena,
                size_t capacity) {
  void *base_ptr = arena_alloc(arena, 1, capacity, alignof(max_align_t));
  if (!base_ptr) {
    LOG_EXIT("Failed to allocate %zu bytes for stack", capacity);
    return;
  }

  stack->base = base_ptr;
  stack->capacity = capacity;
  stack->used = 0;
}

void *stack_alloc(struct StackAllocator *allocator, size_t s) {
  if (allocator->used + s >= allocator->capacity) {
    LOG_EXIT("Stack allocation of %zu exceeds capacity %zu", s,
             allocator->capacity);
    return 0;
  }

  size_t size_rounded_up = round_to_aligned(s, alignof(max_align_t));

  // Assume 'base' is always aligned and 'used' will always
  // be rounded up to a valid alignment
  void *allocated_ptr = allocator->base + allocator->used;
  allocator->used += size_rounded_up;

  return allocated_ptr;
}

int stack_is_most_recent_allocation(struct StackAllocator *allocator, void *ptr,
                                    size_t size_rounded_up) {
  return ptr + size_rounded_up == allocator->base + allocator->used;
}

void stack_dealloc(struct StackAllocator *allocator, void *ptr, size_t s) {
  size_t size_rounded_up = round_to_aligned(s, alignof(max_align_t));

  if (stack_is_most_recent_allocation(allocator, ptr, size_rounded_up)) {
    allocator->used -= size_rounded_up;
  } else {
    LOG_EXIT("Failed to deallocate %x of size %zu", ptr);
  }
}

int stack_dealloc_checked(struct StackAllocator *allocator, void *ptr,
                          size_t s) {
  size_t size_rounded_up = round_to_aligned(s, alignof(max_align_t));

  if (stack_is_most_recent_allocation(allocator, ptr, size_rounded_up)) {
    allocator->used -= size_rounded_up;
  } else {
    LOG_ERROR("Failed to deallocate %x of size %zu", ptr, s);
    return 0;
  }

  return 1;
}

void stack_dealloc_all(struct StackAllocator *allocator) {
  LOG_INFO("Stack dealloc all at %p used/capacity %zu / %zu", allocator->base,
           allocator->used, allocator->capacity);
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
  char *ptr = malloc(1024);

  for (size_t i = 0; i < alignment; ++i) {
    uintptr_t offset =
        (uintptr_t)offset_to_aligned(ptr, i, alignof(max_align_t));
    uintptr_t expected = (uintptr_t)((alignment - i) % alignment);
    Assert(offset == expected);
  }

  free(ptr);
}

void stack_test() {
  test_offset_to_align();

  int test_int = 0xf0e0d0c0;

  struct ArenaAllocator arena = {0};
  arena_init(&arena, 1024 * 1024 * 32);

  struct StackAllocator stack = {0};
  stack_init(&stack, &arena, 1024 * 1024 * 16);

  int *int_ptr = stack_alloc(&stack, sizeof(int));
  Assert((uintptr_t)int_ptr % alignof(int) == 0);
  Assert(stack.used == 8); // compulsive alignment to 8

  *int_ptr = 0xf0e0d0c0;
  TestStructPadded *ts = stack_alloc(&stack, sizeof(TestStructPadded));

  uintptr_t ts_value = (uintptr_t)ts;
  int alignof_ts = alignof(TestStructPadded);
  int res = ts_value % alignof_ts;
  (void)res;
  Assert(ts_value % alignof_ts == 0);
  // should be aligned to eight and we lose 4 bytes because of previous
  // allocation of int when allocating TestStructPadded
  size_t size_tsp = sizeof(TestStructPadded);
  Assert(stack.used == 4 + 4 + size_tsp);
  Assert(stack_dealloc_checked(&stack, int_ptr, sizeof(int)) == 0);
  Assert(stack_dealloc_checked(&stack, ts, sizeof(*ts)) == 1);
  Assert(*int_ptr == test_int);

  Assert(stack_dealloc_checked(&stack, int_ptr, sizeof(int)) == 1);
  Assert(stack.used == 0);

  int *int_ptr_2 = stack_alloc(&stack, sizeof(int));
  TestStructPadded *ts2 = stack_alloc(&stack, sizeof(TestStructPadded));
  TestStructLarge *tsl = stack_alloc(&stack, sizeof(TestStructLarge));
  Assert((uintptr_t)int_ptr_2 % alignof(int) == 0);
  *int_ptr_2 = 1;

  Assert(*int_ptr == *int_ptr_2);

  Assert(stack_dealloc_checked(&stack, tsl, sizeof(TestStructLarge)) == 1);
  Assert(stack_dealloc_checked(&stack, ts2, sizeof(TestStructPadded)) == 1);
  Assert(stack_dealloc_checked(&stack, int_ptr_2, sizeof(int)) == 1);

  Assert(stack.used == 0);

  arena_free(&arena);
}

#endif
