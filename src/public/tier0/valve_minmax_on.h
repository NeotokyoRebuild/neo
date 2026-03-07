//========= Copyright Valve Corporation, All rights reserved. ============//
#ifdef NEO
// Please use Max/Min from basetypes.h instead
#undef max
#undef min
#else

#if !defined(POSIX)
#ifndef min
	#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
	#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif
#endif

#endif // NEO