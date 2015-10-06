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

#include <inttypes.h>
#include <stdlib.h>  // memcpy
#include <string.h>  // memset
#include <stdint.h>
#include <stdio.h>

// ====
// typedefs
// ====

typedef uint32_t sglBool ;

// ====
// MACROS
// ====

#ifndef sgl_malloc
#define sgl_malloc malloc
#endif

#ifndef sgl_calloc
#define sgl_calloc calloc
#endif

#ifndef sgl_realloc
#define sgl_realloc realloc
#endif

#ifndef sgl_free
#define sgl_free free
#endif

#ifndef sgl_log
#ifdef _WIN32
#define sgl_log sgl_win32_log
#else
#define sgl_log printf
#endif
#endif

#define sgl_array_count(a) (sizeof((a))/sizeof((a)[0]))

// ==== stb stretchy buffer, with slight modifications, like zeroing out memory.
//  Shamelessly substituting stb_ for sgl_
// -- Define SGL_OUT_OF_MEMORY to handle failures. Something like `#define SGL_OUT_OF_MEMORY panic("array failed\n")`
#define sb_free(a)      ((a) ? sgl_free(sgl__sbraw(a)),0 : 0)
#define sb_push(a,v)    (sgl__sbmaybegrow(a,1), (a)[sgl__sbcount(a)++] = (v))
#define sb_count(a)     ((a) ? sgl__sbcount(a) : 0)
#define sb_add(a,n)     (sgl__sbmaybegrow(a,n), sgl__sbcount(a)+=(n), &(a)[sgl__sbcount(a)-(n)])
#define sb_last(a)      ((a)[sgl__sbcount(a)-1])
#define sb_reset(a)     memset((a), 0, sizeof(*a)*sgl__sbcount(a)), sgl__sbcount(a) = 0

#define sgl__sbraw(a)       ((int *) (a) - 2)
#define sgl__sbcapacity(a)  sgl__sbraw(a)[0]
#define sgl__sbcount(a)     sgl__sbraw(a)[1]

#define sgl__sbneedgrow(a,n)  ((a)==0 || sgl__sbcount(a)+(n) >= sgl__sbcapacity(a))
#define sgl__sbmaybegrow(a,n) (sgl__sbneedgrow(a,(n)) ? sgl__sbgrow(a,n) : 0)
#define sgl__sbgrow(a,n)      ((a) = sgl__sb_grow_impl((a), (n), sizeof(*(a))))
static void * sgl__sb_grow_impl(void *arr, int increment, int itemsize);


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

#define  arena_alloc_elem(arena, T)         (T *)arena_alloc_bytes((arena), sizeof(T))
#define  arena_alloc_array(arena, count, T) (T *)arena_alloc_bytes((arena), (count) * sizeof(T))
#define  arena_available_space(arena)       ((arena)->size - (arena)->count)

// Create an independent arena from existing arena.
Arena arena_spawn(Arena* parent, size_t size);

// -- Temporary arenas.
// Usage:
//      child = arena_push(my_arena, some_size);
//      use_temporary_arena(&child.arena);
//      arena_pop(child);
Arena    arena_push(Arena* parent, size_t size);
void     arena_pop (Arena* child);

void* arena_alloc_bytes(Arena* arena, size_t num_bytes);

#define ARENA_VALIDATE(arena)           assert ((arena)->num_children == 0)

// Empty arena
void arena_reset(Arena* arena);


// ====
// Threads
// ====


// Platform-agnostic definitions.
typedef struct SglMutex_s SglMutex;
typedef struct SglSemaphore_s SglSemaphore;

int32_t         sgl_cpu_count(void);
SglSemaphore*   sgl_create_semaphore(int32_t value);
int32_t         sgl_semaphore_wait(SglSemaphore* sem);  // Will return non-zero on error
int32_t         sgl_semaphore_signal(SglSemaphore* sem);
SglMutex*       sgl_create_mutex(void);
int32_t         sgl_mutex_lock(SglMutex* mutex);
int32_t         sgl_mutex_unlock(SglMutex* mutex);
void            sgl_destroy_mutex(SglMutex* mutex);
void            sgl_create_thread(void (*thread_func)(void*), void* params);


