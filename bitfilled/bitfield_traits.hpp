// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFIELD_TRAITS_HPP__
#define __BITFIELD_TRAITS_HPP__

#include <array>
#include "bitfilled/size.hpp"

// https://en.cppreference.com/w/cpp/language/bit_field
// http://mjfrazer.org/mjfrazer/bitfields/
namespace bitfield_traits
{
// an implementation either packs consecutive fields into the same underlying
// storage location, or splits them each into separate locations (thus making bit-fields useless)
constexpr inline bool is_packed = []() constexpr
{
    struct s
    {
        char a : 1;
        char b : 1;
    };
    return bitfilled::aligned_size<s> == 1;
}();

// an implementation either straddles fields to ensure that each field doesn't occupy
// more than one underlying storage location, or keeps unaligned bit-fields
// intact (thus requiring access to multiple locations to access a single field)
constexpr inline bool is_straddled = []() constexpr
{
    struct s
    {
        char a : 6;
        char b : 6;
        char c : 4;
    };
    return bitfilled::aligned_size<s> == 3;
}();

// an implementation might support an unnamed padding bit-field with size zero,
// that enforces the splitting of successive fields to a new storage location
constexpr inline bool has_padding_field = []() constexpr
{
    struct s
    {
        char a : 1;
        char : 0;
        char b : 1;
    };
    return is_packed and bitfilled::aligned_size<s> == 2;
}();

// bit-field endianness is either little (least significant bit field first), or big
#if defined(__GNUC__) && (__GNUC__ >= 11)
constexpr inline std::endian endianness = []() constexpr
{
    constexpr struct s
    {
        char a : 1 = 1;
        char b : 7 = 0;
    } val;
    constexpr auto x = std::bit_cast<std::array<char, bitfilled::aligned_size<s>>>(val).front();
    static_assert(x);
    return x == 1 ? std::endian::little : std::endian::big;
}();
#else
inline std::endian endianness = []()
{
    struct s
    {
        char a : 1 = 1;
        char b : 7 = 0;
    } val;
    static const auto x = std::bit_cast<std::array<char, bitfilled::aligned_size<s>>>(val).front();
    return x == 1 ? std::endian::little : std::endian::big;
}();
#endif

// an implementation either wraps overflows, or clamps them
constexpr inline bool overflow_wraps = []() constexpr
{
    struct s
    {
        int a : 2;
    } val{2};
    return val.a == -2;
}();

} // namespace bitfield_traits

#endif // __BITFIELD_TRAITS_HPP__
