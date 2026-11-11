// SPDX-License-Identifier: MPL-2.0

// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <limits>
// SPDX-License-Identifier: MPL-2.0

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

constexpr enum access operator&(enum access lhs, enum access rhs)
{
    return static_cast<enum access>(static_cast<std::uint_fast8_t>(lhs) &
                                    static_cast<std::uint_fast8_t>(rhs));
}
constexpr enum access operator|(enum access lhs, enum access rhs)
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

// SPDX-License-Identifier: MPL-2.0

#define BF_CONCAT_IMPL(X, Y) X##Y
#define BF_CONCAT(X, Y) BF_CONCAT_IMPL(X, Y)
#define BF_UNIQUE_NAME(NAME) BF_CONCAT(NAME, __LINE__)

#if _MSC_VER
#define BITFILLED_OPS_FORWARDING                                                                   \
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>              \
    void _set_field(TVal value)                                                                    \
    {                                                                                              \
        bf_ops::template set_field<FIRST_BIT, LAST_BIT>(*this, value);                             \
    }                                                                                              \
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>              \
    void _set_field(TVal value) volatile                                                           \
    {                                                                                              \
        bf_ops::template set_field<FIRST_BIT, LAST_BIT>(*this, value);                             \
    }                                                                                              \
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>              \
    auto _get_field() const                                                                        \
    {                                                                                              \
        return bf_ops::template get_field<TVal, FIRST_BIT, LAST_BIT>(*this);                       \
    }                                                                                              \
    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT = FIRST_BIT>              \
    auto _get_field() const volatile                                                               \
    {                                                                                              \
        return bf_ops::template get_field<TVal, FIRST_BIT, LAST_BIT>(*this);                       \
    }                                                                                              \
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>    \
    void _set_item(std::size_t index, TVal value)                                                  \
    {                                                                                              \
        bf_ops::template set_item<ITEM_SIZE, ITEM_COUNT, OFFSET>(*this, index, value);             \
    }                                                                                              \
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>    \
    void _set_item(std::size_t index, TVal value) volatile                                         \
    {                                                                                              \
        bf_ops::template set_item<ITEM_SIZE, ITEM_COUNT, OFFSET>(*this, index, value);             \
    }                                                                                              \
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>    \
    auto _get_item(std::size_t index) const                                                        \
    {                                                                                              \
        return bf_ops::template get_item<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>(*this, index);       \
    }                                                                                              \
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>    \
    auto _get_item(std::size_t index) const volatile                                               \
    {                                                                                              \
        return bf_ops::template get_item<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>(*this, index);       \
    }

#define BF_BITS(TYPE, ...)                                                                         \
    TYPE BF_UNIQUE_NAME(_getter)() const                                                           \
    {                                                                                              \
        return this->template _get_field<TYPE, __VA_ARGS__>();                                     \
    }                                                                                              \
    void BF_UNIQUE_NAME(_setter)(TYPE v)                                                           \
    {                                                                                              \
        this->template _set_field<TYPE, __VA_ARGS__>(v);                                           \
    }                                                                                              \
    __declspec(property(get = BF_UNIQUE_NAME(_getter), put = BF_UNIQUE_NAME(_setter))) TYPE

#define BF_BITSET(TYPE, ...)                                                                       \
    TYPE BF_UNIQUE_NAME(_getter)(std::size_t i) const                                              \
    {                                                                                              \
        return this->template _get_item<TYPE, __VA_ARGS__>(i);                                     \
    }                                                                                              \
    void BF_UNIQUE_NAME(_setter)(std::size_t i, TYPE v)                                            \
    {                                                                                              \
        this->template _set_item<TYPE, __VA_ARGS__>(i, v);                                         \
    }                                                                                              \
    __declspec(property(get = BF_UNIQUE_NAME(_getter), put = BF_UNIQUE_NAME(_setter))) TYPE

#define BF_BITSET_POSTFIX []

#else

#define BITFILLED_OPS_FORWARDING
#define BF_BITS(TYPE, ...) [[no_unique_address]] ::bitfilled::bitfield<TYPE, bf_ops, __VA_ARGS__>

#define BF_BITSET(TYPE, ...)                                                                       \
    [[no_unique_address]] ::bitfilled::bitfieldset<TYPE, bf_ops, __VA_ARGS__>

#define BF_BITSET_POSTFIX

#define BF_MMREGBITS_TYPE(TYPE, ACCESS, NAME, ...)                                                 \
    using NAME = ::bitfilled::regbitfield<TYPE, bf_ops, ::bitfilled::access::ACCESS, __VA_ARGS__>; \
    [[no_unique_address]] NAME

#define BF_MMREGBITS(TYPE, ACCESS, ...)                                                            \
    [[no_unique_address]] ::bitfilled::regbitfield<TYPE, bf_ops, ::bitfilled::access::ACCESS,      \
                                                   __VA_ARGS__>

