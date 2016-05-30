#pragma once
#include "pod_hash.h"
#include <assert.h>

// Template for usual hash and equal functions
namespace foundation {
template <typename T> uint64_t usual_hash(T const &k) {
    (void)k;
    assert(0 && "No usual hash function implementated for this type");
    return 0;
}

template <typename T> bool usual_equal(T const &k1, T const &k2) {
    (void)k1;
    (void)k2;
    assert(0 && "No usual equal function implemented for this type");
    return false;
}
} // ns foundation

namespace foundation {
// char* strings
template <> uint64_t usual_hash(char *const &s);
template <> bool usual_equal(char *const &s1, char *const &s2);

// char
template <> uint64_t usual_hash(char const &s);
template <> bool usual_equal(char const &s1, char const &s2);

// int
template <> uint64_t usual_hash(int const& n);
template <> bool usual_equal(int const& n1, int const& n2);

} // namespace foundation
