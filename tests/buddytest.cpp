#include "buddy.h"
#define CATCH_CONFIG_MAIN
// #include "catch.hpp"

#include <new>
#include <iostream>

constexpr uint32_t BUFFER_SIZE = 1 << 20;   // 1 MB
constexpr uint32_t SMALLEST_SIZE = 2 << 10; // 2 KB
constexpr uint32_t NUM_INDICES = BUFFER_SIZE / SMALLEST_SIZE;
constexpr uint32_t NUM_LEVELS = log2_ceil(NUM_INDICES) + 1;

template <uint32_t bytes>
using block = std::array<uint32_t, SMALLEST_SIZE / sizeof(uint32_t)>;

using smallest_block = block<SMALLEST_SIZE>;

using block_8KB = block<BUFFER_SIZE / (1 << 13)>;

template <typename RandomIter> void fill_ints(RandomIter beg, RandomIter end) {
    int x = 0;
    for (auto i = beg; i != end; i++, x++) {
        *i = x;
    }
}

#if 0
TEST_CASE("ALLOCATE SMALLEST AMOUNTS", "[allocate_smallest_buddies]") {
    foundation::memory_globals::init();
    {
        SECTION("Initialized OK and then deleted OK") {
            foundation::BuddyAllocator<1 << 20, 10> ba(
                foundation::memory_globals::default_allocator());
        }
    }
    foundation::memory_globals::shutdown();
}
#endif

int main() {
    foundation::memory_globals::init();
    {
        using BA = foundation::BuddyAllocator<1 << 20, 10>;
        BA ba(foundation::memory_globals::default_allocator());
        std::cout << "SIZE OF SMALLEST ARRAY = " << sizeof(smallest_block)
                  << std::endl;

        smallest_block &p1 = *((smallest_block *)ba.allocate(
            sizeof(smallest_block), alignof(smallest_block)));
        new (&p1) smallest_block();
        fill_ints(p1.begin(), p1.end());

        block_8KB &p2 = *((block_8KB *)ba.allocate(sizeof(block_8KB), alignof(block_8KB)));
        new(&p2) block_8KB();
        fill_ints(p1.begin(), p2.end());

        ba.deallocate(&p1);
        ba.deallocate(&p2);
    }
    foundation::memory_globals::shutdown();
}
