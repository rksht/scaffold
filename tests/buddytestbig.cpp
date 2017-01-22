#include <scaffold/buddy_allocator.h>

#include <iostream>
#include <new>
#include <random>
#include <set>
#include <stdio.h>
#include <time.h>

using BA = fo::BuddyAllocator;

constexpr uint32_t BUFFER_SIZE = 16 << 20;  // 10 MB
constexpr uint32_t SMALLEST_SIZE = 4 << 10; // 4 KB
constexpr uint32_t NUM_INDICES = BUFFER_SIZE / SMALLEST_SIZE;
/*constexpr uint32_t NUM_LEVELS = log2_ceil(NUM_INDICES) + 1;*/

template <uint32_t bytes> using Block = std::array<uint32_t, bytes / sizeof(uint32_t)>;

using SmallestBlock = Block<SMALLEST_SIZE>;

template <typename BlockTy> constexpr uint64_t _align_of() { return alignof(BlockTy); }

using Block_8KB = Block<8 << 10>;

int main(int argc, char **argv) {
    uint64_t seed = argc >= 2 ? strtoull(argv[1], nullptr, 10) : 100;

    log_info("Seed used = %lu\n", seed);

    fo::memory_globals::init();
    {
        BA ba(BUFFER_SIZE, SMALLEST_SIZE, fo::memory_globals::default_allocator());
        printf("SIZE OF SMALLEST ARRAY = %zu, SMALLEST_SIZE = %u\n", sizeof(SmallestBlock), SMALLEST_SIZE);
        puts("Press enter to continue...");
        getchar();

        std::default_random_engine dre(seed);
        std::uniform_int_distribution<int> d(1, 10);

        std::set<void *> allocateds;

        for (int i = 0; i < 5000; ++i) {
            std::cerr << "ITER = " << i << "\n";

            if (i % 500 == 0) {
                // std::cout << ba.get_json_tree() << "\n--\n";
                // std::cin >> n;
            }

            if (d(dre) < 3) {
                SmallestBlock &p1 =
                    *((SmallestBlock *)ba.allocate(sizeof(SmallestBlock), _align_of<SmallestBlock>()));
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
                Block_8KB &p2 = *((Block_8KB *)ba.allocate(sizeof(Block_8KB), _align_of<Block_8KB>()));
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
    fo::memory_globals::shutdown();
    fprintf(stderr, "Seed = %lu\n", seed);
}
