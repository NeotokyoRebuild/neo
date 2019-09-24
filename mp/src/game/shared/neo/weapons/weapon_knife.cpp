#include "cbase.h"
#include "weapon_knife.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED (WeaponKnife, DT_WeaponKnife)

BEGIN_NETWORK_TABLE(CWeaponKnife, DT_WeaponKnife)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA(CWeaponKnife)
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS(weapon_knife, CWeaponKnife);
PRECACHE_WEAPON_REGISTER(weapon_knife);

#ifdef GAME_DLL // NEO FIXME (Rain): fix these values
acttable_t	CWeaponKnife::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE, ACT_IDLE_MELEE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE, false },
	{ ACT_MP_WALK, ACT_WALK, false },
	{ ACT_MP_JUMP, ACT_HOP, false },

	{ ACT_MP_AIRWALK, ACT_HOVER, false },
	{ ACT_MP_SWIM, ACT_SWIM, false },

	{ ACT_LEAP, ACT_LEAP, false },
	{ ACT_DIESIMPLE, ACT_DIESIMPLE, false },

	{ ACT_MP_CROUCH_IDLE, ACT_CROUCHIDLE, false },
	{ ACT_MP_CROUCHWALK, ACT_WALK_CROUCH, false },

	{ ACT_MP_RELOAD_STAND, ACT_RELOAD, false },

	{ ACT_CROUCHIDLE, ACT_HL2MP_IDLE_CROUCH_MELEE, false },
	{ ACT_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_MELEE, false }
};
IMPLEMENT_ACTTABLE(CWeaponKnife);
#endif

CWeaponKnife::CWeaponKnife(void)
{
}

void CWeaponKnife::Drop(const Vector &vecVelocity)
{
#ifdef GAME_DLL
	UTIL_Remove(this); // Knives should not drop in world
#endif
}

void CWeaponKnife::PrimaryAttack(void)
{
	const float thisTime = gpGlobals->curtime;
	static float lastAttack = thisTime;

	if (lastAttack != thisTime)
	{
		const float deltaTime = thisTime - lastAttack;

		if (deltaTime < GetFireRate())
		{
			return;
		}
	}

	BaseClass::PrimaryAttack();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	lastAttack = thisTime;
}