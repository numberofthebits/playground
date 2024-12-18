#ifndef _ARENA_H
#define _ARENA_H

#include <stdint.h>
#include <stdalign.h>

struct ArenaAllocator {
    uint8_t* base;
    size_t capacity;
    size_t used;
};

void arena_init(struct ArenaAllocator* allocator, size_t s);

void* arena_alloc(struct ArenaAllocator* allocator, size_t element_size, size_t element_count, size_t alignment);

void arena_dealloc_all(struct ArenaAllocator* allocator);

// Allocate memory for for 'count' objects of size sizeof('type')
#define ArenaAlloc(allocator, count, type) \
    arena_alloc(allocator, sizeof(type), count, alignof(type)); 
#endif

// Static life time allocations only 'cause that's
// really all we support right now :)
extern struct ArenaAllocator allocator;
