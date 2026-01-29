// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "bitfilled/base_ops.hpp"

namespace bitfilled
{
struct empty_type
{};

template <typename T, bool IsVolatile = false>
struct regbitfield_reference : public T::props_type
{
  private:
    friend T;
    using value_type = typename T::value_type;
    using ref_type = std::conditional_t<IsVolatile, std::add_volatile_t<typename T::props_type>,
                                        typename T::props_type>;
    ref_type& ref_;
    [[no_unique_address]]
    const std::conditional_t<T::dynamic_index, std::size_t, empty_type> index_{};

    constexpr regbitfield_reference(ref_type& ref)
        requires(!T::dynamic_index)
        : ref_(ref)
    {}
    constexpr regbitfield_reference(ref_type& ref, std::size_t index)
        requires(T::dynamic_index)
        : ref_(ref), index_(index)
    {}

  public:
    static constexpr enum access access() { return T::access(); }

    ~regbitfield_reference() = default;
    regbitfield_reference(regbitfield_reference&&) = delete;
    regbitfield_reference& operator=(regbitfield_reference&&) = delete;

    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(value_type other)
        requires(is_writeable<T::access()>)
    {
        if constexpr (T::dynamic_index)
        {
            T::ops_type::set_item(ref_, index_, other);
        }
        else
        {
            T::ops_type::set_field(ref_, other);
        }
        return BITFILLED_ASSIGN_RETURN_EXPR(*this);
    }
    operator auto() const
        requires(is_readable<T::access()>)
    {
        if constexpr (T::dynamic_index)
        {
            return T::ops_type::template get_item<value_type>(ref_, index_);
        }
        else
        {
            return T::ops_type::template get_field<value_type>(ref_);
        }
    }

    regbitfield_reference(const regbitfield_reference&)
        requires(!is_readwrite<T::access()>)
    = delete;
    regbitfield_reference& operator=(const regbitfield_reference&)
        requires(!is_readwrite<T::access()>)
    = delete;

    regbitfield_reference(const regbitfield_reference& other)
        requires(is_readwrite<T::access()>)
        : ref_(other.ref_), index_(other.index_)
    {}
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfield_reference & other)
        requires(is_readwrite<T::access()>)
    {
        return *this = static_cast<value_type>(other);
    }
};

/// \brief  The regbitfield class is an access-constrained bitfield type.
template <typename T, typename TOps, enum access ACCESS, std::size_t FIRST_BIT,
          std::size_t LAST_BIT = FIRST_BIT>
struct regbitfield
{
    using value_type = T;
    using ops_type = TOps;
    using props_type = bitfield_props<FIRST_BIT, LAST_BIT>;
    static constexpr bool dynamic_index = false;

    static constexpr enum access access() { return ACCESS; }
    constexpr static T mask() { return props_type::template mask<T>(); }
    constexpr static T memory_mask() { return props_type::template memory_mask<T>(); }
    constexpr static auto offset() { return props_type::offset(); }
    constexpr static auto size_bits() { return props_type::size_bits(); }

    constexpr regbitfield() = default;
    constexpr regbitfield(T other)
        requires(is_writeable<ACCESS>)
    {
        *this = other;
    }

    ~regbitfield() = default;
    regbitfield(regbitfield&&) = delete;
    regbitfield& operator=(regbitfield&&) = delete;

    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(T other)
        requires(is_writeable<ACCESS>)
    {
        TOps::set_field((props_type&)*this, other);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield>{(props_type&)*this}));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(T other) volatile
        requires(is_writeable<ACCESS>)
    {
        TOps::set_field((volatile props_type&)*this, other);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield, true>{(volatile props_type&)*this}));
    }
    // clang-format on
    operator T() const
        requires(is_readable<ACCESS>)
    {
        return TOps::template get_field<T>((const props_type&)*this);
    }
    // clang-format off
    operator T() const volatile
        requires(is_readable<ACCESS>)
    {
        return TOps::template get_field<T>((const volatile props_type&)*this);
    }
    // clang-format on

    regbitfield(const regbitfield&)
        requires(!is_readwrite<ACCESS>)
    = delete;
    regbitfield& operator=(const regbitfield&)
        requires(!is_readwrite<ACCESS>)
    = delete;
    regbitfield(const regbitfield& other)
        requires(is_readwrite<ACCESS>)
    {
        *this = other;
    }
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(const regbitfield & other)
        requires(is_readwrite<ACCESS>)
    {
        auto value = static_cast<T>(other);
        TOps::set_field((props_type&)*this, value);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield>{(props_type&)*this}));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(const regbitfield & other) volatile
        requires(is_readwrite<ACCESS>)
    {
        auto value = static_cast<T>(other);
        TOps::set_field((volatile props_type&)*this, value);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield, true>{(volatile props_type&)*this}));
    }
    // clang-format on
};

