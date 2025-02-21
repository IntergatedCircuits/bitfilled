// SPDX-License-Identifier: MPL-2.0
#ifndef __BITFILLED_MACROS_HPP__
#define __BITFILLED_MACROS_HPP__

#define BF_BITS(TYPE, ...) [[no_unique_address]] ::bitfilled::bitfield<TYPE, ops, __VA_ARGS__>

#define BF_BITSET(TYPE, ...) [[no_unique_address]] ::bitfilled::bitfieldset<TYPE, ops, __VA_ARGS__>

#define BF_MMREGBITS_TYPE(TYPE, ACCESS, NAME, ...)                                                 \
    using NAME = ::bitfilled::regbitfield<TYPE, ops, ::bitfilled::access::ACCESS, __VA_ARGS__>;    \
    [[no_unique_address]] NAME

#define BF_MMREGBITS(TYPE, ACCESS, ...)                                                            \
    [[no_unique_address]] ::bitfilled::regbitfield<TYPE, ops, ::bitfilled::access::ACCESS,         \
                                                   __VA_ARGS__>

#define BF_MMREGBITSET(TYPE, ACCESS, ...)                                                          \
    [[no_unique_address]] ::bitfilled::regbitfieldset<TYPE, ops, ::bitfilled::access::ACCESS,      \
                                                      __VA_ARGS__>

#define BF_MMREG(TYPE, ACCESS) public ::bitfilled::mmreg<TYPE, ::bitfilled::access::ACCESS>

#define BF_MMREG_BB(TYPE, ACCESS) public ::bitfilled::mmreg<TYPE, ::bitfilled::access::ACCESS,     \
    ::bitfilled::bitband_ops<TYPE, ::bitfilled::access::ACCESS, PERIPH_BASE>>

#endif // __BITFILLED_MACROS_HPP__
