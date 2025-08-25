#include "neo_weapon_loadout.h"

#include "tier0/dbg.h"
#include "tier0/commonmacros.h"
#include "neo_misc.h"

namespace CNEOWeaponLoadout {

#define WEAPON_DEF(M_NAME, M_DISPLAY_NAME) \
	inline const constexpr WeaponInfo WEP_INFO_##M_NAME = { \
		M_DISPLAY_NAME, "loadout/loadout_" #M_NAME, "loadout/loadout_" #M_NAME "_no", "weapon_" #M_NAME };

#define WEAPON_DEF_ALT(M_NAME, M_DISPLAY_NAME, M_NAMEWEP) \
	inline const constexpr WeaponInfo WEP_INFO_##M_NAME = { \
		M_DISPLAY_NAME, "loadout/loadout_" #M_NAME, "loadout/loadout_" #M_NAME "_no", "weapon_" M_NAMEWEP };

#define WEAPON(M_NAME) WEP_INFO_##M_NAME

WEAPON_DEF(empty, "")
WEAPON_DEF(mpn, "MPN45")
WEAPON_DEF(srm, "SRM")
WEAPON_DEF(jitte, "Jitte")
WEAPON_DEF_ALT(srms, "SRM (silenced)", "srm_s")
WEAPON_DEF_ALT(jittes, "Jitte (with scope)", "jittescoped")
WEAPON_DEF(zr68l, "ZR68-L (accurized)")
WEAPON_DEF(zr68c, "ZR68C")
WEAPON_DEF(zr68s, "ZR68-S (silenced)")
WEAPON_DEF(supa7, "Murata Supa-7")
WEAPON_DEF_ALT(mosok, "Mosok", "m41")
WEAPON_DEF_ALT(mosokl, "Mosok Silenced", "m41s")
WEAPON_DEF(mx, "MX")
WEAPON_DEF_ALT(mxs, "MX Silenced", "mx_silenced")
WEAPON_DEF(aa13, "AA13")
WEAPON_DEF(srs, "SRS")
WEAPON_DEF(pz, "PZ252")
WEAPON_DEF(smac, "SMAC")
#ifdef INCLUDE_WEP_PBK
WEAPON_DEF(pbk56s, "PBK56")
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

