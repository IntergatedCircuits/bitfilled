#include <bit>
#include "bitfilled.hpp"
#include <boost/ut.hpp>
#include <type_traits>

using namespace bitfilled;
using namespace boost::ut;

enum enumeration
{
    ENUMERATOR_0 = 0,
    ENUMERATOR_1 = 1,
    ENUMERATOR_2 = 2,
    ENUMERATOR_3 = 3,
};

struct memory_mapped_reg : BF_MMREG(std::uint8_t, rw)
{
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
    using bf_ops = base_type::bf_ops;

    // BF_MMREGBITS(bool, r, 0)  d;

    using boolean_t = bitfilled::regbitfield<bool, bf_ops, ACCESS, 0>;
    [[no_unique_address]] boolean_t boolean;

    using enumeration_t = bitfilled::regbitfield<enumeration, bf_ops, ACCESS, 1, 2>;
    [[no_unique_address]] enumeration_t enumerated;

    using integer_t = bitfilled::regbitfield<std::int32_t, bf_ops, ACCESS, 3, 7>;
    [[no_unique_address]] integer_t integer;
};
static_assert(sizeof(mmr<access::rw>) == sizeof(std::uint8_t));

suite mmreg = []
{
    "mmregs assignment"_test = []
    {
        std::uint8_t v[static_cast<unsigned>(access::rw) + 1]{0, 42, 0, 0};
        volatile mmr<access::rw> rw1;
        auto& rw2 =
            reinterpret_cast<volatile mmr<access::rw>&>(v[static_cast<unsigned>(access::rw)]);
        auto& ro = reinterpret_cast<volatile mmr<access::r>&>(v[static_cast<unsigned>(access::r)]);
        auto& wo = reinterpret_cast<volatile mmr<access::w>&>(v[static_cast<unsigned>(access::w)]);

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
        std::uint8_t v[static_cast<unsigned>(access::rw) + 1]{};
        volatile mmr<access::rw> rw1;
        auto& rw2 =
            reinterpret_cast<volatile mmr<access::rw>&>(v[static_cast<unsigned>(access::rw)]);
        auto& ro = reinterpret_cast<volatile mmr<access::r>&>(v[static_cast<unsigned>(access::r)]);
        auto& wo = reinterpret_cast<volatile mmr<access::w>&>(v[static_cast<unsigned>(access::w)]);

        v[static_cast<unsigned>(access::r)] = 3 << mmr<access::rw>::integer_t::offset();

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
};
