#pragma once

#include "neo_weapon_types.h"

//	Upper name		Lower name		Detailed name, (X_ALT) alt name		Weapon type
#define FOR_LIST_WEAPONS(X, X_ALT) \
	X(AA13, 		aa13, 			"AA13", 							WEP_TYPE_SHOTGUN) \
	X(BALC, 		balc, 			"BALC3", 							WEP_TYPE_MACHINEGUN) \
	X(DETPACK, 		detpack, 		"Detpack", 							WEP_TYPE_THROWABLE) \
	X(GHOST, 		ghost, 			"Ghost", 							WEP_TYPE_NIL) \
	X(FRAG_GRENADE, grenade, 		"Grenade", 							WEP_TYPE_THROWABLE) \
	X(JITTE, 		jitte, 			"Jitte", 							WEP_TYPE_SMG) \
	X_ALT(JITTE_S, 	jittes, 		"Jitte (with scope)", "jittescoped",WEP_TYPE_SMG) \
	X(KNIFE, 		knife, 			"Knife", 							WEP_TYPE_NIL) \
	X(KYLA, 		kyla, 			"Kyla", 							WEP_TYPE_PISTOL) \
	X_ALT(M41, 		mosok, 			"Mosok", "m41", 					WEP_TYPE_RIFLE) \
	X_ALT(M41_L, 	mosokl, 		"Mosok Silenced", "m41s", 			WEP_TYPE_RIFLE) \
	X_ALT(M41_S, 	mosoks, 		"Mosok Scoped", "m41s", 			WEP_TYPE_RIFLE) \
	X(MILSO, 		milso, 			"Milso", 							WEP_TYPE_PISTOL) \
	X(MPN, 			mpn_unsilenced, "MPN45 Unsilenced", 				WEP_TYPE_SMG) \
	X(MPN_S, 		mpn, 			"MPN45", 							WEP_TYPE_SMG) \
	X(MX, 			mx, 			"MX", 								WEP_TYPE_RIFLE) \
	X_ALT(MX_S, 	mxs, 			"MX Silenced", "mx_silenced", 		WEP_TYPE_RIFLE) \
	X(PROX_MINE, 	proxmine, 		"Proximity Mine", 					WEP_TYPE_NIL) \
	X(PZ, 			pz, 			"PZ252", 							WEP_TYPE_MACHINEGUN) \
	X(SMAC, 		smac, 			"SMAC", 							WEP_TYPE_SMG) \
	X(SMOKE_GRENADE,smoke, 			"Smoke Grenade", 					WEP_TYPE_THROWABLE) \
	X(SRM, 			srm, 			"SRM", 								WEP_TYPE_SMG) \
	X_ALT(SRM_S, 	srms, 			"SRM (silenced)", "srm_s", 			WEP_TYPE_SMG) \
	X(SRS, 			srs, 			"SRS", 								WEP_TYPE_SNIPER) \
	X(SUPA7, 		supa7, 			"Murata Supa-7", 					WEP_TYPE_SHOTGUN) \
	X(TACHI, 		tachi, 			"Tachi", 							WEP_TYPE_PISTOL) \
	X(ZR68_C, 		zr68c, 			"ZR68C", 							WEP_TYPE_RIFLE) \
	X(ZR68_L, 		zr68l, 			"ZR68-L (accurized)", 				WEP_TYPE_SNIPER) \
	X(ZR68_S, 		zr68s, 			"ZR68-S (silenced)", 				WEP_TYPE_RIFLE) \
	X(SCOPEDWEAPON,	scopedwep, 		"", 								WEP_TYPE_NIL) \
	X(THROWABLE, 	throwable, 		"", 								WEP_TYPE_NIL) \
	X(SUPPRESSED, 	suppressed, 	"", 								WEP_TYPE_NIL) \
	X(FIREARM, 		firearm, 		"", 								WEP_TYPE_NIL) \
	X(EXPLOSIVE, 	explosive, 		"", 								WEP_TYPE_NIL)

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

#define DEFINE_CROSSHAIR_TYPE(_x1, _x2, _x3, weptype) weptype,
#define DEFINE_CROSSHAIR_TYPE_ALT(_x1, _x2, _x3, _x4, weptype) weptype,

static const ENeoWeaponType NEO_WEAPON_TYPE[NEO_WIDX__TOTAL] = {
	WEP_TYPE_NIL, // 0 = invalid, set none
	FOR_LIST_WEAPONS(DEFINE_CROSSHAIR_TYPE, DEFINE_CROSSHAIR_TYPE_ALT)
#ifdef INCLUDE_WEP_PBK
	WEP_TYPE_MACHINEGUN,
#endif
};

// Some other related type safety checks also rely on this equaling zero.
static_assert(NEO_WEP_INVALID == 0);

#undef DEFINE_ENUM
#undef DEFINE_BITENUM

