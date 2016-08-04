/// Testing the performance of the chaining-based hash table vs a quadratic hash
/// table. The code is in a C-style that doesn't use member functions.

#include "hash.h"
#include "array.h"
#include "const_log.h"
#include "debug.h"
#include "memory.h"
#include "murmur_hash.h"
#include <array>
#include <assert.h>
#include <limits>
#include <stdlib.h>
#include <utility>

/// Since we will try out different sizes of values we use `Obj` to create
/// buffers of the size we want
template <int size_in_bytes> struct Obj {
    uint8_t buf[size_in_bytes];

    void set_first_byte(uint8_t v) { buf[0] = v; }
};

namespace probed_hash {

using namespace foundation;

/// Denotes an invalid index or key.
constexpr uint64_t TOMBSTONE = std::numeric_limits<uint64_t>::max();

/// Tag for probe method
struct ProbeLinear {};
struct ProbeQuadratic {};

/// Selects which probe method to use
template <typename Probe> struct ToAdd;

template <> struct ToAdd<ProbeQuadratic> {
    static uint64_t to_add(uint64_t last) { return (last + 1) * (last + 1); }
};

template <> struct ToAdd<ProbeLinear> {
    static uint64_t to_add(uint64_t last) { return last + 1; }
};

/// The Open-addressed hash-table representation
template <typename T, typename Probe> struct ProbedHash {
    /// Don't support non-POD types
    static_assert(std::is_trivially_copyable<T>::value,
                  "ProbedHash only supports POD-types as values!");

    /// The array used to store entries
    Array<std::pair<uint64_t, T>> array;

    /// Number of valid entries in the array
    uint64_t _num_entries;

    ProbedHash(Allocator &a, uint64_t starting_size)
        : array{a}, _num_entries{0} {
        array::resize(array, clip_to_power_of_2(starting_size));
        for (auto &e : array) {
            e.first = TOMBSTONE;
        }
    }
};
}

namespace probed_hash::_internal {

/// True if number of entries >= 1/2 of the array size.
template <typename T, typename Probe>
inline bool should_rehash(const ProbedHash<T, Probe> &h) {
    return array::size(h.array) <= 2 * h._num_entries;
}

/// Allocates new array and copies elements there. Deallocates previous array.
template <typename T, typename Probe>
void rehash(ProbedHash<T, Probe> &h, uint32_t new_size) {
    new_size = clip_to_power_of_2(new_size);
    ProbedHash<T, Probe> new_hash{*h.array._allocator, new_size};

    for (uint64_t i = 0; i < array::size(h.array); ++i) {
        new_hash.array[i].first = TOMBSTONE;
    }

    // Reinsert already inserted keys
    for (auto &entry : h.array) {
        if (entry.first != TOMBSTONE) {
            probed_hash_set(new_hash, entry.first, entry.second);
        }
    }
    std::swap(h.array, new_hash.array);
}
}

namespace probed_hash {
template <typename T, typename Probe>
uint64_t probed_hash_find(const ProbedHash<T, Probe> &h, uint64_t key) {
    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t add = 0;
    uint64_t i = 0;
    while (i < array::size(h.array)) {
        uint64_t idx = (hash + add) % array::size(h.array);
        if (h.array[idx].first == key) {
            return idx;
        }
        if (h.array[idx].first == TOMBSTONE) {
            return TOMBSTONE;
        }
        add = ToAdd<Probe>::to_add(add);
        i++;
    }
    return TOMBSTONE;
}

template <typename T, typename Probe>
uint64_t probed_hash_set(ProbedHash<T, Probe> &h, uint64_t key, T value) {
    h._num_entries++;
    if (_internal::should_rehash(h)) {
        _internal::rehash(h, array::size(h.array) * 2);
    }

    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);

    uint64_t add = 0;
    uint64_t i = 0;
    while (i < array::size(h.array)) {
        uint64_t idx = (hash + add) % array::size(h.array);
        if (h.array[idx].first == TOMBSTONE || h.array[idx].first == key) {
            h.array[idx].first = key;
            h.array[idx].second = value;
            return idx;
        }
        add = ToAdd<Probe>::to_add(add);
        i++;
    }
    return TOMBSTONE;
}
}

#if 0
int main() {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {

        //setbuf(stdout, nullptr);

        const uint64_t INITIAL_SIZE = 1 << 10;
        const uint64_t MAX_ENTRIES = 1 << 20;

        ProbedHash<uint64_t, ProbeQuadratic> hash{
            memory_globals::default_allocator(), INITIAL_SIZE};

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            uint64_t idx = probed_hash_set(hash, i, i);
            if (idx == TOMBSTONE) {
                log_info("Insertion failed for key = %lu", i);
            }
        }

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            uint64_t idx = probed_hash_find(hash, i);
            if (idx == TOMBSTONE) {
                log_info("Have TOMBSTONE at key = %lu", i);
            } else {
                //printf("Index=%lu, [%lu] = [%lu]\n", idx, i, hash.array[idx].second);
                assert(hash.array[idx].second == i);
            }
        }
    }
    memory_globals::shutdown();
}
#endif

// A test with data size = 128 bytes
int main() {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {

        // setbuf(stdout, nullptr);

        const uint64_t INITIAL_SIZE = 1 << 10;
        const uint64_t MAX_ENTRIES = 1 << 20;
        const uint8_t VALUE_MARKER = 'a';

        ProbedHash<Obj<128>, ProbeQuadratic> hash{
            memory_globals::default_allocator(), INITIAL_SIZE};

        Obj<128> ob{};
        ob.set_first_byte(VALUE_MARKER);

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            uint64_t idx = probed_hash_set(hash, i, ob);
            if (idx == TOMBSTONE) {
                log_info("Insertion failed for key = %lu", i);
            }
        }

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            uint64_t idx = probed_hash_find(hash, i);
            if (idx == TOMBSTONE) {
                log_info("Have TOMBSTONE at key = %lu", i);
            } else {
                assert(hash.array[idx].second.buf[0] == VALUE_MARKER);
            }
        }
    }
    memory_globals::shutdown();
}