#define CATCH_CONFIG_MAIN
#include "catch.hpp"

TEST_CASE("SmallIntArray working correctly", "[SmallIntArray_works]") {
    SmallIntArray<5, 10000> smallints;
    smallints.set(90, 30);
    smallints.set(91, 31);
    smallints.set(89, 10);
    smallints.set(80, 10);

    SECTION("get back value ok") {
        REQUIRE(smallints.get(90) == 30);
        REQUIRE(smallints.get(91) == 31);
        REQUIRE(smallints.get(89) == 10);
        REQUIRE(smallints.get(80) == 10);
    }

    smallints.set(90, 0);
    smallints.set(91, 20);

    SECTION("can reset") {
        REQUIRE(smallints.get(90) == 0);
        REQUIRE(smallints.get(91) == 20);
        REQUIRE(smallints.get(89) == 10);
        REQUIRE(smallints.get(80) == 10);
    }

    SECTION("total size") {
        printf("Sizeof(smallints) = %zu\n", sizeof(smallints));
    }

    SmallIntArray<1, 1000> bits;
    bits.set(1, 0);
    bits.set(999, 1);
    bits.set(990, 1);

    SECTION("1 bit ints i.e bitsets") {
        REQUIRE(bits.get(1) == 0);
        REQUIRE(bits.get(999) == 1);
        REQUIRE(bits.get(990) == 1);
        REQUIRE(bits.get(1) == 0);
    }

    bits.set(999, 0);
    bits.set(990, 0);

    SECTION("resets ok") {
        REQUIRE(bits.get(990) == 0);
        REQUIRE(bits.get(999) == 0);
    }
}