#define BF_MMREGBITSET(TYPE, ACCESS, ...)                                                          \
    [[no_unique_address]] ::bitfilled::regbitfieldset<TYPE, bf_ops, ::bitfilled::access::ACCESS,   \
                                                      __VA_ARGS__>

#endif

/// @brief Check that the bit field owner class doesn't add storage to its superclass.
/// @details The class is incomplete at class scope, so the check is placed in a member function
/// body, where it is evaluated once the class is complete (for non-template classes only).
/// @param CLASS the current class name
#define BF_OWNER_SIZE_CHECK(CLASS)                                                                 \
    static constexpr void bf_owner_size_check_() noexcept                                          \
    {                                                                                              \
        static_assert(sizeof(CLASS) == sizeof(superclass),                                         \
                      "owner grew: has data members or identical bit field types");                \
    }

/// @brief Declare the current class as the owner of its bit fields.
/// @details Must precede the bit field declarations, in a class derived from a type that defines
/// a `superclass` alias (such as `host_integer`, `packed_integer` or `mmreg`). The bit fields
/// only allow their owner to copy them, so that the owner remains (trivially) copy constructible,
/// while copying a bit field descriptor out of its owner (e.g. `auto v = obj.field;`) fails to
/// compile. Without an owner, the class containing bit fields is not copy constructible.
/// The class remains an aggregate.
/// The class must not add storage to its superclass, which is checked at compile time
/// (see @ref BF_OWNER_SIZE_CHECK).
/// @param CLASS the current class name
#if _MSC_VER
#define BF_OWNER(CLASS) BF_OWNER_SIZE_CHECK(CLASS)
#else
#define BF_OWNER(CLASS)                                                                            \
    BF_OWNER_SIZE_CHECK(CLASS)                                                                     \
    using bf_ops = ::bitfilled::owned_ops<typename superclass::bf_ops, CLASS>;
#endif

/// @brief Declare the current class template as the owner of its bit fields (see @ref BF_OWNER).
/// @details A class template can't use @ref BF_OWNER, as its `superclass` alias is inherited from
/// a dependent base, so the base must be named explicitly.
/// @param CLASS the current class (template) name
/// @param ... the base class, e.g. `bitfilled::bytes<SIZE, ENDIAN>`
#if _MSC_VER
#define BF_TEMPLATE_OWNER(CLASS, ...)
#else
#define BF_TEMPLATE_OWNER(CLASS, ...)                                                              \
    using bf_ops = ::bitfilled::owned_ops<typename __VA_ARGS__::bf_ops, CLASS>;
#endif

/// @brief Declare the current class as the owner of its bit fields (see @ref BF_OWNER),
/// and inherit the superclass constructors and assignment operators.
/// @details Intended for subclasses of a type that defines a `superclass` alias, such as
/// `host_integer`. Without this macro, a derived type can remain an aggregate. GCC rejects
/// designated initialization of `BF_BITS` members in that case because those members are
/// `[[no_unique_address]]` bitfield descriptors with a deleted move constructor.
///
/// Inheriting constructors makes the derived type non-aggregate, so designated initializers are
/// unavailable. The inherited base constructor can initialize the underlying value, after which
/// bitfields can be assigned normally. None of the constructors run bit field code, as the bit
/// field descriptors are trivially constructed. The inherited assignment operator also allows
/// assigning a volatile source object to a non-volatile destination; it does not make a volatile
/// destination assignable from a non-volatile source.
/// @param CLASS the current class name
#define BF_COPY_SUPERCLASS(CLASS)                                                                  \
    BF_OWNER(CLASS)                                                                                \
    using superclass::superclass;                                                                  \
    using superclass::operator=;

/// @brief Macro to define a memory-mapped register type with bitfields.
/// @param TYPE The underlying type of the register (e.g., uint32_t).
/// @param ACCESS The access type (e.g., rw, r, w).
/// @param ... Custom bitfield operations when desired (e.g., bitband<PERIPH_BASE>).
#define BF_MMREG(TYPE, ...)                                                                        \
  public                                                                                           \
    ::bitfilled::mmreg<TYPE, ::bitfilled::access::__VA_ARGS__>

#define BF_MMREG_RESERVED(WIDTH, SIZE)                                                             \
  private:                                                                                         \
    const ::std::array<::bitfilled::sized_unsigned_t<WIDTH>, SIZE> BF_UNIQUE_NAME(_reserved_);     \
                                                                                                   \
  public:

// SPDX-License-Identifier: MPL-2.0

