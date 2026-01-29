#include "bitfilled/integer.hpp"
#include "bitfilled/bits.hpp"
#include "bitfilled/macros.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;

template <std::endian ENDIAN>
struct endian_type
{
    static constexpr auto endianness = ENDIAN;
};

template <std::endian ENDIAN, std::size_t SIZE, typename T = sized_unsigned_t<std::bit_ceil(SIZE)>>
struct packed_integer_with_bfs : packed_integer<ENDIAN, SIZE, T>
{
    using base_type = packed_integer<ENDIAN, SIZE, T>;
    using base_type::operator=;
#ifdef _MSC_VER
    using base_type::_get_field;
    using base_type::_set_field;
#endif
    using bf_ops = base_type::bf_ops;
    BF_BITS(unsigned, 0, 15) halfword;
};

const suite integer = []
{
    "integer_storage"_test = []<class TestType>
    {
        const uint8_t u8 = 42;
        const integer_storage<1> us1{u8, TestType::endianness};
        expect(us1.to_integral<uint8_t>(TestType::endianness) == u8);

        const integer_storage<2> us2{u8, TestType::endianness};
        expect(us2.to_integral<uint8_t>(TestType::endianness) == u8);

        const uint32_t u32 = 0x123456;
        const integer_storage<3> us3{u32, TestType::endianness};
        expect(us3.to_integral<uint32_t>(TestType::endianness) == u32);

        const integer_storage<7> us7{u32, TestType::endianness};
        expect(us7.to_integral<uint32_t>(TestType::endianness) == u32);

        const int8_t i8 = -42;
        const integer_storage<1> s1{i8, TestType::endianness};
        expect(s1.to_integral<int8_t>(TestType::endianness) == i8);

        const integer_storage<2> s2{i8, TestType::endianness};
        expect(s1.to_integral<int8_t>(TestType::endianness) == i8);

        const int32_t i32 = -876543;
        const integer_storage<3> s3{i32, TestType::endianness};
        expect(s3.to_integral<int32_t>(TestType::endianness) == i32);

        const integer_storage<7> s7{i32, TestType::endianness};
        expect(s7.to_integral<int32_t>(TestType::endianness) == i32);

#if 1
        constexpr packed_integer<TestType::endianness, 1, uint8_t> pus1{u8};
        static_assert(pus1 == u8);
        constexpr packed_integer<TestType::endianness, 2, uint8_t> pus2{u8};
        static_assert(pus2 == u8);
        constexpr packed_integer<TestType::endianness, 3, uint32_t> pus3{u32};
        static_assert(pus3 == u32);
        constexpr packed_integer<TestType::endianness, 7, uint32_t> pus7{u32};
        static_assert(pus7 == u32);
        constexpr packed_integer<TestType::endianness, 1, int8_t> ps1{i8};
        static_assert(ps1 == i8);
        constexpr packed_integer<TestType::endianness, 2, int8_t> ps2{i8};
        static_assert(ps2 == i8);
        constexpr packed_integer<TestType::endianness, 3, int32_t> ps3{i32};
        static_assert(ps3 == i32);
        constexpr packed_integer<TestType::endianness, 7, int32_t> ps7{i32};
        static_assert(ps7 == i32);

        constexpr uint32_t u32_ = 0x12345678;
        constexpr packed_integer<TestType::endianness, 4> pus4{u32_};
        static_assert(pus4 == u32_);
        constexpr auto i32_ = (int32_t)0x87654321;
        constexpr packed_integer<TestType::endianness, 4, int32_t> ps4{i32_};
        static_assert(ps4 == i32_);
#endif
    } | std::tuple<endian_type<std::endian::little>, endian_type<std::endian::big>>{};

    "packed_integer_bits"_test = []<class TestType>
    {
        packed_integer_with_bfs<TestType::endianness, 4> pus4{};
        expect(pus4.halfword == 0);
        pus4.halfword = 0xabcd;
        expect(pus4.halfword == 0xabcd);
        expect(pus4 == 0xabcd);
        pus4 = 0x12345678;
        expect(pus4.halfword == 0x5678);
    } | std::tuple<endian_type<std::endian::little>, endian_type<std::endian::big>>{};
};