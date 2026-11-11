#include "bitfilled/bytes.hpp"
#include "bitfilled/bits.hpp"
#include "bitfilled/integer.hpp"
#include "bitfilled/macros.hpp"
#include "field_traits.hpp"
#include <boost/ut.hpp>

using namespace bitfilled;
using namespace boost::ut;
using bitfilled_test::value_of;

namespace
{
/// IPv4 header (RFC 791): big endian, MSB0 bit numbering as in the RFC diagram
struct ipv4_header : bytes<20, std::endian::big>
{
    BF_OWNER(ipv4_header)
    BF_BITS(std::uint8_t, 0, 3) version;
    BF_BITS(std::uint8_t, 4, 7) ihl;
    BF_BITS(std::uint8_t, 8, 13) dscp;
    BF_BITS(std::uint8_t, 14, 15) ecn;
    BF_BITS(std::uint16_t, 16, 31) total_length;
    BF_BITS(std::uint16_t, 32, 47) identification;
    BF_BITS(bool, 49) dont_fragment;
    BF_BITS(bool, 50) more_fragments;
    BF_BITS(std::uint16_t, 51, 63) fragment_offset;
    BF_BITS(std::uint8_t, 64, 71) ttl;
    BF_BITS(std::uint8_t, 72, 79) protocol;
    BF_BITS(std::uint16_t, 80, 95) header_checksum;
    BF_BITS(std::uint32_t, 96, 127) source;
    BF_BITS(std::uint32_t, 128, 159) destination;
};
static_assert(sizeof(ipv4_header) == 20);
static_assert(alignof(ipv4_header) == 1);
static_assert(std::is_aggregate_v<ipv4_header>);
static_assert(bitfilled_test::passive_owner<ipv4_header>);

/// TCP header (RFC 9293), the flags as a bit field set: CWR (0) ... FIN (7)
struct tcp_header : bytes<20, std::endian::big>
{
    BF_OWNER(tcp_header)
    BF_BITS(std::uint16_t, 0, 15) source_port;
    BF_BITS(std::uint16_t, 16, 31) destination_port;
    BF_BITS(std::uint32_t, 32, 63) sequence_number;
    BF_BITS(std::uint32_t, 64, 95) acknowledgment_number;
    BF_BITS(std::uint8_t, 96, 99) data_offset;
    BF_BITSET(bool, 1, 8, 104) flags BF_BITSET_POSTFIX;
    BF_BITS(std::uint16_t, 112, 127) window;
};
static_assert(bitfilled_test::passive_owner<tcp_header>);

/// Bluetooth LE advertising physical channel PDU header: little endian, LSB0
struct ble_adv_header : bytes<2, std::endian::little>
{
    BF_OWNER(ble_adv_header)
    BF_BITS(std::uint8_t, 0, 3) pdu_type;
    BF_BITS(bool, 5) ch_sel;
    BF_BITS(bool, 6) tx_add;
    BF_BITS(bool, 7) rx_add;
    BF_BITS(std::uint8_t, 8, 15) length;
};
static_assert(bitfilled_test::passive_owner<ble_adv_header>);

/// IEEE 11073-20601 SFLOAT (e.g. GATT health thermometer): signed fields across bytes
struct sfloat : bytes<2, std::endian::little>
{
    BF_OWNER(sfloat)
    BF_BITS(std::int16_t, 0, 11) mantissa;
    BF_BITS(std::int8_t, 12, 15) exponent;
};

/// SCSI READ(6) CDB: the 21-bit LBA starts mid-byte
struct scsi_read6 : bytes<6, std::endian::big>
{
    BF_OWNER(scsi_read6)
    BF_BITS(std::uint8_t, 0, 7) opcode;
    BF_BITS(std::uint32_t, 11, 31) lba;
    BF_BITS(std::uint8_t, 32, 39) transfer_length;
    BF_BITS(std::uint8_t, 40, 47) control;
};

/// classic CAN frame payload with Intel (little endian) signals
struct can_intel : bytes<8, std::endian::little>
{
    BF_OWNER(can_intel)
    BF_BITS(std::uint16_t, 12, 23) unaligned;    // DBC: 12|12@1+
    BF_BITS(std::uint16_t, 24, 39) engine_speed; // DBC: 24|16@1+
    BF_BITS(std::int32_t, 40, 63) torque;        // DBC: 40|24@1-
};

/// classic CAN frame payload with Motorola (big endian) signals
struct can_motorola : bytes<8, std::endian::big>
{
    BF_OWNER(can_motorola)
    BF_BITS(std::uint16_t, 0, 15) vehicle_speed; // DBC: 7|16@0+
    BF_BITS(std::uint16_t, 19, 28) unaligned;    // DBC: 20|10@0+
};

/// a 64-bit field spanning 9 bytes
template <std::endian ENDIAN>
struct span9 : bytes<9, ENDIAN>
{
    BF_TEMPLATE_OWNER(span9, bytes<9, ENDIAN>)
    BF_BITS(std::uint64_t, 4, 67) wide;
    BF_BITS(std::int64_t, 3, 66) wide_signed;
};
static_assert(bitfilled_test::passive_owner<span9<std::endian::little>>);
static_assert(bitfilled_test::passive_owner<span9<std::endian::big>>);

/// bit field sets of each item layout
template <std::endian ENDIAN>
struct sets : bytes<12, ENDIAN>
{
    BF_TEMPLATE_OWNER(sets, bytes<12, ENDIAN>)
    BF_BITSET(std::uint16_t, 12, 4, 0) samples BF_BITSET_POSTFIX;    // spanning bytes
    BF_BITSET(std::int8_t, 4, 4, 48) nibbles BF_BITSET_POSTFIX;      // within a single byte
    BF_BITSET(std::uint16_t, 16, 2, 64) halfwords BF_BITSET_POSTFIX; // whole bytes
    BF_BITSET(std::uint8_t, 8, 12, 0) all_bytes BF_BITSET_POSTFIX;   // wider than 64 bits
};
static_assert(bitfilled_test::passive_owner<sets<std::endian::little>>);
static_assert(bitfilled_test::passive_owner<sets<std::endian::big>>);

#ifndef _MSC_VER
static_assert(bitfilled_test::stateless_field<decltype(ipv4_header::fragment_offset)>);
static_assert(bitfilled_test::stateless_field<decltype(tcp_header::flags)>);

template <typename T, std::size_t FIRST_BIT, std::size_t LAST_BIT>
concept valid_bytes3_field =
    requires { typename bitfield<T, bytes<3, std::endian::big>::bf_ops, FIRST_BIT, LAST_BIT>; };
static_assert(valid_bytes3_field<unsigned, 0, 23>);
static_assert(not valid_bytes3_field<unsigned, 0, 24>);
static_assert(not valid_bytes3_field<std::uint8_t, 4, 12>);

// const correctness: read-only access through a const owner
static_assert(BF_TEST_VALID(ipv4_header, self.ttl = 1));
static_assert(not BF_TEST_VALID(const ipv4_header, self.ttl = 1));
static_assert(BF_TEST_VALID(const ipv4_header, static_cast<std::uint8_t>(self.ttl)));
static_assert(BF_TEST_VALID(tcp_header, self.flags[0] = true));
static_assert(not BF_TEST_VALID(const tcp_header, self.flags[0] = true));
static_assert(BF_TEST_VALID(const tcp_header, static_cast<bool>(self.flags[0])));
#endif

/// Bit-by-bit reference model of the bit numbering conventions.
template <std::endian ENDIAN>
struct reference
{
    static constexpr std::size_t position(std::size_t first, std::size_t width, std::size_t bit)
    {
        // value bit 0 is the field's LSB: FIRST in LSB0 numbering, LAST in MSB0 numbering
        return ENDIAN == std::endian::little ? first + bit : first + width - 1 - bit;
    }
    static constexpr std::uint8_t mask(std::size_t pos)
    {
        return static_cast<std::uint8_t>(ENDIAN == std::endian::little
                                             ? (1U << (pos % byte_bits))
                                             : (0x80U >> (pos % byte_bits)));
    }
    static std::uint64_t read(const std::uint8_t* data, std::size_t first, std::size_t width)
    {
        std::uint64_t value = 0;
        for (std::size_t bit = 0; bit < width; ++bit)
        {
            const auto pos = position(first, width, bit);
            if ((data[pos / byte_bits] & mask(pos)) != 0)
            {
                value |= std::uint64_t{1} << bit;
            }
        }
        return value;
    }
    static void write(std::uint8_t* data, std::size_t first, std::size_t width, std::uint64_t value)
    {
        for (std::size_t bit = 0; bit < width; ++bit)
        {
            const auto pos = position(first, width, bit);
            if (((value >> bit) & 1U) != 0)
            {
                data[pos / byte_bits] =
                    static_cast<std::uint8_t>(data[pos / byte_bits] | mask(pos));
            }
            else
            {
                data[pos / byte_bits] =
                    static_cast<std::uint8_t>(data[pos / byte_bits] & ~mask(pos));
            }
        }
    }
};

constexpr std::size_t buffer_size = 12;
using buffer = std::array<std::uint8_t, buffer_size>;

struct pseudo_random
{
    std::uint64_t state = 0x9e3779b97f4a7c15ULL;
    std::uint64_t operator()()
    {
        state = (state * 6364136223846793005ULL) + 1442695040888963407ULL;
        return state ^ (state >> 29U);
    }
    buffer fill()
    {
        buffer buf{};
        for (auto& byte : buf)
        {
            byte = static_cast<std::uint8_t>((*this)());
        }
        return buf;
    }
};

template <std::endian ENDIAN, std::size_t WIDTH, bitfilled::detail::item_layout LAYOUT>
void check_dynamic(pseudo_random& rnd, std::size_t first)
{
    using codec = bitfilled::detail::byte_codec<ENDIAN>;
    using ref = reference<ENDIAN>;
    using word = bitfilled::detail::bytes_word_t<WIDTH>;
    auto buf = rnd.fill();
    expect(codec::template read<WIDTH, LAYOUT>(buf, first) == ref::read(buf.data(), first, WIDTH))
        << "read" << WIDTH << "bits at" << first;

    const auto value = rnd();
    auto expected = buf;
    ref::write(expected.data(), first, WIDTH, value);
    codec::template write<WIDTH, LAYOUT>(buf, first, static_cast<word>(value));
    expect(buf == expected) << "write" << WIDTH << "bits at" << first;
}

template <std::endian ENDIAN, std::size_t... WIDTH>
void check_dynamic_all(std::index_sequence<WIDTH...>)
{
    pseudo_random rnd;
    (
        [&rnd]
        {
            constexpr std::size_t width = WIDTH + 1;
            for (std::size_t first = 0; first + width <= buffer_size * byte_bits; ++first)
            {
                check_dynamic<ENDIAN, width, bitfilled::detail::item_layout::any>(rnd, first);
                if ((first / byte_bits) == ((first + width - 1) / byte_bits))
                {
                    check_dynamic<ENDIAN, width, bitfilled::detail::item_layout::single_byte>(
                        rnd, first);
                }
                if constexpr ((width % byte_bits) == 0)
                {
                    if ((first % byte_bits) == 0)
                    {
                        check_dynamic<ENDIAN, width, bitfilled::detail::item_layout::byte_aligned>(
                            rnd, first);
                    }
                }
            }
        }(),
        ...);
}

template <std::endian ENDIAN, std::size_t FIRST, std::size_t WIDTH>
void check_static(pseudo_random& rnd)
{
    using codec = bitfilled::detail::byte_codec<ENDIAN>;
    using ref = reference<ENDIAN>;
    constexpr std::size_t last = FIRST + WIDTH - 1;
    using word = typename codec::template layout<FIRST, last>::word;
    auto buf = rnd.fill();
    expect(codec::template read<FIRST, last>(buf) == ref::read(buf.data(), FIRST, WIDTH))
        << "static read" << WIDTH << "bits at" << FIRST;

    const auto value = rnd();
    auto expected = buf;
    ref::write(expected.data(), FIRST, WIDTH, value);
    codec::template write<FIRST, last>(buf, static_cast<word>(value));
    expect(buf == expected) << "static write" << WIDTH << "bits at" << FIRST;
}

template <std::endian ENDIAN, std::size_t WIDTH, std::size_t... FIRST>
void check_static_firsts(pseudo_random& rnd, std::index_sequence<FIRST...>)
{
    (check_static<ENDIAN, FIRST, WIDTH>(rnd), ...);
}

template <std::endian ENDIAN, std::size_t... WIDTH>
void check_static_all()
{
    pseudo_random rnd;
    (check_static_firsts<ENDIAN, WIDTH>(rnd, std::make_index_sequence<17>{}), ...);
}

template <std::endian ENDIAN>
void check_static_widths()
{
    check_static_all<ENDIAN, 1, 2, 3, 4, 5, 7, 8, 9, 12, 15, 16, 17, 24, 25, 31, 32, 33, 40, 48, 56,
                     57, 58, 63, 64>();
}

template <std::endian ENDIAN>
struct endian_type
{
    static constexpr auto endianness = ENDIAN;
};
using both_endians = std::tuple<endian_type<std::endian::little>, endian_type<std::endian::big>>;

} // namespace

