#include "bitfilled/size.hpp"
#include <catch2/catch_test_macros.hpp>

using namespace bitfilled;

TEST_CASE("sizes")
{
    CHECK(byte_width(255u) == 1);
    CHECK(byte_width(256u) == 2);
    CHECK(byte_width(0) == 1);
    CHECK(byte_width(-128) == 1);
    CHECK(byte_width(-129) == 2);
}
