#include "cbase.h"
#include "weapon_ghost.h"
#include "neo_gamerules.h"
#include "neo_player_shared.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"

#include <engine/ivdebugoverlay.h>
#include <engine/IEngineSound.h>

#include "filesystem.h"
#else
#include "neo_player.h"
#include "neo_gamerules.h"
#include "player_resource.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_ctg_ghost_beacons_when_inactive("sv_neo_ctg_ghost_beacons_when_inactive", "0", FCVAR_NOTIFY|FCVAR_REPLICATED, "Show ghost beacons when the ghost isn't the active weapon.");

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponGhost, DT_WeaponGhost)

BEGIN_NETWORK_TABLE(CWeaponGhost, DT_WeaponGhost)
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE
#ifdef CLIENT_DLL
	RecvPropTime(RECVINFO(m_flDeployTime)),
	RecvPropTime(RECVINFO(m_flPickupTime)),
#else
	SendPropTime(SENDINFO(m_flDeployTime)),
	SendPropTime(SENDINFO(m_flPickupTime)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponGhost)
	DEFINE_NEO_BASE_WEP_PREDICTION
	DEFINE_PRED_FIELD_TOL(m_flDeployTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE, TICKS_TO_TIME(1)),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponGhost)

LINK_ENTITY_TO_CLASS(weapon_ghost, CWeaponGhost);

PRECACHE_WEAPON_REGISTER(weapon_ghost);

CWeaponGhost::CWeaponGhost(void)
{
#ifdef CLIENT_DLL
	m_bHavePlayedGhostEquipSound = false;
	m_bHaveHolsteredTheGhost = false;
#else
	m_flLastGhostBeepTime = 0;
#endif
	m_flDeployTime = 0;
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
#ifdef GAME_DLL
void CWeaponGhost::TryGhostPing(CNEO_Player* ghoster, float closestEnemy)
#else
void CWeaponGhost::TryGhostPing(float closestEnemy)
#endif
{
	if (IsServer() != sv_neo_serverside_beacons.GetBool())
		return;

	if (closestEnemy < 0 || closestEnemy > sv_neo_ghost_view_distance.GetFloat())
		return;

	constexpr auto freqMin = 1.0f, freqMax = 3.5f;
#ifdef GAME_DLL
	// Estimate outbound latency for more accurate server-sided beacon frequency
	const auto avgDelay = g_pPlayerResource->GetPing(ghoster->entindex()) / 1000.f / 2;
	constexpr auto minDivisor = 2;
	const float frequency = Clamp(0.1f * closestEnemy - avgDelay,
		Clamp(freqMin - avgDelay, freqMin / minDivisor, freqMin),
		Clamp(freqMax - avgDelay, freqMax / minDivisor, freqMax));
#else
	const float frequency = Clamp(0.1f * closestEnemy, freqMin, freqMax);
#endif
	const float deltaTime = gpGlobals->curtime - m_flLastGhostBeepTime;

	if (deltaTime > frequency)
	{
		EmitSound("NeoPlayer.GhostPing");
		m_flLastGhostBeepTime = gpGlobals->curtime;
	}
}

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