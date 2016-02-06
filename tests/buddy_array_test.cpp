#include "buddy.h"
#include "array.h"
#include "hash.h"
#include <stdio.h>
#include <random>

struct ObjectA {
    uint64_t a, b, c, d;

    ObjectA(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        : a(a), b(b), c(c), d(d) {}
};

struct ObjectB {
    uint64_t a, b, c, d, e, f, g, h;

    ObjectB(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
        : a(a), b(b), c(c), d(d), e(1), f(1), g(1), h(1) {}
};

constexpr uint32_t BUFFER_SIZE = 16 << 10;          // 16 KB
constexpr uint32_t SMALLEST_SIZE = sizeof(ObjectA); // 4 B
constexpr uint32_t NUM_INDICES = BUFFER_SIZE / SMALLEST_SIZE;
constexpr uint32_t NUM_LEVELS = log2_ceil(NUM_INDICES) + 1; // 13

int main() {
    using namespace foundation;

    std::uniform_int_distribution<int> d(1, 10);
    std::default_random_engine dre;

    memory_globals::init();
    {
        BuddyAllocator<BUFFER_SIZE, NUM_LEVELS> ba(
            memory_globals::default_scratch_allocator());

        printf("Num indices = %u\n", NUM_INDICES);

        Array<ObjectA> arrA(ba);
        Array<ObjectB> arrB(ba);

        Hash<uint32_t> idx_of_memory(memory_globals::default_scratch_allocator());

        char in[1024] = {0};

        for (uint32_t i = 0; i < 10000; ++i) {
            printf("Total allocated = %u\n", ba.total_allocated());

            fgets(in, 1024, stdin);
            in[strlen(in) - 1] = '\0';
        }
    }

    memory_globals::shutdown();
}
