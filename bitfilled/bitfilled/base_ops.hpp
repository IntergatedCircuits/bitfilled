// SPDX-License-Identifier: MPL-2.0
#pragma once

#include <algorithm>
#include <limits>
#include "bitfilled/access.hpp"
#include "bitfilled/macros.hpp"
#include "bitfilled/size.hpp"

/// @brief  Runtime precondition check used by the library (e.g. bit field set index bounds).
///         Define it before including bitfilled to route failures to a custom handler,
///         or to an empty expression to disable the checks.
#ifndef BITFILLED_ASSERT
#include <cassert>
#define BITFILLED_ASSERT(EXPR) assert(EXPR)
#endif

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

#if _MSC_VER
#define BITFILLED_FIELD_PROPS_PARAM_T T
#define BITFILLED_FIELDSET_PROPS_PARAM_T T
#else
#define BITFILLED_FIELD_PROPS_PARAM_T bitfield_props<FIRST_BIT, LAST_BIT>
#define BITFILLED_FIELDSET_PROPS_PARAM_T regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>
#endif

/// @brief  copy const-volatile qualifiers from one reference type to another
// https://stackoverflow.com/a/31173086
template <typename T, typename U>
struct copy_cv_reference
{
  private:
    using R = std::remove_reference_t<T>;
    using U1 = std::conditional_t<std::is_const_v<R>, std::add_const_t<U>, U>;
    using U2 = std::conditional_t<std::is_volatile_v<R>, std::add_volatile_t<U1>, U1>;
    using U3 =
        std::conditional_t<std::is_lvalue_reference_v<T>, std::add_lvalue_reference_t<U2>, U2>;
    using U4 =
        std::conditional_t<std::is_rvalue_reference_v<T>, std::add_rvalue_reference_t<U3>, U3>;

  public:
    using type = U4;
};
template <typename From, typename To>
using copy_cv_t = typename copy_cv_reference<From, To>::type;

/// @brief  The number of bits a bit field value of type T can hold (1 for bool).
template <typename T>
constexpr inline std::size_t value_bits =
    std::is_same_v<std::remove_cv_t<T>, bool>
        ? 1
        : sizeof(T) * static_cast<std::size_t>(std::numeric_limits<unsigned char>::digits);

/// @brief  Bit field values are restricted to the types that standard bit fields support.
template <typename T>
concept BitfieldValue =
    (std::integral<T> or std::is_enum_v<T>) and std::is_same_v<T, std::remove_cv_t<T>>;

namespace detail
{
template <typename TOps>
constexpr std::size_t storage_bits()
{
    if constexpr (requires { TOps::storage_bits(); })
    {
        return TOps::storage_bits();
    }
    else if constexpr (requires { typename TOps::int_type; })
    {
        return value_bits<typename TOps::int_type>;
    }
    else
    {
        return std::numeric_limits<std::size_t>::max();
    }
}
} // namespace detail

/// @brief  The bit range [FIRST_BIT, LAST_BIT] is within the storage of the bit field operations.
template <typename TOps, std::size_t FIRST_BIT, std::size_t LAST_BIT>
concept BitRangeFitsStorage = (FIRST_BIT <= LAST_BIT) and (LAST_BIT < detail::storage_bits<TOps>());

/// @brief  The value type can represent all bits of the bit range [FIRST_BIT, LAST_BIT].
template <typename T, std::size_t FIRST_BIT, std::size_t LAST_BIT>
concept BitRangeFitsValue =
    BitfieldValue<T> and (FIRST_BIT <= LAST_BIT) and (1 + LAST_BIT - FIRST_BIT <= value_bits<T>);

/// @brief  The access rights of a bit field are limited by the access rights of its storage.
template <enum access ACCESS, typename TOps>
inline constexpr enum access effective_access = []() constexpr
{
    if constexpr (DefinesAccess<TOps>)
    {
        return ACCESS & TOps::access();
    }
    else
    {
        return ACCESS;
    }
}();

/// @brief  Bit field operations that also identify the class owning the bit fields.
///         The bit fields grant their owner exclusive access to their (trivial) copy constructor,
///         which keeps the owner copyable, while preventing copies of the bit fields elsewhere.
///         Use it through the @ref BF_OWNER (or @ref BF_COPY_SUPERCLASS) macro.
template <typename TOps, typename TOwner>
struct owned_ops : public TOps
{
    using owner_type = TOwner;
};

namespace detail
{
struct no_owner
{};
template <typename TOps>
struct owner_of
{
    using type = no_owner;
};
template <typename TOps>
    requires requires { typename TOps::owner_type; }
struct owner_of<TOps>
{
    using type = TOps::owner_type;
};
} // namespace detail

/// @brief  The bitfield_props class stores the bitfield location information.
/// @tparam FIRST_BIT: the lowest occupied bit position
/// @tparam LAST_BIT: the highest occupied bit position
template <std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>
struct bitfield_props
{
    static_assert(LAST_BIT >= FIRST_BIT);

