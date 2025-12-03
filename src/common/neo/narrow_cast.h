#pragma once

#ifdef PARANOID_NARROWING
#include <limits>
#include <type_traits>
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
template <typename T>
concept Arithmetic = std::is_arithmetic_v<std::remove_cvref_t<T>>;
// Sanity check narrowing conversions.
// Asserts that value v must fit into type <Container>.
template <Arithmetic Container, Arithmetic Value>
constexpr void AssertFitsInto(Value&& v)
{
	Assert(v <= std::numeric_limits<Container>::max());
	const auto& vAfter = static_cast<Container>(std::forward<Value>(v));
	Assert(v == vAfter);
}
#else
constexpr auto AssertFitsInto(const auto&&) { /* optimized away */ }
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
// static_cast for narrowing conversions.
// It will assert to ensure the conversion is valid.
template <class Container, class Value>
	requires Arithmetic<Container>&& Arithmetic<Value>
constexpr Container narrow_cast(Value&& v)
{
	AssertFitsInto<Container>(v);
	return static_cast<Container>(std::forward<Value>(v));
}
#else
// Alias for static_cast, for marking narrowing conversions.
// If you want to validate the conversions, toggle the NEO_PARANOID_NARROWING in the main CMakeLists.txt.
#define narrow_cast static_cast
#endif // PARANOID_NARROWING
