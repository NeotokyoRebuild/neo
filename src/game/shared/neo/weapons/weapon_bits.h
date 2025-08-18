#pragma once

#ifdef INCLUDE_WEP_PBK
// Type to use if we need to ensure more than 32 bits in the mask.
#define NEO_WEP_BITS_UNDERLYING_TYPE long long int
#else
// Using plain int if we don't need to ensure >32 bits in the mask.
#define NEO_WEP_BITS_UNDERLYING_TYPE int
#endif

// Weapon bit flags
enum NeoWepBits : NEO_WEP_BITS_UNDERLYING_TYPE {
	NEO_WEP_INVALID =			0x0,

	NEO_WEP_AA13 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 0),
	NEO_WEP_DETPACK =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 1),
	NEO_WEP_GHOST =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 2),
	NEO_WEP_FRAG_GRENADE =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 3),
	NEO_WEP_JITTE =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 4),
	NEO_WEP_JITTE_S =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 5),
	NEO_WEP_KNIFE =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 6),
	NEO_WEP_KYLA =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 7),
	NEO_WEP_M41 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 8),
	NEO_WEP_M41_L =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 9),
	NEO_WEP_M41_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 10),
	NEO_WEP_MILSO =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 11),
	NEO_WEP_MPN =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 12),
	NEO_WEP_MPN_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 13),
	NEO_WEP_MX =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 14),
	NEO_WEP_MX_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 15),
	NEO_WEP_PROX_MINE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 16),
	NEO_WEP_PZ =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 17),
	NEO_WEP_SMAC =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 18),
	NEO_WEP_SMOKE_GRENADE =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 19),
	NEO_WEP_SRM =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 20),
	NEO_WEP_SRM_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 21),
	NEO_WEP_SRS =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 22),
	NEO_WEP_SUPA7 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 23),
	NEO_WEP_TACHI =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 24),
	NEO_WEP_ZR68_C =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 25),
	NEO_WEP_ZR68_L =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 26),
	NEO_WEP_ZR68_S =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 27),
	NEO_WEP_SCOPEDWEAPON =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 28), // Scoped weapons should OR this in their flags.
	NEO_WEP_THROWABLE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 29), // Generic for grenades
	NEO_WEP_SUPPRESSED =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 30), // Suppressed weapons

	// NOTE!!! remember to update NEP_WEP_BITS_LAST_VALUE below, if editing this/these last values!
	NEO_WEP_EXPLOSIVE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 31), // Generic for weapons that count as explosive kills on killfeed.
#ifdef INCLUDE_WEP_PBK
	NEO_WEP_PBK56S =			(static_cast <NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 32),
#endif

#ifndef INCLUDE_WEP_PBK
	NEP_WEP_BITS_LAST_VALUE = NEO_WEP_EXPLOSIVE
#else
	NEP_WEP_BITS_LAST_VALUE = NEO_WEP_PBK56S
#endif
};

// Some other related type safety checks also rely on this equaling zero.
COMPILE_TIME_ASSERT(NEO_WEP_INVALID == 0);

