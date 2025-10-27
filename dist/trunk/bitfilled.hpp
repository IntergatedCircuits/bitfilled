// SPDX-License-Identifier: MPL-2.0

// SPDX-License-Identifier: MPL-2.0

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
        return _get_field<TYPE, __VA_ARGS__>();                                                    \
    }                                                                                              \
    void BF_UNIQUE_NAME(_setter)(TYPE v)                                                           \
    {                                                                                              \
        _set_field<TYPE, __VA_ARGS__>(v);                                                          \
    }                                                                                              \
    __declspec(property(get = BF_UNIQUE_NAME(_getter), put = BF_UNIQUE_NAME(_setter))) TYPE

#define BF_BITSET(TYPE, ...)                                                                       \
    TYPE BF_UNIQUE_NAME(_getter)(std::size_t i) const                                              \
    {                                                                                              \
        return _get_item<TYPE, __VA_ARGS__>(i);                                                    \
    }                                                                                              \
    void BF_UNIQUE_NAME(_setter)(std::size_t i, TYPE v)                                            \
    {                                                                                              \
        _set_item<TYPE, __VA_ARGS__>(i, v);                                                        \
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

#define BF_COPY_SUPERCLASS(CLASS)                                                                  \
    using superclass::superclass;                                                                  \
    using superclass::operator=;

/// @brief Macro to define a memory-mapped register type with bitfields.
/// @param TYPE The underlying type of the register (e.g., uint32_t).
/// @param ACCESS The access type (e.g., rw, r, w).
/// @param ... Custom bitfield operations when desired (e.g., bitband<PERIPH_BASE>).
#define BF_MMREG(TYPE, ...) public ::bitfilled::mmreg<TYPE, ::bitfilled::access::__VA_ARGS__>

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
/// @brief  Calculate the minimal number of bytes necessary to fit an integral value.
/// @tparam T the value's integral type
/// @param  c the input value
/// @return the necessary byte size to store the value
template <std::integral T>
constexpr inline std::size_t byte_width(T c)
{
    if constexpr (std::is_signed_v<T>)
    {
        auto x = static_cast<std::int64_t>(c);
        if ((x < std::numeric_limits<std::int32_t>::min()) or
            (x > std::numeric_limits<std::int32_t>::max()))
        {
            return 8;
        }
        if ((x < std::numeric_limits<std::int16_t>::min()) or
            (x > std::numeric_limits<std::int16_t>::max()))
        {
            return 4;
        }
        if ((x < std::numeric_limits<std::int8_t>::min()) or
            (x > std::numeric_limits<std::int8_t>::max()))
        {
            return 2;
        }
        return 1;
    }
    // else
    {
        auto x = static_cast<std::uint64_t>(c);
        return (x > std::numeric_limits<std::uint32_t>::max())   ? 8
               : (x > std::numeric_limits<std::uint16_t>::max()) ? 4
               : (x > std::numeric_limits<std::uint8_t>::max())  ? 2
                                                                 : 1;
    }
}

template <typename T>
constexpr inline std::size_t aligned_size = sizeof(T) / alignof(T);

} // namespace bitfilled

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
        static void set_field(BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::position_field(v));
            }
            else
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::insert_field(getter(bf), v));
            }
        }
        template <std::size_t FIRST_BIT, std::size_t LAST_BIT, typename TVal>
        static void set_field(volatile BITFILLED_FIELD_PROPS_PARAM_T& bf, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::position_field(v));
            }
            else
            {
                setter(bf, bitfield_props<FIRST_BIT, LAST_BIT>::insert_field(getter(bf), v));
            }
        }

        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const BITFILLED_FIELD_PROPS_PARAM_T& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x =
                static_cast<TVal>(bitfield_props<FIRST_BIT, LAST_BIT>::extract_field(getter(bf)));
            return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(x);
        }
        template <typename TVal, std::size_t FIRST_BIT, std::size_t LAST_BIT>
        static TVal get_field(const volatile BITFILLED_FIELD_PROPS_PARAM_T& bf)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x =
                static_cast<TVal>(bitfield_props<FIRST_BIT, LAST_BIT>::extract_field(getter(bf)));
            return bitfield_props<FIRST_BIT, LAST_BIT>::sign_extend(x);
        }

        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index, TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::position_field(
                               v, index));
            }
            else
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::insert_field(
                               getter(bf), v, index));
            }
        }
        template <std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET, typename TVal>
        static void set_item(volatile BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index,
                             TVal value)
            requires(is_writeable<bitfield_ops::access()>)
        {
            const auto v = static_cast<int_type>(value);
            if constexpr (!is_readable<bitfield_ops::access()> or
                          is_ephemeralwrite<bitfield_ops::access()>)
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::position_field(
                               v, index));
            }
            else
            {
                setter(bf, regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::insert_field(
                               getter(bf), v, index));
            }
        }

        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::extract_field(getter(bf),
                                                                                   index));
            return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(x);
        }
        template <typename TVal, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
        static TVal get_item(const volatile BITFILLED_FIELDSET_PROPS_PARAM_T& bf, std::size_t index)
            requires(is_readable<bitfield_ops::access()>)
        {
            auto x = static_cast<TVal>(
                regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::extract_field(getter(bf),
                                                                                   index));
            return regbitfieldset_props<ITEM_SIZE, ITEM_COUNT, OFFSET>::sign_extend(x);
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
      private:
        using base_ops = base::bitfield_ops<T, ACCESS>;
        using base_ops::access;
        using base_ops::int_type;
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
                const auto v = static_cast<base_ops::int_type>(value);
                bitmemory(bf, FIRST_BIT) = v;
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
                const auto v = static_cast<base_ops::int_type>(value);
                bitmemory(bf, FIRST_BIT) = v;
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
                auto x = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
                return bf.sign_extend(x);
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
                auto x = static_cast<TVal>(bitmemory(bf, FIRST_BIT));
                return bf.sign_extend(x);
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
                const auto v = static_cast<base_ops::int_type>(value);
                bitmemory(bf, OFFSET + index) = v;
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
                const auto v = static_cast<base_ops::int_type>(value);
                bitmemory(bf, OFFSET + index) = v;
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
                auto x = static_cast<TVal>(bitmemory(bf, OFFSET + index));
                return bf.sign_extend(x);
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
                auto x = static_cast<TVal>(bitmemory(bf, OFFSET + index));
                return bf.sign_extend(x);
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
        auto v = static_cast<T>(other);
        TOps::set_field((props_type&)*this, v);
        return BITFILLED_ASSIGN_RETURN_EXPR(
            (regbitfield_reference<regbitfield>{(props_type&)*this}));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto)
    operator=(const regbitfield & other) volatile
        requires(is_readwrite<ACCESS>)
    {
        auto v = static_cast<T>(other);
        TOps::set_field((volatile props_type&)*this, v);
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

    constexpr regbitfieldset() = default;

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
        reinterpret_cast<bf_type&>(*this) = reinterpret_cast<const bf_type&>(other);
        return BITFILLED_ASSIGN_RETURN_EXPR(reinterpret_cast<bf_type&>(*this));
    }
    // clang-format off
    BITFILLED_ASSIGN_RETURN_DECL(auto&)
    operator=(const regbitfieldset& other) volatile
        requires(is_readwrite<ACCESS>)
    {
        using bf_type = regbitfield<T, TOps, ACCESS, OFFSET, OFFSET + ITEM_COUNT * ITEM_SIZE - 1>;
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

/// @brief  Lookup unsigned integer of matching size
template <std::size_t SIZE, class T = void>
struct sized_integer
{};

template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 1>>
{
    typedef std::uint8_t unsigned_type;
    typedef std::int8_t signed_type;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 2>>
{
    typedef std::uint16_t unsigned_type;
    typedef std::int16_t signed_type;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 4>>
{
    typedef std::uint32_t unsigned_type;
    typedef std::int32_t signed_type;
};
template <std::size_t SIZE>
struct sized_integer<SIZE, std::enable_if_t<SIZE == 8>>
{
    typedef std::uint64_t unsigned_type;
    typedef std::int64_t signed_type;
};

template <std::size_t SIZE>
using sized_unsigned_t = typename sized_integer<SIZE>::unsigned_type;

template <std::size_t SIZE>
using sized_signed_t = typename sized_integer<SIZE>::signed_type;

/// @brief  An array type for flexibly storing an integer value.
template <std::size_t SIZE>
struct integer_storage : public std::array<sized_unsigned_t<1>, SIZE>
{
    using base_type = std::array<sized_unsigned_t<1>, SIZE>;
    using base_type::operator=;

    constexpr integer_storage() : base_type() {}

    template <std::integral T>
    constexpr explicit integer_storage(T value, std::endian endianness = std::endian::native)
        : base_type()
    {
        // if the value is signed, fill the target with sign extend bytes
        if (std::is_signed_v<T> and (value < static_cast<T>(0)))
        {
            std::fill(this->begin(), this->end(), 0xffu);
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
    }

    template <std::integral T>
    constexpr T to_integral(std::endian endianness = std::endian::native) const
    {
        integer_storage<sizeof(T)> value_repr;

        // if the value is signed, fill the target with sign extend bytes
        if (std::is_signed_v<T> and
            ((endianness == std::endian::little ? this->back() : this->front()) & 0x80u))
        {
            std::fill(value_repr.begin(), value_repr.end(), 0xffu);
        }

        // transfer the bytes to the correct position
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
/// @tparam ENDIAN: the endianness to use to convert between the integral value and the underlying
/// memory storage
/// @tparam SIZE: the integer storage size in octets
/// @tparam T: the native integral representation to use
template <std::endian ENDIAN, std::size_t SIZE, Integral T = sized_unsigned_t<std::bit_ceil(SIZE)>>
struct packed_integer
{
  private:
    integer_storage<SIZE> storage_;

  public:
    using superclass = packed_integer;
    using bf_ops = bitfilled::base::bitfield_ops<packed_integer>;
    using value_type = T;

    static constexpr auto endianness = ENDIAN;

    constexpr packed_integer() : storage_() {}
    constexpr packed_integer(value_type value) : storage_(value, endianness) {}
    constexpr packed_integer& operator=(value_type value)
    {
        storage_ = integer_storage<SIZE>(value, endianness);
        return *this;
    }
    constexpr operator value_type() const
    {
        return storage_.template to_integral<value_type>(endianness);
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
    constexpr operator auto &() { return raw_; }
    constexpr operator auto &() const { return raw_; }
    constexpr operator auto &() volatile { return raw_; }
    constexpr operator auto &() const volatile { return raw_; }
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

    BITFILLED_OPS_FORWARDING
};

} // namespace bitfilled

