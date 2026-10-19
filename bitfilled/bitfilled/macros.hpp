// SPDX-License-Identifier: MPL-2.0
#pragma once

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

/// @brief Inherit the superclass constructors and assignment operators.
/// @details Intended for subclasses of a type that defines a `superclass` alias, such as
/// `host_integer`. Without this macro, a derived type can remain an aggregate. GCC rejects
/// designated initialization of `BF_BITS` members in that case because those members are
/// `[[no_unique_address]]` bitfield descriptors with a deleted move constructor.
///
/// Inheriting constructors makes the derived type non-aggregate, so designated initializers are
/// unavailable. The inherited base constructor can initialize the underlying value, after which
/// bitfields can be assigned normally. The inherited assignment operator also allows assigning a
/// volatile source object to a non-volatile destination; it does not make a volatile destination
/// assignable from a non-volatile source.
/// @param CLASS the current class name
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
