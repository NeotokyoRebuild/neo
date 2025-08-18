#pragma once

#include "neo_enums.h"

// NEO NOTE (nullsystem): If there's NEO_BUILD_WEAPON_PBK56S / INCLUDE_WEP_PBK
// inclusion or new weapons, may want to change/alter the loadouts for it.
static const constexpr int MAX_WEAPON_LOADOUTS = 12;

enum ELoadoutCount
{
	NEO_LOADOUT_DEV = NEO_CLASS__ENUM_COUNT,
	NEO_LOADOUT_INVALID,
	NEO_LOADOUT__COUNT,
};

struct WeaponInfo
{
	const char *m_szWeaponName = "";
	const char *m_szVguiImage = "loadout/loadout_none";
	const char *m_szVguiImageNo = "loadout/loadout_none";
	const char *m_szWeaponEntityName = "";
};

struct CLoadoutWeapon
{
	int m_iWeaponPrice;
	const WeaponInfo &info;
};

namespace CNEOWeaponLoadout
{
	extern const CLoadoutWeapon s_LoadoutWeapons[NEO_LOADOUT__COUNT][MAX_WEAPON_LOADOUTS];

	int GetNumberOfLoadoutWeapons(const int rank, const int classType);

	static const constexpr int XP_ANY = -255;
	static const constexpr int XP_PRIVATE = 0;
	static const constexpr int XP_CORPORAL = 4;
	static const constexpr int XP_SERGEANT = 10;
	static const constexpr int XP_LIEUTENANT = 20;
	static const constexpr int XP_EMPTY = 9999;
} // namespace CNEOWeaponLoadout

