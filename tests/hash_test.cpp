/// Testing the performance of the chaining-based hash table vs a quadratic hash
/// table.

#include "hash.h"
#include "array.h"
#include "const_log.h"
#include "debug.h"
#include "memory.h"
#include "murmur_hash.h"
#include <array>
#include <limits>
#include <stdlib.h>
#include <utility>

using namespace foundation;

/// Since we will try out different sizes of values we use std::array to create
/// buffers of the size we want
template <int size_in_bytes> using Obj = std::array<uint8_t, size_in_bytes>;

/// Denotes an invalid index or key.
constexpr uint64_t TOMBSTONE = std::numeric_limits<uint64_t>::max();

/// Tag for probe method
struct ProbeLinear {};
struct ProbeQuadratic {};

/// Selects which probe method to use
template <typename Probe> struct ToAdd;

template <> struct ToAdd<ProbeLinear> {
    static uint64_t to_add(uint64_t last) { return (last + 1) * (last + 1); }
};

template <> struct ToAdd<ProbeQuadratic> {
    static uint64_t to_add(uint64_t last) { return last + 1; }
};

/// The table representation
template <typename T, typename Probe> struct ProbedHash {
    Array<std::pair<uint64_t, T>> _arr;

    ProbedHash(Allocator &a, uint64_t starting_size) : _arr(a) {
        array::resize(_arr, clip_to_power_of_2(starting_size));
        for (auto &e : _arr) {
            e.first = TOMBSTONE;
        }
    }
};

template <typename T, typename Probe>
void _probed_hash_rehash(ProbedHash<T, Probe> &h, uint32_t new_size) {
    new_size = clip_to_power_of_2(new_size);
    ProbedHash<T, Probe> new_hash{*h._arr._allocator, new_size};

    for (uint64_t i = 0; i < array::size(h._arr); ++i) {
        new_hash._arr[i].first = TOMBSTONE;
    }

    // Rehash already inserted keys
    for (auto &entry : h._arr) {
        if (entry.first != TOMBSTONE) {
            probed_hash_set(new_hash, entry.first, entry.second);
        }
    }
    std::swap(h._arr, new_hash._arr);
}

template <typename T, typename Probe>
uint64_t probed_hash_find(const ProbedHash<T, Probe> &h, uint64_t key) {
    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t add = 0;
    uint64_t i = 0;
    while (i < array::size(h._arr)) {
        uint64_t idx = (hash + add) % array::size(h._arr);
        if (h._arr[idx].first == key) {
            return idx;
        }
        if (h._arr[idx].first == TOMBSTONE) {
            return TOMBSTONE;
        }
        add = ToAdd<Probe>::to_add(add);
        i++;
    }
    return TOMBSTONE;
}

template <typename T, typename Probe>
uint64_t probed_hash_set(ProbedHash<T, Probe> &h, uint64_t key, T value) {
    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t add = 0;
    uint64_t i = 0;
    while (i < array::size(h._arr)) {
        uint64_t idx = (hash + add) % array::size(h._arr);
        if (h._arr[idx].first == TOMBSTONE || h._arr[idx].first == key) {
            h._arr[idx].first = key;
            h._arr[idx].second = value;
            return idx;
        }
        add = ToAdd<Probe>::to_add(add);
        i++;
    }
    return TOMBSTONE;
}

int main() {
    memory_globals::init();
    {

        setbuf(stdout, nullptr);

        ProbedHash<uint64_t, ProbeLinear> hash{
            memory_globals::default_allocator(), 1024};

        for (uint64_t i = 0; i < 512; ++i) {
            auto idx = probed_hash_set(hash, i, 1024 - i);
            if (idx == TOMBSTONE) {
                log_info("Insertion failed for key = %lu", i);
            }
        }

        for (uint64_t i = 0; i < 512; ++i) {
            auto idx = probed_hash_find(hash, i);
            if (idx == TOMBSTONE) {
                log_info("Have TOMBSTONE at key = %lu", i);
            } else {
                printf("Index=%lu, [%lu] = [%lu]\n", idx, i,
                       hash._arr[idx].second);
            }
        }
    }
    memory_globals::shutdown();
}
