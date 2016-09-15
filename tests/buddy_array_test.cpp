#include <scaffold/buddy_allocator.h>
#include <scaffold/array.h>

#include <stdio.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

constexpr uint32_t BUFFER_SIZE = 64 << 10;          // 16 KB
constexpr uint32_t SMALLEST_SIZE = 8; // 8 bytes

template <size_t size>
struct alignas(16) Ob {
    char _arr[size];
};

TEST_CASE("Array with buddy allocation", "[array-buddy-test]") {
    using namespace foundation;

    memory_globals::init();
    {
        const size_t object_sz = 16;

        BuddyAllocator<BUFFER_SIZE, object_sz> ba(
            memory_globals::default_scratch_allocator());

        Array<Ob<object_sz>> a{ba};
        for (int i = 0; i < 1024; ++i) {
            fprintf(stderr, "Pushing %d\n", i);
            array::push_back(a, Ob<object_sz>{});
        }
        array::clear(a);
    }

    memory_globals::shutdown();
}
