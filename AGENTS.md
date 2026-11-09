# Agent Notes

Bitfilled is a header-only C++20 library for defining bit fields and bit-field sets over host
integers, packed integers, and memory-mapped registers. Field members are lightweight descriptors,
not independent value storage: `BF_BITS` and related macros use `[[no_unique_address]]`, and the
containing type's base class and `bf_ops` provide access to the underlying storage. Keep those
storage and operations types consistent; prefer the library's existing host, packed, and register
types over custom layouts.

Important constraints when changing or defining containing types:

- `[[no_unique_address]]` is not a guarantee of zero size. Two fields that resolve to the same
  descriptor type (for example, the same value type, operations, access, and bit range) must still
  have distinct addresses, so layout may grow. For subclasses intended to add no storage, add a
  `static_assert` comparing their size with the storage base, e.g.
  `static_assert(sizeof(my_type) == sizeof(bitfilled::host_integer<std::uint32_t>));`.
- Field ranges are absolute and may overlap intentionally. Avoid duplicate descriptor types unless
  the resulting layout is checked; different ranges usually produce different descriptor types.
- On GCC, designated initialization of `BF_BITS` members can fail because the
  `[[no_unique_address]]` descriptor has a deleted move constructor. This does not prevent
  aggregate use: default-construct the containing type and assign fields afterward. Alternatively,
  `BF_COPY_SUPERCLASS` inherits base constructors and assignment operators, but makes the derived
  type non-aggregate, so designated initializers are unavailable.
- `BF_COPY_SUPERCLASS` also supports assignment from a volatile source to a non-volatile object; it
  does not make a volatile destination assignable from a non-volatile source.
- Field descriptors must never be copied or moved out of their owner (a detached copy accesses
  memory beyond itself). Their copy constructor is trivial but private, and only accessible to the
  owner declared by `BF_OWNER(CLASS)` / `BF_COPY_SUPERCLASS(CLASS)` (via `bitfilled::owned_ops`),
  which must precede the fields. Class templates can't use these macros (dependent superclass),
  declare `using bf_ops = bitfilled::owned_ops<typename base_type::bf_ops, CLASS>;` instead.
  Generic code (e.g. boost.ut `that %`) needs the field value, not the field: convert it first
  (see `bitfilled_test::value_of` in `test/field_traits.hpp`).
- Constructing and copy constructing an owner must not execute bit field code. Keep descriptor
  default/copy constructors trivial, and check owners with `bitfilled_test::passive_owner` and
  descriptors with `bitfilled_test::stateless_field` (the latter not on MSVC, where fields are
  `__declspec(property)` accessors). Copy *assignment* of an owner does assign field by field, as
  `a.field = b.field` must copy the value.
- Field definitions are validated (`BitfieldValue`, `BitRangeFitsStorage`, `BitRangeFitsValue`)
  as class template constraints, and again with `static_assert` in the base operations, which is
  the only check on MSVC.
- Respect register and field access constraints, including volatile access and read-only or
  write-only semantics. Do not bypass them with raw casts or custom operations without a clear
  reason and tests.

Build with CMake and run the focused test suite with CTest, for example:

```sh
cmake -S . -B build/gcc -DCMAKE_CXX_COMPILER=g++
cmake --build build/gcc
ctest --test-dir build/gcc --output-on-failure
```