#include <bit>
#include <concepts>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace bitfilled
{
/// @brief  The number of bits in a byte (the library requires 8-bit bytes, as std::uint8_t does).
inline constexpr std::size_t byte_bits = std::numeric_limits<std::uint8_t>::digits;

/// @brief  Lookup unsigned integer of matching size
template <std::size_t SIZE, class T = void>
struct sized_integer
{};

template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 1>>
{
    using unsigned_type = std::uint8_t;
    using signed_type = std::int8_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 2>>
{
    using unsigned_type = std::uint16_t;
    using signed_type = std::int16_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 4>>
{
    using unsigned_type = std::uint32_t;
    using signed_type = std::int32_t;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 8>>
{
    using unsigned_type = std::uint64_t;
    using signed_type = std::int64_t;
};

template <std::size_t SIZE>
using sized_unsigned_t = sized_integer<SIZE>::unsigned_type;

template <std::size_t SIZE>
using sized_signed_t = sized_integer<SIZE>::signed_type;

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
    else
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
    std::is_same_v<std::remove_cv_t<T>, bool> ? 1 : sizeof(T) * byte_bits;

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
        // a field as wide as its value type needs no extension
        if constexpr (std::is_signed_v<T> and (size_bits() < value_bits<T>))
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
            return std::min(value_bits<int_type>, byte_bits * sizeof(T));
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

namespace bitfilled
{
template <std::uintptr_t BASE_ADDRESS>
struct bitband
{
    /// @brief  These bitfield operations use bit-band memory access for single-bit manipulation,
    ///         as it is implemented on ARM Cortex M3/M4 CPU architectures.
    /// @note   These operations shall only be used on types that map directly to memory
    ///         ( @ref host_integer and @ref mmreg )
    /// @tparam T
    /// @tparam ACCESS
    template <typename T, enum access ACCESS>
    struct bitfield_ops : private base::bitfield_ops<T, ACCESS>
    {
        using base_ops = base::bitfield_ops<T, ACCESS>;

      public:
        using base_ops::access;
        using base_ops::storage_bits;
        using typename base_ops::int_type;

      private:
        static_assert((BASE_ADDRESS & 0x9fffffff) == 0);
        static constexpr std::uintptr_t BITBAND_BASE_ADDRESS = BASE_ADDRESS | 0x02000000;

        template <typename Tptr>
        static auto& bitmemory(Tptr& ptr, std::size_t bit_index)
        {
            auto address = BITBAND_BASE_ADDRESS                      // remapped base
                           | (((std::uintptr_t)&ptr & 0xfffff) << 5) // word offset
                           | (bit_index << 2);                       // bit offset
            return *((std::add_pointer_t<std::remove_reference_t<copy_cv_t<Tptr&, std::uint32_t>>>)
                         address);
        }

      public:
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
            requires(is_writeable<base_ops::access()>)
        {
            if constexpr (FIRST_BIT == LAST_BIT)
            {
                base_ops::template check_field<TVal, FIRST_BIT, LAST_BIT>();
                const auto intval = static_cast<base_ops::int_type>(value);
                bitmemory(bf, FIRST_BIT) = intval;
            }
            else
            {
                return base_ops::set_field(bf, value);
            }
        }
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf, TVal value)
            requires(is_writeable<base_ops::access()>)
        {
            if constexpr (FIRST_BIT == LAST_BIT)
            {
                base_ops::template check_field<TVal, FIRST_BIT, LAST_BIT>();
                const auto intval = static_cast<base_ops::int_type>(value);
                bitmemory(bf, FIRST_BIT) = intval;
            }
            else
            {
                return base_ops::set_field(bf, value);
            }
        }
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<base_ops::access()>)
        {
            if constexpr (FIRST_BIT == LAST_BIT)
            {
                base_ops::template check_field<TVal, FIRST_BIT, LAST_BIT>();
                auto typeval = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
                return bf.sign_extend(typeval);
            }
            else
            {
                return base_ops::template get_field<TVal>(bf);
            }
        }
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const volatile bitfield_props<FIRST_BIT, LAST_BIT>& bf)
            requires(is_readable<base_ops::access()>)
        {
            if constexpr (FIRST_BIT == LAST_BIT)
            {
                base_ops::template check_field<TVal, FIRST_BIT, LAST_BIT>();
                auto typeval = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
                return bf.sign_extend(typeval);
            }
            else
            {
                return base_ops::template get_field<TVal>(bf);
            }
        }
        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index, TVal value)
            requires(is_writeable<base_ops::access()>)
        {
            if constexpr (ITEM_SIZE == 1)
            {
                base_ops::template check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
                const auto intval = static_cast<base_ops::int_type>(value);
                bitmemory(bf, bf.offset(index)) = intval;
            }
            else
            {
                return base_ops::set_item(bf, index, value);
            }
        }
        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index, TVal value)
            requires(is_writeable<base_ops::access()>)
        {
            if constexpr (ITEM_SIZE == 1)
            {
                base_ops::template check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
                const auto intval = static_cast<base_ops::int_type>(value);
                bitmemory(bf, bf.offset(index)) = intval;
            }
            else
            {
                return base_ops::set_item(bf, index, value);
            }
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index)
            requires(is_readable<base_ops::access()>)
        {
            if constexpr (ITEM_SIZE == 1)
            {
                base_ops::template check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
                auto typeval = static_cast<TVal>(bitmemory(bf, bf.offset(index)));
                return bf.sign_extend(typeval);
            }
            else
            {
                return base_ops::template get_item<TVal>(bf, index);
            }
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const volatile regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>& bf,
                             std::size_t index)
            requires(is_readable<base_ops::access()>)
        {
            if constexpr (ITEM_SIZE == 1)
            {
                base_ops::template check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
                auto typeval = static_cast<TVal>(bitmemory(bf, bf.offset(index)));
                return bf.sign_extend(typeval);
            }
            else
            {
                return base_ops::template get_item<TVal>(bf, index);
            }
        }
    };
};

} // namespace bitfilled

