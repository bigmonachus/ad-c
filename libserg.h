/**
 * libserg.h
 *
 * This software is in the public domain. Where that dedication is not
 * recognized, you are granted a perpetual, irrevocable license to copy
 * and modify this file as you see fit.
 *
 */

// Imitation is the sincerest form of flattery.
//
// In the style of stb, this is my "toolbox" for writing quick C99 programs.
//
// Other header-only libraries that might be of more practical use to people
// other than myself:
//
// - Tiny JPEG encoder : github.com/serge-rgb/TinyJpeg

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef assert
#include <assert.h>
#endif

#include <stdlib.h>  // memcpy
#include <string.h>  // memset
#include <stdint.h>
#include <stdio.h>


// =================================================================================================
// MACROS
// =================================================================================================

#ifndef sgl_malloc
#define sgl_malloc malloc
#endif

#ifndef sgl_calloc
#define sgl_calloc calloc
#endif

#ifndef sgl_free
#define sgl_free free
#endif

#define sgl_array_count(a) (sizeof((a))/sizeof((a)[0]))

// =================================================================================================
// MEMORY
// =================================================================================================

typedef struct Arena_s Arena;

struct Arena_s
{
    // Memory:
    size_t  size;
    size_t  count;
    uint8_t*     ptr;

    // For pushing/popping
    Arena*      parent;
    int32_t     id;
    int32_t     num_children;
};

// =========================================
// ==== Arena creation                  ====
// =========================================

// Create a root arena from a memory block.
static Arena arena_init(void* base, size_t size);
// Create a child arena.
static Arena arena_spawn(Arena* parent, size_t size);

// ==== Temporary arenas.
// Usage:
//      child = arena_push(my_arena, some_size);
//      use_temporary_arena(&child.arena);
//      arena_pop(child);
static Arena    arena_push(Arena* parent, size_t size);
static void     arena_pop (Arena* child);

// =========================================
// ====          Allocation             ====
// =========================================
#define      arena_alloc_elem(arena, T)         (T *)arena_alloc_bytes((arena), sizeof(T))
#define      arena_alloc_array(arena, count, T) (T *)arena_alloc_bytes((arena), (count) * sizeof(T))
static void* arena_alloc_bytes(Arena* arena, size_t num_bytes);

// =========================================
// ====        Utility                  ====
// =========================================

#define arena_available_space(arena)    ((arena)->size - (arena)->count)
#define ARENA_VALIDATE(arena)           assert ((arena)->num_children == 0)

// =========================================
// ====            Reuse                ====
// =========================================
static void arena_reset(Arena* arena);

// ==============================================
// ====   Simple stack from arena            ====
// ==== i.e. heap array with bounds checking ====
// ==============================================

/*
    This is a heap array with bounds checking.

    // Example: create an array of 10 elements of type Foo
    Foo* array = arena_make_stack(arena, 10, Foo);
    // Push into the array:
    arena_stack_push(array, some_foo);
    // Get the first element:
    Foo* first = array[0];
    // Loop:
    for (int i = 0; i < arena_stack_count(array); ++i)
    {
        Foo* foo = array[i];
        use_the_foo(foo);
    }
   */

#define arena_make_stack(a, size, type) (type *)arena__stack_typeless(a, sizeof(type) * size)
#define arena_stack_push(a, e) if (arena__stack_check(a)) a[arena__stack_header(a)->count - 1] = e
#define arena_stack_reset(a) (arena__stack_header(a)->count = 0)
#define arena_stack_count(a) (arena__stack_header(a)->count)

#pragma pack(push, 1)
typedef struct
{
    size_t size;
    size_t count;
} StackHeader;
#pragma pack(pop)

#define arena__stack_header(stack) ((StackHeader*)((uint8_t*)stack - sizeof(StackHeader)))

static void* arena__stack_typeless(Arena* arena, size_t size)
{
    Arena child = arena_spawn(arena, size + sizeof(StackHeader));
    StackHeader head = { 0 };
    {
        head.size = child.size;
    }
    memcpy(child.ptr, &head, sizeof(StackHeader));
    return (void*)(((uint8_t*)child.ptr) + sizeof(StackHeader));
}

static int32_t arena__stack_check(void* stack)
{
    StackHeader* head = arena__stack_header(stack);
    if (head->size < (head->count + 1))
    {
        assert (!"Stack full");
        return 0;
    }
    ++head->count;
    return 1;
}

