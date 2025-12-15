#pragma once

#include <version>

#pragma push_macro("STD_BIT_CAST_SUPPORTED")
#undef STD_BIT_CAST_SUPPORTED

#ifdef __cpp_lib_bit_cast
#if __cpp_lib_bit_cast != 201806L // this value gets bumped with major api changes, so we error here to catch your attention
#error Expected to find __cpp_lib_bit_cast version of 201806L, please verify if this is ok
#endif
#define STD_BIT_CAST_SUPPORTED 1 // whether std::bit_cast is available
#else
#define STD_BIT_CAST_SUPPORTED 0 // whether std::bit_cast is available
#endif

#if STD_BIT_CAST_SUPPORTED
#include <bit>
#endif

#include <cstring>
#include <memory>
#include <type_traits>
#include <utility>

namespace neo
{
	template <class To, class From>
	constexpr bool IsBitCastable() noexcept
	{
		return (
			sizeof(To) == sizeof(From)
			&& std::is_trivially_constructible_v<To>
			&& std::is_nothrow_constructible_v<To>
			&& std::is_trivially_copyable_v<From>
			&& std::is_trivially_copyable_v<To>
			);
	}

	// Will transparently call std::bit_cast when it's available and its constraints are satisfied.
	// Else, will perform a well-defined type pun using memcpy.
	// If you want a sanity check for the result, see BC_TEST
	template <class To, class From>
	constexpr auto bit_cast(const From& input) noexcept
	{
		constexpr bool fromVoidPtr = std::is_same_v<void*, std::remove_const_t<std::decay_t<decltype(input)>>>;
#if STD_BIT_CAST_SUPPORTED
		if constexpr (fromVoidPtr)
#endif
		{
			static_assert(fromVoidPtr || IsBitCastable<To, From>());
			std::remove_cv_t<To> output;
			memcpy(std::addressof(output), std::addressof(input), sizeof(output));
			return output;
		}
#if STD_BIT_CAST_SUPPORTED
		else
		{
			return std::bit_cast<To>(input);
		}
#endif
	}

/** @brief Dynamic assertions for neo::bit
*
* For debug builds, dynamically asserts that neo::bitcast<T>(input) == the original SDK type pun result.
* For release builds (i.e. DBGFLAG_ASSERT disabled), the test is optimized away and input passed directly
* to neo::bit_cast.
*
* \code{.cpp}
  // Example:
  neo::bit_cast<int>(BC_TEST(inputToBitCast, 42));
  \endcode
*
* @param[in] input			The input value for the bit cast. Also used as the assertion lambda's capture.
*							If you need to specify the capture, use BC_TEST_EX instead.
* @param[in] expectedOutput	The expected result of the cast
*/
#define BC_TEST(input, expectedOutput) input, [input](const auto& v){ (void)input; Assert(v == expectedOutput); }

// Same as BC_TEST, but allows specifying the lambda capture.
#define BC_TEST_EX(input, capture, expectedOutput) input, [capture](const auto& v){ (void)capture; Assert(v == expectedOutput); }

	template <class To, class From, typename TestFunc>
	constexpr auto bit_cast(const From& input, TestFunc&&
#ifdef DEBUG
		test
#endif
	) noexcept
	{
#ifdef DEBUG
		const auto output = bit_cast<To>(input);
		test(output);
		return output;
#else
		return bit_cast<To>(input);
#endif
	}
};

#undef STD_BIT_CAST_SUPPORTED
#pragma pop_macro("STD_BIT_CAST_SUPPORTED")
