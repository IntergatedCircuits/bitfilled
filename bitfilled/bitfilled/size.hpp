// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitfilled
{
/// @brief  Calculate the minimal number of bytes necessary to fit an integral value.
/// @tparam T the value's integral type
/// @param  integral the input value
/// @return the necessary byte size to store the value
template <typename T>
constexpr std::size_t byte_width(T integral)
{
    if constexpr (std::is_signed_v<T>)
    {
        auto sint = static_cast<std::int64_t>(integral);
        if ((sint < std::numeric_limits<std::int32_t>::min()) or
            (sint > std::numeric_limits<std::int32_t>::max()))
        {
            return 8;
        }
        if ((sint < std::numeric_limits<std::int16_t>::min()) or
            (sint > std::numeric_limits<std::int16_t>::max()))
        {
            return 4;
        }
        if ((sint < std::numeric_limits<std::int8_t>::min()) or
            (sint > std::numeric_limits<std::int8_t>::max()))
        {
            return 2;
        }
        return 1;
    }
    // else
    {
        auto uint = static_cast<std::uint64_t>(integral);
        if (uint > std::numeric_limits<std::uint32_t>::max())
        {
            return 8;
        }
        if (uint > std::numeric_limits<std::uint16_t>::max())
        {
            return 4;
        }
        if (uint > std::numeric_limits<std::uint8_t>::max())
        {
            return 2;
        }
        return 1;
    }
}

template <typename T>
constexpr inline std::size_t aligned_size = sizeof(T) / alignof(T);

} // namespace bitfilled
