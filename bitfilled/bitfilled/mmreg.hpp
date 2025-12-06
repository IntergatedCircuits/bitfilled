// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_MMREG_HPP__
#define __BITFILLED_MMREG_HPP__

#include "bitfilled/access.hpp"
#include "bitfilled/integer.hpp"

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
template <Integral T>
struct mmr_r
{
    constexpr operator const auto &() { return raw; }
    constexpr operator const auto &() volatile { return raw; }
    constexpr operator auto &() const { return raw; }
    constexpr operator auto &() const volatile { return raw; }

  protected:
    T raw{};
};
template <Integral T>
struct mmr_w
{
  protected:
    T raw{};
};
template <Integral T>
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

/// @brief  mmreg represents a memory mapped register.
template <Integral T, enum access ACCESS = access::readwrite, typename TOps = bitfilled::base>
struct mmreg : public detail::accesscondition<ACCESS, detail::mmr_r<T>, detail::mmr_w<T>,
                                              detail::mmr_rw<T>>::type
{
    using superclass = mmreg;
    using value_type = T;
    using bf_ops = typename TOps::template bitfield_ops<mmreg, ACCESS>;

  private:
    using base_type = detail::accesscondition<ACCESS, detail::mmr_r<T>, detail::mmr_w<T>,
                                              detail::mmr_rw<T>>::type;
    using base_type::raw;

  public:
    static constexpr enum access access() { return ACCESS; }
    constexpr mmreg() = default;
    constexpr mmreg(T other)
        requires(is_readwrite<ACCESS>)
    {
        raw = other;
    }

    constexpr static auto size() { return sizeof(raw); }

    constexpr void operator=(T other)
        requires(is_writeonly<ACCESS>)
    {
        raw = other;
    }
    // clang-format off
    constexpr void operator=(T other) volatile
        requires(is_writeonly<ACCESS>)
    {
        raw = other;
    }
    // clang-format on
    constexpr BITFILLED_ASSIGN_RETURN_DECL(auto&) operator=(T other)
        requires(is_readwrite<ACCESS>)
    {
        raw = other;
        return BITFILLED_ASSIGN_RETURN_EXPR(*this);
    }
    // clang-format off
    constexpr BITFILLED_ASSIGN_RETURN_DECL(auto&) operator=(T other) volatile
        requires(is_readwrite<ACCESS>)
    {
        raw = other;
        // when BITFILLED_ASSIGN_RETURNS_REF != 0
        // GCC warning: implicit dereference will not access object of type '' in statement
        // eliminating the warning: [[maybe_unused]] auto& _ = const_cast<decltype(a)::superclass&>(a = b);
        return BITFILLED_ASSIGN_RETURN_EXPR(*this);
    }
    // clang-format on

    constexpr mmreg(const mmreg&)
        requires(!is_readwrite<ACCESS>)
    = delete;
    constexpr mmreg& operator=(const mmreg&)
        requires(!is_readwrite<ACCESS>)
    = delete;
    constexpr mmreg(const mmreg&)
        requires(is_readwrite<ACCESS>)
    = default;
    constexpr mmreg& operator=(const mmreg&)
        requires(is_readwrite<ACCESS>)
    = default;

    BITFILLED_OPS_FORWARDING
};

} // namespace bitfilled

#endif // __BITFILLED_MMREG_HPP__