// SPDX-License-Identifier: MPL-2.0

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

// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <span>
#include <utility>

namespace bitfilled
{
namespace detail
{
/// @brief  The smallest unsigned integer type that holds a bit field of WIDTH bits.
/// @note   Words narrower than int are promoted to int in arithmetic, which is safe: the shifted
///         intermediate values (a byte or a WIDTH bit value, shifted by less than WIDTH bits) stay
///         below 2^15 for std::uint8_t words, and below 2^24 for std::uint16_t words (which are
///         only promoted to int if it is wider than 16 bits). Results are cast back to the word
///         type.
template <std::size_t WIDTH>
using bytes_word_t = sized_unsigned_t<std::bit_ceil((WIDTH + byte_bits - 1) / byte_bits)>;

template <typename TWord, std::size_t WIDTH>
constexpr TWord low_mask()
{
    static_assert((WIDTH > 0) and (WIDTH <= value_bits<TWord>));
    constexpr auto all_ones = static_cast<TWord>(~TWord{});
    return static_cast<TWord>(all_ones >> (value_bits<TWord> - WIDTH));
}

/// @brief  The layout of the bytes in a bit field set's items.
enum class item_layout : std::uint_fast8_t
{
    any,          ///< items may span multiple bytes, at any bit offset
    single_byte,  ///< each item is contained within a single byte
    byte_aligned, ///< each item consists of whole bytes
};

/// @brief  Reads and writes bit fields located in a byte array.
/// @details The bit numbering follows the endianness convention of network protocol specifications:
///          - little endian: LSB0, bit N is (1 << (N % 8)) of byte (N / 8),
///            the bit field's least significant bit is FIRST_BIT
///          - big endian: MSB0, bit N is (0x80 >> (N % 8)) of byte (N / 8),
///            the bit field's most significant bit is FIRST_BIT
///          Either way, a bit field is a contiguous [FIRST_BIT, LAST_BIT] range, its bytes ordered
///          according to the endianness. The bytes of a field are indexed by their significance
///          (k = 0 is the least significant byte), the field value is shifted left
///          by `shift` bits within the bytes.
template <std::endian ENDIAN>
    requires((ENDIAN == std::endian::little) or (ENDIAN == std::endian::big))
struct byte_codec
{
    static constexpr bool little = ENDIAN == std::endian::little;

    static constexpr unsigned shift_of(std::size_t first_bit, std::size_t last_bit)
    {
        return static_cast<unsigned>(little ? (first_bit % byte_bits)
                                            : (byte_bits - 1 - (last_bit % byte_bits)));
    }

    /// @return the bits of the k-th least significant byte of a field, at their position
    template <typename TWord>
    static constexpr TWord gather(std::uint8_t byte, std::size_t k, unsigned shift)
    {
        if (k == 0)
        {
            return static_cast<TWord>(static_cast<TWord>(byte) >> shift);
        }
        return static_cast<TWord>(static_cast<TWord>(byte) << ((byte_bits * k) - shift));
    }
    /// @return the k-th least significant byte of the field value
    template <typename TWord>
    static constexpr std::uint8_t scatter(TWord value, std::size_t k, unsigned shift)
    {
        if (k == 0)
        {
            return static_cast<std::uint8_t>(value << shift);
        }
        return static_cast<std::uint8_t>(value >> ((byte_bits * k) - shift));
    }

