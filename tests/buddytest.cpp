#include <scaffold/buddy_allocator.h>

#include <iostream>
#include <new>
#include <random>
#include <set>
#include <time.h>

constexpr uint32_t BUFFER_SIZE = 1 << 20;   // 1 MB
constexpr uint32_t SMALLEST_SIZE = 2 << 10; // 2 KB

template <uint32_t bytes>
using Block = std::array<uint32_t, bytes / sizeof(uint32_t)>;

using SmallestBlock = Block<SMALLEST_SIZE>;

using Block_8KB = Block<8 << 10>;

int main(int argc, char **argv) {
    uint64_t seed = argc >= 2 ? strtoull(argv[1], nullptr, 10) : 100;

    log_info("--Seed = %lu", seed);

    foundation::memory_globals::init();
    {
        using BA = foundation::BuddyAllocator<BUFFER_SIZE, SMALLEST_SIZE>;
        BA ba(foundation::memory_globals::default_allocator());
        std::cerr << "SIZE OF SMALLEST ARRAY = " << sizeof(SmallestBlock)
                  << std::endl;

        std::default_random_engine dre(seed);
        std::uniform_int_distribution<int> d(1, 10);

        std::set<void *> allocateds;

        for (int i = 0; i < 100; ++i) {
            std::cerr << "ITER = " << i << "\n";

            if (d(dre) < 3) {
                SmallestBlock &p1 = *((SmallestBlock *)ba.allocate(
                    sizeof(SmallestBlock), alignof(SmallestBlock)));
                new (&p1) SmallestBlock();

                allocateds.insert((void *)&p1);

                // ba.print_level_map();
            }

            int r = d(dre);
            if (3 <= r && r < 8) {
                for (void *p : allocateds) {
                    if (d(dre) > 4) {
                        ba.deallocate(p);
                        allocateds.erase(p);
                        // ba.print_level_map();
                        break;
                    }
                }
            }

            r = d(dre);

            if (r >= 8) {
                Block_8KB &p2 = *((Block_8KB *)ba.allocate(sizeof(Block_8KB),
                                                           alignof(Block_8KB)));
                new (&p2) Block_8KB();
                allocateds.insert((void *)&p2);
                // ba.print_level_map();
            }

            r = d(dre);

            if (3 <= r && r < 6) {
                for (void *p : allocateds) {
                    if (d(dre) > 4) {
                        ba.deallocate(p);
                        allocateds.erase(p);
                        // ba.print_level_map();
                        break;
                    }
                }
            }
        }
        for (void *p : allocateds) {
            std::cerr << "Have to deallocate extra buddy"
                      << "\n";
            ba.deallocate(p);
        }
    }
    foundation::memory_globals::shutdown();
    fprintf(stderr, "Seed = %lu\n", seed);
}
