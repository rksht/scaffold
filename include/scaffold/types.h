/*
#pragma once

#include <stdint.h>

#ifndef alignof
#define alignof(x) __alignof(x)
#endif
*/

#include <stdint.h>

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8 = int8_t;

using f32 = float;
using f64 = double;

using uint = unsigned int;
using ulong = unsigned long;

using bool32 = i32;

using zu = size_t;

#if defined(_MSC_VER) && !defined(__PRETTY_FUNCTION__)
#define __PRETTY_FUNCTION__ __FUNCSIG__
#endif
