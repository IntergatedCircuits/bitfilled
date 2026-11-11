#include <bit>
#include "bitfilled.hpp"
#include "field_traits.hpp"
#include <boost/ut.hpp>
#include <type_traits>

using namespace bitfilled;
using namespace boost::ut;

enum enumeration : std::uint8_t
{
    ENUMERATOR_0 = 0,
    ENUMERATOR_1 = 1,
    ENUMERATOR_2 = 2,
    ENUMERATOR_3 = 3,
};

struct memory_mapped_reg : BF_MMREG(std::uint8_t, rw)
{
    BF_OWNER(memory_mapped_reg)
    using superclass::operator=;

    BF_MMREGBITS(bool, r, 0) boolean;
    BF_MMREGBITS(std::endian, rw, 1, 2) enumerated;
    BF_MMREGBITS(std::int32_t, w, 3, 7) integer;
};
static_assert(sizeof(memory_mapped_reg) == sizeof(std::uint8_t));

template <enum bitfilled::access ACCESS>
struct mmr : public bitfilled::mmreg<std::uint8_t, ACCESS>
{
    using base_type = bitfilled::mmreg<std::uint8_t, ACCESS>;
    using base_type::operator=;
    BF_TEMPLATE_OWNER(mmr, base_type)

    // BF_MMREGBITS(bool, r, 0)  d;

    using boolean_t = bitfilled::regbitfield<bool, bf_ops, ACCESS, 0>;
    [[no_unique_address]] boolean_t boolean;

    using enumeration_t = bitfilled::regbitfield<enumeration, bf_ops, ACCESS, 1, 2>;
    [[no_unique_address]] enumeration_t enumerated;

    using integer_t = bitfilled::regbitfield<std::int32_t, bf_ops, ACCESS, 3, 7>;
    [[no_unique_address]] integer_t integer;
};
static_assert(sizeof(mmr<access::rw>) == sizeof(std::uint8_t));

// register definitions in the style of the svd2mmregmap.py generator
struct rw_reg : BF_MMREG(std::uint32_t, rw)
{
    BF_COPY_SUPERCLASS(rw_reg)
    BF_MMREGBITS(bool, r, 16) COUNTFLAG;
    BF_MMREGBITS(bool, rw, 0) ENABLE;
    BF_MMREGBITS(std::uint32_t, rw, 4, 7) MODE;
    BF_MMREGBITSET(bool, rw, 1, 4, 8) LINES;
};
struct r_reg : BF_MMREG(std::uint32_t, r)
{
    BF_COPY_SUPERCLASS(r_reg)
    BF_MMREGBITS(bool, r, 31) NOREF;
    BF_MMREGBITS(std::uint32_t, rw, 0, 23) TENMS; // declared rw, but the register is read-only
    BF_MMREGBITSET(bool, r, 1, 4, 24) STATES;
};
struct w_reg : BF_MMREG(std::uint32_t, w)
{
    BF_COPY_SUPERCLASS(w_reg)
    BF_MMREGBITS(std::uint32_t, w, 0, 15) DATA;
    BF_MMREGBITSET(bool, w, 1, 4, 16) SET;
};

// construction and copying never executes bit field code
static_assert(bitfilled_test::passive_owner<memory_mapped_reg>);
static_assert(bitfilled_test::passive_owner<mmr<access::rw>>);
static_assert(bitfilled_test::passive_owner<rw_reg>);
static_assert(
    []
    {
        const rw_reg original{0x10003};
        const rw_reg copy{original}; // NOLINT(performance-unnecessary-copy-initialization)
        return copy == 0x10003;
    }());
// read-only and write-only registers can't be copied (regardless of their fields)
static_assert(sizeof(r_reg) == sizeof(std::uint32_t) and not std::is_copy_constructible_v<r_reg>);
static_assert(sizeof(w_reg) == sizeof(std::uint32_t) and not std::is_copy_constructible_v<w_reg>);

static_assert(bitfilled_test::stateless_field<decltype(rw_reg::COUNTFLAG)>);
static_assert(bitfilled_test::stateless_field<decltype(rw_reg::LINES)>);
static_assert(bitfilled_test::stateless_field<decltype(r_reg::STATES)>);
static_assert(bitfilled_test::stateless_field<decltype(w_reg::SET)>);
static_assert(bitfilled_test::stateless_field<mmr<access::r>::integer_t>);

// field access is limited by both the field's and the register's access rights
static_assert(decltype(rw_reg::COUNTFLAG)::access() == access::r);
static_assert(decltype(r_reg::TENMS)::access() == access::r);
static_assert(decltype(w_reg::DATA)::access() == access::w);
static_assert(BF_TEST_VALID(volatile rw_reg, static_cast<bool>(self.COUNTFLAG)));
static_assert(not BF_TEST_VALID(volatile rw_reg, self.COUNTFLAG = true));
static_assert(not BF_TEST_VALID(volatile rw_reg, self.COUNTFLAG = self.COUNTFLAG));
static_assert(BF_TEST_VALID(volatile rw_reg, self.ENABLE = self.ENABLE));
static_assert(BF_TEST_VALID(volatile rw_reg, self.LINES[2] = true));
static_assert(BF_TEST_VALID(volatile rw_reg, static_cast<bool>(self.LINES[2])));
static_assert(BF_TEST_VALID(volatile r_reg, static_cast<std::uint32_t>(self.TENMS)));
static_assert(not BF_TEST_VALID(volatile r_reg, self.TENMS = 1U));
static_assert(BF_TEST_VALID(volatile r_reg, static_cast<bool>(self.STATES[1])));
static_assert(not BF_TEST_VALID(volatile r_reg, self.STATES[1] = true));
static_assert(not BF_TEST_VALID(volatile r_reg, self = 1U));
static_assert(BF_TEST_VALID(volatile w_reg, self.DATA = 1U));
static_assert(not BF_TEST_VALID(volatile w_reg, static_cast<std::uint32_t>(self.DATA)));
static_assert(BF_TEST_VALID(volatile w_reg, self.SET[1] = true));
static_assert(not BF_TEST_VALID(volatile w_reg, static_cast<bool>(self.SET[1])));
static_assert(not BF_TEST_VALID(volatile w_reg, self.SET[1] = self.SET[2]));
// bit fields of volatile registers can't be copied out either
static_assert(not BF_TEST_VALID(volatile rw_reg,
                                std::remove_cvref_t<decltype(self.ENABLE)>{self.ENABLE}));

