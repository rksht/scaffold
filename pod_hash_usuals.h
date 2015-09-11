#pragma once
#include "pod_hash.h"
#include <assert.h>

// Template for usual hash and equal functions
namespace pod_hash {
template <typename T> uint64_t usual_hash(T const *k) {
    (void)k;
    assert(0 && "No usual hash function implementated for this type");
}

template <typename T> bool usual_equal(const T &k1, const T &k2) {
    (void)k1;
    (void)k2;
    assert(0 && "No usual equal function implemented for this type");
}
} // ns pod_hash

namespace pod_hash {
// char* strings
template <> uint64_t usual_hash(char *const *s);
template <> bool usual_equal(char *const &s1, char *const &s2);

// char
template <> uint64_t usual_hash(char const *s);
template <> bool usual_equal(char const &s1, char const &s2);
} // namespace pod_hash
