#include "cbase.h"
#include "weapon_ghost.h"
#include "neo_gamerules.h"
#include "neo_player_shared.h"

#ifdef CLIENT_DLL
#include "c_neo_npc_dummy.h"
#include "c_neo_player.h"
#include "c_team.h"

#include <engine/ivdebugoverlay.h>
#include <engine/IEngineSound.h>

#include "filesystem.h"
#else
#include "neo_npc_dummy.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "player_resource.h"
#include "team.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_ctg_ghost_beacons_when_inactive("sv_neo_ctg_ghost_beacons_when_inactive", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "Show ghost beacons when the ghost isn't the active weapon.");

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGhost, DT_WeaponGhost)

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGhost)
	DEFINE_NEO_BASE_WEP_PREDICTION
	DEFINE_PRED_FIELD_TOL(m_flDeployTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TICKS_TO_TIME(1)),
	DEFINE_PRED_FIELD(m_flNearestEnemyDist, FIELD_FLOAT, FTYPEDESC_INSENDTABLE | FTYPEDESC_NOERRORCHECK),
END_PREDICTION_DATA()
#endif

BEGIN_NETWORK_TABLE(CWeaponGhost, DT_WeaponGhost)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
#ifdef CLIENT_DLL
	RecvPropTime(RECVINFO(m_flDeployTime)),
	RecvPropTime(RECVINFO(m_flNearestEnemyDist)),
	RecvPropTime(RECVINFO(m_flPickupTime)),
#else
	SendPropTime(SENDINFO(m_flDeployTime)),
	SendPropTime(SENDINFO(m_flNearestEnemyDist)),
	SendPropTime(SENDINFO(m_flPickupTime)),
#endif
END_NETWORK_TABLE()

NEO_IMPLEMENT_ACTTABLE(CWeaponGhost)

LINK_ENTITY_TO_CLASS(weapon_ghost, CWeaponGhost);

PRECACHE_WEAPON_REGISTER(weapon_ghost);

CWeaponGhost::CWeaponGhost(void)
{
#ifdef CLIENT_DLL
	m_bHavePlayedGhostEquipSound = false;
	m_bHaveHolsteredTheGhost = false;
	m_flLastGhostBeepTime = 0;
#endif
	m_flDeployTime = 0;
	m_flNearestEnemyDist = FLT_MAX;
	m_flPickupTime = 0;
}

#ifdef GAME_DLL
CWeaponGhost::~CWeaponGhost()
{
	// At map exit, the rules may be gone before the ghost, so we check.
	if (auto* rules = NEORules())
		rules->ResetGhost();
}
#endif

void CWeaponGhost::ItemPreFrame(void)
{
#ifdef CLIENT_DLL
	if (!sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		HandleGhostEquip();
	}
#endif
}

#ifdef GAME_DLL
int CWeaponGhost::UpdateTransmitState()
{
	return SetTransmitState(FL_EDICT_ALWAYS);
}

int CWeaponGhost::ShouldTransmit(const CCheckTransmitInfo *pInfo)
{
	if (!pInfo)
	{
		return BaseClass::ShouldTransmit(pInfo);
	}
	assert_cast<CNEO_Player*>(Instance(pInfo->m_pClientEnt));
	return FL_EDICT_ALWAYS;
}
#endif

// Consider calling HandleGhostEquip instead.
void CWeaponGhost::PlayGhostSound(float volume)
{
#ifdef CLIENT_DLL
	auto owner = static_cast<C_BasePlayer*>(GetOwner());
#else
	auto owner = static_cast<CBasePlayer*>(GetOwner());
#endif // CLIENT_DLL
	if (!owner)
	{
		return;
	}

#ifdef CLIENT_DLL
	C_RecipientFilter filter;
#else
	CRecipientFilter filter;
#endif // CLIENT_DLL
	filter.AddRecipient(owner);

	EmitSound_t emitSoundType;
	emitSoundType.m_flVolume = volume;
	emitSoundType.m_pSoundName = "HUD.GhostEquip";
	emitSoundType.m_nFlags |= SND_SHOULDPAUSE | SND_DO_NOT_OVERWRITE_EXISTING_ON_CHANNEL;

	EmitSound(filter, entindex(), emitSoundType);
}

