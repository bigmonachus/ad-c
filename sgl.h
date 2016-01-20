#pragma once

#include <stdlib.h>
#include <stdio.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

#define sgl_assert(expr)    \
do {   \
    if ( ! (expr) ) { \
        fprintf(stderr, "Assertion failed at %s:%d", __FILE__, __LINE__);   \
        exit(EXIT_FAILURE);     \
    }\
} while (0)

#define sgl_min(a, b) ((a) < (b) ? a : b)
#define sgl_max(a, b) ((a) < (b) ? b : a)
#define sgl_abs(a, b) sgl_max(a,b) - sgl_min(a,b)


#define sgl_calloc(type, count) (type*)calloc(sizeof(type), count);
