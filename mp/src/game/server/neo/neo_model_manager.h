#ifndef NEO_MODEL_MANAGER_H
#define NEO_MODEL_MANAGER_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_gamerules.h"
#include "neo_player_shared.h"

enum NeoGib {
	NEO_GIB_ALL = 0,
	NEO_GIB_HEAD,
	NEO_GIB_LARM,
	NEO_GIB_LLEG,
	NEO_GIB_RARM,
	NEO_GIB_RLEG,

	NEO_GIB_ENUM_COUNT
};

enum NeoViewmodel {
	NEO_VM_AA13 = 0,
	NEO_VM_DETPACK,
	NEO_VM_FRAG,
	NEO_VM_JITTE,
	NEO_VM_JITTE_S,
	NEO_VM_KYLA,
	NEO_VM_KNIFE,
	NEO_VM_MILSO,
	NEO_VM_MILSOS,
	NEO_VM_MOSOK41,
	NEO_VM_MOSOK41_S,
	NEO_VM_MPN,
	NEO_VM_MPN_S,
	NEO_VM_MX,
	NEO_VM_MX_S,
	NEO_VM_PROXMINE,
	NEO_VM_PZ,
	NEO_VM_SUPA7,
	NEO_VM_SMOKEGRENADE,
	NEO_VM_SRM,
	NEO_VM_SRM_S,
	NEO_VM_SRS,
	NEO_VM_TACHI,
	NEO_VM_ZR68_C,
	NEO_VM_ZR68_L,
	NEO_VM_ZR68_S,
	NEO_VM_GHOST,

	// NOTENOTE: this *must* be last, as we are using
	// array offsets, and VIP is not in that array
	NEO_VM_VIP_SMAC,

	NEO_VM_ENUM_COUNT
};

enum NeoWeapon {
	NEO_WEP_AA13 = 0,
	NEO_WEP_DETPACK,
	NEO_WEP_DETPACK_REMOTE,
	NEO_WEP_EQ_FRAGGRENADE,
	NEO_WEP_FRAGGRENADE,
	NEO_WEP_FRAGGRENADE_THROWN,
	NEO_WEP_FRAG,
	NEO_WEP_FRAG_THROWN,
	NEO_WEP_JITTE,
	NEO_WEP_JITTE_S,
	NEO_WEP_KYLA,
	NEO_WEP_KNIFE,
	NEO_WEP_MILSO,
	NEO_WEP_MILSO_S,
	NEO_WEP_MOSOK41,
	NEO_WEP_MOSOK41_S,
	NEO_WEP_MPN,
	NEO_WEP_MPN_FOLDED,
	NEO_WEP_MPN_S,
	NEO_WEP_MPN_S_FOLDED,
	NEO_WEP_MX,
	NEO_WEP_MX_S,
	NEO_WEP_PROXMINE,
	NEO_WEP_PZ252,
	NEO_WEP_SMAC,
	NEO_WEP_SMOKEGRENADE,
	NEO_WEP_SMOKEGRENADE_THROWN,
	NEO_WEP_SRM,
	NEO_WEP_SRM_S,
	NEO_WEP_SRS,
	NEO_WEP_SUPA7,
	NEO_WEP_TACHI,
	NEO_WEP_ZR68_C,
	NEO_WEP_ZR68_L,
	NEO_WEP_ZR68_S,
	NEO_WEP_GHOST,

	NEO_WEP_ENUM_COUNT
};

class CNEOModelManager;

static CNEOModelManager *instance;
class CNEOModelManager
{
public:
	static CNEOModelManager *Instance()
	{
		if (!instance)
		{
			instance = new CNEOModelManager();
		}
		return instance;
	}

	virtual ~CNEOModelManager()
	{
		instance = NULL;
	}

	void Precache(void) const;

	const char *GetCorpseModel(NeoSkin nSkin, NeoClass nClass,
		int iTeam, NeoGib nGib = NEO_GIB_ALL) const;
	
	const char *GetPlayerModel(NeoSkin nSkin,
		NeoClass nClass, int iTeam) const;
	
	const char *GetViewModel(NeoViewmodel nWepVm, int iTeam = TEAM_JINRAI) const;

	const char *GetWeaponModel(NeoWeapon nWep) const;

private:
	// We are singleton
	CNEOModelManager() { }
	CNEOModelManager( CNEOModelManager &);
	CNEOModelManager& operator=(CNEOModelManager const &);
};

#endif // NEO_MODEL_MANAGER_H