#include "cbase.h"
#include "weapon_kyla.h"

#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponKyla, DT_WeaponKyla);

BEGIN_NETWORK_TABLE(CWeaponKyla, DT_WeaponKyla)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponKyla)
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponKyla)

LINK_ENTITY_TO_CLASS(weapon_kyla, CWeaponKyla);

PRECACHE_WEAPON_REGISTER(weapon_kyla);

CWeaponKyla::CWeaponKyla(void)
{
	m_bReloadsSingly = false;
	m_bFiresUnderwater = false;

	m_weaponSeeds = {
		"kylapx",
		"kylapy",
		"kylarx",
		"kylary",
	};
}

Activity CWeaponKyla::GetPrimaryAttackActivity()
{
	return ACT_VM_PRIMARYATTACK;
}

bool CWeaponKyla::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}