// =========================================
//        Arena implementation
// =========================================

static void* arena_alloc_bytes(Arena* arena, size_t num_bytes)
{
    size_t total = arena->count + num_bytes;
    if (total > arena->size)
    {
        return NULL;
    }
    void* result = arena->ptr + arena->count;
    arena->count += num_bytes;
    return result;
}

static Arena arena_init(void* base, size_t size)
{
    Arena arena = { 0 };
    arena.ptr = (uint8_t*)base;
    if (arena.ptr)
    {
        arena.size = size;
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

static Arena arena_push(Arena* parent, size_t size)
{
    assert ( size <= arena_available_space(parent));
    Arena child = { 0 };
    {
        child.parent           = parent;
        child.id               = parent->num_children;
        void* ptr              = arena_alloc_bytes(parent, size);
        child.ptr              = ptr;
        child.size             = size;

        parent->num_children += 1;
    }
    return child;
}

static void arena_pop(Arena* child)
{
    Arena* parent = child->parent;
    assert(parent);

    // Assert that this child was the latest push.
    assert ((parent->num_children - 1) == child->id);

    parent->count -= child->size;
    char* ptr = (char*)(parent->ptr) + parent->count;
    memset(ptr, 0, child->count);
    parent->num_children -= 1;

    *child = (Arena){ 0 };
}

static void arena_reset(Arena* arena)
{
    memset (arena->ptr, 0, arena->count);
    arena->count = 0;
}

// =================================================================================================
// THREADING
// =================================================================================================

// Platform-agnostic definitions.
typedef struct SglMutex_s SglMutex;
typedef struct SglSemaphore_s SglSemaphore;

static int32_t          sgl_cpu_count();
static SglSemaphore*    sgl_create_semaphore(int32_t value);
static int32_t          sgl_semaphore_wait(SglSemaphore* sem);
static int32_t          sgl_semaphore_signal(SglSemaphore* sem);
static SglMutex*        sgl_create_mutex();
static int32_t          sgl_mutex_lock(SglMutex* mutex);
static int32_t          sgl_mutex_unlock(SglMutex* mutex);
static void             sgl_destroy_mutex(SglMutex* mutex);
static void             sgl_create_thread(void (*thread_func)(void*), void* params);

// =================================
// Windows
// =================================
#ifdef WIN32
#include <Windows.h>
#include <process.h>


#define SGL_MAX_SEMAPHORE_VALUE (1 << 16)

struct SglMutex_s
{
    CRITICAL_SECTION critical_section;
};

struct SglSemaphore_s
{
    HANDLE  handle;
    LONG    value;
};

static int32_t sgl_cpu_count()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int32_t count = info.dwNumberOfProcessors;
    return count;
}

static SglSemaphore* sgl_create_semaphore(int32_t value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
    sem->handle = CreateSemaphore(0, value, SGL_MAX_SEMAPHORE_VALUE, NULL);
    sem->value = value;
    if (!sem->handle)
    {
        sgl_free (sem);
        sem = NULL;
    }
    return sem;
}

// Will return non-zero on error
static int32_t sgl_semaphore_wait(SglSemaphore* sem)
{
    int32_t result;

    if (!sem) {
        return -1;
    }

    switch (WaitForSingleObject(sem->handle, INFINITE))
    {
    case WAIT_OBJECT_0:
        {
            InterlockedDecrement(&sem->value);
            result = 0;
            break;
        }
    case WAIT_TIMEOUT:
        {
            result = -1;
            break;
        }
    default:
        {
            result = -1;
            break;
        }
    }
    return result;
}

static int32_t sgl_semaphore_signal(SglSemaphore* sem)
{
    InterlockedIncrement(&sem->value);
    if (ReleaseSemaphore(sem->handle, 1, NULL) == FALSE)
    {
        InterlockedDecrement(&sem->value);
        return -1;
    }
    return 0;
}

static SglMutex* sgl_create_mutex()
{
    SglMutex* mutex = (SglMutex*) sgl_malloc(sizeof(SglMutex));
    InitializeCriticalSectionAndSpinCount(&mutex->critical_section, 2000);
    return mutex;
}

static int32_t sgl_mutex_lock(SglMutex* mutex)
{
    int32_t result = 0;
    EnterCriticalSection(&mutex->critical_section);
    return result;
}

static int32_t sgl_mutex_unlock(SglMutex* mutex)
{
    int32_t result = 0;
    LeaveCriticalSection(&mutex->critical_section);
    return result;
}

static void sgl_destroy_mutex(SglMutex* mutex)
{
    if (mutex)
    {
        DeleteCriticalSection(&mutex->critical_section);
        sgl_free(mutex);
    }
}

static void sgl_create_thread(void (*thread_func)(void*), void* params)
{
    _beginthread(thread_func, 0, params);
}

// =================================
// End of Windows
// =================================

// =================================
// Start of Linux
// =================================
#elif __linux

#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

static int32_t sgl_cpu_count()
{
    static int32_t sgli__cpu_count = -1;
    if (sgli__cpu_count <= 0)
    {
        sgli__cpu_count = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    }
    assert (sgli__cpu_count >= 1);
    return sgli__cpu_count;
}

struct SglSemaphore_s
{
    sem_t sem;
};

static SglSemaphore* sgl_create_semaphore(int32_t value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
    int32_t err = sem_init(&sem->sem, 0, value);
    if (err < 0)
    {
        sgl_free(sem);
        return NULL;
    }
    return sem;
}

static int32_t sgl_semaphore_wait(SglSemaphore* sem)
{
    return sem_wait(&sem->sem);
}

static int32_t sgl_semaphore_signal(SglSemaphore* sem)
{
    return sem_post(&sem->sem);
}
struct SglMutex_s
{
    pthread_mutex_t handle;
};

static SglMutex* sgl_create_mutex()
{
    SglMutex* mutex = (SglMutex*) sgl_malloc(sizeof(SglMutex));

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);

    if (pthread_mutex_init(&mutex->handle, &attr) != 0) {
        sgl_free(mutex);
        return NULL;
    }
    return mutex;
}

