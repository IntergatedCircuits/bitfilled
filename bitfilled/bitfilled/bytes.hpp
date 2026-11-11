// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <utility>
#include "bitfilled/base_ops.hpp"

namespace bitfilled
{
namespace detail
{
/// @brief  The smallest unsigned integer type that holds a bit field of WIDTH bits.
/// @note   Words narrower than int are promoted to int in arithmetic, which is safe: the shifted
///         intermediate values (a byte or a WIDTH bit value, shifted by less than WIDTH bits) stay
///         below 2^15 for std::uint8_t words, and below 2^24 for std::uint16_t words (which are
///         only promoted to int if it is wider than 16 bits). Results are cast back to the word
///         type.
template <std::size_t WIDTH>
using bytes_word_t = sized_unsigned_t<std::bit_ceil((WIDTH + byte_bits - 1) / byte_bits)>;

template <typename TWord, std::size_t WIDTH>
constexpr TWord low_mask()
{
    static_assert((WIDTH > 0) and (WIDTH <= value_bits<TWord>));
    constexpr auto all_ones = static_cast<TWord>(~TWord{});
    return static_cast<TWord>(all_ones >> (value_bits<TWord> - WIDTH));
}

/// @brief  The layout of the bytes in a bit field set's items.
enum class item_layout : std::uint_fast8_t
{
    any,          ///< items may span multiple bytes, at any bit offset
    single_byte,  ///< each item is contained within a single byte
    byte_aligned, ///< each item consists of whole bytes
};

/// @brief  Reads and writes bit fields located in a byte array.
/// @details The bit numbering follows the endianness convention of network protocol specifications:
///          - little endian: LSB0, bit N is (1 << (N % 8)) of byte (N / 8),
///            the bit field's least significant bit is FIRST_BIT
///          - big endian: MSB0, bit N is (0x80 >> (N % 8)) of byte (N / 8),
///            the bit field's most significant bit is FIRST_BIT
///          Either way, a bit field is a contiguous [FIRST_BIT, LAST_BIT] range, its bytes ordered
///          according to the endianness. The bytes of a field are indexed by their significance
///          (k = 0 is the least significant byte), the field value is shifted left
///          by `shift` bits within the bytes.
template <std::endian ENDIAN>
    requires((ENDIAN == std::endian::little) or (ENDIAN == std::endian::big))
struct byte_codec
{
    static constexpr bool little = ENDIAN == std::endian::little;

    static constexpr unsigned shift_of(std::size_t first_bit, std::size_t last_bit)
    {
        return static_cast<unsigned>(little ? (first_bit % byte_bits)
                                            : (byte_bits - 1 - (last_bit % byte_bits)));
    }

    /// @return the bits of the k-th least significant byte of a field, at their position
    template <typename TWord>
    static constexpr TWord gather(std::uint8_t byte, std::size_t k, unsigned shift)
    {
        if (k == 0)
        {
            return static_cast<TWord>(static_cast<TWord>(byte) >> shift);
        }
        return static_cast<TWord>(static_cast<TWord>(byte) << ((byte_bits * k) - shift));
    }
    /// @return the k-th least significant byte of the field value
    template <typename TWord>
    static constexpr std::uint8_t scatter(TWord value, std::size_t k, unsigned shift)
    {
        if (k == 0)
        {
            return static_cast<std::uint8_t>(value << shift);
        }
        return static_cast<std::uint8_t>(value >> ((byte_bits * k) - shift));
    }

    /// @brief  The compile-time layout of a bit field.
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    struct layout
    {
        static constexpr std::size_t width = 1 + LAST_BIT - FIRST_BIT;
        using word = bytes_word_t<width>;
        static constexpr std::size_t first_byte = FIRST_BIT / byte_bits;
        static constexpr std::size_t last_byte = LAST_BIT / byte_bits;
        static constexpr std::size_t count = 1 + last_byte - first_byte;
        static constexpr unsigned shift = shift_of(FIRST_BIT, LAST_BIT);
        static constexpr word mask = low_mask<word, width>();
        /// the field ends at the top of its most significant byte, reading needs no masking
        static constexpr bool top_aligned = (byte_bits * count) == (shift + width);

        static constexpr std::size_t index(std::size_t k)
        {
            return little ? (first_byte + k) : (last_byte - k);
        }
    };

    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr auto read(std::span<const std::uint8_t> data)
    {
        using L = layout<FIRST_BIT, LAST_BIT>;
        using word = L::word;
        if constexpr (L::count == 1)
        {
            // single byte access, no assembly of bytes
            const auto byte = static_cast<std::uint8_t>(data[L::first_byte] >> L::shift);
            if constexpr (L::top_aligned)
            {
                return static_cast<word>(byte);
            }
            else
            {
                return static_cast<word>(byte & static_cast<std::uint8_t>(L::mask));
            }
        }
        else
        {
            const auto value = [data]<std::size_t... K>(std::index_sequence<K...>)
            {
                return static_cast<word>((gather<word>(data[L::index(K)], K, L::shift) | ...));
            }(std::make_index_sequence<L::count>{});
            if constexpr (L::top_aligned)
            {
                return value;
            }
            else
            {
                return static_cast<word>(value & L::mask);
            }
        }
    }

