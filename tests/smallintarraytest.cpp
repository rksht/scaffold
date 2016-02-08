#define CATCH_CONFIG_MAIN
#include "catch.hpp"
#include "smallintarray.h"

using foundation::SmallIntArray;

TEST_CASE("SmallIntArray working correctly", "[SmallIntArray_works]") {
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
    //smallints.print();

    SECTION("total size") {
        printf("Sizeof(smallints) = %zu\n", sizeof(smallints));
    }
}
