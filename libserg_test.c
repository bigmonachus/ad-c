#include "libserg.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

static SglMutex* g_mutex;
static int32_t total_sum;
static SglSemaphore* g_sem;

static void sum_thread(void* params)
{

    int32_t i = *(int32_t*)params;
    sgl_mutex_lock(g_mutex);
    total_sum += i;
    sgl_mutex_unlock(g_mutex);
    sgl_semaphore_signal(g_sem);
}

#define TEST_STACK_SIZE 10
int main()
{
    g_mutex = sgl_create_mutex();
    g_sem = sgl_create_semaphore(0);

    size_t sz = (1L << 15);
    Arena arena = arena_init(calloc(sz, 1), sz);
    {
        size_t saz = 2048;

        Arena sub_arenas[4] = {{ 0 }};
        int32_t* arrays[sgl_array_count(sub_arenas)];

        for (int32_t i = 0; i < sgl_array_count(sub_arenas); ++i)
        {
            sub_arenas[i] = arena_push(&arena, saz);
            Arena* sa = &sub_arenas[i];
            int32_t* seq = arena_make_stack(sa, TEST_STACK_SIZE, int32_t);
            for (int32_t j = 0; j < TEST_STACK_SIZE; ++j)
            {
                arena_stack_push(seq, j);
            }
            arrays[i] = seq;
        }

        for (int32_t i = 0; i < sgl_array_count(sub_arenas); ++i)
        {
            for (int32_t j = 0; j < arena_stack_count(arrays[i]); ++j)
            {
                assert ( arrays[i][j] == j);
            }
        }

        for (int32_t i = 0; i < sgl_array_count(sub_arenas); ++i)
        {
            arena_pop(&sub_arenas[sgl_array_count(sub_arenas) - 1 - i]);
        }
    }

    int32_t cpu_count = sgl_cpu_count();
    printf("The cpu count for this machine is %d\n", cpu_count);


    // Thread test:
    {
        int32_t num_fired = 0;
        for (int32_t i = 0; i <= 200; ++i)
        {
            int* elem = arena_alloc_elem(&arena, int);
            *elem = i;
            sgl_create_thread(sum_thread, (void*)elem);
            num_fired++;
        }

        do
        {
            sgl_semaphore_wait(g_sem);
        } while (--num_fired);
    }

    assert (total_sum == 20100);

    {
        size_t size = 0;
        char* file_contents = sgl_slurp_file("libserg_test.c", &size);
        int num_lines = sgl_count_lines(file_contents);
        printf("The number of lines in this source file is %d\n", num_lines);
    }


    ARENA_VALIDATE(&arena);
    arena_reset(&arena);
    sgl_destroy_mutex(g_mutex);
}

