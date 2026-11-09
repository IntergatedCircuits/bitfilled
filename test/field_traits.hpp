#pragma once

#include <type_traits>

namespace bitfilled_test
{

/// @brief  Properties every bit field descriptor (BF_BITS, BF_BITSET, BF_MMREGBITS, ...) must have.
///         They are empty, stateless members of their owner, so:
///         - constructing or destroying them must not execute any code,
///         - they must not be copyable or movable outside of their owner's own copy, as a copy
///           detached from its owner would access memory beyond itself.
///         Not applicable to MSVC, where bit fields are `__declspec(property)` accessors.
template <typename F>
concept stateless_field =
    std::is_empty_v<F> and std::is_trivially_default_constructible_v<F> and
    std::is_trivially_destructible_v<F> and not std::is_copy_constructible_v<F> and
    not std::is_constructible_v<F, F&> and not std::is_move_constructible_v<F> and
    not std::is_convertible_v<const F&, F> and not std::is_convertible_v<F&, F> and
    not std::is_move_assignable_v<F>;

/// @brief  The owner's construction never executes bit field code, and adds no storage.
template <typename T>
concept passive_owner =
    std::is_trivially_copy_constructible_v<T> and std::is_trivially_destructible_v<T> and
    (sizeof(T) == sizeof(typename T::superclass));

/// @brief  Read a bit field (or a bit field set item proxy) as its value type.
///         boost.ut and other generic code take their arguments by value or copy them,
///         which bit field descriptors don't allow (by design).
template <typename F>
constexpr auto value_of(const F& field)
{
    if constexpr (requires { typename F::value_type; })
    {
        return static_cast<typename F::value_type>(field);
    }
    else
    {
        return field;
    }
}

} // namespace bitfilled_test

/// @brief  Evaluates to true if EXPR (using `self` of type TYPE&) is a valid expression.
///         Used to verify that unintended uses of the API fail to compile.
#define BF_TEST_VALID(TYPE, EXPR)                                                                  \
    std::is_invocable_v<decltype([](auto& self) -> decltype((void)(EXPR)) {}), TYPE&>
