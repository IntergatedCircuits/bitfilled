#include "bitfilled.hpp"
#include <catch2/catch_test_macros.hpp>

enum enumeration
{
    ENUMERATOR_0 = 0,
    ENUMERATOR_1 = 1,
    ENUMERATOR_2 = 2,
    ENUMERATOR_3 = 3,
};

struct legacy
{
    bool boolean : 1;
    enumeration enumerated : 2;
    std::int32_t integer : 5;
};

struct eightbits : public bitfilled::host_integer<std::uint8_t>
{
    BF_COPY_SUPERCLASS(eightbits);

    BF_BITS(bool, 0) boolean;
    BF_BITS(enumeration, 1, 2) enumerated;
    BF_BITS(std::int32_t, 3, 7) integer;

    BF_BITS(std::uint8_t, 0, 7) overlapping;

    BF_BITSET(std::int32_t, 2, 3, 1) signs;
};
static_assert(sizeof(eightbits::superclass) == sizeof(eightbits));

TEST_CASE("variable default construct")
{
    eightbits var;

    CHECK(var == 0);
    CHECK(var.overlapping == 0);
    CHECK(var.boolean == false);
    CHECK(var.enumerated == enumeration::ENUMERATOR_0);
    CHECK(var.integer == 0);

    CHECK(var.signs[0] == 0);
    CHECK(var.signs[1] == 0);
    CHECK(var.signs[2] == 0);
}

TEST_CASE("variable construct")
{
    eightbits var{0xff};

    CHECK(var == 0xff);
    CHECK(var.overlapping == var);
    CHECK(var.boolean == true);
    CHECK(var.enumerated == enumeration::ENUMERATOR_3);
    CHECK(var.integer == -1);

    CHECK(var.signs[0] == -1);
    CHECK(var.signs[1] == -1);
    CHECK(var.signs[2] == -1);
}

TEST_CASE("variable sign extend")
{
    eightbits var{0x0};

    var.integer = var.integer - 1;
    CHECK(var.integer == -1);
    var.integer = -16;
    CHECK(var.integer == -16);
    var.integer = var.integer - 1;
    CHECK(var.integer == 15);

    var = 0;
    var.signs[0] = var.signs[0] - 1;
    CHECK(var.signs[0] == -1);
    var.signs[0] = var.signs[0] - 1;
    CHECK(var.signs[0] == -2);
    var.signs[0] = var.signs[0] - 1;
    CHECK(var.signs[0] == 1);
    CHECK(var.signs[1] == 0);
    CHECK(var.signs[2] == 0);
}

TEST_CASE("variable assignment")
{
    eightbits var, var2;
    std::uint8_t raw{25};

    var2 = var = raw;
    CHECK(var == 25);
    CHECK(var2 == 25);

    var.signs[2] = -2;
    raw = var;
    CHECK(raw == 25 + 0x40);

    var.overlapping = 24;
    raw = var;
    CHECK(raw == 24);
}

TEST_CASE("variable reference")
{
    eightbits var;
    std::uint8_t& raw = var;
    CHECK((std::uintptr_t)&raw == (std::uintptr_t)&var);

    raw = 0xaa;
    CHECK(var == 0xaa);

    var = 0x55;
    CHECK(raw == 0x55);
}

TEST_CASE("variable field assignment")
{
    eightbits var1, var2;

#if BITFILLED_ASSIGN_RETURNS_REF
    var2.integer = var1.integer = 1;
#else
    var1.integer = 1;
    var2.integer = var1.integer;
#endif
    CHECK(var1 == (1 << var1.integer.offset()));
    CHECK(var2 == (1 << var2.integer.offset()));
    CHECK(var2.integer == var1.integer);
    var1.integer = 2;
    var2.integer = var1.integer;
    CHECK(var2.integer == var1.integer);
}

TEST_CASE("variable set assignment")
{
    eightbits var1{0}, var2{0xff};

    var2.signs[0] = -2;
    var2.signs[1] = -2;
    var2.signs[2] = -2;
    var1.signs = var2.signs;
    CHECK((var1 & 0x81) == 0);
    CHECK(var1.signs[0] == -2);
    CHECK(var1.signs[1] == -2);
    CHECK(var1.signs[2] == -2);
}
