#include "bitfilled.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;

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

struct eightbits : public host_integer<std::uint8_t>
{
    BF_COPY_SUPERCLASS(eightbits);

    BF_BITS(bool, 0) boolean;
    BF_BITS(enumeration, 1, 2) enumerated;
    BF_BITS(std::int32_t, 3, 7) integer;

    BF_BITS(std::uint8_t, 0, 7) overlapping;

    BF_BITSET(std::int32_t, 2, 3, 1) signs;
};
static_assert(sizeof(eightbits::superclass) == sizeof(eightbits));

suite variable_bits = []
{
    "variable default construct"_test = []
    {
        eightbits var;

        expect(that % var == 0);
        expect(that % var.overlapping == 0);
        expect(that % var.boolean == false);
        expect(that % var.enumerated == enumeration::ENUMERATOR_0);
        expect(that % var.integer == 0);

        expect(that % var.signs[0] == 0);
        expect(that % var.signs[1] == 0);
        expect(that % var.signs[2] == 0);
    };

    "variable construct"_test = []
    {
        eightbits var{0xff};

        expect(that % var == 0xff);
        expect(that % var.overlapping == var);
        expect(that % var.boolean == true);
        expect(that % var.enumerated == enumeration::ENUMERATOR_3);
        expect(that % var.integer == -1);

        expect(that % var.signs[0] == -1);
        expect(that % var.signs[1] == -1);
        expect(that % var.signs[2] == -1);
    };

    "variable sign extend"_test = []
    {
        eightbits var{0x0};

        var.integer = var.integer - 1;
        expect(that % var.integer == -1);
        var.integer = -16;
        expect(that % var.integer == -16);
        var.integer = var.integer - 1;
        expect(that % var.integer == 15);

        var = 0;
        var.signs[0] = var.signs[0] - 1;
        expect(that % var.signs[0] == -1);
        var.signs[0] = var.signs[0] - 1;
        expect(that % var.signs[0] == -2);
        var.signs[0] = var.signs[0] - 1;
        expect(that % var.signs[0] == 1);
        expect(that % var.signs[1] == 0);
        expect(that % var.signs[2] == 0);
    };

    "variable assignment"_test = []
    {
        eightbits var, var2;
        std::uint8_t raw{25};

        var2 = var = raw;
        expect(that % var == 25);
        expect(that % var2 == 25);

        var.signs[2] = -2;
        raw = var;
        expect(that % raw == 25 + 0x40);

        var.overlapping = 24;
        raw = var;
        expect(that % raw == 24);
    };

    "variable reference"_test = []
    {
        eightbits var;
        std::uint8_t& raw = var;
        expect((std::uintptr_t)&raw == (std::uintptr_t)&var);

        raw = 0xaa;
        expect(that % var == 0xaa);

        var = 0x55;
        expect(that % raw == 0x55);
    };

    "variable field assignment"_test = []
    {
        eightbits var1, var2;

#if BITFILLED_ASSIGN_RETURNS_REF
        var2.integer = var1.integer = 1;
#else
        var1.integer = 1;
        var2.integer = var1.integer;
#endif
        expect(var1 == (1 << var1.integer.offset()));
        expect(var2 == (1 << var2.integer.offset()));
        expect(var2.integer == var1.integer);
        var1.integer = 2;
        var2.integer = var1.integer;
        expect(var2.integer == var1.integer);
    };

    "variable set assignment"_test = []
    {
        eightbits var1{0}, var2{0xff};

        var2.signs[0] = -2;
        var2.signs[1] = -2;
        var2.signs[2] = -2;
        var1.signs = var2.signs;
        expect(that % (var1 & 0x81) == 0);
        expect(that % var1.signs[0] == -2);
        expect(that % var1.signs[1] == -2);
        expect(that % var1.signs[2] == -2);
    };
};
