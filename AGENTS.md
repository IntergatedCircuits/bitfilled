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
- Respect register and field access constraints, including volatile access and read-only or
  write-only semantics. Do not bypass them with raw casts or custom operations without a clear
  reason and tests.

Build with CMake and run the focused test suite with CTest, for example:

```sh
cmake --preset clang20
cmake --build build/clang20
ctest --test-dir build/clang20 --output-on-failure
```