    template <typename L, std::size_t K>
    static constexpr void write_byte(std::span<std::uint8_t> data, L::word value)
    {
        constexpr auto byte_mask = scatter(L::mask, K, L::shift);
        std::uint8_t& byte = data[L::index(K)];
        if constexpr (byte_mask == 0xffU)
        {
            // the whole byte belongs to the field, no read is necessary
            byte = scatter(value, K, L::shift);
        }
        else
        {
            byte = static_cast<std::uint8_t>((byte & ~byte_mask) |
                                             (scatter(value, K, L::shift) & byte_mask));
        }
    }

    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr void write(std::span<std::uint8_t> data,
                                layout<FIRST_BIT, LAST_BIT>::word value)
    {
        using L = layout<FIRST_BIT, LAST_BIT>;
        if constexpr (L::count == 1)
        {
            write_byte<L, 0>(data, value);
        }
        else
        {
            [data, value]<std::size_t... K>(std::index_sequence<K...>)
            {
                (write_byte<L, K>(data, value), ...);
            }(std::make_index_sequence<L::count>{});
        }
    }

    /// @brief  Read a WIDTH bits wide field, located at a run-time bit position.
    /// @note   The bit position is bounds checked by the caller (bit field set index assertion).
    template <std::size_t WIDTH, item_layout LAYOUT = item_layout::any>
    static constexpr bytes_word_t<WIDTH> read(std::span<const std::uint8_t> data,
                                              std::size_t first_bit)
    {
        using word = bytes_word_t<WIDTH>;
        constexpr word mask = low_mask<word, WIDTH>();
        const std::size_t last_bit = first_bit + WIDTH - 1;
        const std::size_t first_byte = first_bit / byte_bits;
        if constexpr (LAYOUT == item_layout::single_byte)
        {
            return static_cast<word>(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                (static_cast<word>(data[first_byte]) >> shift_of(first_bit, last_bit)) & mask);
        }
        else
        {
            const std::size_t last_byte = last_bit / byte_bits;
            const std::size_t count = 1 + last_byte - first_byte;
            const unsigned shift =
                (LAYOUT == item_layout::byte_aligned) ? 0U : shift_of(first_bit, last_bit);
            word value = 0;
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                const std::uint8_t byte = data[little ? (first_byte + k) : (last_byte - k)];
                value = static_cast<word>(value | gather<word>(byte, k, shift));
            }
            if constexpr (LAYOUT == item_layout::byte_aligned)
            {
                return value;
            }
            else
            {
                return static_cast<word>(value & mask);
            }
        }
    }

    /// @brief  Write a WIDTH bits wide field, located at a run-time bit position.
    template <std::size_t WIDTH, item_layout LAYOUT = item_layout::any>
    static constexpr void write(std::span<std::uint8_t> data, std::size_t first_bit,
                                bytes_word_t<WIDTH> value)
    {
        using word = bytes_word_t<WIDTH>;
        constexpr word mask = low_mask<word, WIDTH>();
        const std::size_t last_bit = first_bit + WIDTH - 1;
        const std::size_t first_byte = first_bit / byte_bits;
        const std::size_t last_byte = last_bit / byte_bits;
        const std::size_t count = 1 + last_byte - first_byte;
        if constexpr (LAYOUT == item_layout::byte_aligned)
        {
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                data[little ? (first_byte + k) : (last_byte - k)] = scatter(value, k, 0);
            }
        }
        else
        {
            const unsigned shift = shift_of(first_bit, last_bit);
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                std::uint8_t& byte = data[little ? (first_byte + k) : (last_byte - k)];
                const auto byte_mask = scatter(mask, k, shift);
                byte = static_cast<std::uint8_t>((byte & ~byte_mask) |
                                                 (scatter(value, k, shift) & byte_mask));
            }
        }
    }
};
} // namespace detail

/// @brief  The bit field operations of byte arrays (see @ref bytes).
/// @tparam T: the owner type, which provides `endianness`, `size()` and `as_array()`
template <typename T>
struct bytes_ops
{
    /// @brief  The widest bit field value type (also used to copy bit field sets as a whole).
    using int_type = std::uint64_t;
    static constexpr std::size_t storage_bits() { return T::size() * byte_bits; }