const suite<"bytes"> bytes_suite = []
{
    "codec_matches_reference"_test = []<class TestType>
    {
        check_dynamic_all<TestType::endianness>(std::make_index_sequence<64>{});
        check_static_widths<TestType::endianness>();
    } | both_endians{};

    "ipv4"_test = []
    {
        ipv4_header hdr{};
        hdr.as_array() = {0x45, 0x00, 0x00, 0x73, 0x00, 0x00, 0x40, 0x00, 0x40, 0x11,
                          0xb8, 0x61, 0xc0, 0xa8, 0x00, 0x01, 0xc0, 0xa8, 0x00, 0xc7};
        expect(that % value_of(hdr.version) == 4);
        expect(that % value_of(hdr.ihl) == 5);
        expect(that % value_of(hdr.dscp) == 0);
        expect(that % value_of(hdr.total_length) == 0x73);
        expect(value_of(hdr.dont_fragment));
        expect(not value_of(hdr.more_fragments));
        expect(that % value_of(hdr.fragment_offset) == 0);
        expect(that % value_of(hdr.ttl) == 64);
        expect(that % value_of(hdr.protocol) == 17);
        expect(that % value_of(hdr.header_checksum) == 0xb861);
        expect(that % value_of(hdr.source) == 0xc0a80001U);
        expect(that % value_of(hdr.destination) == 0xc0a800c7U);

        hdr.fragment_offset = 0x1abc;
        expect(that % hdr.as_array()[6] == 0x5a);
        expect(that % hdr.as_array()[7] == 0xbc);
        expect(value_of(hdr.dont_fragment));
        hdr.ecn = 3;
        hdr.dscp = 0x2e;
        expect(that % hdr.as_array()[1] == 0xbb);

        const ipv4_header& chdr = hdr;
        expect(that % value_of(chdr.fragment_offset) == 0x1abc);
        const auto copy = hdr;
        expect(copy == hdr);
    };

    "tcp"_test = []
    {
        tcp_header hdr{};
        hdr.source_port = 443;
        hdr.data_offset = 5;
        hdr.flags[6] = true; // SYN
        hdr.flags[3] = true; // ACK
        expect(that % hdr.as_array()[0] == 0x01);
        expect(that % hdr.as_array()[1] == 0xbb);
        expect(that % hdr.as_array()[12] == 0x50);
        expect(that % hdr.as_array()[13] == 0x12);
        expect(static_cast<bool>(hdr.flags[6]));
        expect(not static_cast<bool>(hdr.flags[7]));

        tcp_header other{};
#ifdef _MSC_VER
        // bit field sets are indexed properties on MSVC, they can only be copied item by item
        for (std::size_t i = 0; i < 8; ++i)
        {
            other.flags[i] = hdr.flags[i];
        }
#else
        other.flags = hdr.flags;
#endif
        expect(that % other.as_array()[13] == 0x12);
        expect(that % other.as_array()[12] == 0x00);
    };

    "ble_adv_header"_test = []
    {
        ble_adv_header hdr{};
        hdr.pdu_type = 0; // ADV_IND
        hdr.tx_add = true;
        hdr.length = 0x25;
        expect(that % hdr.as_array()[0] == 0x40);
        expect(that % hdr.as_array()[1] == 0x25);
        hdr.as_array() = {0x32, 0x0c};
        expect(that % value_of(hdr.pdu_type) == 2);
        expect(value_of(hdr.ch_sel));
        expect(not value_of(hdr.tx_add));
        expect(not value_of(hdr.rx_add));
        expect(that % value_of(hdr.length) == 12);
    };

    "sfloat"_test = []
    {
        sfloat value{};
        value.as_array() = {0x6c, 0xf1}; // 36.4: 364 * 10^-1
        expect(that % value_of(value.mantissa) == 364);
        expect(that % value_of(value.exponent) == -1);

        value.mantissa = -5;
        value.exponent = 2;
        expect(that % value.as_array()[0] == 0xfb);
        expect(that % value.as_array()[1] == 0x2f);
        expect(that % value_of(value.mantissa) == -5);
    };

    "scsi_read6"_test = []
    {
        scsi_read6 cdb{};
        cdb.as_array()[1] = 0xe0; // reserved bits are preserved
        cdb.opcode = 0x08;
        cdb.lba = 0x1abcde;
        cdb.transfer_length = 1;
        const std::array<std::uint8_t, 6> expected{0x08, 0xfa, 0xbc, 0xde, 0x01, 0x00};
        expect(cdb.as_array() == expected);
        expect(that % value_of(cdb.lba) == 0x1abcdeU);
    };

    "can_signals"_test = []
    {
        can_intel intel{};
        intel.unaligned = 0xabc;
        intel.engine_speed = 0x1234;
        intel.torque = -2;
        const std::array<std::uint8_t, 8> intel_expected{0x00, 0xc0, 0xab, 0x34,
                                                         0x12, 0xfe, 0xff, 0xff};
        expect(intel.as_array() == intel_expected);
        expect(that % value_of(intel.torque) == -2);
        expect(that % value_of(intel.unaligned) == 0xabc);

        can_motorola motorola{};
        motorola.vehicle_speed = 0x1234;
        motorola.unaligned = 0x2ab;
        const std::array<std::uint8_t, 8> motorola_expected{0x12, 0x34, 0x15, 0x58,
                                                            0x00, 0x00, 0x00, 0x00};
        expect(motorola.as_array() == motorola_expected);
        expect(that % value_of(motorola.unaligned) == 0x2ab);
    };

    "nine_byte_span"_test = []<class TestType>
    {
        span9<TestType::endianness> s{};
        s.wide = 0x0123456789abcdefULL;
        expect(that % value_of(s.wide) == 0x0123456789abcdefULL);
        if constexpr (TestType::endianness == std::endian::little)
        {
            const std::array<std::uint8_t, 9> expected{0xf0, 0xde, 0xbc, 0x9a, 0x78,
                                                       0x56, 0x34, 0x12, 0x00};
            expect(s.as_array() == expected);
        }
        else
        {
            const std::array<std::uint8_t, 9> expected{0x00, 0x12, 0x34, 0x56, 0x78,
                                                       0x9a, 0xbc, 0xde, 0xf0};
            expect(s.as_array() == expected);
        }
        s.wide_signed = -3;
        expect(that % value_of(s.wide_signed) == -3);
    } | both_endians{};

    "sets"_test = []<class TestType>
    {
        sets<TestType::endianness> s{};
        for (std::size_t i = 0; i < 4; ++i)
        {
            s.samples[i] = static_cast<std::uint16_t>(0xa01 + i);
            s.nibbles[i] = static_cast<std::int8_t>(static_cast<int>(i) - 2);
        }
        s.halfwords[0] = 0x1234;
        s.halfwords[1] = 0x5678;
        for (std::size_t i = 0; i < 4; ++i)
        {
            expect(that % static_cast<std::uint16_t>(s.samples[i]) == 0xa01 + i);
            expect(that % static_cast<std::int8_t>(s.nibbles[i]) == static_cast<int>(i) - 2);
        }
        expect(that % static_cast<std::uint16_t>(s.halfwords[0]) == 0x1234);
        expect(that % static_cast<std::uint16_t>(s.halfwords[1]) == 0x5678);
        if constexpr (TestType::endianness == std::endian::little)
        {
            expect(that % s.as_array()[0] == 0x01);
            expect(that % s.as_array()[1] == 0x2a);
            expect(that % s.as_array()[8] == 0x34);
            expect(that % s.as_array()[9] == 0x12);
        }
        else
        {
            expect(that % s.as_array()[0] == 0xa0);
            expect(that % s.as_array()[1] == 0x1a);
            expect(that % s.as_array()[8] == 0x12);
            expect(that % s.as_array()[9] == 0x34);
        }

        sets<TestType::endianness> copy{};
#ifdef _MSC_VER
        for (std::size_t i = 0; i < 12; ++i)
        {
            copy.all_bytes[i] = s.all_bytes[i];
        }
#else
        copy.all_bytes = s.all_bytes;
#endif
        expect(copy == s);
    } | both_endians{};
};
