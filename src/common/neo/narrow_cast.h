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
#include <concepts>
#include <limits>
#include <type_traits>
#include <utility>
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
template <typename T>
concept Numeric = ((std::is_arithmetic_v<std::remove_cvref_t<T>> && std::numeric_limits<std::remove_cvref_t<T>>::is_bounded)
	|| std::is_enum_v<std::remove_cvref_t<T>>);

template <typename T, typename U>
concept DifferentTypes = (!std::is_same_v<T, U>);

// Sanity check narrowing conversions.
// Asserts that value v must fit into type <Container>.
template <Numeric Container, Numeric Value>
	requires DifferentTypes<Container, Value>
constexpr void AssertFitsInto(const Value& input)
{
	using inputType = std::remove_reference_t<decltype(input)>;
	using outputType = std::add_const_t<Container>;

	static_assert(!std::is_reference_v<inputType>);
	static_assert(!std::is_reference_v<outputType>);

	[[maybe_unused]] const auto& output = static_cast<outputType&>(input);
	static_assert(!std::is_same_v<decltype(input), decltype(output)>);

	[[maybe_unused]] const auto& roundtrip = static_cast<inputType>(output);
	static_assert(std::is_same_v<decltype(input), decltype(roundtrip)>);

	// Value of type A converted to type B, and then back to type A, should equal the original value.
	Assert(roundtrip == input);

	// Value of type A converted to type B, where the signedness of A and B differs, should not underflow.
	if (std::is_signed_v<inputType> != std::is_signed_v<outputType>)
	{
		Assert((input < inputType{}) == (output < outputType{}));
	}
}
#else
consteval void AssertFitsInto(const auto&) { /* optimized away */ }
#endif // PARANOID_NARROWING

#ifdef PARANOID_NARROWING
// static_cast for narrowing conversions.
// It will assert to ensure the conversion is valid.
template <Numeric Container, Numeric Value>
	requires DifferentTypes<Container, Value>
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
