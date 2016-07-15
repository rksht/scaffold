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

/// Returns floor(log_2(n))  (integer logarithm of n)
inline constexpr uint32_t log2_floor(uint32_t n) {
    uint32_t i = 0;
    while (n > 1) {
        n = n / 2;
        ++i;
    }
    return i;
}

constexpr uint64_t _t[6] = {0xFFFFFFFF00000000ull, 0x00000000FFFF0000ull,
                            0x000000000000FF00ull, 0x00000000000000F0ull,
                            0x000000000000000Cull, 0x0000000000000002ull};

inline constexpr int log2_ceil(uint64_t x) {
    uint64_t y = (((x & (x - 1)) == 0) ? 0 : 1);
    uint64_t j = 32;

    for (uint32_t i = 0; i < 6; i++) {
        int k = (((x & _t[i]) == 0) ? 0 : j);
        y += k;
        x >>= k;
        j >>= 1;
    }

    return y;
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
