#pragma once

#include <stdint.h>
#include <assert.h>
#include <iostream>
#include <stdlib.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t byte;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;

#define C64(x) UINT64_C(x)
#define C64_s(x) INT64_C(x)

//#define STATS_ENABLED
//#define EVAL_DEBUG
#ifdef EVAL_DEBUG
#define EVAL_INFO(field, color, val_mg, val_eg) \
	do {eval_info.Update(field, color, val_mg, val_eg);} while (0)
#else
#define EVAL_INFO(field, color, val_mg, val_eg)
#endif

#ifdef STATS_ENABLED
#define STATS(x) \
	do {x;} while (0)
#else
#define STATS(x)
#endif
	

#if defined(_WIN32) || defined(_WIN64)
#define PLATFORM_WINDOWS
#elif defined(__unix__)
#define PLATFORM_UNIX
#endif

#ifdef ANKA_DEBUG
#define ANKA_ASSERT(x) \
			do {if (!(x)) {std::cerr << "Assertion failed. File: "<<__FILE__<<" , Line: "<<__LINE__<<"\n"; abort();}} while (0)
#else
#define ANKA_ASSERT(x)
#endif

#if defined(__GNUG__)
#define force_inline __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define force_inline __forceinline
#else
#define force_inline inline
#endif