const suite<"mmreg"> mmreg = []
{
    "mmregs assignment"_test = []
    {
        std::uint8_t values[static_cast<unsigned>(access::rw) + 1]{0, 42, 0, 0};
        volatile mmr<access::rw> rw1;
        auto& rw2 =
            reinterpret_cast<volatile mmr<access::rw>&>(values[static_cast<unsigned>(access::rw)]);
        auto& ro =
            reinterpret_cast<volatile mmr<access::r>&>(values[static_cast<unsigned>(access::r)]);
        auto& wo =
            reinterpret_cast<volatile mmr<access::w>&>(values[static_cast<unsigned>(access::w)]);

#if BITFILLED_ASSIGN_RETURNS_REF
        wo = rw1 = rw2 = ro;
#else
        rw2 = ro;
        rw1 = rw2;
        wo = rw1;
#endif
        expect(ro == 42);
        expect(rw2 == 42);
        expect(rw1 == 42);
    };

    "mmregs field assignment"_test = []
    {
        std::uint8_t values[static_cast<unsigned>(access::rw) + 1]{};
        volatile mmr<access::rw> rw1;
        auto& rw2 =
            reinterpret_cast<volatile mmr<access::rw>&>(values[static_cast<unsigned>(access::rw)]);
        auto& ro =
            reinterpret_cast<volatile mmr<access::r>&>(values[static_cast<unsigned>(access::r)]);
        auto& wo =
            reinterpret_cast<volatile mmr<access::w>&>(values[static_cast<unsigned>(access::w)]);

        values[static_cast<unsigned>(access::r)] = 3 << mmr<access::rw>::integer_t::offset();

        int integer = ro.integer;
        expect(ro.integer == 3);
        expect(integer == 3);
        wo.integer = integer;

#if BITFILLED_ASSIGN_RETURNS_REF
        wo.integer = rw1.integer = rw2.integer = ro.integer;
#else
        rw2.integer = ro.integer;
        rw1.integer = rw2.integer;
        wo.integer = rw1.integer;
#endif
        expect(ro == (3 << ro.integer.offset()));
        expect(rw1 == (3 << rw1.integer.offset()));
        expect(rw2 == (3 << rw2.integer.offset()));
    };

    "mmregs reference"_test = []
    {
        std::uint8_t v[static_cast<unsigned>(access::rw) + 1]{};
        volatile mmr<access::rw> rw1;
        auto& rw2 =
            reinterpret_cast<volatile mmr<access::rw>&>(v[static_cast<unsigned>(access::rw)]);
        auto& ro = reinterpret_cast<volatile mmr<access::r>&>(v[static_cast<unsigned>(access::r)]);
        auto& wo = reinterpret_cast<volatile mmr<access::w>&>(v[static_cast<unsigned>(access::w)]);
        volatile std::uint8_t& raw_rw1 = rw1;
        volatile std::uint8_t& raw_rw2 = rw2;
        const volatile std::uint8_t& raw_ro = ro;

        expect((std::uintptr_t)&raw_rw1 == (std::uintptr_t)&rw1);
        expect((std::uintptr_t)&raw_rw2 == (std::uintptr_t)&rw2);
        expect((std::uintptr_t)&raw_ro == (std::uintptr_t)&ro);
        raw_rw1 = 0xaa;
        expect(rw1 == 0xaa);
        wo = 0xaa;
        wo = ro;
    };

    "mmregs bit field sets"_test = []
    {
        std::uint32_t values[3]{0x05000000, 0, 0};
        auto& ro = reinterpret_cast<volatile r_reg&>(values[0]);
        auto& wo = reinterpret_cast<volatile w_reg&>(values[1]);
        auto& rw = reinterpret_cast<volatile rw_reg&>(values[2]);

        expect(that % bitfilled_test::value_of(ro.STATES[0]) == true);
        expect(that % bitfilled_test::value_of(ro.STATES[1]) == false);
        expect(that % bitfilled_test::value_of(ro.STATES[2]) == true);

        // write-only: the other bits are written as zero, without reading the register first
        values[1] = 0xffffffff;
        wo.SET[2] = true;
        expect(that % values[1] == (1U << 18));

        rw.LINES[1] = true;
        rw.LINES[3] = true;
        rw.ENABLE = true;
        expect(that % values[2] == ((1U << 9) | (1U << 11) | 1U));
        rw.LINES[1] = false;
        expect(that % bitfilled_test::value_of(rw.LINES[1]) == false);
        expect(that % values[2] == ((1U << 11) | 1U));
    };
};