/// @brief  The bitfield is a shortcut to define bitfields in variables.
template <typename T, typename TOps, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>
using bitfield = regbitfield<T, TOps, access::readwrite, FIRST_BIT, LAST_BIT>;

template <typename T, typename TOps, enum access ACCESS, std::size_t ITEM_SIZE,
          std::size_t ITEM_COUNT, std::size_t OFFSET = 0>
struct regbitfieldset
{
    using value_type = T;
    using ops_type = TOps;
    using props_type = regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>;
    static constexpr bool dynamic_index = true;

    static constexpr enum access access() { return ACCESS; }

    static constexpr std::size_t size() { return ITEM_COUNT; }

    constexpr regbitfieldset() = default;

    ~regbitfieldset() = default;
    regbitfieldset(regbitfieldset&&) = delete;
    regbitfieldset& operator=(regbitfieldset&&) = delete;

    regbitfieldset(const regbitfieldset&)
        requires(!is_readwrite<ACCESS>)
    = delete;
    regbitfieldset& operator=(const regbitfieldset&)
        requires(!is_readwrite<ACCESS>)
    = delete;

    regbitfieldset(const regbitfieldset& other)
        requires(is_readwrite<ACCESS>)
    {
        *this = other;
    }
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfieldset & other)
        requires(is_readwrite<ACCESS>)
    {
        using bf_type = regbitfield<T, TOps, ACCESS, OFFSET, OFFSET + ITEM_COUNT * ITEM_SIZE - 1>;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<bf_type&>(*this) = reinterpret_cast<const bf_type&>(other);
        return BITFILLED_ASSIGN_RETURN_EXPR(reinterpret_cast<bf_type&>(*this));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfieldset& other) volatile
        requires(is_readwrite<ACCESS>)
    {
        using bf_type = regbitfield<T, TOps, ACCESS, OFFSET, OFFSET + ITEM_COUNT * ITEM_SIZE - 1>;
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
        reinterpret_cast<volatile bf_type&>(*this) = reinterpret_cast<const bf_type&>(other);
        return BITFILLED_ASSIGN_RETURN_EXPR(reinterpret_cast<volatile bf_type&>(*this));
    }
    // clang-format on
    auto
    operator[](std::size_t pos) const
    {
        return TOps::template get_item<T>((const props_type&)*this, pos);
    }
    auto operator[](std::size_t pos) const volatile
    {
        return TOps::template get_item<T>((const volatile props_type&)*this, pos);
    }
    auto operator[](std::size_t pos)
        requires(is_readonly<ACCESS>)
    {
        return TOps::template get_item<T>((props_type&)*this, pos);
    }
    auto operator[](std::size_t pos)
        requires(is_readwrite<ACCESS>)
    {
        return regbitfield_reference<regbitfieldset>{(props_type&)*this, pos};
    }
    // clang-format off
    auto operator[](std::size_t pos) volatile
        requires(is_readonly<ACCESS>)
    {
        return TOps::template get_item<T>((volatile props_type&)*this, pos);
    }
    auto operator[](std::size_t pos) volatile
        requires(is_readwrite<ACCESS>)
    {
        return regbitfield_reference<regbitfieldset, true>{(volatile props_type&)*this, pos};
    }
    // clang-format on

    // TODO: iterators, reverse iterators
    // TODO: initializer list constructor
};

/// @brief  The bitfield is a shortcut to define bitfields in variables.
template <typename T, typename TOps, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT,
          std::size_t OFFSET = 0>
using bitfieldset = regbitfieldset<T, TOps, access::readwrite, ITEM_SIZE, ITEM_COUNT, OFFSET>;

} // namespace bitfilled
