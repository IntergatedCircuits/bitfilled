# ~~Bitfield~~ Bitfilled support for C++

These couple of C++20 headers provide a new, more flexible way of manipulating bit-fields.
While the current codebase is functional, it is overall still in the concept development phase,
with important features on the horizon. Feedback is very welcome!

## Introduction

Let's look at the C(++) language's [built-in bit field][stdbitfield] first:

```cpp
struct legacy
{
    bool boolean : 1;
    std::endian enumerated : 2;
    std::int32_t integer : 5;
};
```

We can manipulate each field as normal members of `struct legacy`, their values don't cross bit boundaries,
the signed integers even get sign extended, that's all well.
There are a number of drawbacks to using standard bitfields, such as:
1. Lack of standardization (and thus portability),
a C++ compiler implementation can freely decide how these bits are stored in memory
(see also [bitfield traits](./bitfilled/bitfield_traits.hpp) for an overview).
2. Lack of flexibility,
e.g. they may only have integral or enum type,
they cannot be passed by reference,
nor can they be organized into arrays, etc.

This is a problem in many use cases involving bit-fields. So let's solve that:

```cpp
struct nextgen : bitfilled::host_integer<std::uint8_t>
{
    BF_COPY_SUPERCLASS(nextgen)
    BF_BITS(bool, 0) boolean;
    BF_BITS(std::endian, 1, 2) enumerated;
    BF_BITS(std::int32_t, 3, 7) integer;
};
```

Our `nextgen` type's bit-fields work with the exact same syntax as their `legacy` counterparts.
The difference here is that a `nextgen` object can be converted to and from any `uint8_t` type,
no more explicit casting necessary to get the underlying integer type.
One thing to note is that the nextgen fields have absolute bit offsets, as opposed to the legacy
fields (which in turn only have a bit size specifier).
This characteristic of the behavior also means that nextgen's bit-fields can be made to overlap one another.

## Theory of operation

The bitfilled logic consists of two building blocks, that work in tandem to provide the desired functionality:
1. The bitfilled member variables hold the bit-field's **properties** (`props`): the position of the bits and the access rights,
and also - indirectly - the memory location of the bits (more on that later).
2. The encapsulating object type defines the **operators** (`ops`), which are used by the bitfilled members
to perform the memory access and bit operations needed to read or modify the bit-field.

The term "property" is used to refer to the bitfilled members due to them functioning as [properties][property wiki]
as known in other programming languages (C# and Python to name a few):
 - they don't increase the size of the encapsulating type [(achieved with `[[no_unique_address]]`)](https://en.cppreference.com/w/cpp/language/attributes/no_unique_address)
 - their value is derived from the encapsulating type's state

Note that `[[no_unique_address]]` isn't effective when two bitfields with the same type parameters are defined,
(i.e. same value type, access, operations, bit position) as the C++ core rule of unique identity would be violated
(this applies even if the two bitfield types are distinct, but inherit the same base class).

The key point to understand, and the reason for the design requiring the *operators* in the tandem, is this:
By dereferencing their address, the bitfilled members provide access to memory that is not theirs,
but rather whichever member is preceeding them in the encapsulating type layout.
Therefore the encapsulating type must have a preceeding member variable for bit-field use,
and the bitfilled members must be made aware of this member variable's type - this is what the operators are achieving.
Mismatches between the storage member variable and the operators type is impossible to catch at compile time,
therefore it is recommended to use predefined helper base classes such as `bitfilled::host_integer` and `bitfilled::mmreg`,
instead of defining the storage member variable and the operators independently.

## Memory mapped registers

This library goes beyond simple bit-field manipulation by providing an accurate and easy-to-use API
for memory-mapped register and bit-field manipulation, including access limitations on all levels,
ensuring safe and optimized use.

[stdbitfield]: https://en.cppreference.com/w/cpp/language/bit_field
[no_unique_address]: https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
[property wiki]: https://en.wikipedia.org/wiki/Property_(programming)
[armcortexmbitband]: https://atadiat.com/en/e-bit-banding-explained-a-feature-of-arm-cortex-m3/
