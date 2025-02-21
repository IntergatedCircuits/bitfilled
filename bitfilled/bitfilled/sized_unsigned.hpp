// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_SIZED_UNSIGNED_HPP_
#define __BITFILLED_SIZED_UNSIGNED_HPP_

#include <cstdint>
#include <type_traits>

namespace bitfilled
{
/// @brief  Lookup unsigned integer of matching size
template <std::size_t SIZE, class T = void>
struct sized_unsigned
{};

template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 1>>
{
    typedef std::uint8_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 2>>
{
    typedef std::uint16_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 4>>
{
    typedef std::uint32_t type;
};
template <std::size_t SIZE>
struct sized_unsigned<SIZE, std::enable_if_t<SIZE == 8>>
{
    typedef std::uint64_t type;
};

template <std::size_t SIZE>
using sized_unsigned_t = typename sized_unsigned<SIZE>::type;

template <typename T>
concept IntegerConvertable = std::is_convertible_v<T, sized_unsigned_t<sizeof(T)>>;

} // namespace bitfilled

#endif // __BITFILLED_SIZED_UNSIGNED_HPP_
