// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_SIZE_HPP__
#define __BITFILLED_SIZE_HPP__

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitfilled
{
/// @brief  Calculate the minimal number of bytes necessary to fit an integral value.
/// @tparam T the value's integral type
/// @param  c the input value
/// @return the necessary byte size to store the value
template <std::integral T>
constexpr inline std::size_t byte_width(T c)
{
    if constexpr (std::is_signed_v<T>)
    {
        auto x = static_cast<std::int64_t>(c);
        if ((x < std::numeric_limits<std::int32_t>::min()) or
            (x > std::numeric_limits<std::int32_t>::max()))
        {
            return 8;
        }
        if ((x < std::numeric_limits<std::int16_t>::min()) or
            (x > std::numeric_limits<std::int16_t>::max()))
        {
            return 4;
        }
        if ((x < std::numeric_limits<std::int8_t>::min()) or
            (x > std::numeric_limits<std::int8_t>::max()))
        {
            return 2;
        }
        return 1;
    }
    // else
    {
        auto x = static_cast<std::uint64_t>(c);
        return (x > std::numeric_limits<std::uint32_t>::max())   ? 8
               : (x > std::numeric_limits<std::uint16_t>::max()) ? 4
               : (x > std::numeric_limits<std::uint8_t>::max())  ? 2
                                                                 : 1;
    }
}

template <typename T>
constexpr inline std::size_t aligned_size = sizeof(T) / alignof(T);

} // namespace bitfilled

#endif // __BITFILLED_SIZE_HPP__
