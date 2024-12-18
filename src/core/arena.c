#include "arena.h"
#include "log.h"

#include <stdlib.h>
#include <stdint.h>

struct ArenaAllocator allocator;

void arena_init(struct ArenaAllocator* allocator, size_t capacity) {
    if(!allocator) {
        LOG_EXIT("Null pointer allocator");
    }
            
    if(allocator->base != 0) {
        LOG_EXIT("Can't initialize same arena twice");
    }
    
    if (!capacity) {
        LOG_EXIT("Allocating size 0 is forbidden");
    }

    allocator->base = malloc(capacity);
    allocator->capacity = capacity;
    allocator->used = 0;
}

void* arena_alloc(struct ArenaAllocator* allocator,
                  size_t element_size,
                  size_t element_count,
                  size_t alignment) {
    if(!allocator) {
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
    size_t alignment_bump = 0;
    if(alignment != 1) {
        alignment_bump = (alignment - (uintptr_t)(allocator->base + allocator->used)) % alignment;
    }

    const size_t count_bytes = element_count * element_size;

    void* return_ptr = allocator->base + allocator->used + alignment_bump;
    const size_t total_bytes = count_bytes + alignment_bump;
    
    if(allocator->used + total_bytes >= allocator->capacity) {
        LOG_ERROR("Allocator used %lu + new allocation %lu bytes exceeds allocator capacity %lu",
                  allocator->used, total_bytes, allocator->capacity);
        return 0;
    }

    allocator->used += total_bytes;

    LOG_INFO("Allocated %lu bytes with alignment %lu (%lu bytes total with alignment bump %lu) at %p. %lu / %lu bytes used",
             count_bytes,
             alignment,
             total_bytes,
             alignment_bump,
             return_ptr,
             allocator->used,
             allocator->capacity);
    
    return return_ptr;    
}

void arena_dealloc_all(struct ArenaAllocator* allocator) {
    allocator->used = 0;
}