// ====
// IO
// -- Small functions for simple text processing. Meant for script-like programs.
// ====

// All of the text manipulations allocate new memory, they don't modify the original input

char*   sgl_slurp_file(const char* path, int64_t *out_size);
char**  sgl_split_lines(char* contents, int32_t* out_num_lines);
char**  sgl_tokenize(char* string, char* separator);
char*   sgl_strip_whitespace(char* in);
int32_t sgl_count_lines(char* contents);


// ====
// Windows helpers
// ====


#ifdef _WIN32
#include <windows.h>
// print to win console
void sgl_win32_log(char *format, ...);
#endif


// ==== Implementation


#ifdef LIBSERG_IMPLEMENTATION

static void* sgl__sb_grow_impl(void *arr, int increment, int itemsize)
{
    int dbl_cur = arr ? 2*sgl__sbcapacity(arr) : 0;
    int min_needed = sb_count(arr) + increment;
    int m = dbl_cur > min_needed ? dbl_cur : min_needed;
    int *p = (int *) sgl_realloc(arr ? sgl__sbraw(arr) : 0, itemsize * m + sizeof(int)*2);
    if (p) {
        if (!arr) {
            p[1] = 0;
        }
        p[0] = m;
        return p + 2;
    } else {
#ifdef SGL_OUT_OF_MEMORY
        SGL_OUT_OF_MEMORY;
#endif
        return (void *) (2*sizeof(int)); // try to force a NULL pointer exception later
    }
}


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
    char** result = (char**)sgl_calloc(num_lines, sizeof(char*));

    char* line = contents;
    for (int32_t line_i = 0; line_i < num_lines; ++line_i) {
        int32_t line_length = 0;
        char* iter = line;
        while (*iter++ != '\n') {
            ++line_length;
        }
        iter = line;
        char* split = (char*)sgl_calloc(line_length + 1, sizeof(char));
        memcpy(split, line, line_length);
        line += line_length + 1;
        result[line_i] = split;
    }
    *out_num_lines = num_lines;
    return result;
}

char** sgl_tokenize(char* string, char* separator)
{
    char** token_array = NULL;

    char* tok_begin = string;
    char* tok_iter = string;
    int32_t tok_len = 0;
    while (*tok_begin != '\0') {
        char* sep_iter = separator;
        while (*sep_iter == *tok_iter++) { ++sep_iter; }
        if (*sep_iter == '\0' || *(tok_iter - 1) == '\0') {
            // Push token.
            if ( tok_len ) {
                char* new_tok = (char*) sgl_calloc(tok_len + 1, sizeof(char));
                memcpy(new_tok, tok_begin, tok_len);
                sb_push(token_array, new_tok);
            }
            tok_len = 0;
            tok_begin = --tok_iter;
        } else {
            ++tok_len;
        }
    }
    return token_array;
}

char* sgl_strip_whitespace(char* in)
{
    char* str = (char*)sgl_calloc(strlen(in) + 1, sizeof(char));
    memcpy(str, in, strlen(in));  // terminating 0 from calloc ;)
    char* i = str;
    char* begin = NULL;
    while(isspace(*i++)) { }
    begin = i - 1;

    char* end = begin;
    while(*end++ != '\0') {}
    --end;
    while(isspace(*end)) {
        *end-- = '\0';
    }

    return begin;
}

sglBool sgl_is_number(char* s)
{
    sglBool ok = 1;

    if (*s == '-' || *s == '+') {
        ++s;
    }
    for(;*s != '\0';++s) {
        if (!isdigit(*s)) {
            ok = 0;
            break;
        }
    }

    return ok;
}

#ifdef _WIN32
void sgl_win32_log(char *format, ...)
{
    static char message[ 1024 ];

    int num_bytes_written = 0;

    va_list args;

    assert ( format );

    va_start( args, format );

    num_bytes_written = _vsnprintf(message, sizeof( message ) - 1, format, args);

    if ( num_bytes_written > 0 ) {
        OutputDebugStringA( message );
    }

    va_end( args );
}
#endif


#endif  // LIBSERG_IMPLEMENTATION

#ifdef __cplusplus
}
#endif

// HISTORY
// 2015-09-25 -- Added LIBSERG_IMPLEMENTATION macro, sgl_split_lines()