    /// @brief  The compile-time layout of a bit field.
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    struct layout
    {
        static constexpr std::size_t width = 1 + LAST_BIT - FIRST_BIT;
        using word = bytes_word_t<width>;
        static constexpr std::size_t first_byte = FIRST_BIT / byte_bits;
        static constexpr std::size_t last_byte = LAST_BIT / byte_bits;
        static constexpr std::size_t count = 1 + last_byte - first_byte;
        static constexpr unsigned shift = shift_of(FIRST_BIT, LAST_BIT);
        static constexpr word mask = low_mask<word, width>();
        /// the field ends at the top of its most significant byte, reading needs no masking
        static constexpr bool top_aligned = (byte_bits * count) == (shift + width);

        static constexpr std::size_t index(std::size_t k)
        {
            return little ? (first_byte + k) : (last_byte - k);
        }
    };

    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr auto read(std::span<const std::uint8_t> data)
    {
        using L = layout<FIRST_BIT, LAST_BIT>;
        using word = L::word;
        if constexpr (L::count == 1)
        {
            // single byte access, no assembly of bytes
            const auto byte = static_cast<std::uint8_t>(data[L::first_byte] >> L::shift);
            if constexpr (L::top_aligned)
            {
                return static_cast<word>(byte);
            }
            else
            {
                return static_cast<word>(byte & static_cast<std::uint8_t>(L::mask));
            }
        }
        else
        {
            const auto value = [data]<std::size_t... K>(std::index_sequence<K...>)
            {
                return static_cast<word>((gather<word>(data[L::index(K)], K, L::shift) | ...));
            }(std::make_index_sequence<L::count>{});
            if constexpr (L::top_aligned)
            {
                return value;
            }
            else
            {
                return static_cast<word>(value & L::mask);
            }
        }
    }

    template <typename L, std::size_t K>
    static constexpr void write_byte(std::span<std::uint8_t> data, L::word value)
    {
        constexpr auto byte_mask = scatter(L::mask, K, L::shift);
        std::uint8_t& byte = data[L::index(K)];
        if constexpr (byte_mask == 0xffU)
        {
            // the whole byte belongs to the field, no read is necessary
            byte = scatter(value, K, L::shift);
        }
        else
        {
            byte = static_cast<std::uint8_t>((byte & ~byte_mask) |
                                             (scatter(value, K, L::shift) & byte_mask));
        }
    }

    template <std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr void write(std::span<std::uint8_t> data,
                                layout<FIRST_BIT, LAST_BIT>::word value)
    {
        using L = layout<FIRST_BIT, LAST_BIT>;
        if constexpr (L::count == 1)
        {
            write_byte<L, 0>(data, value);
        }
        else
        {
            [data, value]<std::size_t... K>(std::index_sequence<K...>)
            {
                (write_byte<L, K>(data, value), ...);
            }(std::make_index_sequence<L::count>{});
        }
    }

    /// @brief  Read a WIDTH bits wide field, located at a run-time bit position.
    /// @note   The bit position is bounds checked by the caller (bit field set index assertion).
    template <std::size_t WIDTH, item_layout LAYOUT = item_layout::any>
    static constexpr bytes_word_t<WIDTH> read(std::span<const std::uint8_t> data,
                                              std::size_t first_bit)
    {
        using word = bytes_word_t<WIDTH>;
        constexpr word mask = low_mask<word, WIDTH>();
        const std::size_t last_bit = first_bit + WIDTH - 1;
        const std::size_t first_byte = first_bit / byte_bits;
        if constexpr (LAYOUT == item_layout::single_byte)
        {
            return static_cast<word>(
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                (static_cast<word>(data[first_byte]) >> shift_of(first_bit, last_bit)) & mask);
        }
        else
        {
            const std::size_t last_byte = last_bit / byte_bits;
            const std::size_t count = 1 + last_byte - first_byte;
            const unsigned shift =
                (LAYOUT == item_layout::byte_aligned) ? 0U : shift_of(first_bit, last_bit);
            word value = 0;
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                const std::uint8_t byte = data[little ? (first_byte + k) : (last_byte - k)];
                value = static_cast<word>(value | gather<word>(byte, k, shift));
            }
            if constexpr (LAYOUT == item_layout::byte_aligned)
            {
                return value;
            }
            else
            {
                return static_cast<word>(value & mask);
            }
        }
    }

    /// @brief  Write a WIDTH bits wide field, located at a run-time bit position.
    template <std::size_t WIDTH, item_layout LAYOUT = item_layout::any>
    static constexpr void write(std::span<std::uint8_t> data, std::size_t first_bit,
                                bytes_word_t<WIDTH> value)
    {
        using word = bytes_word_t<WIDTH>;
        constexpr word mask = low_mask<word, WIDTH>();
        const std::size_t last_bit = first_bit + WIDTH - 1;
        const std::size_t first_byte = first_bit / byte_bits;
        const std::size_t last_byte = last_bit / byte_bits;
        const std::size_t count = 1 + last_byte - first_byte;
        if constexpr (LAYOUT == item_layout::byte_aligned)
        {
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                data[little ? (first_byte + k) : (last_byte - k)] = scatter(value, k, 0);
            }
        }
        else
        {
            const unsigned shift = shift_of(first_bit, last_bit);
            for (std::size_t k = 0; k < count; ++k)
            {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-avoid-unchecked-container-access)
                std::uint8_t& byte = data[little ? (first_byte + k) : (last_byte - k)];
                const auto byte_mask = scatter(mask, k, shift);
                byte = static_cast<std::uint8_t>((byte & ~byte_mask) |
                                                 (scatter(value, k, shift) & byte_mask));
            }
        }
    }
};
} // namespace detail