    constexpr static std::size_t size_bits() { return 1 + LAST_BIT - FIRST_BIT; }
    constexpr static std::size_t offset() { return FIRST_BIT; }
    template <std::integral T = unsigned>
        requires(!std::is_same_v<T, bool>)
    constexpr static T mask()
    {
        using unsigned_type = std::make_unsigned_t<T>;
        static_assert(size_bits() <= value_bits<T>, "bit field is wider than the mask type");
        if constexpr (size_bits() == value_bits<T>)
        {
            return static_cast<T>(static_cast<unsigned_type>(~unsigned_type{}));
        }
        else
        {
            return static_cast<T>((unsigned_type{1} << size_bits()) - 1U);
        }
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
            } const storage{.field = v};
            return storage.field;
        }
        else
        {
            return v;
        }
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

    constexpr static std::size_t offset(std::size_t index)
    {
        BITFILLED_ASSERT(index < ITEM_COUNT);
        return base::offset() + (index * base::size_bits());
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
        /// @return the number of bits accessible through int_type (packed integers might
        ///         store fewer bytes than the size of their integral type)
        static constexpr std::size_t storage_bits()
        {
            return std::min(value_bits<int_type>, value_bits<unsigned char> * sizeof(T));
        }

      protected:
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static constexpr void check_field()
        {
            static_assert(BitfieldValue<TVal>,
                          "bit field value type must be a cv-unqualified integral or enum type");
            static_assert(BitRangeFitsStorage<bitfield_ops, FIRST_BIT, LAST_BIT>,
                          "bit field exceeds the width of its storage");
            static_assert(BitRangeFitsValue<TVal, FIRST_BIT, LAST_BIT>,
                          "bit field is wider than its value type");
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static constexpr void check_items()
        {
            static_assert(BitfieldValue<TVal>,
                          "bit field value type must be a cv-unqualified integral or enum type");
            static_assert(
                BitRangeFitsStorage<bitfield_ops, OFFSET, OFFSET + (ITEM_SIZE * ITEM_COUNT) - 1>,
                "bit field set exceeds the width of its storage");
            static_assert(BitRangeFitsValue<TVal, 0, ITEM_SIZE - 1>,
                          "bit field set item is wider than its value type");
        }

        /// @brief  The bit field properties are located at the address of their owner object.
        template <typename Tptr>
        static copy_cv_t<Tptr&, T> owner(Tptr& ptr)
        {
            // cast through void* as the empty bit field type has a smaller alignment requirement
            return *static_cast<std::add_pointer_t<copy_cv_t<Tptr, T>>>(
                static_cast<std::add_pointer_t<copy_cv_t<Tptr, void>>>(&ptr));
        }
        template <typename Tptr>
        static auto getter(Tptr& ptr)
        {
            return static_cast<int_type>(owner(ptr));
        }
        template <typename Tptr>
        static void setter(Tptr& ptr, int_type v)
        {
            // NOLINTNEXTLINE(bugprone-assignment-in-if-condition)
            if constexpr (std::is_void_v<decltype(owner(ptr) = v)>)
            {
                owner(ptr) = v;
            }
            else
            {
                // the assignment might return a volatile-qualified reference
                // avoid reading it by keeping it a reference, and casting away the qualifier
                // all this is to avoid warnings
                // NOLINTNEXTLINE(readability-identifier-length)
                [[maybe_unused]] auto& _ = const_cast<T&>(owner(ptr) = v);
            }
        }

      public:
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            check_field<TVal, FIRST_BIT, LAST_BIT>();
            const auto intval = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::position_field(intval));
            }
            else
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::insert_field(getter(bf), intval));
            }
        }
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(volatile BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            check_field<TVal, FIRST_BIT, LAST_BIT>();
            const auto intval = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::position_field(intval));
            }
            else
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::insert_field(getter(bf), intval));
            }
        }

        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const BITFILLED_FIELD_PROPS_PARAM_T& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            check_field<TVal, FIRST_BIT, LAST_BIT>();
            auto typeval =
                static_cast<TVal>(bitfield_props<FIRST_BIT, LAST_BIT>::extract_field(getter(bf)));
            return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(typeval);
        }
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const volatile BITFILLED_FIELD_PROPS_PARAM_T& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            check_field<TVal, FIRST_BIT, LAST_BIT>();
            auto typeval =
                static_cast<TVal>(bitfield_props<FIRST_BIT, LAST_BIT>::extract_field(getter(bf)));
            return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(typeval);
        }

        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
            const auto intval = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::position_field(
                               intval, index));
            }
            else
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::insert_field(
                               getter(bf), intval, index));
            }
        }
        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(volatile BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index,
                             TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
            const auto intval = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::position_field(
                               intval, index));
            }
            else
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::insert_field(
                               getter(bf), intval, index));
            }
        }

        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
            auto typeval = static_cast<TVal>(
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::extract_field(getter(bf),
                                                                                   index));
            return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(typeval);
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const volatile BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
            auto typeval = static_cast<TVal>(
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::extract_field(getter(bf),
                                                                                   index));
            return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(typeval);
        }
    };
};

} // namespace bitfilled
