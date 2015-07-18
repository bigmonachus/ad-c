/**
 * threads.h
 *  Sergio Gonzalez
 *
 *  Thin abstraction over Win32/pthreads threading libraries
 *
 */

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef SGL_DO_NOT_DEFINE_DEFAULTS
#include "defaults.h"
#endif

// Platform-agnostic definitions.
typedef struct SglMutex_s SglMutex;
typedef struct SglSemaphore_s SglSemaphore;

// =================================
// Windows
// =================================
#ifdef WIN32
#include <Windows.h>
#include <process.h>
#include <malloc.h>  // For simplicity, we are alloating stuff with mallocs.

#ifndef sgl_malloc
#define sgl_malloc malloc
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
    int count = info.dwNumberOfProcessors;
    return count;
}

static SglSemaphore* sgl_create_semaphore(i32 value)
{
    SglSemaphore* sem = (SglSemaphore*)sgl_malloc(sizeof(SglSemaphore));
    sem->handle = CreateSemaphore(0, value, SGL_MAX_SEMAPHORE_VALUE, NULL);
    sem->value = value;
    if (!sem->handle)
    {
        free (sem);
        sem = NULL;
    }
    return sem;
}

// Will return non-zero on error
static int sgl_semaphore_wait(SglSemaphore* sem)
{
    int result;

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

static int sgl_semaphore_signal(SglSemaphore* sem)
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
        free(mutex);
    }
}

static void sgl_create_thread(void (*thread_func)(void*), void* params)
{
    _beginthread(thread_func, 0, params);
}

#endif  // WIN32
// =================================
// End of Windows
// =================================

#ifndef SGL_DO_NOT_DEFINE_DEFAULTS
#include "undef_defaults.h"
#endif

#ifdef __cplusplus
}
#endif