/// @brief  The bit field operations of byte arrays (see @ref bytes).
/// @tparam T: the owner type, which provides `endianness`, `size()` and `as_array()`
template <typename T>
struct bytes_ops
{
    /// @brief  The widest bit field value type (also used to copy bit field sets as a whole).
    using int_type = std::uint64_t;
    static constexpr std::size_t storage_bits() { return T::size() * byte_bits; }

  protected:
    using codec = detail::byte_codec<T::endianness>;

    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static constexpr void check_field()
    {
        static_assert(BitfieldValue<TVal>,
                      "bit field value type must be a cv-unqualified integral or enum type");
        static_assert(BitRangeFitsStorage<bytes_ops, FIRST_BIT, LAST_BIT>,
                      "bit field exceeds the width of its storage");
        static_assert(BitRangeFitsValue<TVal, FIRST_BIT, LAST_BIT>,
                      "bit field is wider than its value type");
        static_assert(LAST_BIT - FIRST_BIT < value_bits<int_type>,
                      "bit field is wider than the widest supported value type");
    }
    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static constexpr void check_items()
    {
        static_assert(BitfieldValue<TVal>,
                      "bit field value type must be a cv-unqualified integral or enum type");
        static_assert(BitRangeFitsStorage<bytes_ops, OFFSET, OFFSET + (ITEM_SIZE * ITEM_COUNT) - 1>,
                      "bit field set exceeds the width of its storage");
        static_assert(BitRangeFitsValue<TVal, 0, ITEM_SIZE - 1>,
                      "bit field set item is wider than its value type");
        static_assert(ITEM_SIZE <= value_bits<int_type>,
                      "bit field set item is wider than the widest supported value type");
    }
    template <std::size_t ITEM_SIZE, std::size_t OFFSET>
    static constexpr detail::item_layout items_layout()
    {
        if constexpr (((byte_bits % ITEM_SIZE) == 0) and ((OFFSET % ITEM_SIZE) == 0))
        {
            return detail::item_layout::single_byte;
        }
        else if constexpr (((ITEM_SIZE % byte_bits) == 0) and ((OFFSET % byte_bits) == 0))
        {
            return detail::item_layout::byte_aligned;
        }
        else
        {
            return detail::item_layout::any;
        }
    }

    /// @brief  The bit field properties are located at the address of their owner object.
    template <typename Tptr>
    static copy_cv_t<Tptr&, T> owner(Tptr& ptr)
    {
        // cast through void* as the empty bit field type has a smaller alignment requirement
        return *static_cast<std::add_pointer_t<copy_cv_t<Tptr, T>>>(
            static_cast<std::add_pointer_t<copy_cv_t<Tptr, void>>>(&ptr));
    }

