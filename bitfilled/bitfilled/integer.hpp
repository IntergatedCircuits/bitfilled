// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include "bitfilled/base_ops.hpp"

namespace bitfilled
{

template <typename T>
concept Integral = std::is_integral_v<T>;

/// @brief  Lookup unsigned integer of matching size
template <std::size_t SIZE, class T = void>
struct sized_integer
{};

template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 1>>
{
    using unsigned_type = std::uint8_t;
    using signed_type = std::int8_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 2>>
{
    using unsigned_type = std::uint16_t;
    using signed_type = std::int16_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 4>>
{
    using unsigned_type = std::uint32_t;
    using signed_type = std::int32_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 8>>
{
    using unsigned_type = std::uint64_t;
    using signed_type = std::int64_t;
};

template <std::size_t SIZE>
using sized_unsigned_t = typename sized_integer<SIZE>::unsigned_type;

template <std::size_t SIZE>
using sized_signed_t = typename sized_integer<SIZE>::signed_type;

/// @brief  An array type for flexibly storing an integer value.
template <std::size_t SIZE>
struct integer_storage : public std::array<sized_unsigned_t<1>, SIZE>
{
    using base_type = std::array<sized_unsigned_t<1>, SIZE>;
    using base_type::operator=;

    constexpr integer_storage() : base_type() {}
    // NOLINTNEXTLINE
    constexpr explicit integer_storage(const sized_unsigned_t<1> (&arr)[SIZE])
    {
        std::copy_n(arr, SIZE, base_type::data());
    }
    template <std::integral T>
    constexpr explicit integer_storage(T value, std::endian endianness = std::endian::native)
        : base_type()
    {
        // if the value is signed, fill the target with sign extend bytes
        if (std::is_signed_v<T> and (value < static_cast<T>(0)))
        {
            std::fill(this->begin(), this->end(), 0xffu);
        }

        // byteswap if the endian is not native
#if __cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            value = std::byteswap(value);
        }
#endif
        auto value_repr = std::bit_cast<std::array<sized_unsigned_t<1>, sizeof(T)>>(value);
#if !__cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            std::reverse(value_repr.begin(), value_repr.end());
        }
#endif
        // transfer the bytes to the correct position
        constexpr long size_diff = (long)sizeof(T) - (long)SIZE;
        constexpr auto min_size = std::min(sizeof(T), SIZE);
        if (endianness == std::endian::little)
        {
            std::copy(value_repr.data(), value_repr.data() + min_size, this->data());
        }
        else
        {
            if constexpr (size_diff == 0)
            {
                *this = value_repr;
            }
            else if constexpr (size_diff > 0)
            {
                std::copy(value_repr.data() + size_diff, value_repr.data() + size_diff + SIZE,
                          this->data());
            }
            else // size_diff < 0
            {
                std::copy(value_repr.data(), value_repr.data() + sizeof(T),
                          this->data() - size_diff);
            }
        }
    }

    template <std::integral T>
    [[nodiscard]] constexpr T to_integral(std::endian endianness = std::endian::native) const
    {
        integer_storage<sizeof(T)> value_repr;

        // if the value is signed, fill the target with sign extend bytes
        if (std::is_signed_v<T> and
            ((endianness == std::endian::little ? this->back() : this->front()) & 0x80u))
        {
            std::fill(value_repr.begin(), value_repr.end(), 0xffu);
        }

        // transfer the bytes to the correct position
        constexpr long size_diff = (long)SIZE - (long)sizeof(T);
        constexpr auto min_size = std::min(sizeof(T), SIZE);
        if (endianness == std::endian::little)
        {
            std::copy(this->data(), this->data() + min_size, value_repr.data());
        }
        else
        {
            if constexpr (size_diff == 0)
            {
                value_repr = *this;
            }
            else if constexpr (size_diff > 0)
            {
                std::copy(this->data() + size_diff, this->data() + size_diff + sizeof(T),
                          value_repr.data());
            }
            else // size_diff < 0
            {
                std::copy(this->data(), this->data() + SIZE, value_repr.data() - size_diff);
            }
        }

        // if not native endianness, reverse byte order
#if !__cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            std::reverse(value_repr.begin(), value_repr.end());
        }
#endif
        auto value = std::bit_cast<T>(value_repr);
#if __cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            value = std::byteswap(value);
        }
#endif
        return value;
    }
};

/// @brief  packed_integer stores an integer value in a packed byte array, with a defined
///         endianness.
/// @tparam ENDIAN: the endianness to use to convert between the integral value and the underlying
/// memory storage
/// @tparam SIZE: the integer storage size in octets
/// @tparam T: the native integral representation to use
template <std::endian ENDIAN, std::size_t SIZE, Integral T = sized_unsigned_t<std::bit_ceil(SIZE)>>
struct packed_integer
{
  private:
    integer_storage<SIZE> storage_;

  public:
    using superclass = packed_integer;
    using bf_ops = bitfilled::base::bitfield_ops<packed_integer>;
    using value_type = T;

    static constexpr auto endianness = ENDIAN;

    constexpr packed_integer() : storage_() {}
    constexpr packed_integer(value_type value) : storage_(value, endianness) {}
    // NOLINTNEXTLINE
    constexpr explicit packed_integer(const sized_unsigned_t<1> (&arr)[SIZE]) : storage_(arr) {}

    constexpr packed_integer& operator=(value_type value)
    {
        storage_ = integer_storage<SIZE>(value, endianness);
        return *this;
    }
    constexpr operator value_type() const
    {
        return storage_.template to_integral<value_type>(endianness);
    }
    [[nodiscard]] constexpr std::array<sized_unsigned_t<1>, SIZE> to_array() { return storage_; }
    [[nodiscard]] constexpr const std::array<sized_unsigned_t<1>, SIZE>& as_array() const
    {
        return storage_;
    }

    BITFILLED_OPS_FORWARDING
};

/// @brief  The host_integer class wraps an arithmetic type to allow subclassing it
///         (e.g. for the purpose of adding bitfields to it).
/// @tparam T: the arithmetic type to wrap
/// @tparam TOps: the type containing the bitfield_ops type for bitfield operations
template <Integral T, typename TOps = bitfilled::base>
struct host_integer
{
    using superclass = host_integer;
    using value_type = T;
    using bf_ops = typename TOps::template bitfield_ops<host_integer>;

    constexpr host_integer() = default;
    constexpr host_integer(T v) : raw_(v) {}
    constexpr host_integer& operator=(T other)
    {
        raw_ = other;
        return *this;
    }
    BITFILLED_OPS_FORWARDING

  private:
    T raw_{};

  public:
    constexpr operator auto &() { return raw_; }
    constexpr operator auto &() const { return raw_; }
    constexpr operator auto &() volatile { return raw_; }
    constexpr operator auto &() const volatile { return raw_; }
    // constexpr bool operator<=>(const defund&) const = default;
};

} // namespace bitfilled