  protected:
    using codec = detail::byte_codec<T::endianness>;

    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr void check_field()
    {
        static_assert(BitfieldValue<TVal>,
                      "bit field value type must be a cv-unqualified integral or enum type");
        static_assert(BitRangeFitsStorage<bytes_ops, FIRST_BIT, LAST_BIT>,
                      "bit field exceeds the width of its storage");
        static_assert(BitRangeFitsValue<TVal, FIRST_BIT, LAST_BIT>,
                      "bit field is wider than its value type");
        static_assert(LAST_BIT - FIRST_BIT < value_bits<int_type>,
                      "bit field is wider than the widest supported value type");
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static constexpr void check_items()
    {
        static_assert(BitfieldValue<TVal>,
                      "bit field value type must be a cv-unqualified integral or enum type");
        static_assert(BitRangeFitsStorage<bytes_ops, OFFSET, OFFSET + (ITEM_SIZE * ITEM_COUNT) - 1>,
                      "bit field set exceeds the width of its storage");
        static_assert(BitRangeFitsValue<TVal, 0, ITEM_SIZE - 1>,
                      "bit field set item is wider than its value type");
        static_assert(ITEM_SIZE <= value_bits<int_type>,
                      "bit field set item is wider than the widest supported value type");
    }
    template <std::size_t ITEM_SIZE, std::size_t OFFSET>
    static constexpr detail::item_layout items_layout()
    {
        if constexpr (((byte_bits % ITEM_SIZE) == 0) and ((OFFSET % ITEM_SIZE) == 0))
        {
            return detail::item_layout::single_byte;
        }
        else if constexpr (((ITEM_SIZE % byte_bits) == 0) and ((OFFSET % byte_bits) == 0))
        {
            return detail::item_layout::byte_aligned;
        }
        else
        {
            return detail::item_layout::any;
        }
    }

    /// @brief  The bit field properties are located at the address of their owner object.
    template <typename Tptr>
    static copy_cv_t<Tptr&, T> owner(Tptr& ptr)
    {
        // cast through void* as the empty bit field type has a smaller alignment requirement
        return *static_cast<std::add_pointer_t<copy_cv_t<Tptr, T>>>(
            static_cast<std::add_pointer_t<copy_cv_t<Tptr, void>>>(&ptr));
    }

  public:
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
    {
        check_field<TVal, FIRST_BIT, LAST_BIT>();
        using word = codec::template layout<FIRST_BIT, LAST_BIT>::word;
        codec::template write<FIRST_BIT, LAST_BIT>(owner(bf).as_array(), static_cast<word>(value));
    }

    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const BITFILLED_FIELD_PROPS_PARAM_T& bf)
    {
        check_field<TVal, FIRST_BIT, LAST_BIT>();
        auto typeval =
            static_cast<TVal>(codec::template read<FIRST_BIT, LAST_BIT>(owner(bf).as_array()));
        return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(typeval);
    }

    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index, TVal value)
    {
        check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
        codec::template write<ITEM_SIZE, items_layout<ITEM_SIZE, OFFSET>()>(
            owner(bf).as_array(),
            regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::offset(index),
            static_cast<detail::bytes_word_t<ITEM_SIZE>>(value));
    }

    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
    {
        check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
        auto typeval =
            static_cast<TVal>(codec::template read<ITEM_SIZE, items_layout<ITEM_SIZE, OFFSET>()>(
                owner(bf).as_array(),
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::offset(index)));
        return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(typeval);
    }
};

/// @brief  bytes is a byte array with a defined endianness, typically representing (a part of)
///         a network protocol data unit, whose bit fields may span multiple bytes.
/// @details The bit fields follow the bit numbering convention of the endianness:
///          - little endian: LSB0, bit 0 is the least significant bit of byte 0,
///            multi-byte fields are little endian (e.g. Bluetooth LE, USB, CAN Intel signals)
///          - big endian: MSB0, bit 0 is the most significant bit of byte 0,
///            multi-byte fields are big endian (e.g. IETF RFC diagrams: IP, TCP, UDP)
///          The type has no alignment requirement and no padding, so it can be copied to
///          and from network buffers as-is.
/// @tparam SIZE: the number of bytes
/// @tparam ENDIAN: the byte order and bit numbering convention of the bit fields
template <std::size_t SIZE, std::endian ENDIAN>
    requires((SIZE > 0) and ((ENDIAN == std::endian::little) or (ENDIAN == std::endian::big)))
struct bytes
{
  protected: // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::array<std::uint8_t, SIZE> storage{};

  public:
    using superclass = bytes;
    using bf_ops = bytes_ops<bytes>;
    using array_type = std::array<std::uint8_t, SIZE>;

    static constexpr auto endianness = ENDIAN;

    constexpr bytes() = default;
    constexpr explicit bytes(const array_type& arr) : storage(arr) {}
    // NOLINTNEXTLINE
    constexpr explicit bytes(const std::uint8_t (&arr)[SIZE])
    {
        std::copy_n(arr, SIZE, storage.data());
    }

    static constexpr std::size_t size() { return SIZE; }
    [[nodiscard]] constexpr std::uint8_t* data() { return storage.data(); }
    [[nodiscard]] constexpr std::span<const std::uint8_t> data() const { return storage.data(); }
    [[nodiscard]] constexpr array_type& as_array() { return storage; }
    [[nodiscard]] constexpr const array_type& as_array() const { return storage; }

    friend constexpr bool operator==(const bytes& lhs, const bytes& rhs)
    {
        return lhs.storage == rhs.storage;
    }

    BITFILLED_OPS_FORWARDING
};

} // namespace bitfilled
