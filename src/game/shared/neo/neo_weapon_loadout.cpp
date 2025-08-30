#include "neo_weapon_loadout.h"

#include "tier0/dbg.h"
#include "tier0/commonmacros.h"
#include "neo_misc.h"

namespace CNEOWeaponLoadout {

#define WEAPON_DEF(M_BITWNAME, M_NAME, M_DISPLAY_NAME, ...) \
	[[maybe_unused]] inline const constexpr WeaponInfo WEP_INFO_##M_NAME = { \
		M_DISPLAY_NAME, "loadout/loadout_" #M_NAME, "loadout/loadout_" #M_NAME "_no", "weapon_" #M_NAME, NEO_WEP_##M_BITWNAME };

#define WEAPON_DEF_ALT(M_BITWNAME, M_NAME, M_DISPLAY_NAME, M_NAMEWEP, ...) \
	[[maybe_unused]] inline const constexpr WeaponInfo WEP_INFO_##M_NAME = { \
		M_DISPLAY_NAME, "loadout/loadout_" #M_NAME, "loadout/loadout_" #M_NAME "_no", "weapon_" M_NAMEWEP, NEO_WEP_##M_BITWNAME };

#define WEAPON(M_NAME) WEP_INFO_##M_NAME

WEAPON_DEF(INVALID, empty, "")
FOR_LIST_WEAPONS(WEAPON_DEF, WEAPON_DEF_ALT)
#ifdef INCLUDE_WEP_PBK
WEAPON_DEF(PBK56S, pbk56s, "PBK56")
#endif

inline const CLoadoutWeapon s_LoadoutWeapons[NEO_LOADOUT__COUNT][MAX_WEAPON_LOADOUTS] =
{
	// Recon
	{
		{XP_ANY, WEAPON(mpn)},			{XP_PRIVATE, WEAPON(srm)},		{XP_PRIVATE, WEAPON(jitte)},
		{XP_CORPORAL, WEAPON(srms)},	{XP_CORPORAL, WEAPON(jittes)},	{XP_CORPORAL, WEAPON(zr68l)},
		{XP_SERGEANT, WEAPON(zr68c)},	{XP_LIEUTENANT, WEAPON(supa7)},	{XP_LIEUTENANT, WEAPON(mosokl)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
	},
	// Assault
	{
		{XP_ANY, WEAPON(mpn)},			{XP_PRIVATE, WEAPON(srm)},		{XP_PRIVATE, WEAPON(jitte)},
		{XP_PRIVATE, WEAPON(zr68c)},	{XP_PRIVATE, WEAPON(zr68s)},	{XP_CORPORAL, WEAPON(supa7)},
		{XP_CORPORAL, WEAPON(mosok)},	{XP_SERGEANT, WEAPON(mosokl)},	{XP_SERGEANT, WEAPON(mx)},
		{XP_SERGEANT, WEAPON(mxs)},		{XP_LIEUTENANT, WEAPON(aa13)},	{XP_LIEUTENANT, WEAPON(srs)},
	},
	// Support
	{
		{XP_ANY, WEAPON(mpn)},			{XP_PRIVATE, WEAPON(srm)},		{XP_PRIVATE, WEAPON(zr68c)},
		{XP_PRIVATE, WEAPON(mosok)},	{XP_PRIVATE, WEAPON(supa7)},	{XP_CORPORAL, WEAPON(mx)},
		{XP_CORPORAL, WEAPON(mosokl)},	{XP_SERGEANT, WEAPON(mxs)},		{XP_LIEUTENANT, WEAPON(pz)},
#ifndef INCLUDE_WEP_PBK
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
#else
		{XP_LIEUTENANT, WEAPON(pbk56s)},{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
#endif
	},
	// VIP
	{
		{XP_ANY, WEAPON(smac)},			{XP_CORPORAL, WEAPON(mpn)},		{XP_CORPORAL, WEAPON(srm)},
		{XP_SERGEANT, WEAPON(jitte)},	{XP_SERGEANT, WEAPON(srms)},	{XP_LIEUTENANT, WEAPON(jittes)},
		{XP_LIEUTENANT, WEAPON(zr68c)},	{XP_LIEUTENANT, WEAPON(zr68l)},	{XP_LIEUTENANT, WEAPON(supa7)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
	},
	// Juggernaut (empty, cannot spawn as juggernaut)
	{
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
	},
	// Developer (special, non-class)
	{
		{XP_ANY, WEAPON(mpn)}, 			{XP_ANY, WEAPON(srm)}, 			{XP_ANY, WEAPON(srms)},
		{XP_ANY, WEAPON(jitte)}, 		{XP_ANY, WEAPON(jittes)}, 		{XP_ANY, WEAPON(zr68c)},
		{XP_ANY, WEAPON(zr68s)}, 		{XP_ANY, WEAPON(zr68l)}, 		{XP_ANY, WEAPON(mx)},
#ifndef INCLUDE_WEP_PBK
		{XP_ANY, WEAPON(pz)}, 			{XP_ANY, WEAPON(supa7)}, 		{XP_ANY, WEAPON(mosok)}
#else
		{XP_ANY, WEAPON(pz)}, 			{XP_ANY, WEAPON(supa7)}, 		{XP_ANY, WEAPON(pbk56s)}
#endif
	},
	// Invalid loadout (empty fallback)
	{
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},		{XP_EMPTY, WEAPON(empty)},
	},
};

int GetNumberOfLoadoutWeapons(const int rank, const int classType)
{
	int amount = 0;
	if (IN_BETWEEN_AR(0, classType, NEO_LOADOUT__COUNT))
	{
		for (int i = 0; i < MAX_WEAPON_LOADOUTS; i++)
		{
			const int iWepPrice = s_LoadoutWeapons[classType][i].m_iWeaponPrice;
			if (iWepPrice > rank || iWepPrice == XP_EMPTY)
			{
				break;
			}
			amount++;
		}
	}
	else
	{
		Assert(false);
	}
	return amount;
}

} // namespace CNEOWeaponLoadout

