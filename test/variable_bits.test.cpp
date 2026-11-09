#include "bitfilled.hpp"
#include "field_traits.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;
using bitfilled_test::value_of;

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

    BF_BITSET(std::int32_t, 2, 3, 1) signs BF_BITSET_POSTFIX;
};
static_assert(sizeof(eightbits::superclass) == sizeof(eightbits));

// an aggregate owner of bit fields
struct aggregate32 : public host_integer<std::uint32_t>
{
    BF_OWNER(aggregate32)

    BF_BITS(std::uint32_t, 0, 31) all;
    BF_BITS(std::int8_t, 24, 31) top;
    BF_BITSET(bool, 1, 16, 8) flags BF_BITSET_POSTFIX;
};

// bit fields without an owner
struct ownerless : public host_integer<std::uint16_t>
{
    BF_BITS(unsigned, 0, 3) nibble;
};

// construction and copying of the owners never executes bit field code
static_assert(bitfilled_test::passive_owner<eightbits>);
static_assert(bitfilled_test::passive_owner<aggregate32>);
static_assert(not std::is_aggregate_v<eightbits>);
// if any bit field constructor ran bit field operations (which aren't constexpr),
// these wouldn't be constant expressions
static_assert(eightbits{0x5a} == 0x5a);
static_assert(
    []
    {
        const eightbits original{0xa5};
        const eightbits copy{original}; // NOLINT(performance-unnecessary-copy-initialization)
        return copy == 0xa5;
    }());
static_assert(
    []
    {
        aggregate32 value{};
        const aggregate32 copy = value;
        return copy == 0;
    }());

#ifndef _MSC_VER
static_assert(std::is_aggregate_v<aggregate32>);
// each bit field is a stateless descriptor, that can't be detached from its owner
static_assert(bitfilled_test::stateless_field<decltype(eightbits::boolean)>);
static_assert(bitfilled_test::stateless_field<decltype(eightbits::enumerated)>);
static_assert(bitfilled_test::stateless_field<decltype(eightbits::integer)>);
static_assert(bitfilled_test::stateless_field<decltype(eightbits::overlapping)>);
static_assert(bitfilled_test::stateless_field<decltype(eightbits::signs)>);
static_assert(bitfilled_test::stateless_field<decltype(aggregate32::all)>);
static_assert(bitfilled_test::stateless_field<decltype(aggregate32::flags)>);
static_assert(bitfilled_test::stateless_field<decltype(ownerless::nibble)>);
// without an owner, the containing type can't be copy constructed
static_assert(not std::is_copy_constructible_v<ownerless>);
static_assert(BF_TEST_VALID(ownerless, self.nibble = 1U));

// bit field values are only accessible as values
static_assert(BF_TEST_VALID(eightbits, static_cast<int>(self.integer)));
static_assert(BF_TEST_VALID(eightbits, self.integer = self.integer));
static_assert(BF_TEST_VALID(const eightbits, static_cast<int>(self.integer)));
static_assert(not BF_TEST_VALID(const eightbits, self.integer = 1));
static_assert(not BF_TEST_VALID(eightbits,
                                self.integer = std::move(std::declval<eightbits&>().integer)));
static_assert(not BF_TEST_VALID(eightbits,
                                std::remove_cvref_t<decltype(self.integer)>{self.integer}));
static_assert(not BF_TEST_VALID(const eightbits,
                                std::remove_cvref_t<decltype(self.integer)>{self.integer}));
static_assert(BF_TEST_VALID(eightbits, self.signs[1] = 1));
static_assert(BF_TEST_VALID(eightbits, self.signs = self.signs));
static_assert(not BF_TEST_VALID(const eightbits, self.signs[1] = 1));
static_assert(not BF_TEST_VALID(eightbits, std::remove_cvref_t<decltype(self.signs)>{self.signs}));

// invalid bit field definitions are rejected
template <typename T, std::size_t FIRST_BIT, std::size_t LAST_BIT>
concept valid_bitfield8 =
    requires { typename bitfield<T, host_integer<std::uint8_t>::bf_ops, FIRST_BIT, LAST_BIT>; };
