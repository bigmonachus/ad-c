/**
 * memory.h
 *  Sergio Gonzalez
 *
 * Interface for simple arena/zone/region memory managment.
 * Includes functions for arrays.
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef assert
#include <assert.h>
#endif

#include <stdint.h>

typedef struct Arena_s
{
    size_t size;
    size_t count;
    int8_t* ptr;
} Arena;

// =========================================
// ==== Arena creation                  ====
// =========================================

// Create a root arena from a memory block.
Arena arena_init(void* base, size_t size);
// Create a child arena.
Arena arena_spawn(Arena* parent, size_t size);

// =========================================
// ====          Allocation             ====
// =========================================
#define      arena_alloc_elem(arena, T)         (T *)arena_alloc_bytes(arena, sizeof(T))
#define      arena_alloc_array(arena, count, T) (T *)arena_alloc_bytes(arena, (count) * sizeof(T))
static void* arena_alloc_bytes (Arena* arena, size_t num_bytes);

// =========================================
// ====            Reuse                ====
// =========================================
static void arena_reset(Arena* arena);

// =========================================
// ====       Use as array              ====
// =========================================

// Usage example
//  int* foo = arena_array(&arena, max_capacity);
//  array_push(foo, 42);
//  array[0];  // -> 42
//  array_count(foo) // -> 1;
//
// *Will* reserve max_capacity, so it is not
// exactly like std::vector.
// You can forget about it when you clear its parent.

#define arena_array(a, type, size) (type *)arena__array_typeless(a, sizeof(type) * size)
#define array_push(a, e) (arena__array_try_grow(a), a[arena__array_header(a)->count - 1] = e)
#define array_reset(a) (arena__array_header(a)->count = 0)
#define array_count(a) (arena__array_header(a)->count)

// =========================================
//        Implementation
// =========================================

#pragma pack(push, 1)
typedef struct
{
    size_t size;
    size_t count;
} ArrayHeader;
#pragma pack(pop)

#define arena__array_header(array) \
    ((ArrayHeader*)((uint8_t*)array - sizeof(ArrayHeader)))

static void* arena__array_typeless(Arena* arena, size_t size)
{
    Arena child = arena_spawn(arena, size + sizeof(ArrayHeader));
    ArrayHeader head = { 0 };
    {
        head.size = child.size;
    }
    memcpy(child.ptr, &head, sizeof(ArrayHeader));
    return (void*)(((uint8_t*)child.ptr) + sizeof(ArrayHeader));
}

static void arena__array_try_grow(void* array)
{
    ArrayHeader* head = arena__array_header(array);
    assert(head->size >= (head->count + 1));
    ++head->count;
}

static void* arena_alloc_bytes(Arena* arena, size_t num_bytes)
{
    size_t total = arena->count + num_bytes;
    if (total > arena->size)
    {
        assert(!"Arena full.");
    }
    void* result = arena->ptr + arena->count;
    arena->count += num_bytes;
    return result;
}

static Arena arena_init(void* base, size_t size)
{
    Arena arena = { 0 };
    arena.ptr = (int8_t*)base;
    if (arena.ptr)
    {
        arena.size   = size;
    }
    return arena;
}

static Arena arena_spawn(Arena* parent, size_t size)
{
    void* ptr = arena_alloc_bytes(parent, size);
    assert(ptr);

    Arena child = { 0 };
    {
        child.ptr    = ptr;
        child.size   = size;
    }

    return child;
}

static void arena_reset(Arena* arena)
{
    arena->count = 0;
}

#ifdef __cplusplus
}
#endif
