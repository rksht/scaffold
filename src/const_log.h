#pragma once

#include <stdint.h>

namespace {

/// Clips the given integer x to the closest power of 2 greater than or equal
/// to x.
inline constexpr uint32_t clip_to_power_of_2(uint32_t x) {
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 16);
    return x + 1;
}

inline constexpr uint32_t log2_ceil(uint32_t n) {
    uint32_t i = 0;
    while (n > 0) {
        n = n >> 1;
        ++i;
    }
    return i;
}

/// Returns `ceil(a/b)`
inline constexpr uint32_t ceil_div(uint32_t a, uint32_t b) {
    uint32_t mod = a % b;
    if (mod) {
        return a / b + 1;
    }
    return a / b;
}
}
