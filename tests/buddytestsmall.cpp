#include "buddy_allocator.h"

#include <new>
#include <iostream>
#include <set>
#include <time.h>

constexpr uint32_t BUFFER_SIZE = 1 << 20;   // 1 MB
constexpr uint32_t SMALLEST_SIZE = 1 << 18; // 128 KB
constexpr uint32_t NUM_INDICES = BUFFER_SIZE / SMALLEST_SIZE;
constexpr uint32_t NUM_LEVELS = 3;

static_assert(NUM_INDICES == 4, " <--");

template <uint32_t bytes>
using Block = std::array<uint32_t, bytes / sizeof(uint32_t)>;

using SmallestBlock = Block<SMALLEST_SIZE>;

using Block_8KB = Block<8 << 10>;

int main() {
    foundation::memory_globals::init();
    {
        using BA = foundation::BuddyAllocator<BUFFER_SIZE, NUM_LEVELS>;
        BA ba(foundation::memory_globals::default_allocator());
        std::cout << "SIZE OF SMALLEST ARRAY = " << sizeof(SmallestBlock)
                  << std::endl;

        auto b0 = ba.allocate(SMALLEST_SIZE, SMALLEST_SIZE);
        auto b1 = ba.allocate(SMALLEST_SIZE, SMALLEST_SIZE);
        auto b2 = ba.allocate(SMALLEST_SIZE, SMALLEST_SIZE);
        auto b3 = ba.allocate(SMALLEST_SIZE, SMALLEST_SIZE);

        ba.deallocate(b2);
        ba.deallocate(b1);
        ba.deallocate(b0);
        ba.deallocate(b3);

        // dbg here
    }
    foundation::memory_globals::shutdown();
}
