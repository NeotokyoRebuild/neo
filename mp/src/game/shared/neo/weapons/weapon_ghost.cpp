#include "cbase.h"
#include "weapon_ghost.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGhost, DT_WeaponGhost)

BEGIN_NETWORK_TABLE(CWeaponGhost, DT_WeaponGhost)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bShouldShowEnemies)),
#else
	SendPropBool(SENDINFO(m_bShouldShowEnemies)),
#endif
END_NETWORK_TABLE()

LINK_ENTITY_TO_CLASS(weapon_ghost, CWeaponGhost);
PRECACHE_WEAPON_REGISTER(weapon_ghost);

#ifdef GAME_DLL
acttable_t CWeaponGhost::m_acttable[] =
{
	{ ACT_IDLE,				ACT_IDLE_PISTOL,				false },
	{ ACT_RUN,				ACT_RUN_PISTOL,					false },
	{ ACT_CROUCHIDLE,		ACT_HL2MP_IDLE_CROUCH_PISTOL,	false },
	{ ACT_WALK_CROUCH,		ACT_HL2MP_WALK_CROUCH_PISTOL,	false },
	{ ACT_RANGE_ATTACK1,	ACT_RANGE_ATTACK_PISTOL,		false },
	{ ACT_RELOAD,			ACT_RELOAD_PISTOL,				false },
	{ ACT_JUMP,				ACT_HL2MP_JUMP_PISTOL,			false },
};
IMPLEMENT_ACTTABLE(CWeaponGhost);
#endif

CWeaponGhost::CWeaponGhost(void)
{
	m_bShouldShowEnemies = false;
}

void CWeaponGhost::ItemPreFrame(void)
{
	
}

void CWeaponGhost::ItemHolsterFrame(void)
{
	BaseClass::ItemHolsterFrame();
}

void CWeaponGhost::PrimaryAttack(void)
{
#ifdef CLIENT_DLL
	DevMsg("PrimaryAttack\n");
#endif
	SetShowEnemies(true);
	ShowEnemies();
}

void CWeaponGhost::SetShowEnemies(bool enabled)
{
	m_bShouldShowEnemies = enabled;
}

void CWeaponGhost::ShowEnemies(void)
{
	if (m_bShouldShowEnemies)
	{
#ifdef CLIENT_DLL
		C_NEO_Player *player = (C_NEO_Player*)GetOwner();
		if (player)
		{

		}
#else
		{
			DevMsg("Server block\n");

			// FIXME/HACK: we never deallocate, move this to class member variable
			const int pvsMaxSize = (engine->GetClusterCount() / 8);
			Assert(pvsMaxSize > 0);
			static unsigned char *pvs = new unsigned char[pvsMaxSize];

			CNEO_Player *player = (CNEO_Player*)GetOwner();
			if (player)
			{
				const int cluster = engine->GetClusterForOrigin(player->GetAbsOrigin());
				const int pvsSize = engine->GetPVSForCluster(cluster, pvsMaxSize, pvs);
				Assert(pvsSize > 0);

				for (int i = 1; i <= MAX_PLAYERS; i++)
				{
					CNEO_Player *otherPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

					// We're only interested in valid players that aren't the ghost owner.
					if (!otherPlayer || otherPlayer == player)
					{
						continue;
					}

					// If the other player is already in ghoster's PVS, we can skip them.
					else if (engine->CheckOriginInPVS(otherPlayer->GetAbsOrigin(), pvs, pvsSize))
					{
						continue;
					}

					DevMsg("Found player outside ghoster's PVS, coords: %f %f %f\n",
						otherPlayer->GetAbsOrigin().x, otherPlayer->GetAbsOrigin().y, otherPlayer->GetAbsOrigin().z);
				}
			}
		}
#endif
	}
}