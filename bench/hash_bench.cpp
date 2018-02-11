#include <benchmark/benchmark.h>
#include <scaffold/array.h>
#include <scaffold/const_log.h>
#include <scaffold/debug.h>
#include <scaffold/hash.h>
#include <scaffold/memory.h>
#include <scaffold/murmur_hash.h>
#include <scaffold/open_hash.h>

#include <array>
#include <assert.h>
#include <limits>
#include <stdlib.h>
#include <utility>

#include <unordered_map>

namespace fo {

// OpenNil for our uint64_t
template <> struct OpenNil<uint64_t> {
    static uint64_t get() { return 8888; }
};

// OpenDeleted for our uint64_t
template <> struct OpenDeleted<uint64_t> {
    static uint64_t get() { return 9999; }
};
} // namespace fo

using namespace fo;

static void BM_stdumap_hash_search(benchmark::State &bm_state) {

    std::unordered_map<uint64_t, uint64_t> h;

    const uint32_t max_entries = bm_state.range(0);

    h.reserve(max_entries);

    for (uint64_t i = 0; i < max_entries; ++i) {
        h[i] = 0xdeadbeeflu;
    }

    while (bm_state.KeepRunning()) {
        for (uint64_t i = 0; i < max_entries; ++i) {
            auto it = h.find(i);
            benchmark::DoNotOptimize(it);
            if (it == h.end()) {
                bm_state.SkipWithError("Should not happen");
            }
        }
    }
}

static void BM_open_hash_search(benchmark::State &bm_state) {
    memory_globals::init();
    {
        auto &alloc = memory_globals::default_allocator();

        using hash_type = OpenHash<uint64_t, uint64_t>;

        hash_type h{alloc, 16, [](const auto &i) { return i & 0xffffffffu; },
                    [](const auto &i, const auto &j) { return i == j; }};

        const uint32_t max_entries = bm_state.range(0);

        for (uint64_t i = 0; i < max_entries; ++i) {
            open_hash::set(h, i, 0xdeadbeeflu);
        }

        while (bm_state.KeepRunning()) {
            for (uint64_t i = 0; i < max_entries; ++i) {
                auto idx = open_hash::find(h, i);
                benchmark::DoNotOptimize(idx);
                if (idx == open_hash::NOT_FOUND) {
                    bm_state.SkipWithError("Should not happen");
                }
            }
        }
    }
    memory_globals::shutdown();
}

constexpr uint32_t max_entries = 4 << 20;

BENCHMARK(BM_stdumap_hash_search)->RangeMultiplier(2)->Range(1024, max_entries);
BENCHMARK(BM_open_hash_search)->RangeMultiplier(2)->Range(1024, max_entries);

BENCHMARK_MAIN();