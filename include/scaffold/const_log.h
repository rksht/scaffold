#pragma once

#include <algorithm>
#include <stdint.h>
#include <type_traits>

namespace {

/// Clips the given integer x to the closest power of 2 greater than or equal
/// to x.
template <typename T> inline constexpr T clip_to_power_of_2(T x) {
    static_assert(std::is_integral<T>::value, "Must be integral");
    x = x - 1;
    x = x | (x >> 1);
    x = x | (x >> 2);
    x = x | (x >> 4);
    x = x | (x >> 8);
    x = x | (x >> 16);
    return x + 1;
}

/// Returns floor(log_2(n))  (integer logarithm of n)
template <typename T> inline constexpr T log2_floor(T n) {
    static_assert(std::is_integral<T>::value, "Must be integral");
    T i = 0;
    while (n > 1) {
        n = n / 2;
        ++i;
    }
    return i;
}

constexpr uint64_t _t[6] = {0xFFFFFFFF00000000ull, 0x00000000FFFF0000ull, 0x000000000000FF00ull,
                            0x00000000000000F0ull, 0x000000000000000Cull, 0x0000000000000002ull};

inline constexpr uint64_t log2_ceil(uint64_t x) {
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

template <typename T1, typename T2, typename T3> inline constexpr auto clamp(T1 &&max, T2 &&min, T3 &&value) {
    return std::min(max, std::max(min, value));
}
