#pragma once

#ifndef NOMINMAX
#error Please use Max/Min from basetypes.h instead
#endif
#ifdef max
#error Please use Max from basetypes.h instead
#endif
#ifdef min
#error Please use Min from basetypes.h instead
#endif

#ifdef PARANOID_NARROWING
#include <limits>
#include <type_traits>
#include <utility>
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
template <typename T>
concept Arithmetic = std::is_arithmetic_v<std::remove_cvref_t<T>>;

template <typename T, typename U>
concept DifferentTypes = (!std::is_same_v<T, U>);

// Sanity check narrowing conversions.
// Asserts that value v must fit into type <Container>.
template <Arithmetic Container, Arithmetic Value>
	requires DifferentTypes<Container, Value>
constexpr void AssertFitsInto(Value&& v)
{
	[[maybe_unused]] const auto& input = std::forward<std::remove_reference_t<decltype(v)>>(v);
	[[maybe_unused]] const auto& output = static_cast<std::add_const_t<Container>&>(input);
	[[maybe_unused]] const auto& roundtrip = static_cast<decltype(input)>(output);
	static_assert(!std::is_same_v<decltype(input), decltype(output)>);
	static_assert(std::is_same_v<decltype(input), decltype(roundtrip)>);

	// Value of type A converted to type B, and then back to type A, should equal the original value.
	Assert(roundtrip == input);

	// Value of type A converted to type B, where the signedness of A and B differs, should not underflow.
	if (std::is_signed_v<decltype(input)> != std::is_signed_v<decltype(output)>)
	{
		Assert(input < decltype(input){} == output < decltype(output){});
	}
}
#else
constexpr void AssertFitsInto(auto&&) { /* optimized away */ }
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
// static_cast for narrowing conversions.
// It will assert to ensure the conversion is valid.
template <Arithmetic Container, Arithmetic Value>
	requires DifferentTypes<Container, Value>
constexpr Container narrow_cast(Value&& v)
{
	AssertFitsInto<Container>(std::as_const(v));
	return static_cast<Container>(std::forward<Value>(v));
}
#else
// Alias for static_cast, for marking narrowing conversions.
// If you want to validate the conversions, toggle the NEO_PARANOID_NARROWING in the main CMakeLists.txt.
#define narrow_cast static_cast
#endif // PARANOID_NARROWING
