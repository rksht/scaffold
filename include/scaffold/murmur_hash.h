#pragma once

#include <scaffold/debug.h>
#include <scaffold/types.h>

namespace fo {

/// Implementation of the 64 bit MurmurHash2 function http://murmurhash.googlepages.com/
DLL_PUBLIC uint64_t murmur_hash_64(const void *key, uint32_t len, uint64_t seed);
} // namespace fo