#ifdef CLIENT_DLL
void CWeaponGhost::HandleGhostEquip(void)
{
	if (!m_bHavePlayedGhostEquipSound)
	{
		PlayGhostSound();
		m_bHavePlayedGhostEquipSound = true;
		m_bHaveHolsteredTheGhost = false;
	}
}

void CWeaponGhost::HandleGhostUnequip(void)
{
	if (!m_bHaveHolsteredTheGhost)
	{
		StopGhostSound();
		m_bHaveHolsteredTheGhost = true;
		m_bHavePlayedGhostEquipSound = false;
	}
}

// Consider calling HandleGhostUnequip instead.
void CWeaponGhost::StopGhostSound(void)
{
	StopSound(this->entindex(), "HUD.GhostEquip");
}
#endif

// Emit a ghost ping at a refire interval based on distance.
#ifdef CLIENT_DLL
void CWeaponGhost::TryGhostPing()
{
	const auto* owner = GetOwner();
	if (!owner)
		return;

	if (!NEORules()->IsRoundLive() && NEORules()->GetGameType() != NEO_GAME_TYPE_TUT)
		return;

	if (!sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		const auto* wep = owner->GetActiveWeapon();
		if (!wep)
			return;
		if (!assert_cast<const CNEOBaseCombatWeapon*>(wep)->IsGhost())
			return;
	}

	if (m_flNearestEnemyDist > GetGhostRangeInHammerUnits())
		return;
	Assert(m_flNearestEnemyDist >= 0);

	const float closestEnemyInMeters = m_flNearestEnemyDist * METERS_PER_INCH;
	const float frequency = Clamp(0.1f * closestEnemyInMeters, 1.0f, 3.5f);
	const float deltaTime = gpGlobals->curtime - m_flLastGhostBeepTime;

	if (deltaTime > frequency)
	{
		EmitSound("NeoPlayer.GhostPing");
		m_flLastGhostBeepTime = gpGlobals->curtime;
	}
}
#endif

void CWeaponGhost::ItemHolsterFrame(void)
{
	BaseClass::ItemHolsterFrame();

#ifdef CLIENT_DLL
	if (!sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		HandleGhostUnequip(); // is there some callback, so we don't have to keep calling this?
	}
#endif
}

