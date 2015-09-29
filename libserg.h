/**
 * libserg.h
 *  - Sergio Gonzalez
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
// Features
//  - C99, but avoids some of its nicer features for C++ compatibility.
//
// History at the bottom of the file.
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


// ====
// MACROS
// ====

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


// ====
// MEMORY
// ====


typedef struct Arena_s Arena;
struct Arena_s {
    // Memory:
    size_t  size;
    size_t  count;
    uint8_t*     ptr;

    // For pushing/popping
    Arena*      parent;
    int32_t     id;
    int32_t     num_children;
};
// Note: Arenas are guaranteed to be zero-filled

// Create a root arena from a memory block.
Arena arena_init(void* base, size_t size);
// Create a child arena.
Arena arena_spawn(Arena* parent, size_t size);

// ==== Temporary arenas.
// Usage:
//      child = arena_push(my_arena, some_size);
//      use_temporary_arena(&child.arena);
//      arena_pop(child);
Arena    arena_push(Arena* parent, size_t size);
void     arena_pop (Arena* child);

#define      arena_alloc_elem(arena, T)         (T *)arena_alloc_bytes((arena), sizeof(T))
#define      arena_alloc_array(arena, count, T) (T *)arena_alloc_bytes((arena), (count) * sizeof(T))
void* arena_alloc_bytes(Arena* arena, size_t num_bytes);

#define arena_available_space(arena)    ((arena)->size - (arena)->count)
#define ARENA_VALIDATE(arena)           assert ((arena)->num_children == 0)

// Empty arena
void arena_reset(Arena* arena);


// ====
// Threads
// ====


// Platform-agnostic definitions.
typedef struct SglMutex_s SglMutex;
typedef struct SglSemaphore_s SglSemaphore;

int32_t          sgl_cpu_count(void);
SglSemaphore*    sgl_create_semaphore(int32_t value);
int32_t          sgl_semaphore_wait(SglSemaphore* sem);  // Will return non-zero on error
int32_t          sgl_semaphore_signal(SglSemaphore* sem);
SglMutex*        sgl_create_mutex(void);
int32_t          sgl_mutex_lock(SglMutex* mutex);
int32_t          sgl_mutex_unlock(SglMutex* mutex);
void             sgl_destroy_mutex(SglMutex* mutex);
void             sgl_create_thread(void (*thread_func)(void*), void* params);


// ====
// IO
// ====



char*   sgl_slurp_file(const char* path, int64_t *out_size);  // Allocates and fills a whole file into memory.
char**  sgl_split_lines(char* contents, int32_t* out_num_lines);  // Allocates *out_num_lines.
int32_t sgl_count_lines(char* contents);


// ==== Implementation


#ifdef LIBSERG_IMPLEMENTATION


void* arena_alloc_bytes(Arena* arena, size_t num_bytes)
{
    size_t total = arena->count + num_bytes;
    if (total > arena->size) {
        return NULL;
    }
    void* result = arena->ptr + arena->count;
    arena->count += num_bytes;
    return result;
}

Arena arena_init(void* base, size_t size)
{
    Arena arena = { 0 };
    arena.ptr = (uint8_t*)base;
    if (arena.ptr) {
        arena.size = size;
    }
    return arena;
}

Arena arena_spawn(Arena* parent, size_t size)
{
    uint8_t* ptr = (uint8_t*)arena_alloc_bytes(parent, size);
    assert(ptr);

    Arena child = { 0 };
    {
        child.ptr    = ptr;
        child.size   = size;
    }

    return child;
}

Arena arena_push(Arena* parent, size_t size)
{
    assert ( size <= arena_available_space(parent));
    Arena child = { 0 };
    {
        child.parent           = parent;
        child.id               = parent->num_children;
        uint8_t* ptr           = (uint8_t*)arena_alloc_bytes(parent, size);
        child.ptr              = ptr;
        child.size             = size;

        parent->num_children += 1;
    }
    return child;
}

void arena_pop(Arena* child)
{
    Arena* parent = child->parent;
    assert(parent);

    // Assert that this child was the latest push.
    assert ((parent->num_children - 1) == child->id);

    parent->count -= child->size;
    char* ptr = (char*)(parent->ptr) + parent->count;
    memset(ptr, 0, child->count);
    parent->num_children -= 1;

    memset(child, 0, sizeof(Arena));
}

void arena_reset(Arena* arena)
{
    memset (arena->ptr, 0, arena->count);
    arena->count = 0;
}

// =================================================================================================
// THREADING
// =================================================================================================


int32_t          sgl_cpu_count(void);
SglSemaphore*    sgl_create_semaphore(int32_t value);
int32_t          sgl_semaphore_wait(SglSemaphore* sem);
int32_t          sgl_semaphore_signal(SglSemaphore* sem);
SglMutex*        sgl_create_mutex(void);
int32_t          sgl_mutex_lock(SglMutex* mutex);
int32_t          sgl_mutex_unlock(SglMutex* mutex);
void             sgl_destroy_mutex(SglMutex* mutex);
void             sgl_create_thread(void (*thread_func)(void*), void* params);

// =================================
// Windows
// =================================
#if defined(_WIN32)
#include <Windows.h>
#include <process.h>


#define SGL_MAX_SEMAPHORE_VALUE (1 << 16)

struct SglMutex_s {
    CRITICAL_SECTION critical_section;
};

struct SglSemaphore_s {
    HANDLE  handle;
    LONG    value;
};

int32_t sgl_cpu_count()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    int32_t count = info.dwNumberOfProcessors;
    return count;
}

SglSemaphore* sgl_create_semaphore(int32_t value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
    sem->handle = CreateSemaphore(0, value, SGL_MAX_SEMAPHORE_VALUE, NULL);
    sem->value = value;
    if (!sem->handle) {
        sgl_free (sem);
        sem = NULL;
    }
    return sem;
}

// Will return non-zero on error
int32_t sgl_semaphore_wait(SglSemaphore* sem)
{
    int32_t result;

    if (!sem) {
        return -1;
    }

    switch (WaitForSingleObject(sem->handle, INFINITE)) {
    case WAIT_OBJECT_0:
        InterlockedDecrement(&sem->value);
        result = 0;
        break;
    case WAIT_TIMEOUT:
        result = -1;
        break;
    default:
        result = -1;
        break;
    }

    return result;
}

int32_t sgl_semaphore_signal(SglSemaphore* sem)
{
    InterlockedIncrement(&sem->value);
    if (ReleaseSemaphore(sem->handle, 1, NULL) == FALSE) {
        InterlockedDecrement(&sem->value);
        return -1;
    }
    return 0;
}

SglMutex* sgl_create_mutex()
{
    SglMutex* mutex = (SglMutex*) sgl_malloc(sizeof(SglMutex));
    InitializeCriticalSectionAndSpinCount(&mutex->critical_section, 2000);
    return mutex;
}

int32_t sgl_mutex_lock(SglMutex* mutex)
{
    int32_t result = 0;
    EnterCriticalSection(&mutex->critical_section);
    return result;
}

int32_t sgl_mutex_unlock(SglMutex* mutex)
{
    int32_t result = 0;
    LeaveCriticalSection(&mutex->critical_section);
    return result;
}

void sgl_destroy_mutex(SglMutex* mutex)
{
    if (mutex)
    {
        DeleteCriticalSection(&mutex->critical_section);
        sgl_free(mutex);
    }
}

void sgl_create_thread(void (*thread_func)(void*), void* params)
{
    _beginthread(thread_func, 0, params);
}

// =================================
// End of Windows
// =================================

// =================================
// Start of Linux
// =================================
#elif defined(__linux__) || defined(__MACH__)
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#if defined(__MACH__)
#include <fcntl.h>
#include <sys/stat.h>
#endif

int32_t sgl_cpu_count()
{
    static int32_t sgli__cpu_count = -1;
    if (sgli__cpu_count <= 0) {
        sgli__cpu_count = (int32_t)sysconf(_SC_NPROCESSORS_ONLN);
    }
    assert (sgli__cpu_count >= 1);
    return sgli__cpu_count;
}

struct SglSemaphore_s
{
    sem_t* sem;
};

SglSemaphore* sgl_create_semaphore(int32_t value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
#if defined(__linux__)
    sem->sem = (sem_t*)sgl_malloc(sizeof(sem_t));
    int err = sem_init(sem->sem, 0, value);
#elif defined(__MACH__)
    sem->sem = sem_open("sgl semaphore", O_CREAT, S_IRWXU, value);
    int err = (sem->sem == SEM_FAILED) ? -1 : 0;
#endif
    if (err < 0) {
        sgl_free(sem);
        return NULL;
    }
    return sem;
}

int32_t sgl_semaphore_wait(SglSemaphore* sem)
{
    return sem_wait(sem->sem);
}

int32_t sgl_semaphore_signal(SglSemaphore* sem)
{
    return sem_post(sem->sem);
}
struct SglMutex_s
{
    pthread_mutex_t handle;
};

SglMutex* sgl_create_mutex()
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

int32_t sgl_mutex_lock(SglMutex* mutex)
{
    if (pthread_mutex_lock(&mutex->handle) < 0)
    {
        return -1;
    }
    return 0;
}

int32_t sgl_mutex_unlock(SglMutex* mutex)
{
    if (pthread_mutex_unlock(&mutex->handle) < 0)
    {
        return -1;
    }

    return 0;
}

void sgl_destroy_mutex(SglMutex* mutex)
{
    pthread_mutex_destroy(&mutex->handle);
    sgl_free(mutex);
}

void sgl_create_thread(void (*thread_func)(void*), void* params)
{
    pthread_attr_t attr;

    /* Set the thread attributes */
    if (pthread_attr_init(&attr) != 0)
    {
        assert(!"Not handling thread attribute failure.");
    }
    //pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    pthread_t leaked_thread;
    if (pthread_create(&leaked_thread, &attr, (void*(*)(void*))(thread_func), params) != 0)
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