  public:
    template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
    static void set_field(BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
    {
        check_field<TVal, FIRST_BIT, LAST_BIT>();
        using word = codec::template layout<FIRST_BIT, LAST_BIT>::word;
        codec::template write<FIRST_BIT, LAST_BIT>(owner(bf).as_array(), static_cast<word>(value));
    }

    template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
    static TVal get_field(const BITFILLED_FIELD_PROPS_PARAM_T& bf)
    {
        check_field<TVal, FIRST_BIT, LAST_BIT>();
        auto typeval =
            static_cast<TVal>(codec::template read<FIRST_BIT, LAST_BIT>(owner(bf).as_array()));
        return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(typeval);
    }

    template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
    static void set_item(BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index, TVal value)
    {
        check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
        codec::template write<ITEM_SIZE, items_layout<ITEM_SIZE, OFFSET>()>(
            owner(bf).as_array(),
            regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::offset(index),
            static_cast<detail::bytes_word_t<ITEM_SIZE>>(value));
    }

    template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
    static TVal get_item(const BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
    {
        check_items<TVal, ITEM_SIZE, ITEM_COUNT, OFFSET>();
        auto typeval =
            static_cast<TVal>(codec::template read<ITEM_SIZE, items_layout<ITEM_SIZE, OFFSET>()>(
                owner(bf).as_array(),
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::offset(index)));
        return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(typeval);
    }
};

/// @brief  bytes is a byte array with a defined endianness, typically representing (a part of)
///         a network protocol data unit, whose bit fields may span multiple bytes.
/// @details The bit fields follow the bit numbering convention of the endianness:
///          - little endian: LSB0, bit 0 is the least significant bit of byte 0,
///            multi-byte fields are little endian (e.g. Bluetooth LE, USB, CAN Intel signals)
///          - big endian: MSB0, bit 0 is the most significant bit of byte 0,
///            multi-byte fields are big endian (e.g. IETF RFC diagrams: IP, TCP, UDP)
///          The type has no alignment requirement and no padding, so it can be copied to
///          and from network buffers as-is.
/// @tparam SIZE: the number of bytes
/// @tparam ENDIAN: the byte order and bit numbering convention of the bit fields
template <std::size_t SIZE, std::endian ENDIAN>
    requires((SIZE > 0) and ((ENDIAN == std::endian::little) or (ENDIAN == std::endian::big)))
struct bytes
{
  protected: // NOLINTNEXTLINE(cppcoreguidelines-non-private-member-variables-in-classes)
    std::array<std::uint8_t, SIZE> storage{};

  public:
    using superclass = bytes;
    using bf_ops = bytes_ops<bytes>;
    using array_type = std::array<std::uint8_t, SIZE>;

    static constexpr auto endianness = ENDIAN;

    constexpr bytes() = default;
    constexpr explicit bytes(const array_type& arr) : storage(arr) {}
    // NOLINTNEXTLINE
    constexpr explicit bytes(const std::uint8_t (&arr)[SIZE])
    {
        std::copy_n(arr, SIZE, storage.data());
    }

    static constexpr std::size_t size() { return SIZE; }
    [[nodiscard]] constexpr std::uint8_t* data() { return storage.data(); }
    [[nodiscard]] constexpr std::span<const std::uint8_t> data() const { return storage.data(); }
    [[nodiscard]] constexpr array_type& as_array() { return storage; }
    [[nodiscard]] constexpr const array_type& as_array() const { return storage; }

    friend constexpr bool operator==(const bytes& lhs, const bytes& rhs)
    {
        return lhs.storage == rhs.storage;
    }

    BITFILLED_OPS_FORWARDING
};

} // namespace bitfilled

// SPDX-License-Identifier: MPL-2.0

// SPDX-License-Identifier: MPL-2.0

#include <algorithm>
#include <array>
#include <bit>
#include <concepts>
#include <cstdint>

namespace bitfilled
{

template <typename T>
concept Integral = std::is_integral_v<T>;

/// @brief  An array type for flexibly storing an integer value.
template <std::size_t SIZE>
struct integer_storage : public std::array<sized_unsigned_t<1>, SIZE>
{
    using base_type = std::array<sized_unsigned_t<1>, SIZE>;
    using base_type::operator=;
    static constexpr sized_unsigned_t<1> sign_extend_byte = 0xff;

    constexpr integer_storage() : base_type() {}
    constexpr explicit integer_storage(const base_type& arr) : base_type(arr) {}
    // NOLINTNEXTLINE
    constexpr explicit integer_storage(const sized_unsigned_t<1> (&arr)[SIZE])
    {
        std::copy_n(arr, SIZE, base_type::data());
    }
    template <std::integral T>
    constexpr explicit integer_storage(T value, std::endian endianness = std::endian::native)
        : base_type()
    {
        // if the value is signed, fill the target with sign extend bytes
        if constexpr (std::is_signed_v<T>)
        {
            if (value < static_cast<T>(0))
            {
                std::fill(this->begin(), this->end(), sign_extend_byte);
            }
        }

        // byteswap if the endian is not native
#if __cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            value = std::byteswap(value);
        }
#endif
        auto value_repr = std::bit_cast<std::array<sized_unsigned_t<1>, sizeof(T)>>(value);
#if !__cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            std::reverse(value_repr.begin(), value_repr.end());
        }
#endif
        // transfer the bytes to the correct position
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        constexpr long size_diff = (long)sizeof(T) - (long)SIZE;
        constexpr auto min_size = std::min(sizeof(T), SIZE);
        if (endianness == std::endian::little)
        {
            std::copy(value_repr.data(), value_repr.data() + min_size, this->data());
        }
        else
        {
            if constexpr (size_diff == 0)
            {
                *this = value_repr;
            }
            else if constexpr (size_diff > 0)
            {
                std::copy(value_repr.data() + size_diff, value_repr.data() + size_diff + SIZE,
                          this->data());
            }
            else // size_diff < 0
            {
                std::copy(value_repr.data(), value_repr.data() + sizeof(T),
                          this->data() - size_diff);
            }
        }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
    }

