// threads.h
// (c) Copyright 2015 Sergio Gonzalez
//
// Released under the MIT license. See LICENSE.txt

/**
 *
 *  Thin abstraction over Win32/pthreads threading libraries
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include <assert.h>

#ifndef SGL_DO_NOT_DEFINE_DEFAULTS
#include "defaults.h"
#endif

// Platform-agnostic definitions.
typedef struct SglMutex_s SglMutex;
typedef struct SglSemaphore_s SglSemaphore;

static i32 sgl_cpu_count();
static SglSemaphore* sgl_create_semaphore(i32 value);
static i32 sgl_semaphore_wait(SglSemaphore* sem);
static i32 sgl_semaphore_signal(SglSemaphore* sem);
static SglMutex* sgl_create_mutex();
static i32 sgl_mutex_lock(SglMutex* mutex);
static i32 sgl_mutex_unlock(SglMutex* mutex);
static void sgl_destroy_mutex(SglMutex* mutex);
static void sgl_create_thread(void (*thread_func)(void*), void* params);

// =================================
// Windows
// =================================
#ifdef WIN32
#include <Windows.h>
#include <process.h>
#include <malloc.h>  // For simplicity, we are alloating stuff with malloc.

#ifndef sgl_malloc
#define sgl_malloc malloc
#endif

#ifndef sgl_free
#define sgl_free free
#endif

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

static i32 sgl_cpu_count()
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    i32 count = info.dwNumberOfProcessors;
    return count;
}

static SglSemaphore* sgl_create_semaphore(i32 value)
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
static i32 sgl_semaphore_wait(SglSemaphore* sem)
{
    i32 result;

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

static i32 sgl_semaphore_signal(SglSemaphore* sem)
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

static i32 sgl_mutex_lock(SglMutex* mutex)
{
    i32 result = 0;
    EnterCriticalSection(&mutex->critical_section);
    return result;
}

static i32 sgl_mutex_unlock(SglMutex* mutex)
{
    i32 result = 0;
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

static i32 sgl_cpu_count()
{
    // Pretty much copy paste from SDL
    i32 count = -1;
#if defined(HAVE_SYSCONF) && defined(_SC_NPROCESSORS_ONLN)
        if (count <= 0) {
            count = (i32)sysconf(_SC_NPROCESSORS_ONLN);
        }
#endif
#ifdef HAVE_SYSCTLBYNAME
        if (count <= 0) {
            size_t size = sizeof(count);
            sysctlbyname("hw.ncpu", &count, &size, NULL, 0);
        }
#endif
        if (count <= 0)
        {
            count = 1;
        }
        return count;
}

struct SglSemaphore_s
{
    sem_t sem;
};

static SglSemaphore* sgl_create_semaphore(i32 value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
    i32 err = sem_init(&sem->sem, 0, value);
    if (err < 0)
    {
        sgl_free(sem);
        return NULL;
    }
    return sem;
}

static i32 sgl_semaphore_wait(SglSemaphore* sem)
{
    return sem_wait(&sem->sem);
}

static i32 sgl_semaphore_signal(SglSemaphore* sem)
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

static i32 sgl_mutex_lock(SglMutex* mutex)
{
    if (pthread_mutex_lock(&mutex->handle) < 0)
    {
        return -1;
    }
    return 0;
}

static i32 sgl_mutex_unlock(SglMutex* mutex)
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
    if (pthread_create(&leaked_thread, &attr, thread_func, params) != 0)
    {
        assert (!"God dammit");
    }
}

// =================================
// End of Linux
// =================================
#endif  // Platforms
#ifndef SGL_DO_NOT_DEFINE_DEFAULTS
#include "undef_defaults.h"
#endif

#ifdef __cplusplus
}
#endif
