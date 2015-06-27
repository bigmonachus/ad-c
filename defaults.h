// defaults.h
// (c) Copyright 2015 Sergio Gonzalez

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

typedef int8_t	    i8;
typedef int16_t	    i16;
typedef int32_t     i32;
typedef int64_t     i64;

typedef uint8_t	    u8;
typedef uint16_t    u16;
typedef uint32_t    u32;
typedef uint64_t    u64;

typedef float	    f32;

#ifndef b32
#define		    b32 int
#else
#error b32 already defined
#endif

#ifndef true
#define true 1
#else
#error "true is already defined"
#endif

#ifndef false
#define false 0
#else
#error "false is already defined"
#endif

#ifndef stack_count
#define stack_count(a) (sizeof((a)) / sizeof((a)[0]))
#else
#error "stack_count is already defined"
#endif

#ifdef __cplusplus
}
#endif
