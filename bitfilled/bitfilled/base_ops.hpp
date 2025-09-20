// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_BASE_OPS_HPP__
#define __BITFILLED_BASE_OPS_HPP__

#include "bitfilled/access.hpp"
#include "bitfilled/size.hpp"

namespace bitfilled
{

// #define BITFILLED_ASSIGN_RETURNS_REF 1
#if BITFILLED_ASSIGN_RETURNS_REF
#define BITFILLED_ASSIGN_RETURN_DECL(EXPR) EXPR
#define BITFILLED_ASSIGN_RETURN_EXPR(EXPR) (EXPR)
#else
#define BITFILLED_ASSIGN_RETURN_DECL(EXPR) void
#define BITFILLED_ASSIGN_RETURN_EXPR(EXPR)
#endif

/// @brief  copy const-volatile qualifiers from one reference type to another
// https://stackoverflow.com/a/31173086
template <typename T, typename U>
struct copy_cv_reference
{
  private:
    using R = std::remove_reference_t<T>;
    using U1 = std::conditional_t<std::is_const<R>::value, std::add_const_t<U>, U>;
    using U2 = std::conditional_t<std::is_volatile<R>::value, std::add_volatile_t<U1>, U1>;
    using U3 =
        std::conditional_t<std::is_lvalue_reference<T>::value, std::add_lvalue_reference_t<U2>, U2>;
    using U4 =
        std::conditional_t<std::is_rvalue_reference<T>::value, std::add_rvalue_reference_t<U3>, U3>;

  public:
    using type = U4;
};
template <typename From, typename To>
using copy_cv_t = typename copy_cv_reference<From, To>::type;

/// @brief  The bitfield_props class stores the bitfield location information.
/// @tparam FIRST_BIT: the lowest occupied bit position
/// @tparam LAST_BIT: the highest occupied bit position
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
        return (T)(memory & ~(mask<T>() << offset())) | position_field(value);
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

/// @brief  The regbitfieldset_props class stores the bitfield location information for a set/array
/// of bit fields.
/// @tparam ITEM_SIZE: the size of each item in bits
/// @tparam ITEM_COUNT: the number of items in the set
/// @tparam OFFSET: the bit offset where the first (lowest position) item is located
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

struct base
{
    /// @brief  The bitfield_ops class defines the bitfield operations on its containing type
    /// @tparam T: The owner type, which provides the memory storage and access for bitfield
    ///         operations
    /// @tparam ACCESS: The type of access that is permitted on this memory location
    template <typename T, enum access ACCESS = access::readwrite>
    struct bitfield_ops
    {
        static constexpr enum access access() { return ACCESS; }
        using int_type = T::value_type;

      protected:
        template <typename Tptr>
        static auto getter(Tptr& ptr)
        {
            return static_cast<int_type>((copy_cv_t<Tptr&, T>)ptr);
        }
        template <typename Tptr>
        static void setter(Tptr& ptr, int_type v)
        {
            if constexpr (std::is_void_v<decltype((copy_cv_t<Tptr&, T>)ptr = v)>)
            {
                (copy_cv_t<Tptr&, T>)ptr = v;
            }
            else
            {
                // the assignment might return a volatile-qualified reference
                // avoid reading it by keeping it a reference, and casting away the qualifier
                // all this is to avoid warnings
                [[maybe_unused]] auto& _ = const_cast<T&>((copy_cv_t<Tptr&, T>)ptr = v);
            }
        }

      public:
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bf.position_field(v));
            }
            else
            {
                setter(bf, bf.insert_field(getter(bf), v));
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
                setter(bf, bf.position_field(v));
            }
            else
            {
                setter(bf, bf.insert_field(getter(bf), v));
            }
        }

        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(bf.extract_field(getter(bf)));
            return bf.sign_extend(x);
        }
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(bf.extract_field(getter(bf)));
            return bf.sign_extend(x);
        }

        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bf.position_field(v, index));
            }
            else
            {
                setter(bf, bf.insert_field(getter(bf), v, index));
            }
        }
        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bf.position_field(v, index));
            }
            else
            {
                setter(bf, bf.insert_field(getter(bf), v, index));
            }
        }

        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(bf.extract_field(getter(bf), index));
            return bf.sign_extend(x);
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(bf.extract_field(getter(bf), index));
            return bf.sign_extend(x);
        }
    };
};

} // namespace bitfilled

#endif // __BITFILLED_BASE_OPS_HPP__
