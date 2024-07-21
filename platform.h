#pragma once

#include <stdint.h>
#define HANDMADE_MATH_USE_DEGREES
#include "HandmadeMath.h"

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;
typedef b32 bool;

#define true 1
#define false 0

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

typedef HMM_Mat4 m4;
typedef HMM_Mat3 m3;
typedef HMM_Vec3 v3;
typedef HMM_Vec2 v2;

#define v3(x, y, z) HMM_V3(x, y, z)
#define m4_diagonal(value) HMM_M4D(value)

#define internal static
#define local_persist static
#define global static

#define kilobytes(value) ((value)*1024LL)
#define megabytes(value) (kilobytes(value)*1024LL)
#define gigabytes(value) (megabytes(value)*1024LL)
#define terabytes(value) (gigabytes(value)*1024LL)

#define array_count(arr) (sizeof(arr) / sizeof((arr)[0]))

#if BLOWBACK_SLOW
#define asserts(expression) if(!(expression)) { *(int *)0 = 0; }
#else
#define asserts(expression)
#endif

