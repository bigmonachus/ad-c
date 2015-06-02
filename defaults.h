// defaults.h
// (c) Copyright 2015 Sergio Gonzalez

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

// Rename types for convenience
typedef int8_t      int8;
typedef uint8_t     uint8;
typedef int16_t     int16;
typedef uint16_t    uint16;
typedef int32_t     int32;
typedef uint32_t    uint32;
typedef int64_t     int64;
typedef uint64_t    uint64;
typedef int32_t     bool32;

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
