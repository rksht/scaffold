#include <string.h>
#include "pod_hash_usuals.h"
#include "murmur_hash.h"

/// Usual hash functions for selected types.
namespace pod_hash {
// cstring hash
template <> uint64_t usual_hash(char *const *s) {
    unsigned l = strlen(*s);
    return (uint64_t)foundation::murmur_hash_64(*s, l, 0xDEADBEEF);
}

// cstring equal
template <> bool usual_equal(char *const &s1, char *const &s2) {
    return strcmp(s1, s2) == 0;
}

// char hash
template <> uint64_t usual_hash(char const *s) { return *s; }

// char equal
template <> bool usual_equal(char const &s1, char const &s2) {
    return s1 == s2;
}
} // ns pod_hash
