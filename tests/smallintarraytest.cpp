#define CATCH_CONFIG_MAIN
#include "smallintarray.h"
#include "catch.hpp"
#include <map>
#include <stdlib.h> // rand()

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

    {
        SmallIntArray<1, 1000> bits;
        std::map<int, bool> is_set;

        srand(0xbeef);

        SECTION("Use as a bitset") {
            for (int i = 0; i < 1000; ++i) {
                if (rand() % 1000 < 500) {
                    bits.set(i, 1);
                    is_set[i] = true;
                } else {
                    is_set[i] = false;
                }
            }

            for (auto i = is_set.cbegin(), e = is_set.cend(); i != e; ++i) {
                if (i->second) {
                    REQUIRE(bits.get(i->first) == 1);
                } else {
                    REQUIRE(bits.get(i->first) == 0);
                }
            }
        }

        SECTION("resets ok") {
            bits.set(500, 0);
            bits.set(501, 0);
            REQUIRE(bits.get(501) == 0);
            REQUIRE(bits.get(500) == 0);
        }
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
#if 0
        ints.print();
#endif
    }
    foundation::memory_globals::shutdown();
}
