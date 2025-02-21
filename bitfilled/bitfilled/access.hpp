// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_ACCESS_HPP__
#define __BITFILLED_ACCESS_HPP__

#include <cstdint>
#include <type_traits>

namespace bitfilled
{

/// @brief access describes the access rights to a memory location
enum class access : std::uint_fast8_t
{
    none = 0,
    read = 1,
    r = read,
    write = 2,
    w = write,
    readwrite = 3,
    rw = readwrite,
    read_ephemeralwrite = 7, // writes don't get stored when read
};

inline constexpr enum access operator&(enum access lhs, enum access rhs)
{
    return static_cast<enum access>(static_cast<std::uint_fast8_t>(lhs) &
                                    static_cast<std::uint_fast8_t>(rhs));
}
inline constexpr enum access operator|(enum access lhs, enum access rhs)
{
    return static_cast<enum access>(static_cast<std::uint_fast8_t>(lhs) |
                                    static_cast<std::uint_fast8_t>(rhs));
}

template <typename T>
concept DefinesAccess = requires { T::access(); };

template <enum access ACCESS>
inline constexpr bool is_readable = (ACCESS & access::read) != access::none;
template <enum access ACCESS>
inline constexpr bool is_writeable = (ACCESS & access::write) != access::none;
template <enum access ACCESS>
inline constexpr bool is_readonly = (ACCESS & access::readwrite) == access::read;
template <enum access ACCESS>
inline constexpr bool is_writeonly = (ACCESS & access::readwrite) == access::write;
template <enum access ACCESS>
inline constexpr bool is_readwrite = (ACCESS & access::readwrite) == access::readwrite;
template <enum access ACCESS>
inline constexpr bool is_ephemeralwrite =
    (ACCESS & access::read_ephemeralwrite) == access::read_ephemeralwrite;

} // namespace bitfilled

#endif // __BITFILLED_ACCESS_HPP__
