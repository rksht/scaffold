/// Testing the performance of the chaining-based hash table vs a quadratic
/// hash table. The code is in a C-style that doesn't use member functions.
/// This one uses 2 arrays, one for keys and one for values. This should yield
/// less cache misses because we only look into the keys of the entries during
/// lookup.

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
static constexpr uint64_t TOMBSTONE = std::numeric_limits<uint64_t>::max();

/// The Open-addressed hash-table representation. We use two arrays, one for
/// the keys and one for the corresponding values. Both always have the same
/// size. The values array is _not_ kept compressed. So the table is good when
/// you have a small number of key-value pairs to store.
template <typename T> struct ProbedHash {
    /// Don't support non-POD types
    static_assert(std::is_trivially_copyable<T>::value,
                  "ProbedHash only supports POD-types as values!");

    /// The array used to store keys
    Array<uint64_t> _keys;
    /// And the array for corresponding values
    Array<T> values;

    /// Number of valid entries in the array
    uint32_t _num_entries;

    ProbedHash(Allocator &keys_alloc, Allocator &values_alloc,
               uint32_t starting_size)
        : _keys{keys_alloc}, values{values_alloc}, _num_entries{0} {
        assert(starting_size > 2);
        starting_size = clip_to_power_of_2(starting_size);
        array::resize(_keys, clip_to_power_of_2(starting_size));
        array::resize(values, clip_to_power_of_2(starting_size));
        for (uint64_t &key : _keys) {
            key = TOMBSTONE;
        }
    }
};
}

namespace probed_hash::_internal {

// Constant terms
constexpr uint64_t _c1 = 3;
constexpr uint64_t _c2 = 5;

/// True if number of entries >= 1/2 of the array size.
template <typename T> inline bool should_rehash(const ProbedHash<T> &h) {
    static constexpr float load_factor = 0.50;
    return h._num_entries / float(array::size(h._keys)) >= load_factor;
}

/// Allocates new array and copies elements there. Deallocates previous array.
template <typename T> void rehash(ProbedHash<T> &h, uint32_t new_size) {
    new_size = clip_to_power_of_2(new_size);

    ProbedHash<T> new_hash{*h._keys._allocator, *h.values._allocator, new_size};

    for (uint64_t i = 0; i < array::size(h._keys); ++i) {
        new_hash._keys[i] = TOMBSTONE;
    }

    // Reinsert already inserted keys
    for (uint64_t i = 0; i < array::size(h._keys); ++i) {
        if (h._keys[i] != TOMBSTONE) {
            probed_hash_set(new_hash, h._keys[i], h.values[i]);
        }
    }
    std::swap(h._keys, new_hash._keys);
    std::swap(h.values, new_hash.values);
}
}

namespace probed_hash {
template <typename T>
uint64_t probed_hash_find(const ProbedHash<T> &h, uint64_t key) {
    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        uint64_t idx = (hash + _internal::_c1 * i + _internal::_c2 * (i * i)) %
                       array::size(h._keys);
        if (h._keys[idx] == key) {
            return idx;
        }
        if (h._keys[idx] == TOMBSTONE) {
            return TOMBSTONE;
        }
    }
    return TOMBSTONE;
}

template <typename T>
uint64_t probed_hash_set(ProbedHash<T> &h, uint64_t key, T value) {
    if (_internal::should_rehash(h)) {
        _internal::rehash(h, array::size(h._keys) * 2);
    }

    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);

    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        uint64_t idx = (hash + _internal::_c1 * i + _internal::_c2 * (i * i)) %
                       array::size(h._keys);
        if (h._keys[idx] == TOMBSTONE || h._keys[idx] == key) {
            h._keys[idx] = key;
            h.values[idx] = value;
            h._num_entries++;
            return idx;
        }
    }
    assert(0);
}

template <typename T>
uint64_t probed_hash_set_default(ProbedHash<T> &h, uint64_t key,
                                 T default_value) {
    if (_internal::should_rehash(h)) {
        _internal::rehash(h, array::size(h._keys) * 2);
    }

    const uint64_t hash = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);

    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        uint64_t idx = (hash + _internal::_c1 * i + _internal::_c2 * (i * i)) %
                       array::size(h._keys);
        if (h._keys[idx] == TOMBSTONE) {
            h._keys[idx] = key;
            h.values[idx] = default_value;
            h._num_entries++;
            return idx;
        }
        if (h._keys[idx] == key) {
            return idx;
        }
    }
    assert(0);
}
}

#if 1
int main() {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {

        // setbuf(stdout, nullptr);

        const uint64_t INITIAL_SIZE = 1 << 10;
        const uint64_t MAX_ENTRIES = 1 << 25;
        const uint8_t VALUE_MARKER = 'a';

        ProbedHash<Obj<4>> hash{memory_globals::default_allocator(),
                                memory_globals::default_allocator(),
                                INITIAL_SIZE};

        Obj<4> ob{};
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
                assert(hash.values[idx].buf[0] == VALUE_MARKER);
            }
        }
    }
    memory_globals::shutdown();
}

#endif

#if 0

#include <stdio.h>

using namespace foundation;
using namespace probed_hash;

struct WordStore {
    ProbedHash<char *> map;

    WordStore()
        : map{memory_globals::default_allocator(),
              memory_globals::default_allocator(), 1024} {}

    char *add_string(char *buffer, uint32_t length) {
        char *str =
            (char *)memory_globals::default_allocator().allocate(length, 1);
        memcpy(str, buffer, length);
        uint64_t idx = probed_hash_set_default(map, (uint64_t)str, str);
        char *prev_str = map.values[idx];
        if (prev_str != str) {
            memory_globals::default_allocator().deallocate(str);
            return prev_str;
        }
        return str;
    }

    ~WordStore() {
        for (char *str : map.values) {
            memory_globals::default_allocator().deallocate(str);
        }
    }
};

int main() {
    memory_globals::init();
    {
        WordStore w{};
        for (;;) {
            char buf[1024] = {0};
            fgets(buf, sizeof(buf), stdin);
            auto len = strlen(buf);
            if (buf[len - 1] == '\n') {
                buf[len - 1] = '\0';
            }
            if (buf[0] == '\0') {
                break;
            }
            w.add_string(buf, len);
        }
        fprintf(stderr, "Inserted\n");
        for (auto str : w.map.values) {
            printf("%s\n", str);
        }
    }
    memory_globals::shutdown();
}
#endif