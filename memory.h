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
    int64_t size;
    int64_t count;
    int8_t* ptr;
    int     num_children;  // For arena_push
} Arena;

typedef struct TempArena_s
{
    Arena*  parent;
    Arena   arena;
    int     id;
} TempArena;

// =========================================
// ==== Arena creation                  ====
// =========================================

// Create a root arena from a memory block.
static Arena arena_init(void* base, int64_t size);
// Create a child arena.
static Arena arena_spawn(Arena* parent, int64_t size);

// ==== Temporary arenas.
// Usage:
//      child = arena_push(my_arena, some_size);
//      use_temporary_arena(&child.arena);
//      arena_pop(child);
static TempArena    arena_push(Arena* parent, int64_t size);
static void         arena_pop (TempArena* child);

// =========================================
// ====          Allocation             ====
// =========================================
#define      arena_alloc_elem(arena, T)         (T *)arena_alloc_bytes((arena), sizeof(T))
#define      arena_alloc_array(arena, count, T) (T *)arena_alloc_bytes((arena), (count) * sizeof(T))
static void* arena_alloc_bytes (Arena* arena, size_t num_bytes);

// =========================================
// ====        Utility                  ====
// =========================================

#define arena_available_space(arena)    ((arena)->size - (arena)->count)
#define ARENA_VALIDATE(arena)           assert ((arena)->num_children == 0)

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
    int64_t size;
    int64_t count;
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
    int64_t total = arena->count + num_bytes;
    if (total > arena->size)
    {
        assert(!"Arena full.");
    }
    void* result = arena->ptr + arena->count;
    arena->count += num_bytes;
    return result;
}

static Arena arena_init(void* base, int64_t size)
{
    Arena arena = { 0 };
    arena.ptr = (int8_t*)base;
    if (arena.ptr)
    {
        arena.size   = size;
    }
    return arena;
}

static Arena arena_spawn(Arena* parent, int64_t size)
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

static TempArena arena_push(Arena* parent, int64_t size)
{
    assert ( size <= arena_available_space(parent));
    TempArena child = { 0 };
    {
        child.parent = parent;
        child.id     = parent->num_children;
        Arena arena = { 0 };
        {
            void* ptr = arena_alloc_bytes(parent, size);
            parent->num_children += 1;
            arena.ptr = ptr;
            arena.size = size;
        }
        child.arena = arena;
    }
    return child;
}

static void arena_pop(TempArena* child)
{
    Arena* parent = child->parent;

    // Assert that this child was the latest push.
    assert ((parent->num_children - 1) == child->id);
    for (int64_t i = 0; i < child->arena.count; ++i)
    {
        child->arena.ptr[i] = 0;
    }

    parent->count -= child->arena.size;
    parent->num_children -= 1;

    *child = (TempArena){ 0 };
}

static void arena_reset(Arena* arena)
{
    for (int64_t i = 0; i < arena->count; ++i) { arena->ptr[i] = 0; }
    arena->count = 0;
}

#ifdef __cplusplus
}
#endif
