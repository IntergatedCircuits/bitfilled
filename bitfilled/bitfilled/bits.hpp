// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_BITS_HPP__
#define __BITFILLED_BITS_HPP__

#include "bitfilled/access.hpp"
#include "bitfilled/sized_unsigned.hpp"

namespace bitfilled
{

// define a concept to check if a type supports bitwise operators
// TODO: limit this further
template <typename T>
concept BitwiseAccessible = IntegerConvertable<T>;

/// \brief The bitfield_props class stores the bitfield location information.
template <std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>
struct bitfield_props
{
    static_assert(LAST_BIT >= FIRST_BIT);

    constexpr static std::size_t size_bits() { return 1 + LAST_BIT - FIRST_BIT; }
    constexpr static std::size_t offset() { return FIRST_BIT; }
    template <typename T = unsigned>
    constexpr static T mask()
    {
        return ((1 << size_bits()) - 1);
    }
    template <typename T>
    constexpr static T extract_field(T memory)
    {
        return (memory >> offset()) & mask<T>();
    }
    template <typename T>
    constexpr static T position_field(T value)
    {
        return (value & mask<T>()) << offset();
    }
    template <typename T>
    constexpr static T insert_field(T memory, T value)
    {
        return (memory & ~(mask<T>() << offset())) | position_field(value);
    }
    template <typename T>
    constexpr static T sign_extend(T v)
    {
        if constexpr (std::is_signed_v<T>)
        {
            struct
            {
                T field : size_bits();
            } s{.field = v};
            return s.field;
        }
        return v;
    }
};

template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET = 0>
struct regbitfieldset_props : public bitfield_props<OFFSET, ITEM_SIZE + OFFSET - 1>
{
  private:
    using base = bitfield_props<OFFSET, ITEM_SIZE + OFFSET - 1>;

  public:
    static_assert(ITEM_SIZE > 0);
    static_assert(ITEM_COUNT > 0);
    static_assert(OFFSET >= 0);

    constexpr static std::size_t offset(std::size_t index)
    {
        return base::offset() + index * base::size_bits();
    }
    template <typename T>
    constexpr static T extract_field(T memory, std::size_t index)
    {
        return (memory >> offset(index)) & base::template mask<T>();
    }
    template <typename T>
    constexpr static T position_field(T value, std::size_t index)
    {
        return (value & base::template mask<T>()) << offset(index);
    }
    template <typename T>
    constexpr static T insert_field(T memory, T value, std::size_t index)
    {
        return (memory & ~(base::template mask<T>() << offset(index))) |
               position_field(value, index);
    }
};

template <typename From, typename To>
struct copy_cv
{
  private:
    using ctype = std::conditional_t<std::is_const_v<From>, std::add_const_t<To>, To>;