static int32_t sgl_mutex_lock(SglMutex* mutex)
{
    if (pthread_mutex_lock(&mutex->handle) < 0)
    {
        return -1;
    }
    return 0;
}

static int32_t sgl_mutex_unlock(SglMutex* mutex)
{
    if (pthread_mutex_unlock(&mutex->handle) < 0)
    {
        return -1;
    }

    return 0;
}

static void sgl_destroy_mutex(SglMutex* mutex)
{
    pthread_mutex_destroy(&mutex->handle);
    sgl_free(mutex);
}

static void sgl_create_thread(void (*thread_func)(void*), void* params)
{
    pthread_attr_t attr;

    /* Set the thread attributes */
    if (pthread_attr_init(&attr) != 0)
    {
        assert(!"Not handling thread attribute failure.");
    }
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_t leaked_thread;
    if (pthread_create(&leaked_thread, &attr, (void*)(void*)thread_func, params) != 0)
    {
        assert (!"God dammit");
    }
}

// =================================
// End of Linux
// =================================
#endif  // Platforms

// =================================================================================================
// IO
// =================================================================================================

static const int sgli__bytes_in_fd(FILE* fd)
{
    fpos_t fd_pos;
    fgetpos(fd, &fd_pos);
    fseek(fd, 0, SEEK_END);
    int len = ftell(fd);
    fsetpos(fd, &fd_pos);
    return len;
}

static char* sgl_slurp_file(const char* path, size_t *out_size)
{
    FILE* fd = fopen(path, "r");
    if (!fd)
    {
        fprintf(stderr, "ERROR: couldn't slurp %s\n", path);
        assert(fd);
        *out_size = 0;
        return NULL;
    }
    int len = sgli__bytes_in_fd(fd);
    char* contents = sgl_malloc(len + 1);
    if (contents)
    {
        const size_t read = (int)fread((void*)contents, 1, (size_t)len, fd);
        assert (read <= len);
        fclose(fd);
        *out_size = read + 1;
        contents[read] = '\0';
    }
    return contents;
}

static int sgl_count_lines(char* contents)
{
    int num_lines = 0;
    int64_t len = strlen(contents);
    for(int64_t i = 0; i < len; ++i)
    {
        if (contents[i] == '\n')
        {
            num_lines++;
        }
    }
    return num_lines;
}

#ifdef __cplusplus
}
#endif
