//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=====================================================================================//

#include "mathlib/ssemath.h"

#ifdef NEO
#include "../common/neo/bit_cast.h"
#if defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT)
#include "tier0/commonmacros.h"
#include "vstdlib/random.h"
#endif // ACTUALLY_COMPILER_MSVC && DBGFLAG_ASSERT
#endif // NEO

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"


fltx4 Pow_FixedPoint_Exponent_SIMD( const fltx4 & x, int exponent)
{
	fltx4 rslt=Four_Ones;									// x^0=1.0
	int xp=abs(exponent);
	if (xp & 3)												// fraction present?
	{
		fltx4 sq_rt=SqrtEstSIMD(x);
		if (xp & 1)											// .25?
			rslt=SqrtEstSIMD(sq_rt);						// x^.25
		if (xp & 2)
			rslt=MulSIMD(rslt,sq_rt);
	}
	xp>>=2;													// strip fraction
	fltx4 curpower=x;										// curpower iterates through  x,x^2,x^4,x^8,x^16...

	while(1)
	{
		if (xp & 1)
			rslt=MulSIMD(rslt,curpower);
		xp>>=1;
		if (xp)
			curpower=MulSIMD(curpower,curpower);
		else
			break;
	}
	if (exponent<0)
		return ReciprocalEstSaturateSIMD(rslt);				// pow(x,-b)=1/pow(x,b)
	else
		return rslt;
}




/*
 * (c) Ian Stephenson
 *
 * ian@dctsystems.co.uk
 *
 * Fast pow() reference implementation
 */


#if defined(NEO) && defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT)
namespace Reference {
#endif
#if !defined(NEO) || (defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT))
static float shift23=(1<<23);
static float OOshift23=1.0/(1<<23);

float FastLog2(float i)
{
	float LogBodge=0.346607f;
	float x;
	float y;
	x=*(int *)&i;
	x*= OOshift23; //1/pow(2,23);
	x=x-127;

	y=x-floorf(x);
	y=(y-y*y)*LogBodge;
	return x+y;
}
float FastPow2(float i)
{
	float PowBodge=0.33971f;
	float x;
	float y=i-floorf(i);
	y=(y-y*y)*PowBodge;

	x=i+127-y;
	x*= shift23; //pow(2,23);
	*(int*)&x=(int)x;
	return x;
}
float FastPow(float a, float b)
{
	if (a <= OOshift23)
	{
		return 0.0f;
	}
	return FastPow2(b*FastLog2(a));
}
float FastPow10( float i )
{
	return FastPow2( i * 3.321928f );
}
#endif // !defined(NEO) || (defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT))
#if defined(NEO) && defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT)
} // end of Reference namespace
#endif

#ifdef NEO
float FastLog2(const float i)
{
	constexpr float LogBodge = 0.346607f;
	float x;
	float y;
	x = neo::bit_cast<int>(BC_TEST(i, *(int*)&i));
	constexpr float OOshift23 = 1.0 / (1 << 23);
	x *= OOshift23; //1/pow(2,23);
	x = x - 127;

	y = x - floorf(x);
	y = (y - y * y) * LogBodge;

	return x + y;
}
float FastPow2(const float i)
{
	constexpr float PowBodge = 0.33971f;
	float x;
	float y = i - floorf(i);
	y = (y - y * y) * PowBodge;

	x = i + 127 - y;
	constexpr float shift23 = (1 << 23);
	x *= shift23; //pow(2,23);
	x = neo::bit_cast<decltype(x)>(neo::bit_cast<int>((int)x));
	return x;
}

#if defined(ACTUALLY_COMPILER_MSVC) && defined(DBGFLAG_ASSERT)
template <typename Namespace>
auto TestFun()
{
	return Namespace::FastLog2();
}

void ValidateFastFuncs()
{
	const auto fn1 = FastLog2, ref1 = Reference::FastLog2;
	const auto fn2 = FastPow2, ref2 = Reference::FastPow2;

	const auto compareFns = [](auto input, decltype(ref1) fn, decltype(fn) ref)
	{
		static_assert(&fn != &ref);
		const float fnOutput = fn(input);
		const float refOutput = ref(input);
		const bool resultsEqual = (fnOutput == refOutput);
		AssertMsg3(resultsEqual, "FF output differs for input %f: fn %f ref %f", input, fnOutput, refOutput);
	};

	constexpr float vals[] = {
		0, 1, 2, 3, 4, 0.1234, 1.234, 1.01234,
	};
	for (int i = 0; i < ARRAYSIZE(vals); ++i)
	{
		const auto val = vals[i];
		Assert(val >= 0); // because we already try both val and -val below

		compareFns(val, fn1, ref1);
		compareFns(-val, fn1, ref1);

		compareFns(val, fn2, ref2);
		compareFns(-val, fn2, ref2);
	}

	constexpr auto numIters = 10;
	for (auto i = 0; i < numIters; ++i)
	{
		{
			const float input = RandomFloat(0, 1);

			compareFns(input, fn1, ref1);
			compareFns(-input, fn1, ref1);

			compareFns(input, fn2, ref2);
			compareFns(-input, fn2, ref2);
		}
		{
			const float input = RandomFloat(0, 0xffff);

			compareFns(input, fn1, ref1);
			compareFns(-input, fn1, ref1);

			compareFns(input, fn2, ref2);
			compareFns(-input, fn2, ref2);
		}
	}
}
#endif // ACTUALLY_COMPILER_MSVC && DBGFLAG_ASSERT
#endif // NEO