  public:
    using type = std::conditional_t<std::is_volatile_v<From>, std::add_volatile_t<ctype>, ctype>;
};
template <typename From, typename To>
using copy_cv_t = typename copy_cv<From, To>::type;

// clang-format off

/// \brief The bitfield_ops class defines the bitfield operations on its containing type
template<BitwiseAccessible T, enum access ACCESS>
struct bitfield_ops
{
    static constexpr enum access access() { return ACCESS; }
    // TODO: need better selection here
    using int_type = std::conditional_t<std::is_floating_point_v<T>, sized_unsigned_t<sizeof(T)>, T>;
protected:
    template<typename Tptr>
    static auto& memory(Tptr& ptr) { return (copy_cv_t<Tptr&, int_type&>)ptr; }
public:
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
    {
        const auto v = static_cast<int_type>(value);
        if constexpr (!is_readable<bitfield_ops::access()> or
                      is_ephemeralwrite<bitfield_ops::access()>)
        {
            memory(bf) = bf.position_field(v);
        }
        else
        {
            memory(bf) = bf.insert_field(memory(bf), v);
        }
    }
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
    {
        const auto v = static_cast<int_type>(value);
        if constexpr (!is_readable<bitfield_ops::access()> or
                      is_ephemeralwrite<bitfield_ops::access()>)
        {
            memory(bf) = bf.position_field(v);
        }
        else
        {
            memory(bf) = bf.insert_field(memory(bf), v);
        }
    }
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<bitfield_ops::access()>)
    {
        auto x = static_cast<TVal>(bf.extract_field(memory(bf)));
        return bf.sign_extend(x);
    }
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<bitfield_ops::access()>)
    {
        auto x = static_cast<TVal>(bf.extract_field(memory(bf)));
        return bf.sign_extend(x);
    }
    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf, std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
    {
        const auto v = static_cast<int_type>(value);
        if constexpr (!is_readable<bitfield_ops::access()> or
                      is_ephemeralwrite<bitfield_ops::access()>)
        {
            memory(bf) = bf.position_field(v, index);
        }
        else
        {
            memory(bf) = bf.insert_field(memory(bf), v, index);
        }
    }
    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf, std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
    {
        const auto v = static_cast<int_type>(value);
        if constexpr (!is_readable<bitfield_ops::access()> or
                      is_ephemeralwrite<bitfield_ops::access()>)
        {
            memory(bf) = bf.position_field(v, index);
        }
        else
        {
            memory(bf) = bf.insert_field(memory(bf), v, index);
        }
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
    {
        auto x = static_cast<TVal>(bf.extract_field(memory(bf), index));
        return bf.sign_extend(x);
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
    {
        auto x = static_cast<TVal>(bf.extract_field(memory(bf), index));
        return bf.sign_extend(x);
    }
};

// clang-format on
// clang-format off

/// \brief  The regbitfield class is an access-constrained bitfield type.
template <typename T, typename TOps, enum access ACCESS, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>
struct regbitfield : public bitfield_props<FIRST_BIT, LAST_BIT>
{
private:
    using base_type = bitfield_props<FIRST_BIT, LAST_BIT>;

public:
    static constexpr enum access access() { return ACCESS; }
    constexpr static T mask()
    {
        return base_type::template mask<T>();
    }
    constexpr static T memory_mask()
    {
        return base_type::template memory_mask<T>();
    }

    constexpr regbitfield() = default;
    constexpr regbitfield(T other) requires(is_writeable<regbitfield::access()>)
    {
        *this = other;
    }

    T operator=(T other) requires(is_writeable<regbitfield::access()>)
    {
        TOps::set_field(*this, other);
        return other;
    }
    T operator=(T other) volatile requires(is_writeable<regbitfield::access()>)
    {
        TOps::set_field(*this, other);
        return other;
    }
    operator T() const requires(is_readable<regbitfield::access()>)
    {
        return TOps::template get_field<T>(*this);
    }
    operator T() const volatile requires(is_readable<regbitfield::access()>)
    {
        return TOps::template get_field<T>(*this);
    }

    regbitfield(const regbitfield&) requires(!is_readwrite<regbitfield::access()>) = delete;
    regbitfield& operator=(const regbitfield&) requires(!is_readwrite<regbitfield::access()>) = delete;
    regbitfield(const regbitfield& other) requires(is_readwrite<regbitfield::access()>)
    {
        *this = other;
    }
    T operator=(const regbitfield& other) requires(is_readwrite<regbitfield::access()>)
    {
        auto v = static_cast<T>(other);
        TOps::set_field(*this, v);
        return v;
    }
    T operator=(const regbitfield& other) volatile requires(is_readwrite<regbitfield::access()>)
    {
        auto v = static_cast<T>(other);
        TOps::set_field(*this, v);
        return v;
    }
};
// clang-format on

/// @brief  The bitfield is a shortcut to define bitfields in variables.
template <typename T, typename TOps, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>
using bitfield = regbitfield<T, TOps, access::readwrite, FIRST_BIT, LAST_BIT>;

// clang-format off

template <typename T, typename TOps, enum access ACCESS, std::size_t ITEM_SIZE,
          std::size_t ITEM_COUNT, std::size_t OFFSET = 0>
struct regbitfield_reference : public regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>
{
  private:
    using base_type = regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>;

    template <typename T_, typename TOps_, enum access ACCESS_, std::size_t ITEM_SIZE_,
              std::size_t ITEM_COUNT_, std::size_t OFFSET_>
    friend struct regbitfieldset;

    base_type& ref_;
    std::size_t index_;
    constexpr regbitfield_reference(base_type& ref, std::size_t index) : ref_(ref), index_(index) {}

  public:
    static constexpr enum access access() { return ACCESS; }

    T operator=(T other) requires(is_writeable<regbitfield_reference::access()>)
    {
        TOps::set_item(ref_, index_, other);
        return other;
    }
    T operator=(T other) volatile requires(is_writeable<regbitfield_reference::access()>)
    {
        TOps::set_item(ref_, index_, other);
        return other;
    }
    operator T() const requires(is_readable<regbitfield_reference::access()>)
    {
        return TOps::template get_item<T>(ref_, index_);
    }
    operator T() const volatile requires(is_readable<regbitfield_reference::access()>)
    {
        return TOps::template get_item<T>(ref_, index_);
    }

    regbitfield_reference(const regbitfield_reference&) requires(!is_readwrite<regbitfield_reference::access()>) = delete;
    regbitfield_reference& operator=(const regbitfield_reference&) requires(!is_readwrite<regbitfield_reference::access()>) = delete;
    regbitfield_reference(const regbitfield_reference& other) requires(is_readwrite<regbitfield_reference::access()>) { *this = other; }
    T operator=(const regbitfield_reference& other) requires(is_readwrite<regbitfield_reference::access()>)
    {
        auto v = static_cast<T>(other);
        TOps::set_item(ref_, index_, v);
        return v;
    }
    T operator=(const regbitfield_reference& other) volatile requires(is_readwrite<regbitfield_reference::access()>)
    {
        auto v = static_cast<T>(other);
        TOps::set_item(ref_, index_, v);
        return v;
    }
};
// clang-format on
// clang-format off
template <typename T, typename TOps, enum access ACCESS, std::size_t ITEM_SIZE,
          std::size_t ITEM_COUNT, std::size_t OFFSET = 0>
struct regbitfieldset : public regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>
{
  private:
    using base_type = regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>;
    using item_type = regbitfield_reference<T, TOps, ACCESS, ITEM_SIZE, ITEM_COUNT, OFFSET>;

  public:
    // TODO: iterators, reverse iterators
    // TODO: initializer list constructor
    static constexpr enum access access() { return ACCESS; }

    constexpr regbitfieldset() = default;

    regbitfieldset(const regbitfieldset&) requires(!is_readwrite<regbitfieldset::access()>) = delete;
    regbitfieldset& operator=(const regbitfieldset&) requires(!is_readwrite<regbitfieldset::access()>) = delete;
    regbitfieldset(const regbitfieldset& other) requires(is_readwrite<regbitfieldset::access()>) { *this = other; }
    T operator=(const regbitfieldset& other) requires(is_readwrite<regbitfieldset::access()>)
    {
        // TODO
        return 0;
    }
    T operator=(const regbitfieldset& other) volatile requires(is_readwrite<regbitfieldset::access()>)
    {
        // TODO
        return 0;
    }
    T operator[](std::size_t pos) const
    {
        return TOps::template get_item<T>(*this, pos);
    }
    T operator[](std::size_t pos) const volatile
    {
        return TOps::template get_item<T>(*this, pos);
    }
    item_type operator[](std::size_t pos)
    {
        return item_type(*this, pos);
    }
    /* TODO volatile ? */ item_type operator[](std::size_t pos) volatile
    {
        return item_type(*this, pos);
    }
};
// clang-format on

/// @brief  The bitfield is a shortcut to define bitfields in variables.
template <typename T, typename TOps, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT,
          std::size_t OFFSET = 0>
using bitfieldset = regbitfieldset<T, TOps, access::readwrite, ITEM_SIZE, ITEM_COUNT, OFFSET>;

/// @brief  The defund class wraps a fundamental type to allow subclassing it.
/// @tparam T: the fundamental type to wrap
template <BitwiseAccessible T>
struct defund
{
    constexpr defund() = default;
    constexpr defund(const T& v) : raw(v) {}
    constexpr defund& operator=(T other)
    {
        raw = other;
        return *this;
    }

  private:
    T raw{};

  public:
    constexpr operator auto &() { return raw; }
    constexpr operator auto &() const { return raw; }
    constexpr operator auto &() volatile { return raw; }
    constexpr operator auto &() const volatile { return raw; }
    // constexpr bool operator<=>(const defund&) const = default;
};

/// @brief  The host class is a helper to create a type that can contain bitfields.
template <BitwiseAccessible T>
struct host : public std::conditional_t<std::is_fundamental_v<T>, defund<T>, T>
{
    using std::conditional_t<std::is_fundamental_v<T>, defund<T>, T>::operator=;
    using superclass = host;
    using ops = bitfield_ops<T, access::readwrite>;
};

} // namespace bitfilled

#endif // __BITFILLED_BITS_HPP__
