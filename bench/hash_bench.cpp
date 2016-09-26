/// Testing the performance of the chaining-based hash table vs an open-
/// addressed quadratic hash table. The code is in a C-style that doesn't use
/// member functions. This one uses 2 arrays, one for keys and one for values.
/// This should yield less cache misses because we only look into the keys of
/// the entries during lookup.

#include <scaffold/array.h>
#include <scaffold/const_log.h>
#include <scaffold/debug.h>
#include <scaffold/hash.h>
#include <scaffold/memory.h>
#include <scaffold/murmur_hash.h>

#include <array>
#include <assert.h>
#include <limits>
#include <stdlib.h>
#include <utility>

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

namespace probed_hash {
namespace _internal {

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
}

namespace probed_hash {

/// Returns the array position at which the associated value exists, if it
/// does exist. Otherwise returns `TOMBSTONE`
template <typename T>
uint64_t probed_hash_find(const ProbedHash<T> &h, uint64_t key) {
    // uint64_t idx = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t idx = key;
    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        idx = (idx + i) % array::size(h._keys);
        if (h._keys[idx] == key) {
            return idx;
        }
        if (h._keys[idx] == TOMBSTONE) {
            return TOMBSTONE;
        }
    }
    return TOMBSTONE;
}

/// Associates the given value with the given key. May trigger a rehash if key
/// doesn't exist already. Returns the position of the value.
template <typename T>
uint64_t probed_hash_set(ProbedHash<T> &h, uint64_t key, T value) {
    if (_internal::should_rehash(h)) {
        _internal::rehash(h, array::size(h._keys) * 2);
    }

    //uint64_t idx = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t idx = key;
    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        idx = (idx + i) % array::size(h._keys);
        if (h._keys[idx] == TOMBSTONE || h._keys[idx] == key) {
            h._keys[idx] = key;
            h.values[idx] = value;
            h._num_entries++;
            return idx;
        }
    }
    assert(0);
}

/// Associates the given value with the given key but only if the key doesn't
/// exist in the table already. Returns the position of the value.
template <typename T>
uint64_t probed_hash_set_default(ProbedHash<T> &h, uint64_t key,
                                 T default_value) {
    if (_internal::should_rehash(h)) {
        _internal::rehash(h, array::size(h._keys) * 2);
    }

    // uint64_t idx = murmur_hash_64(&key, sizeof(key), 0xCAFEBABE);
    uint64_t idx = key;
    for (uint32_t i = 0; i < array::size(h._keys); ++i) {
        idx = (idx + i) % array::size(h._keys);
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

#include <benchmark/benchmark.h>

// argument 0 is the number of entries to insert
static void BM_probed_hash_insertion(benchmark::State &bm_state) {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {
        while (bm_state.KeepRunning()) {
            bm_state.PauseTiming();
            const uint32_t MAX_ENTRIES = bm_state.range(0);

            ProbedHash<uint64_t> hash{memory_globals::default_allocator(),
                                      memory_globals::default_allocator(),
                                      MAX_ENTRIES};

            bm_state.ResumeTiming();

            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                uint64_t idx = probed_hash_set(hash, i, 0xdeadbeeflu);
                if (idx == TOMBSTONE) {
                    log_info("Insertion failed for key = %lu", i);
                    bm_state.SkipWithError(
                        "Insertion failed, never should've happened. NEVER.");
                }
            }
        }
    }
    memory_globals::shutdown();
}

#include <scaffold/hash.h>

// argument 0 is the number of entries to insert
static void BM_uint_hash_insertion(benchmark::State &bm_state) {
    using namespace foundation;

    memory_globals::init();
    {
        while (bm_state.KeepRunning()) {
            bm_state.PauseTiming();

            const uint32_t MAX_ENTRIES = bm_state.range(0);

            Hash<uint64_t> h{memory_globals::default_allocator()};
            hash::reserve(h, MAX_ENTRIES);

            bm_state.ResumeTiming();

            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                hash::set(h, i, 0xdeadbeeflu);
            }
        }
    }
    memory_globals::shutdown();
}

#include <scaffold/pod_hash.h>
#include <scaffold/pod_hash_usuals.h>

// argument 0 is the number of entries to insert
static void BM_pod_hash_insertion(benchmark::State &bm_state) {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {
        while (bm_state.KeepRunning()) {
            bm_state.PauseTiming();

            const uint32_t MAX_ENTRIES = bm_state.range(0);

#if 1

            PodHash<uint64_t, uint64_t> h{memory_globals::default_allocator(),
                                          memory_globals::default_allocator(),
                                          usual_hash<uint64_t>,
                                          usual_equal<uint64_t>};
#endif

#if 0
            PodHash<uint64_t, uint64_t> h{
                memory_globals::default_allocator(),
                memory_globals::default_allocator(),
                [](const uint64_t &key) {
                    return murmur_hash_64(&key, sizeof(uint64_t), 0xCAFEBABE);
                },
                [](const uint64_t &key1, const uint64_t &key2) {
                    return key1 == key2;
                }};

#endif

            pod_hash::reserve(h, MAX_ENTRIES);

            bm_state.ResumeTiming();

            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                pod_hash::set(h, i, 0xdeadbeeflu);
            }
        }
    }
    memory_globals::shutdown();
}

