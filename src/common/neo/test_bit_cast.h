#pragma once

#if defined(DEBUG) && defined(DBGFLAG_ASSERT)

#include "bit_cast.h"
#include "narrow_cast.h"

#include "tier0/dbg.h"

#include <limits>
#include <random>
#include <type_traits>

namespace neo::test
{
	namespace
	{
		template <class To, class From>
			requires (neo::IsBitCastable<To, From>())
		void _verifyRoundtrip(From input)
		{
			To a = neo::bit_cast<To>(input);
			From b = neo::bit_cast<From>(a);
			static_assert(std::is_same_v<decltype(b), decltype(input)>);
			Assert(b == input);
		}

		template <typename T=float>
			requires std::is_floating_point_v<T>
		static T Rnd()
		{
			static std::default_random_engine e;
			static std::uniform_real_distribution d{ T{0},T{1} };
			return d(e);
		}

		template <class To, class From>
			requires (neo::IsBitCastable<To, From>()
				&& std::is_signed_v<From> && std::is_signed_v<To>)
		constexpr void testToFrom()
		{
			constexpr bool bothTypesIntegral = (
				std::is_integral_v<std::remove_cvref_t<To>> &&
				std::is_integral_v<std::remove_cvref_t<From>>);

			_verifyRoundtrip<To>(From{});
			if constexpr (bothTypesIntegral)
				_verifyRoundtrip<std::make_unsigned_t<To>>(std::make_unsigned_t<From>{});

			for (auto i = 0; i < 10; ++i)
			{
				const auto r = Rnd();

				// Just something that most types can fit, for testing.
				constexpr auto range = std::numeric_limits<char>::max();

				static_assert(range <= std::numeric_limits<From>::max());
				static_assert(range <= std::numeric_limits<To>::max());
				_verifyRoundtrip<To>(static_cast<From>(range * r));

				if constexpr (bothTypesIntegral)
					_verifyRoundtrip<std::make_unsigned_t<To>>(static_cast<From>(range * r));

				static_assert(-range >= std::numeric_limits<From>::lowest());
				static_assert(-range >= std::numeric_limits<To>::lowest());
				_verifyRoundtrip<To>(static_cast<From>(-range * r));
			}
		}

		// For a sequence of signed types, tests all bitcastable permutations against each other.
		template <typename... SignedTs>
		void testTypes()
		{
			size_t nTests = 0, nTestsSkipped = 0;
			([&]<typename T>()
			{
				([&]<typename U>()
				{
					++nTests;
					if constexpr (neo::IsBitCastable<T, U>())
					{
						testToFrom<T, U>();
					}
					else
					{
						++nTestsSkipped;
					}
				}.template operator()<SignedTs>(), ...);
			}.template operator()<SignedTs>(), ...);
			Assert(nTests >= nTestsSkipped);
			AssertMsg1(nTests != nTestsSkipped, "All %zu test(s) skipped!", nTests);
		}
	}

	// Test type conversions using the custom helper utilities.
	void conversions()
	{
		// bool doesn't play nice with the signed/unsigned tests in testTypes,
		// so test it manually here
		constexpr char vals[] = {
			std::numeric_limits<char>::lowest(),
			0x00,
			0x01,
			0x10,
			0x11,
			std::numeric_limits<char>::max(),
		};
		for (const auto v : vals)
		{
			_verifyRoundtrip<bool, bool>(v);
			_verifyRoundtrip<char, char>(v);
			_verifyRoundtrip<bool, char>(v);
			_verifyRoundtrip<char, bool>(v);
		}

		testTypes<
			 char
			,int
			,float
			,double
			,long
			,long long
		>();
	}
}
#endif // DEBUG