char* sgl_slurp_file(const char* path, int64_t* out_size)
{
    FILE* fd = fopen(path, "r");
    if (!fd) {
        fprintf(stderr, "ERROR: couldn't slurp %s\n", path);
        assert(fd);
        *out_size = 0;
        return NULL;
    }
    int64_t len = sgli__bytes_in_fd(fd);
    char* contents = (char*)sgl_malloc(len + 1);
    if (contents) {
        const int64_t read = fread((void*)contents, 1, (size_t)len, fd);
        assert (read <= len);
        fclose(fd);
        *out_size = read + 1;
        contents[read] = '\0';
    }
    return contents;
}

int32_t sgl_count_lines(char* contents)
{
    int32_t num_lines = 0;
    int64_t len = strlen(contents);
    for(int64_t i = 0; i < len; ++i) {
        if (contents[i] == '\n') {
            num_lines++;
        }
    }
    return num_lines;
}

char** sgl_split_lines(char* contents, int32_t* out_num_lines)
{
    int32_t num_lines = sgl_count_lines(contents);
    char** result = (char**)calloc(num_lines, sizeof(char*));

    char* line = contents;
    for (int32_t line_i = 0; line_i < num_lines; ++line_i) {
        int32_t line_length = 0;
        char* iter = line;
        while (*iter++ != '\n') {
            ++line_length;
        }
        iter = line;
        char* split = (char*)calloc(line_length + 1, sizeof(char));
        memcpy(split, line, line_length);
        line += line_length + 1;
        result[line_i] = split;
    }
    *out_num_lines = num_lines;
    return result;
}


#endif  // LIBSERG_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

// HISTORY
// 2015-09-25 -- Added LIBSERG_IMPLEMENTATION macro, sgl_split_lines()
