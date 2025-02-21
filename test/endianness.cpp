#include "bitfilled/bits.hpp"
#include "bitfilled/macros.hpp"
#include <catch2/catch_test_macros.hpp>
#include <etl/unaligned_type.h>

struct bigend : public bitfilled::host<etl::be_uint16_t>
{
    BF_BITS(bool, 0) bit0;
};

struct littleend : public bitfilled::host<etl::le_uint16_t>
{
    BF_BITS(bool, 0) bit0;
};

TEST_CASE("endianness")
{
    bigend be;
    littleend le;
    be.bit0 = le.bit0 = 1;

    CHECK(be[0] == 0);
    CHECK(be[1] == 1);
    CHECK(be.value() == 1);

    CHECK(le[0] == 1);
    CHECK(le[1] == 0);
    CHECK(le.value() == 1);
}
