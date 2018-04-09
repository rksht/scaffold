#include <scaffold/buddy_allocator.h>
#include <scaffold/timed_block.h>

#include <array>
#include <iostream>
#include <new>
#include <random>
#include <set>
#include <stdio.h>
#include <time.h>

using BA = fo::BuddyAllocator;

constexpr uint64_t BUFFER_SIZE = u64(4) << 30; // 4 GB
constexpr uint64_t SMALLEST_SIZE = 4 << 10;    // 4 KB
constexpr uint64_t NUM_INDICES = BUFFER_SIZE / SMALLEST_SIZE;
/*constexpr uint64_t NUM_LEVELS = log2_ceil(NUM_INDICES) + 1;*/

template <uint64_t bytes> using Block = std::array<uint64_t, bytes / sizeof(uint64_t)>;

using SmallestBlock = Block<SMALLEST_SIZE>;

template <typename BlockTy> constexpr uint64_t _align_of() { return alignof(BlockTy); }

using Block_8KB = Block<8 << 10>;

int main(int argc, char **argv) {
    uint64_t seed = argc >= 2 ? strtoull(argv[1], nullptr, 10) : 100;

    log_info("Seed used = %lu\n", seed);

    fo::memory_globals::init();
    {
        log_info("Initializing buddy allocator");
        BA ba(BUFFER_SIZE, SMALLEST_SIZE, fo::memory_globals::default_allocator());
        printf("Done initializing. SIZE OF SMALLEST ARRAY = %zu, SMALLEST_SIZE = %lu\n",
               sizeof(SmallestBlock),
               SMALLEST_SIZE);
        puts("Press enter to continue...");
        getchar();

        std::default_random_engine dre(seed);
        std::uniform_int_distribution<int> d(1, 10);

        std::vector<void *> allocateds;
        allocateds.reserve(BUFFER_SIZE / SMALLEST_SIZE + 1);

        while (true) {
            auto block = ba.allocate(sizeof(SmallestBlock), alignof(SmallestBlock));
            if (!block) {
                break;
            }
            allocateds.push_back(block);
        }

        printf("Could allocate %zu small blocks\n", allocateds.size());

        puts("Will deallocate now... press enter");
        getchar();

        for (void *p : allocateds) {
            std::cerr << "Have to deallocate extra buddy\n";
            ba.deallocate(p);
        }

        timedblock::print_record_table(stdout);
    }
    fo::memory_globals::shutdown();
    fprintf(stderr, "Seed = %lu\n", seed);
}
