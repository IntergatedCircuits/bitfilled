// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_MMREG_HPP__
#define __BITFILLED_MMREG_HPP__

#include "bitfilled/access.hpp"
#include "bitfilled/bits.hpp"

namespace bitfilled
{
namespace detail
{

// the conversion operators cannot be selectively enabled via concepts,
// so separate base classes are used instead
template <enum access ACCESS, class TReadOnly, class TWriteOnly, class TReadWrite,
          class TRead_EphemeralWrite = TReadWrite>
struct accesscondition
{};
template <class TReadOnly, class TWriteOnly, class TReadWrite, class TRead_EphemeralWrite>
struct accesscondition<access::read, TReadOnly, TWriteOnly, TReadWrite, TRead_EphemeralWrite>
{
    using type = TReadOnly;
};
template <class TReadOnly, class TWriteOnly, class TReadWrite, class TRead_EphemeralWrite>
struct accesscondition<access::write, TReadOnly, TWriteOnly, TReadWrite, TRead_EphemeralWrite>
{
    using type = TWriteOnly;
};
template <class TReadOnly, class TWriteOnly, class TReadWrite, class TRead_EphemeralWrite>
struct accesscondition<access::readwrite, TReadOnly, TWriteOnly, TReadWrite, TRead_EphemeralWrite>
{
    using type = TReadWrite;
};
template <class TReadOnly, class TWriteOnly, class TReadWrite, class TRead_EphemeralWrite>
struct accesscondition<access::read_ephemeralwrite, TReadOnly, TWriteOnly, TReadWrite,
                       TRead_EphemeralWrite>
{
    using type = TRead_EphemeralWrite;
};
template <BitwiseAccessible T>
struct mmr_r
{
    constexpr operator const auto &() { return raw; }
    constexpr operator const auto &() volatile { return raw; }
    constexpr operator auto &() const { return raw; }
    constexpr operator auto &() const volatile { return raw; }

  protected:
    T raw{};
};
template <BitwiseAccessible T>
struct mmr_w
{
  protected:
    T raw{};
};
template <BitwiseAccessible T>
struct mmr_rw
{
    constexpr operator auto &() { return raw; }
    constexpr operator auto &() volatile { return raw; }
    constexpr operator auto &() const { return raw; }
    constexpr operator auto &() const volatile { return raw; }

  protected:
    T raw{};
};
} // namespace detail

// clang-format off

/// @brief  mmreg represents a memory mapped register.
template <BitwiseAccessible T, enum access ACCESS = access::readwrite, typename Tops = bitfield_ops<T, ACCESS>>
struct mmreg : public detail::accesscondition<ACCESS, detail::mmr_r<T>, detail::mmr_w<T>, detail::mmr_rw<T>>::type
{
    using superclass = mmreg;
    using ops = Tops;

private:
    using base_type = detail::accesscondition<ACCESS, detail::mmr_r<T>, detail::mmr_w<T>, detail::mmr_rw<T>>::type;
    using base_type::raw;

public:
    static constexpr enum access access() { return ACCESS; }
    constexpr mmreg() = default;

    constexpr operator T() const requires(is_readable<ACCESS>) { return raw; }
    constexpr operator T() const volatile requires(is_readable<ACCESS>) { return raw; }
    constexpr static auto size() { return sizeof(raw); }

    constexpr void operator=(T other) requires(is_writeonly<ACCESS>) { raw = other; }
    constexpr void operator=(T other) volatile requires(is_writeonly<ACCESS>) { raw = other; }
    constexpr mmreg& operator=(T other) requires(is_readwrite<ACCESS>) { raw = other; return *this;}
    constexpr volatile mmreg& operator=(T other) volatile requires(is_readwrite<ACCESS>) { raw = other; return *this;}

    constexpr mmreg(const mmreg&) requires(is_readwrite<ACCESS>) = default;
    constexpr mmreg(const mmreg&) requires(!is_readwrite<ACCESS>) = delete;
    constexpr mmreg& operator=(const mmreg&) requires(is_readwrite<ACCESS>) = default;
    constexpr mmreg& operator=(const mmreg&) requires(!is_readwrite<ACCESS>) = delete;
};

// clang-format on

} // namespace bitfilled

#endif // __BITFILLED_MMREG_HPP__
