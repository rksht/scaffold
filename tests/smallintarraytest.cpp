#define CATCH_CONFIG_MAIN
#include "smallintarray.h"
#include "catch.hpp"

using foundation::SmallIntArray;

TEST_CASE("SmallIntArray working correctly", "[SmallIntArray_works]") {
    foundation::memory_globals::init();
    SmallIntArray<4, 512> smallints;
    smallints.set(0, 9);
    smallints.set(90, 12);
    smallints.set(91, 9);
    smallints.set(89, 10);
    smallints.set(80, 10);

    SECTION("get back value ok") {
        REQUIRE(smallints.get(0) == 9);
        REQUIRE(smallints.get(90) == 12);
        REQUIRE(smallints.get(91) == 9);
        REQUIRE(smallints.get(89) == 10);
        REQUIRE(smallints.get(80) == 10);
    }

    smallints.set(90, 0);
    smallints.set(91, 8);
    smallints.set(0, 7);

    SECTION("can reset") {
        REQUIRE(smallints.get(90) == 0);
        REQUIRE(smallints.get(91) == 8);
        REQUIRE(smallints.get(89) == 10);
        REQUIRE(smallints.get(80) == 10);
        REQUIRE(smallints.get(0) == 7);
    }

    SmallIntArray<1, 1000> bits;
    bits.set(1, 0);
    bits.set(500, 1);
    bits.set(501, 1);

    SECTION("1 bit ints i.e bitsets") {
        REQUIRE(bits.get(1) == 0);
        REQUIRE(bits.get(500) == 1);
        REQUIRE(bits.get(501) == 1);
        REQUIRE(bits.get(1) == 0);
    }

    bits.set(500, 0);
    bits.set(501, 0);

    SECTION("resets ok") {
        REQUIRE(bits.get(501) == 0);
        REQUIRE(bits.get(500) == 0);
    }
    // smallints.print();

    SECTION("total size") {
        printf("Sizeof(smallints) = %zu\n", sizeof(smallints));
    }

    SECTION("iterator") {
        const int num_ints = 9990;
        SmallIntArray<4, num_ints> ints;
        int loop_count = 0;
        for (auto i = ints.cbegin(), e = ints.cend(); i != e; ++i) {
            ++loop_count;
        }
        REQUIRE(loop_count == num_ints);
    }

    SECTION("smallintarray range set") {
        SmallIntArray<4, 9990> ints;
        ints.set_range(100, 1000, 9);
        ints.set_range(1000, 2000, 8);
        for (int i = 100; i < 1000; ++i) {
            REQUIRE(ints.get(i) == 9);
        }
        for (int i = 1000; i < 2000; ++i) {
            REQUIRE(ints.get(i) == 8);
        }
        ints.set_range(101, 102, 7);
        for (int i = 101; i < 102; ++i) {
            REQUIRE(ints.get(i) == 7);
        }
        REQUIRE(ints.get(100) == 9);
        for (int i = 102; i < 1000; ++i) {
            REQUIRE(ints.get(i) == 9);
        }
        ints.print();
    }
    foundation::memory_globals::shutdown();
}
