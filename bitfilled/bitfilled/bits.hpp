// SPDX-License-Identifier: MPL-2.0
#pragma once

#include "bitfilled/base_ops.hpp"

namespace bitfilled
{
struct empty_type
{};

/// @brief  Proxy to a single bit field (set item), returned by bit field set indexing
///         (and by bit field assignment when BITFILLED_ASSIGN_RETURNS_REF is enabled).
///         Unlike the bit field descriptors, it refers to its storage explicitly, so it can be
///         passed around like any other proxy reference (e.g. std::vector<bool>::reference).
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
    operator value_type() const
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

/// @brief  The regbitfield class is an access-constrained bitfield type.
/// @details A bit field is a stateless descriptor: it is an empty member of its containing
///          type (via `[[no_unique_address]]`), and it accesses the containing type's storage
///          through its own address. Therefore it is only meaningful as a member subobject:
///          - its default and copy constructors are trivial, so constructing or copying the
///            containing type never executes bit field code (the storage is copied by the base),
///          - the copy constructor is only accessible to the owner class (see @ref BF_OWNER),
///            so `auto v = obj.field;` or passing a field by value fails to compile instead of
///            creating a detached descriptor (convert to the value type instead),
///          - assignment between bit fields copies the field value, not the descriptor.
///          The converting constructor only exists to support aggregate initialization of the
///          containing type (designated initializers), and must not be used otherwise.
template <BitfieldValue T, typename TOps, enum access ACCESS, std::size_t FIRST_BIT,
          std::size_t LAST_BIT = FIRST_BIT>
    requires(BitRangeFitsStorage<TOps, FIRST_BIT, LAST_BIT> and
             BitRangeFitsValue<T, FIRST_BIT, LAST_BIT>)
struct regbitfield
{
  private:
    static constexpr enum access access_rights = effective_access<ACCESS, TOps>;

    // only the owner class may copy the descriptor, as part of copying itself
    friend typename detail::owner_of<TOps>::type;
    explicit constexpr regbitfield(const regbitfield&) = default;

  public:
    using value_type = T;
    using ops_type = TOps;
    using props_type = bitfield_props<FIRST_BIT, LAST_BIT>;
    static constexpr bool dynamic_index = false;

    /// @return the effective access rights (the field's declared rights, limited by the storage's)
    static constexpr enum access access() { return access_rights; }
    constexpr static T mask()
        requires(std::integral<T> and !std::is_same_v<T, bool>)
    {
        return props_type::template mask<T>();
    }
    constexpr static auto offset() { return props_type::offset(); }
    constexpr static auto size_bits() { return props_type::size_bits(); }

    constexpr regbitfield() = default;
    /// @brief  Only for aggregate initialization of the containing type.
    /// @note   Accepts plain values only (no bit fields, proxies or other classes),
    ///         so it is never used to convert a bit field into a detached copy of itself.
    template <BitfieldValue U>
        requires(is_writeable<access_rights> and std::is_convertible_v<U, T>)
    constexpr regbitfield(U other)
    {
        *this = static_cast<T>(other);
    }

    ~regbitfield() = default;
    regbitfield(regbitfield&) = delete;
    regbitfield(regbitfield&&) = delete;
    regbitfield& operator=(regbitfield&&) = delete;

    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(T other)
        requires(is_writeable<access_rights>)
    {
        TOps::set_field((props_type&)*this, other);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield>{(props_type&)*this}));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(T other) volatile
        requires(is_writeable<access_rights>)
    {
        TOps::set_field((volatile props_type&)*this, other);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield, true>{(volatile props_type&)*this}));
    }
    // clang-format on
    operator T() const
        requires(is_readable<access_rights>)
    {
        return TOps::template get_field<T>((const props_type&)*this);
    }
    // clang-format off
    operator T() const volatile
        requires(is_readable<access_rights>)
    {
        return TOps::template get_field<T>((const volatile props_type&)*this);
    }
    // clang-format on

    regbitfield& operator=(const regbitfield&)
        requires(!is_readwrite<access_rights>)
    = delete;
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(const regbitfield & other)
        requires(is_readwrite<access_rights>)
    {
        auto value = static_cast<T>(other);
        TOps::set_field((props_type&)*this, value);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield>{(props_type&)*this}));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(const regbitfield & other) volatile
        requires(is_readwrite<access_rights>)
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