enum {
	NEO_GHOST_ONLY_ENEMIES = 0,
	NEO_GHOST_ONLY_PLAYABLE_TEAMS,
	NEO_GHOST_ANY_TEAMS
};
ConVar neo_ghost_debug_ignore_teams("neo_ghost_debug_ignore_teams", "0", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Debug ghost team filter. If 0, only ghost the enemy team. If 1, ghost both playable teams. If 2, ghost any team, including spectator and unassigned.", true, 0.0, true, 2.0);
ConVar neo_ghost_debug_spew("neo_ghost_debug_spew", "0", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Whether to spew debug logs to console about ghosting positions and the data PVS/server origin.", true, 0.0, true, 1.0);
ConVar neo_ghost_debug_hudinfo("neo_ghost_debug_hudinfo", "0", FCVAR_CHEAT | FCVAR_REPLICATED,
	"Whether to overlay debug text information to screen about ghosting targets.", true, 0.0, true, 1.0);

void CWeaponGhost::Equip(CBaseCombatCharacter *pNewOwner)
{
	BaseClass::Equip(pNewOwner);

	if (!pNewOwner)
		return;

	auto neoOwner = assert_cast<CNEO_Player*>(pNewOwner);

	// Prevent ghoster from sprinting
	if (neoOwner->IsSprinting())
	{
		neoOwner->StopSprinting();
	}

	neoOwner->m_bCarryingGhost = true;
	m_flPickupTime = gpGlobals->curtime; // passed as netprop to the client

#ifdef GAME_DLL // NEO NOTE (Adam) Fairly sure the above will never run client side and this whole thing could be surrounded by ifdef GAME_DLL, but I don't want weapons falling through the floor again if im wrong, so just leaving this comment here
	EmitSound_t soundParams;
	soundParams.m_pSoundName = "HUD.GhostPickUp";
	soundParams.m_nChannel = CHAN_GHOST_PICKUP;
	soundParams.m_bWarnOnDirectWaveReference = false;
	soundParams.m_bEmitCloseCaption = false;
	soundParams.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_NONE);

	CRecipientFilter soundFilter;
	soundFilter.AddAllPlayers();
	int ownerIndex = pNewOwner->entindex();
	soundFilter.RemoveRecipientByPlayerIndex(ownerIndex);
	soundFilter.MakeReliable();
	EmitSound(soundFilter, ownerIndex, soundParams);

	if (sv_neo_ctg_ghost_beacons_when_inactive.GetBool())
	{
		PlayGhostSound();
	}
#endif
}

void CWeaponGhost::UpdateNearestGhostBeaconDist()
{
	if (sv_neo_serverside_beacons.GetBool() != IsServer())
		return;

	CNEO_Player* ghoster;
	if (auto owner = GetOwner())
	{
		Assert(owner->IsPlayer());
		ghoster = assert_cast<CNEO_Player*>(owner);
	}
	else
		return;

	if (ghoster->IsObserver())
	{
		auto obsTarget = ghoster->GetObserverTarget();
		if (obsTarget && obsTarget->IsPlayer())
			ghoster = assert_cast<CNEO_Player*>(obsTarget);
	}
	Assert(ghoster->m_bCarryingGhost);

	const int ghosterTeam = ghoster->GetTeamNumber();
	Assert(ghosterTeam == TEAM_JINRAI || ghosterTeam == TEAM_NSF);
	const int enemyTeamId = ghosterTeam == TEAM_JINRAI ? TEAM_NSF : TEAM_JINRAI;
	auto* enemyTeam = GetGlobalTeam(enemyTeamId);
	const int enemyCount = enemyTeam->GetNumPlayers();

	float closestEnemy = FLT_MAX;
	// Human and bot enemies
	for (int i = 0; i < enemyCount; ++i)
	{
		float distTo;
		if (BeaconRange(enemyTeam->GetPlayer(i), distTo))
			closestEnemy = Min(closestEnemy, distTo);
	}
#ifdef CLIENT_DLL
#ifndef CNEO_NPCDummy
#define CNEO_NPCDummy C_NEO_NPCDummy
#endif
#endif
	// Dummy entity enemies (used in tutorial)
	for (auto* dummy = CNEO_NPCDummy::GetList(); dummy; dummy = dummy->m_pNext)
	{
		float distTo;
		if (BeaconRange(dummy, distTo))
			closestEnemy = Min(closestEnemy, distTo);
	}

	m_flNearestEnemyDist = closestEnemy;
}

bool CWeaponGhost::BeaconRange(CBaseEntity* enemy, float& outDistance) const
{
	if (!enemy)
		return false;
	const auto* ghoster = GetOwner();
	if (!ghoster)
		return false;
	if (!enemy->IsAlive() || enemy->IsDormant())
		return false;

	const auto& ghosterPos = ghoster->GetAbsOrigin();
	const auto& enemyPos = enemy->GetAbsOrigin();
	const auto dist = ghosterPos.DistTo(enemyPos);
	if (dist > GetGhostRangeInHammerUnits())
		return false;
	outDistance = dist;
	return true;
}

bool CWeaponGhost::Deploy()
{
	if (!BaseClass::Deploy())
		return false;
	m_flDeployTime = gpGlobals->curtime;
	return true;
}

void CWeaponGhost::Drop(const Vector &vecVelocity)
{
	if (GetOwner())
	{
		auto neoOwner = static_cast<CNEO_Player*>(GetOwner());
		neoOwner->m_bCarryingGhost = false;
	}
	BaseClass::Drop(vecVelocity);
#if !defined( CLIENT_DLL )
	SetRemoveable(false);
#endif
}

float CWeaponGhost::GetGhostRangeInHammerUnits()
{
	const float maxGhostRangeMeters = sv_neo_ghost_view_distance.GetFloat();
	return (maxGhostRangeMeters / METERS_PER_INCH);
}

bool CWeaponGhost::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}