# ~~Bitfield~~ Bitfilled support for C++

This header-only C++20 library provides a new, portable and more flexible way of manipulating bit-fields.
While the current codebase is functional, it is overall still in the concept development phase,
with important features on the horizon. [Feedback](https://github.com/IntergatedCircuits/bitfilled/discussions) is very welcome!

## Introduction

What makes bitfilled different from the language standard's [bit fields][stdbitfield]?

1. **Portability** - regardless of platform or toolchain, the code behavior is the same (**except MSVC**, as it refuses to implement `[[no_unique_address]]`)
2. **Performance** - optimized binary is identical to standard bit fields
3. **Flexibility** - allows bit fields on custom types, bit field arrays, and customizing bit operations (e.g. bit-banding)

## Containing types

One key thing to note in advance is that bitfilled fields are closely tied with their containing class,
as it defines the available memory size and alignment, and bit field operations.
Therefore let's go through these types first:

### 1. Host integers

The `host_integer` type is simply encapsulating an integral type, forwarding all operations to it,
and provides the necessary scope information to the bitfilled member fields:

```cpp
#include "bitfilled/integer.hpp"
namespace bitfilled {
  template <Integral T, typename TOps = bitfilled::base>
  struct host_integer;
}
```

### 2. Packed integers with fixed endianness

The `packed_integer` type is stored as a byte array, but accessible as an integral type,
the conversion being performed based on the endianness of the type.
The purpose of this type is to facilitate portable definition of various network protocol data units.
```cpp
#include "bitfilled/integer.hpp"
namespace bitfilled {
  template <std::endian ENDIAN, std::size_t SIZE, Integral T = sized_unsigned_t<std::bit_ceil(SIZE)>>
  struct packed_integer;
}
```

### 3. Memory-mapped I/O registers

The `mmreg` type serves as an accurate representation of a memory-mapped register,
with specific access limitation (e.g. read-write / read-only / write-only).
Its bit fields have their own access specifier as well.
```cpp
#include "bitfilled/mmreg.hpp"
namespace bitfilled {
  template <Integral T, enum access ACCESS = access::readwrite, typename TOps = bitfilled::base>
  struct mmreg;
}
```

## Bit field types

There are currently two types of fields supported:
1. Regular bit fields, which take a contiguous bit range in memory
2. Bit field sets, a adjacent bit fields organized into an array indexible set

```cpp
#include "bitfilled/bits.hpp"
namespace bitfilled {
  template <typename T, typename TOps, std::size_t FIRST_BIT, std::size_t LAST_BIT>
  using bitfield = regbitfield<T, TOps, access::readwrite, FIRST_BIT, LAST_BIT>;

  template <typename T, typename TOps, std::size_t ITEM_SIZE, std::size_t ITEM_COUNT, std::size_t OFFSET>
  using bitfieldset = regbitfieldset<T, TOps, access::readwrite, ITEM_SIZE, ITEM_COUNT, OFFSET>;
}
```

The main difference compared to standard bit fields is that their position is absolute,
which also means that they can overlap one another. (Don't repeat the same field with the same type
at the same position though, as that breaks `[[no_unique_address]]` guarantee!)
Both of these types have an explicit access specified `reg-` version, for `mmreg` use.
Some examples are due:

(Do not be alarmed by the macros, their main purpose is to reduce the character count,
as having `[[no_unique_address]]` and a long type name isn't all that informative in this context.)
```cpp
#include <bitfilled/bitfilled.hpp>
struct myint : bitfilled::host_integer<unsigned>
{
    BF_BITS(bool, 0) boolean; // 1 bit at offset 0
    BF_BITS(std::memory_order, 1, 3) enumerated; // 3 bits at offset 1
    BF_BITSET(bool, 1, 16, 4) bitset; // 16 * 1 bits at offset 4
};
```

These fields can be accesses as regular members, however their value is stored inside the containing class's (superclass's) memory. Bit field set elements are accessible via `operator[]`.

I encourage everyone to try it online:
https://godbolt.org/z/bba7a8sTT

### Register bit fields

Let's look at a more advanced use-case, memory-mapped register definition.
We will use the SysTick timer, found in most popular ARM MCUs:

```cpp
#include <bitfilled/bitfilled.hpp>
struct systick {
  struct csr : BF_MMREG(std::uint32_t, rw) {
    BF_COPY_SUPERCLASS(csr)
    BF_MMREGBITS(bool, r, 16) COUNTFLAG;
    BF_MMREGBITS(bool, rw, 2) CLKSOURCE;
    BF_MMREGBITS(bool, rw, 1) TICKINT;
    BF_MMREGBITS(bool, rw, 0) ENABLE;
  } CSR;
  struct rvr : BF_MMREG(std::uint32_t, rw) {
    BF_COPY_SUPERCLASS(rvr)
    BF_MMREGBITS(bool, rw, 0, 23) RELOAD; // optional, same as accessing the register itself
  } RVR;
  struct cvr : BF_MMREG(std::uint32_t, rw) {
    BF_COPY_SUPERCLASS(cvr)
    BF_MMREGBITS(bool, rw, 0, 23) CURRENT; // any write clears the field and COUNTFLAG to 0
  } CVR;
  struct calib : BF_MMREG(std::uint32_t, r) {
    BF_COPY_SUPERCLASS(calib)
    BF_MMREGBITS(bool, r, 31) NOREF;
    BF_MMREGBITS(bool, r, 30) SKEW;
    BF_MMREGBITS(bool, r, 0, 23) TENMS;
  } CALIB;
} & SYSTICK = *reinterpret_cast<volatile systick*>(SysTick_BASE);
```

The code is self-explanatory, and provides an accurate interface to the hardware, by accessing the `SYSTICK` reference. As an example, the `COUNTFLAG` bit is read-only in an otherwise read-write register, which is reflected in its definition, and consequently assigning a value to this member is a compile-time error. The same is true for the `CALIB` register, and all its fields.

A fully functional MM I/O example is available [here][bitfilled-stm32f4],
where the **significant** code size savings are also illustrated.

The project also comes with a [python code generator](tools/svd2mmregmap.py) (draft version),
that let's you create register map definition out of CMSIS SVD files.

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
therefore it is necessary to use predefined helper base classes such as `bitfilled::host_integer` and `bitfilled::mmreg`,
instead of defining the storage member variable and the operators independently.

[stdbitfield]: https://en.cppreference.com/w/cpp/language/bit_field
[no_unique_address]: https://en.cppreference.com/w/cpp/language/attributes/no_unique_address
[property wiki]: https://en.wikipedia.org/wiki/Property_(programming)
[armcortexmbitband]: https://atadiat.com/en/e-bit-banding-explained-a-feature-of-arm-cortex-m3/
[bitfilled-stm32f4]: https://github.com/IntergatedCircuits/bitfilled-stm32f4