template <typename T, std::size_t SIZE, std::size_t COUNT, std::size_t OFFSET>
concept valid_bitfieldset8 =
    requires { typename bitfieldset<T, host_integer<std::uint8_t>::bf_ops, SIZE, COUNT, OFFSET>; };
static_assert(valid_bitfield8<std::int8_t, 0, 7>);
static_assert(valid_bitfield8<enumeration, 6, 7>);
static_assert(not valid_bitfield8<int, 4, 8>, "exceeds storage");
static_assert(not valid_bitfield8<bool, 0, 1>, "exceeds value type");
static_assert(not valid_bitfield8<std::int8_t, 3, 2>, "reversed range");
static_assert(not valid_bitfield8<float, 0, 1>, "not integral");
static_assert(not valid_bitfield8<const int, 0, 1>, "cv-qualified");
static_assert(valid_bitfieldset8<bool, 1, 8, 0>);
static_assert(not valid_bitfieldset8<bool, 1, 8, 1>, "exceeds storage");
static_assert(not valid_bitfieldset8<bool, 2, 2, 0>, "exceeds value type");
static_assert(not valid_bitfieldset8<int, 1, 0, 0>, "empty");
#endif

const suite<"variable_bits"> variable_bits = []
{
    "variable default construct"_test = []
    {
        eightbits var;

        expect(that % var == 0);
        expect(that % value_of(var.overlapping) == 0);
        expect(that % value_of(var.boolean) == false);
        expect(that % value_of(var.enumerated) == enumeration::ENUMERATOR_0);
        expect(that % value_of(var.integer) == 0);

        expect(that % var.signs[0] == 0);
        expect(that % var.signs[1] == 0);
        expect(that % var.signs[2] == 0);
    };

    "variable construct"_test = []
    {
        eightbits var{0xff};

        expect(that % var == 0xff);
        expect(that % value_of(var.overlapping) == var);
        expect(that % value_of(var.boolean) == true);
        expect(that % value_of(var.enumerated) == enumeration::ENUMERATOR_3);
        expect(that % value_of(var.integer) == -1);

        expect(that % var.signs[0] == -1);
        expect(that % var.signs[1] == -1);
        expect(that % var.signs[2] == -1);
    };

    "variable sign extend"_test = []
    {
        eightbits var{0x0};

        var.integer = var.integer - 1;
        expect(that % value_of(var.integer) == -1);
        var.integer = -16;
        expect(that % value_of(var.integer) == -16);
        var.integer = var.integer - 1;
        expect(that % value_of(var.integer) == 15);

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
#ifndef _MSC_VER
        static_assert(var1.integer.offset() == 3);
#endif
        expect(var1 == (1 << 3));
        expect(var2 == (1 << 3));
        expect(value_of(var2.integer) == value_of(var1.integer));
        var1.integer = 2;
        var2.integer = var1.integer;
        expect(value_of(var2.integer) == value_of(var1.integer));
    };
#ifndef _MSC_VER
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
#endif

    "aggregate full width field"_test = []
    {
        aggregate32 var{};
        var.all = 0xdeadbeef;
        expect(that % var == 0xdeadbeef);
        expect(that % value_of(var.all) == 0xdeadbeef);
        expect(that % value_of(var.top) == static_cast<std::int8_t>(0xde));
        var.top = -1;
        expect(that % var == 0xffadbeef);

        const aggregate32 copy = var;
        expect(that % copy == 0xffadbeef);
    };
#ifndef _MSC_VER
    "aggregate bool set copy"_test = []
    {
        aggregate32 var1{}, var2{};
        var2.flags[3] = true;
        var2.flags[15] = true;
        var1.flags = var2.flags;
        expect(that % var1 == ((1U << 11) | (1U << 23)));
        expect(that % value_of(var1.flags[3]) == true);
        expect(that % value_of(var1.flags[4]) == false);
        expect(that % value_of(var1.flags[15]) == true);
    };
#endif
};
