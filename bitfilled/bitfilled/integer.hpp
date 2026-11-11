// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>
#include "bitfilled/base_ops.hpp"
#include "bitfilled/bytes.hpp"

namespace bitfilled
{

template <typename T>
concept Integral = std::is_integral_v<T>;

/// @brief  An array type for flexibly storing an integer value.
template <std::size_t SIZE>
struct integer_storage : public std::array<sized_unsigned_t<1>, SIZE>
{
    using base_type = std::array<sized_unsigned_t<1>, SIZE>;
    using base_type::operator=;
    static constexpr sized_unsigned_t<1> sign_extend_byte = 0xff;

    constexpr integer_storage() : base_type() {}
    constexpr explicit integer_storage(const base_type& arr) : base_type(arr) {}
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
        if constexpr (std::is_signed_v<T>)
        {
            if (value < static_cast<T>(0))
            {
                std::fill(this->begin(), this->end(), sign_extend_byte);
            }
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
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
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
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    }

    template <std::integral T>
    [[nodiscard]] constexpr T to_integral(std::endian endianness = std::endian::native) const
    {
        integer_storage<sizeof(T)> value_repr;

        // if the value is signed, fill the target with sign extend bytes
        if constexpr (std::is_signed_v<T>)
        {
            if ((endianness == std::endian::little ? this->back() : this->front()) & 0x80U)
            {
                std::fill(value_repr.begin(), value_repr.end(), sign_extend_byte);
            }
        }

        // transfer the bytes to the correct position
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
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
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

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
/// @details The bit fields follow the bit numbering convention of @ref bytes:
///          LSB0 for little endian (bit 0 is the least significant bit of the value),
///          MSB0 for big endian (bit 0 is the most significant bit of the value).
/// @tparam ENDIAN: the endianness to use to convert between the integral value and the underlying
/// memory storage
/// @tparam SIZE: the integer storage size in octets
/// @tparam T: the native integral representation to use
template <std::endian ENDIAN, std::size_t SIZE, Integral T = sized_unsigned_t<std::bit_ceil(SIZE)>>
struct packed_integer : public bytes<SIZE, ENDIAN>
{
  private:
    using bytes_type = bytes<SIZE, ENDIAN>;
    using bytes_type::storage;

  public:
    using superclass = packed_integer;
    using bf_ops = bytes_ops<packed_integer>;
    using value_type = T;

    constexpr packed_integer() = default;
    constexpr packed_integer(value_type value)
        : bytes_type(integer_storage<SIZE>(value, bytes_type::endianness))
    {}
    // NOLINTNEXTLINE
    constexpr explicit packed_integer(const sized_unsigned_t<1> (&arr)[SIZE]) : bytes_type(arr) {}

    constexpr packed_integer& operator=(value_type value)
    {
        storage = integer_storage<SIZE>(value, bytes_type::endianness);
        return *this;
    }
    constexpr operator value_type() const
    {
        return integer_storage<SIZE>(storage).template to_integral<value_type>(
            bytes_type::endianness);
    }
    [[nodiscard]] constexpr std::array<sized_unsigned_t<1>, SIZE> to_array() const
    {
        return storage;
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
    constexpr operator auto&() { return raw_; }
    constexpr operator auto&() const { return raw_; }
    constexpr operator auto&() volatile { return raw_; }
    constexpr operator auto&() const volatile { return raw_; }
    // constexpr bool operator<=>(const defund&) const = default;
};

} // namespace bitfilled
