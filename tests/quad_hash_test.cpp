#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include <scaffold/quad_hash.h>

namespace foundation {

// QuadNil for our uint64_t
template <> struct QuadNil<uint64_t> {
    static uint64_t get() { return 1024; }
};

// QuadDeleted for our uint64_t
template <> struct QuadDeleted<uint64_t> {
    static uint64_t get() { return 1025; }
};
}

TEST_CASE("QuadHash find", "[QuadHash_find]") {
    foundation::memory_globals::init();

    auto &alloc = foundation::memory_globals::default_allocator();

    using hash_type = foundation::QuadHash<uint64_t, uint64_t>;

    namespace quad_hash = foundation::quad_hash;

    {
        hash_type h{alloc, 16, [](const auto &i) { return i & 0xffffffffu; },
                    [](const auto &i, const auto &j) { return i == j; }};

        for (uint64_t k = 0; k < 1024; ++k) {
            for (uint64_t prev_k = 0; prev_k < k; ++prev_k) {
                auto idx = quad_hash::find(h, prev_k);
                REQUIRE(idx != quad_hash::NOT_FOUND);
                REQUIRE(h._values[idx] == 1024 - prev_k);
            }
            quad_hash::set(h, k, 1024 - k);
        }
    }

    foundation::memory_globals::shutdown();
}
