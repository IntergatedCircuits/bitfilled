// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_MACROS_HPP__
#define __BITFILLED_MACROS_HPP__

#define BF_BITS(TYPE, ...) [[no_unique_address]] ::bitfilled::bitfield<TYPE, bf_ops, __VA_ARGS__>

#define BF_BITSET(TYPE, ...)                                                                       \
    [[no_unique_address]] ::bitfilled::bitfieldset<TYPE, bf_ops, __VA_ARGS__>

#define BF_COPY_SUPERCLASS(CLASS)                                                                  \
    using superclass::superclass;                                                                  \
    using superclass::operator=;

#define BF_MMREGBITS_TYPE(TYPE, ACCESS, NAME, ...)                                                 \
    using NAME = ::bitfilled::regbitfield<TYPE, bf_ops, ::bitfilled::access::ACCESS, __VA_ARGS__>; \
    [[no_unique_address]] NAME

#define BF_MMREGBITS(TYPE, ACCESS, ...)                                                            \
    [[no_unique_address]] ::bitfilled::regbitfield<TYPE, bf_ops, ::bitfilled::access::ACCESS,      \
                                                   __VA_ARGS__>

#define BF_MMREGBITSET(TYPE, ACCESS, ...)                                                          \
    [[no_unique_address]] ::bitfilled::regbitfieldset<TYPE, bf_ops, ::bitfilled::access::ACCESS,   \
                                                      __VA_ARGS__>

/// @brief Macro to define a memory-mapped register type with bitfields.
/// @param TYPE The underlying type of the register (e.g., uint32_t).
/// @param ACCESS The access type (e.g., rw, r, w).
/// @param ... Custom bitfield operations when desired (e.g., bitband<PERIPH_BASE>).
#define BF_MMREG(TYPE, ...) public ::bitfilled::mmreg<TYPE, ::bitfilled::access::__VA_ARGS__>

#define BF_CONCAT_IMPL(X, Y) X##Y
#define BF_CONCAT(X, Y) BF_CONCAT_IMPL(X, Y)
#define BF_UNIQUE_NAME(NAME) BF_CONCAT(NAME, __LINE__)

#define BF_MMREG_RESERVED(WIDTH, SIZE)                                                             \
  private:                                                                                         \
    const ::std::array<::bitfilled::sized_unsigned_t<WIDTH>, SIZE> BF_UNIQUE_NAME(_reserved_);     \
                                                                                                   \
  public:

#endif // __BITFILLED_MACROS_HPP__
