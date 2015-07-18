// defaults.h
// (c) Copyright 2015 Sergio Gonzalez

#pragma once

#ifdef __cplusplus
extern "C"
{
#endif


#ifndef i8
#define i8 int8_t
#else
#error "i8 already defined"
#endif
#ifndef i16
#define i16 int16_t
#else
#error "i16 already defined"
#endif
#ifndef i32
#define i32 int32_t
#else
#error "i32 already defined"
#endif
#ifndef i64
#define i64 int64_t
#else
#error "i64 already defined"
#endif

#ifndef u8
#define u8 uint8_t
#else
#error "u8 already defined"
#endif
#ifndef u16
#define u16 uint16_t
#else
#error "u16 already defined"
#endif
#ifndef u32
#define u32 uint32_t
#else
#error "u32 already defined"
#endif
#ifndef u64
#define u64 uint64_t
#else
#error "u64 already defined"
#endif

#ifndef f32
#define f32 float
#else
#error "f32 already defined"
#endif

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