/// @brief  The regbitfieldset class is an access-constrained array of adjacent bit fields.
/// @details Like @ref regbitfield, it is a stateless descriptor with the same construction and
///          copy restrictions. Its items are accessed by operator[], which returns the item value
///          for read-only sets, and a @ref regbitfield_reference proxy for writeable sets.
template <BitfieldValue T, typename TOps, enum access ACCESS, std::size_t ITEM_SIZE,
          std::size_t ITEM_COUNT, std::size_t OFFSET = 0>
    requires((ITEM_SIZE > 0) and (ITEM_COUNT > 0) and
             BitRangeFitsStorage<TOps, OFFSET, OFFSET + (ITEM_SIZE * ITEM_COUNT) - 1> and
             BitRangeFitsValue<T, 0, ITEM_SIZE - 1>)
struct regbitfieldset
{
  private:
    static constexpr enum access access_rights = effective_access<ACCESS, TOps>;

    // only the owner class may copy the descriptor, as part of copying itself
    friend typename detail::owner_of<TOps>::type;
    explicit constexpr regbitfieldset(const regbitfieldset&) = default;

  public:
    using value_type = T;
    using ops_type = TOps;
    using props_type = regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>;
    static constexpr bool dynamic_index = true;

    /// @return the effective access rights (the field's declared rights, limited by the storage's)
    static constexpr enum access access() { return access_rights; }

    static constexpr std::size_t size() { return ITEM_COUNT; }

    constexpr regbitfieldset() = default;

    ~regbitfieldset() = default;
    regbitfieldset(regbitfieldset&) = delete;
    regbitfieldset(regbitfieldset&&) = delete;
    regbitfieldset& operator=(regbitfieldset&&) = delete;

    regbitfieldset& operator=(const regbitfieldset&)
        requires(!is_readwrite<access_rights>)
    = delete;

  private:
    static constexpr std::size_t total_bits = ITEM_COUNT * ITEM_SIZE;
    // whether all items fit in the storage integer type, so that they can be copied at once
    // (the item value type might be narrower, e.g. bool)
    static constexpr bool copy_whole = []() constexpr
    {
        if constexpr (requires { typename TOps::int_type; })
        {
            return total_bits <= value_bits<typename TOps::int_type>;
        }
        else
        {
            return false;
        }
    }();

  public:
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfieldset & other)
        requires(is_readwrite<access_rights>)
    {
        if constexpr (copy_whole)
        {
            using whole_type =
                regbitfield<typename TOps::int_type, TOps, ACCESS, OFFSET, OFFSET + total_bits - 1>;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<whole_type&>(*this) = reinterpret_cast<const whole_type&>(other);
            return BITFILLED_ASSIGN_RETURN_EXPR(reinterpret_cast<whole_type&>(*this));
        }
        else
        {
            for (std::size_t i = 0; i < ITEM_COUNT; ++i)
            {
                TOps::set_item((props_type&)*this, i,
                               TOps::template get_item<T>((const props_type&)other, i));
            }
            return BITFILLED_ASSIGN_RETURN_EXPR(*this);
        }
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfieldset& other) volatile
        requires(is_readwrite<access_rights>)
    {
        if constexpr (copy_whole)
        {
            using whole_type =
                regbitfield<typename TOps::int_type, TOps, ACCESS, OFFSET, OFFSET + total_bits - 1>;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
            reinterpret_cast<volatile whole_type&>(*this) =
                reinterpret_cast<const whole_type&>(other);
            return BITFILLED_ASSIGN_RETURN_EXPR(reinterpret_cast<volatile whole_type&>(*this));
        }
        else
        {
            for (std::size_t i = 0; i < ITEM_COUNT; ++i)
            {
                TOps::set_item((volatile props_type&)*this, i,
                               TOps::template get_item<T>((const props_type&)other, i));
            }
            return BITFILLED_ASSIGN_RETURN_EXPR(*this);
        }
    }
    // clang-format on
    auto operator[](std::size_t pos) const
        requires(is_readable<access_rights>)
    {
        return TOps::template get_item<T>((const props_type&)*this, pos);
    }
    // clang-format off
    auto operator[](std::size_t pos) const volatile
        requires(is_readable<access_rights>)
    {
        return TOps::template get_item<T>((const volatile props_type&)*this, pos);
    }
    // clang-format on
    auto operator[](std::size_t pos)
        requires(is_writeable<access_rights>)
    {
        return regbitfield_reference<regbitfieldset>{(props_type&)*this, pos};
    }
    // clang-format off
    auto operator[](std::size_t pos) volatile
        requires(is_writeable<access_rights>)
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