    template <std::integral T>
    [[nodiscard]] constexpr T to_integral(std::endian endianness = std::endian::native) const
    {
        integer_storage<sizeof(T)> value_repr;

        // if the value is signed, fill the target with sign extend bytes
        if constexpr (std::is_signed_v<T>)
        {
            if ((endianness == std::endian::little ? this->back() : this->front()) & 0x80U)
            {
                std::fill(value_repr.begin(), value_repr.end(), sign_extend_byte);
            }
        }

        // transfer the bytes to the correct position
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Warray-bounds"
#endif
        constexpr long size_diff = (long)SIZE - (long)sizeof(T);
        constexpr auto min_size = std::min(sizeof(T), SIZE);
        if (endianness == std::endian::little)
        {
            std::copy(this->data(), this->data() + min_size, value_repr.data());
        }
        else
        {
            if constexpr (size_diff == 0)
            {
                value_repr = *this;
            }
            else if constexpr (size_diff > 0)
            {
                std::copy(this->data() + size_diff, this->data() + size_diff + sizeof(T),
                          value_repr.data());
            }
            else // size_diff < 0
            {
                std::copy(this->data(), this->data() + SIZE, value_repr.data() - size_diff);
            }
        }
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

        // if not native endianness, reverse byte order
#if !__cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            std::reverse(value_repr.begin(), value_repr.end());
        }
#endif
        auto value = std::bit_cast<T>(value_repr);
#if __cpp_lib_byteswap
        if (endianness != std::endian::native)
        {
            value = std::byteswap(value);
        }
#endif
        return value;
    }
};

/// @brief  packed_integer stores an integer value in a packed byte array, with a defined
///         endianness.
/// @details The bit fields follow the bit numbering convention of @ref bytes:
///          LSB0 for little endian (bit 0 is the least significant bit of the value),
///          MSB0 for big endian (bit 0 is the most significant bit of the value).
/// @tparam ENDIAN: the endianness to use to convert between the integral value and the underlying
/// memory storage
/// @tparam SIZE: the integer storage size in octets
/// @tparam T: the native integral representation to use
template <std::endian ENDIAN, std::size_t SIZE, Integral T = sized_unsigned_t<std::bit_ceil(SIZE)>>
struct packed_integer : public bytes<SIZE, ENDIAN>
{
  private:
    using bytes_type = bytes<SIZE, ENDIAN>;
    using bytes_type::storage;

  public:
    using superclass = packed_integer;
    using bf_ops = bytes_ops<packed_integer>;
    using value_type = T;

    constexpr packed_integer() = default;
    constexpr packed_integer(value_type value)
        : bytes_type(integer_storage<SIZE>(value, bytes_type::endianness))
    {}
    // NOLINTNEXTLINE
    constexpr explicit packed_integer(const sized_unsigned_t<1> (&arr)[SIZE]) : bytes_type(arr) {}

    constexpr packed_integer& operator=(value_type value)
    {
        storage = integer_storage<SIZE>(value, bytes_type::endianness);
        return *this;
    }
    constexpr operator value_type() const
    {
        return integer_storage<SIZE>(storage).template to_integral<value_type>(
            bytes_type::endianness);
    }
    [[nodiscard]] constexpr std::array<sized_unsigned_t<1>, SIZE> to_array() const
    {
        return storage;
    }

    BITFILLED_OPS_FORWARDING
};

/// @brief  The host_integer class wraps an arithmetic type to allow subclassing it
///         (e.g. for the purpose of adding bitfields to it).
/// @tparam T: the arithmetic type to wrap
/// @tparam TOps: the type containing the bitfield_ops type for bitfield operations
template <Integral T, typename TOps = bitfilled::base>
struct host_integer
{
    using superclass = host_integer;
    using value_type = T;
    using bf_ops = typename TOps::template bitfield_ops<host_integer>;

    constexpr host_integer() = default;
    constexpr host_integer(T v) : raw_(v) {}
    constexpr host_integer& operator=(T other)
    {
        raw_ = other;
        return *this;
    }
    BITFILLED_OPS_FORWARDING

  private:
    T raw_{};

  public:
    constexpr operator auto&() { return raw_; }
    constexpr operator auto&() const { return raw_; }
    constexpr operator auto&() volatile { return raw_; }
    constexpr operator auto&() const volatile { return raw_; }
    // constexpr bool operator<=>(const defund&) const = default;
};

} // namespace bitfilled

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
    T raw{}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};
template <Integral T>
struct mmr_w
{
  protected:
    T raw{}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
};
template <Integral T>
struct mmr_rw
{
    constexpr operator auto &() { return raw; }
    constexpr operator auto &() volatile { return raw; }
    constexpr operator auto &() const { return raw; }
    constexpr operator auto &() const volatile { return raw; }

  protected:
    T raw{}; // NOLINT(cppcoreguidelines-non-private-member-variables-in-classes)
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

    ~mmreg() = default;
    mmreg(mmreg&&) = delete;
    mmreg& operator=(mmreg&&) = delete;

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

