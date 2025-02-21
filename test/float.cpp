#include <cmath>
#include "bitfilled/bits.hpp"
#include "bitfilled/macros.hpp"
#include <catch2/catch_test_macros.hpp>

struct spfloat : public bitfilled::host<float>
{
    using superclass::operator=;

    BF_BITS(unsigned, 0, 22) fraction;
    BF_BITS(unsigned, 23, 30) exponent;
    BF_BITS(bool, 31) sign;

    bool is_nan() const { return (exponent == 0b1111'1111) and (fraction != 0); }
    bool is_inf() const { return (exponent == 0b1111'1111) and (fraction == 0); }
};

TEST_CASE("float nan")
{
    spfloat nan1{std::nanf("")};

    CHECK(nan1.is_nan());
    CHECK(std::isnan(nan1));
}

TEST_CASE("float infinity")
{
    spfloat inf{std::numeric_limits<float>::infinity()};

    CHECK(inf.is_inf());
    CHECK(std::isinf(inf));
}

TEST_CASE("float 0.15625")
{
    spfloat num{.fraction = 0b010'0000'0000'0000'0000'0000, .exponent = 0b01111100, .sign = 0};

    CHECK(num == 0.15625f);
}
