#pragma once

#define FOR_LIST_WEAPONS(X, X_ALT) \
	X(AA13, aa13, "AA13") \
	X(BALC, balc, "BALC3") \
	X(DETPACK, detpack, "Detpack") \
	X(GHOST, ghost, "Ghost") \
	X(FRAG_GRENADE, grenade, "Grenade") \
	X(JITTE, jitte, "Jitte") \
	X_ALT(JITTE_S, jittes, "Jitte (with scope)", "jittescoped") \
	X(KNIFE, knife, "Knife") \
	X(KYLA, kyla, "Kyla") \
	X_ALT(M41, mosok, "Mosok", "m41") \
	X_ALT(M41_L, mosokl, "Mosok Silenced", "m41s") \
	X_ALT(M41_S, mosoks, "Mosok Scoped", "m41s") \
	X(MILSO, milso, "Milso") \
	X(MPN, mpn_unsilenced, "MPN45 Unsilenced") \
	X(MPN_S, mpn, "MPN45") \
	X(MX, mx, "MX") \
	X_ALT(MX_S, mxs, "MX Silenced", "mx_silenced") \
	X(PROX_MINE, proxmine, "Proximity Mine") \
	X(PZ, pz, "PZ252") \
	X(SMAC, smac, "SMAC") \
	X(SMOKE_GRENADE, smoke, "Smoke Grenade") \
	X(SRM, srm, "SRM") \
	X_ALT(SRM_S, srms, "SRM (silenced)", "srm_s") \
	X(SRS, srs, "SRS") \
	X(SUPA7, supa7, "Murata Supa-7") \
	X(TACHI, tachi, "Tachi") \
	X(ZR68_C, zr68c, "ZR68C") \
	X(ZR68_L, zr68l, "ZR68-L (accurized)") \
	X(ZR68_S, zr68s, "ZR68-S (silenced)") \
	X(SCOPEDWEAPON, scopedwep, "") \
	X(THROWABLE, throwable, "") \
	X(SUPPRESSED, suppressed, "") \
	X(FIREARM, firearm, "") \
	X(EXPLOSIVE, explosive, "")

// SCOPEDWEAPON - Scoped weapons should OR this in their flags.
// THROWABLE - Generic for grenades 
// SUPPRESSED - Suppressed weapons
// EXPLOSIVE - Generic for weapons that count as explosive kills on killfeed.

#define NEO_WEP_BITS_UNDERLYING_TYPE long long int

#define DEFINE_ENUM(name, ...) NEO_WIDX_##name,
#define DEFINE_BITENUM(name, ...) NEO_WEP_##name = (static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(NEO_WIDX_##name - 1)),

// Weapon bit flags
enum NeoWepBits : NEO_WEP_BITS_UNDERLYING_TYPE
{
	// Index section
	NEO_WIDX_INVALID = 0,
	FOR_LIST_WEAPONS(DEFINE_ENUM, DEFINE_ENUM)
#ifdef INCLUDE_WEP_PBK
	DEFINE_ENUM(PBK56S)
#endif
	NEO_WIDX__TOTAL,

	// Bitwise section
	NEO_WEP_INVALID =			0x0,
	FOR_LIST_WEAPONS(DEFINE_BITENUM, DEFINE_BITENUM)

	// NOTE!!! remember to update NEO_WEP_BITS_LAST_VALUE below, if editing this/these last values!
#ifndef INCLUDE_WEP_PBK
	NEO_WEP_BITS_LAST_VALUE = NEO_WEP_EXPLOSIVE
#else
	DEFINE_BITENUM(PBK56S)
	NEO_WEP_BITS_LAST_VALUE = NEO_WEP_PBK56S
#endif
};

// Some other related type safety checks also rely on this equaling zero.
static_assert(NEO_WEP_INVALID == 0);

#undef DEFINE_ENUM
#undef DEFINE_BITENUM