// argument 0 is the number of entries to insert
static void BM_probed_hash_search(benchmark::State &bm_state) {
    using namespace foundation;
    using namespace probed_hash;

    memory_globals::init();
    {
        const uint32_t MAX_ENTRIES = bm_state.range(0);

        ProbedHash<uint64_t> hash{memory_globals::default_allocator(),
                                  memory_globals::default_allocator(),
                                  MAX_ENTRIES};

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            uint64_t idx = probed_hash_set(hash, i, 0xdeadbeeflu);
            if (idx == TOMBSTONE) {
                log_info("Insertion failed for key = %lu", i);
                bm_state.SkipWithError(
                    "Insertion failed, never should've happened. NEVER.");
            }
        }

        while (bm_state.KeepRunning()) {
            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                uint64_t idx = probed_hash_find(hash, i);
                if (idx == TOMBSTONE) {
                    log_info("Search failed for key = %lu", i);
                    bm_state.SkipWithError(
                        "Search failed, shouldn't have happened in this test");
                }
            }
        }
    }
    memory_globals::shutdown();
}

static void BM_pod_hash_search(benchmark::State &bm_state) {
    using namespace foundation;
    using namespace pod_hash;

    memory_globals::init();
    {

#if 0
        PodHash<uint64_t, uint64_t> h{
            memory_globals::default_allocator(),
            memory_globals::default_allocator(),
            [](const uint64_t &key) {
                return murmur_hash_64(&key, sizeof(uint64_t), 0xCAFEBABE);
            },
            [](const uint64_t &key1, const uint64_t &key2) {
                return key1 == key2;
            }};

#endif

        PodHash<uint64_t, uint64_t> h{memory_globals::default_allocator(),
                                      memory_globals::default_allocator(),
                                      usual_hash<uint64_t>,
                                      usual_equal<uint64_t>};

        const uint32_t MAX_ENTRIES = bm_state.range(0);
            // bm_state.range(0) <= (16 << 10) ? (16 << 10) : bm_state.range(0);

        pod_hash::reserve(h, MAX_ENTRIES);

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            pod_hash::set(h, i, 0xdeadbeeflu);
        }

        while (bm_state.KeepRunning()) {
            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                auto ret = pod_hash::get(h, i);
                // assert(ret.present);
                // assert(ret.value == 0xdeadbeeflu);
                if (!ret.present) {
                    bm_state.SkipWithError(
                        "Didn't find a ket, shouldn't happen.");
                }
            }
        }
    }
    memory_globals::shutdown();
}

static void BM_uint_hash_search(benchmark::State &bm_state) {
    using namespace foundation;
    using namespace pod_hash;

    memory_globals::init();
    {

        Hash<uint64_t> h{memory_globals::default_allocator()};

        const uint32_t MAX_ENTRIES = bm_state.range(0);
        hash::reserve(h, MAX_ENTRIES);

        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            hash::set(h, i, 0xdeadbeeflu);
        }

        while (bm_state.KeepRunning()) {
            for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
                auto val = hash::get(h, i, 0lu);
                if (val != 0xdeadbeeflu) {
                    bm_state.SkipWithError("Should not happen");
                }
            }
        }
    }
    memory_globals::shutdown();
}

#include <unordered_map>

static void BM_stdumap_hash_search(benchmark::State &bm_state) {

    std::unordered_map<uint64_t, uint64_t> h;

    const uint32_t MAX_ENTRIES = bm_state.range(0);

    h.reserve(MAX_ENTRIES);

    for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
        h[i] = 0xdeadbeeflu;
    }

    while (bm_state.KeepRunning()) {
        for (uint64_t i = 0; i < MAX_ENTRIES; ++i) {
            auto it = h.find(i);
            if (it == h.end()) {
                bm_state.SkipWithError("Should not happen");
            }
        }
    }
}

// BENCHMARK(BM_probed_hash_insertion)->RangeMultiplier(2)->Range(16, 8 << 10);
// BENCHMARK(BM_uint_hash_insertion)->RangeMultiplier(2)->Range(16, 8 << 10);
// BENCHMARK(BM_pod_hash_insertion)->RangeMultiplier(2)->Range(16, 8 << 10);

constexpr uint32_t MAX_ENTRIES = 4 << 20;

BENCHMARK(BM_stdumap_hash_search)->RangeMultiplier(2)->Range(1024, MAX_ENTRIES);
BENCHMARK(BM_uint_hash_search)->RangeMultiplier(2)->Range(1024, MAX_ENTRIES);
BENCHMARK(BM_probed_hash_search)->RangeMultiplier(2)->Range(1024, MAX_ENTRIES);
BENCHMARK(BM_pod_hash_search)->RangeMultiplier(2)->Range(1024, MAX_ENTRIES);

BENCHMARK_MAIN();