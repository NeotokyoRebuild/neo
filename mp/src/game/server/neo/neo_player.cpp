#include "cbase.h"
#include "neo_player.h"

#include "neo_playeranimstate.h"
#include "neo_predicted_viewmodel.h"
#include "in_buttons.h"
#include "neo_gamerules.h"
#include "team.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"
#include "datacache/imdlcache.h"

#include "neo_model_manager.h"

#include "weapon_ghost.h"
#include "weapon_supa7.h"

#include "shareddefs.h"
#include "inetchannelinfo.h"
#include "eiface.h"

#include "sequence_Transitioner.h"

#include "neo_te_tocflash.h"

#include "weapon_grenade.h"
#include "weapon_smokegrenade.h"

#include "weapon_knife.h"

#include "viewport_panel_names.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void NEODropPrimedFragGrenade(CNEO_Player *pPlayer, CBaseCombatWeapon *pGrenade);
void NEODropPrimedSmokeGrenade(CNEO_Player* pPlayer, CBaseCombatWeapon* pSmokeGrenade);

LINK_ENTITY_TO_CLASS(player, CNEO_Player);

IMPLEMENT_SERVERCLASS_ST(CNEO_Player, DT_NEO_Player)
SendPropInt(SENDINFO(m_iNeoClass)),
SendPropInt(SENDINFO(m_iNeoSkin)),
SendPropInt(SENDINFO(m_iXP)),
SendPropInt(SENDINFO(m_iCapTeam), 3),
SendPropInt(SENDINFO(m_iGhosterTeam)),
SendPropInt(SENDINFO(m_iLoadoutWepChoice)),
SendPropInt(SENDINFO(m_iNextSpawnClassChoice)),

SendPropBool(SENDINFO(m_bGhostExists)),
SendPropBool(SENDINFO(m_bInThermOpticCamo)),
SendPropBool(SENDINFO(m_bLastTickInThermOpticCamo)),
SendPropBool(SENDINFO(m_bInVision)),
SendPropBool(SENDINFO(m_bHasBeenAirborneForTooLongToSuperJump)),
SendPropBool(SENDINFO(m_bShowTestMessage)),
SendPropBool(SENDINFO(m_bInAim)),

SendPropTime(SENDINFO(m_flCamoAuxLastTime)),
SendPropInt(SENDINFO(m_nVisionLastTick)),

SendPropString(SENDINFO(m_pszTestMessage)),

SendPropVector(SENDINFO(m_vecGhostMarkerPos), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT),

SendPropArray(SendPropVector(SENDINFO_ARRAY(m_rvFriendlyPlayerPositions), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rvFriendlyPlayerPositions),

SendPropInt(SENDINFO(m_NeoFlags), 4, SPROP_UNSIGNED),
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Player)
DEFINE_FIELD(m_iNeoClass, FIELD_INTEGER),
DEFINE_FIELD(m_iNeoSkin, FIELD_INTEGER),
DEFINE_FIELD(m_iXP, FIELD_INTEGER),
DEFINE_FIELD(m_iCapTeam, FIELD_INTEGER),
DEFINE_FIELD(m_iGhosterTeam, FIELD_INTEGER),
DEFINE_FIELD(m_iLoadoutWepChoice, FIELD_INTEGER),
DEFINE_FIELD(m_iNextSpawnClassChoice, FIELD_INTEGER),

DEFINE_FIELD(m_bGhostExists, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInThermOpticCamo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bLastTickInThermOpticCamo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInVision, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHasBeenAirborneForTooLongToSuperJump, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShowTestMessage, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInAim, FIELD_BOOLEAN),

DEFINE_FIELD(m_flCamoAuxLastTime, FIELD_TIME),
DEFINE_FIELD(m_nVisionLastTick, FIELD_TICK),

DEFINE_FIELD(m_pszTestMessage, FIELD_STRING),

DEFINE_FIELD(m_vecGhostMarkerPos, FIELD_VECTOR),

DEFINE_FIELD(m_rvFriendlyPlayerPositions, FIELD_CUSTOM),

DEFINE_FIELD(m_NeoFlags, FIELD_CHARACTER),
END_DATADESC()

CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;
extern CBaseEntity *g_pLastSpawn;

extern ConVar neo_sv_ignore_wep_xp_limit;

ConVar sv_neo_can_change_classes_anytime("sv_neo_can_change_classes_anytime", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Can players change classes at any moment, even mid-round?",
	true, 0.0f, true, 1.0f);

void CNEO_Player::RequestSetClass(int newClass)
{
	if (newClass < 0 || newClass >= NEO_CLASS_ENUM_COUNT)
	{
		Assert(false);
		return;
	}

	if (IsDead() || sv_neo_can_change_classes_anytime.GetBool())
	{
		m_iNeoClass = newClass;
		m_iNextSpawnClassChoice = -1;

		SetPlayerTeamModel();
		SetViewOffset(VEC_VIEW_NEOSCALE(this));
		InitSprinting();
	}
	else
	{
		m_iNextSpawnClassChoice = newClass;

		const char* classes[] = { "recon", "assault", "support", "VIP" };
		const char* msg = "You will spawn as %s class next round.\n";
		Msg(msg, classes[newClass]);
		ConMsg(msg, classes[newClass]);
	}
}

void CNEO_Player::RequestSetSkin(int newSkin)
{
	bool canChangeImmediately = true; // TODO

	if (canChangeImmediately)
	{
		m_iNeoSkin = newSkin;

		SetPlayerTeamModel();
	}
	else
	{
		// TODO set for next spawn
	}
}

static bool IsNeoPrimary(CNEOBaseCombatWeapon *pNeoWep)
{
	if (!pNeoWep)
	{
		return false;
	}

	const auto bits = pNeoWep->GetNeoWepBits();
	Assert(bits > NEO_WEP_INVALID);
	const auto primaryBits = NEO_WEP_AA13 | NEO_WEP_GHOST | NEO_WEP_JITTE | NEO_WEP_JITTE_S |
		NEO_WEP_M41 | NEO_WEP_M41_L | NEO_WEP_M41_S | NEO_WEP_MPN | NEO_WEP_MPN_S |
		NEO_WEP_MX | NEO_WEP_MX_S | NEO_WEP_PZ | NEO_WEP_SMAC | NEO_WEP_SRM |
		NEO_WEP_SRM_S | NEO_WEP_SRS | NEO_WEP_SUPA7 | NEO_WEP_ZR68_C | NEO_WEP_ZR68_L |
		NEO_WEP_ZR68_S;

	return (primaryBits & bits) ? true : false;
}

bool CNEO_Player::RequestSetLoadout(int loadoutNumber)
{
	const char *pszWepName = GetWeaponByLoadoutId(loadoutNumber);

	if (FStrEq(pszWepName, ""))
	{
		Assert(false);
		Warning("CNEO_Player::RequestSetLoadout: Loadout request failed\n");
		return false;
	}

	EHANDLE pEnt;
	pEnt = CreateEntityByName(pszWepName);

	if (!pEnt)
	{
		Assert(false);
		Warning("NULL Ent in RequestSetLoadout!\n");
		return false;
	}

	pEnt->SetLocalOrigin(GetLocalOrigin());
	pEnt->AddSpawnFlags(SF_NORESPAWN);

	CNEOBaseCombatWeapon *pNeoWeapon = dynamic_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pEnt);
	if (!pNeoWeapon)
	{
		if (pEnt != NULL && !(pEnt->IsMarkedForDeletion()))
		{
			UTIL_Remove((CBaseEntity*)pEnt);
			Assert(false);
			Warning("CNEO_Player::RequestSetLoadout: Not a Neo base weapon: %s\n", pszWepName);
			return false;
		}
	}

	bool result = true;

	if (!IsNeoPrimary(pNeoWeapon))
	{
		Assert(false);
		Warning("CNEO_Player::RequestSetLoadout: Not a Neo primary weapon: %s\n", pszWepName);
		result = false;
	}

	if (!neo_sv_ignore_wep_xp_limit.GetBool() && m_iXP < pNeoWeapon->GetNeoWepXPCost(GetClass()))
	{
		DevMsg("Insufficient XP for %s\n", pszWepName);
		result = false;
	}

	if (result)
	{
		m_iLoadoutWepChoice = loadoutNumber;
	}
	
	if (pEnt != NULL && !(pEnt->IsMarkedForDeletion()))
	{
		UTIL_Remove((CBaseEntity*)pEnt);
	}

	return result;
}

void SetClass(const CCommand &command)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());

	if (!player)
	{
		return;
	}

	if (command.ArgC() == 2)
	{
		// Class number from the .res button click action.
		// Our NeoClass enum is zero indexed, so we subtract one.
		int nextClass = atoi(command.ArgV()[1]) - 1;
		
		clamp(nextClass, NEO_CLASS_RECON, NEO_CLASS_SUPPORT);

		player->RequestSetClass(nextClass);
	}
}

void SetSkin(const CCommand &command)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());

	if (!player)
	{
		return;
	}

	if (command.ArgC() == 2)
	{
		// Skin number from the .res button click action.
		// These are actually zero indexed by the .res already,
		// so we don't need to subtract from the value.
		int nextSkin = atoi(command.ArgV()[1]);

		clamp(nextSkin, NEO_SKIN_FIRST, NEO_SKIN_THIRD);

		player->RequestSetSkin(nextSkin);
	}
}

void SetLoadout(const CCommand &command)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());

	if (!player)
	{
		return;
	}

	if (command.ArgC() == 2)
	{
		int loadoutNumber = atoi(command.ArgV()[1]);
		player->RequestSetLoadout(loadoutNumber);
	}
}

ConCommand setclass("setclass", SetClass, "Set class", FCVAR_USERINFO);
ConCommand setskin("SetVariant", SetSkin, "Set skin", FCVAR_USERINFO);
ConCommand loadout("loadout", SetLoadout, "Set loadout", FCVAR_USERINFO);

static int GetNumOtherPlayersConnected(CNEO_Player *asker)
{
	if (!asker)
	{
		Assert(false);
		return 0;
	}

	int numConnected = 0;
	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));

		if (player && player != asker && player->IsConnected())
		{
			numConnected++;
		}
	}

	return numConnected;
}

CNEO_Player::CNEO_Player()
{
	m_iNeoClass = NEO_CLASS_ASSAULT;
	m_iNeoSkin = NEO_SKIN_FIRST;
	m_iXP.GetForModify() = 0;

	m_bGhostExists = false;
	m_bInThermOpticCamo = m_bInVision = false;
	m_bHasBeenAirborneForTooLongToSuperJump = false;
	m_bInAim = false;

	m_iCapTeam = TEAM_UNASSIGNED;
	m_iGhosterTeam = TEAM_UNASSIGNED;
	m_iLoadoutWepChoice = 0;
	m_iNextSpawnClassChoice = -1;

	m_bShowTestMessage = false;
	V_memset(m_pszTestMessage.GetForModify(), 0, sizeof(m_pszTestMessage));

	m_vecGhostMarkerPos = vec3_origin;

	ZeroFriendlyPlayerLocArray();

	m_flCamoAuxLastTime = 0;
	m_nVisionLastTick = 0;
	m_flLastAirborneJumpOkTime = 0;
	m_flLastSuperJumpTime = 0;

	m_bFirstDeathTick = true;
	m_bPreviouslyReloading = false;
	m_bLastTickInThermOpticCamo = false;
	m_bIsPendingSpawnForThisRound = false;

	m_flNextTeamChangeTime = gpGlobals->curtime + 0.5f;

	m_NeoFlags = 0;

	m_pPlayerAnimState = CreatePlayerAnimState(this, CreateAnimStateHelpers(this),
		NEO_ANIMSTATE_LEGANIM_TYPE, NEO_ANIMSTATE_USES_AIMSEQUENCES);
}

CNEO_Player::~CNEO_Player( void )
{
	m_pPlayerAnimState->Release();
}

void CNEO_Player::ZeroFriendlyPlayerLocArray(void)
{
	Assert(m_rvFriendlyPlayerPositions.Count() == MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		m_rvFriendlyPlayerPositions.Set(i, vec3_origin);
	}
	NetworkStateChanged();
}

void CNEO_Player::UpdateNetworkedFriendlyLocations()
{
#if(0)
#define PVS_MAX_SIZE (MAX_MAP_CLUSTERS + 1)
	byte pvs[PVS_MAX_SIZE]{};

	const int cluster = engine->GetClusterForOrigin(GetAbsOrigin());
	const int pvsSize = engine->GetPVSForCluster(cluster, PVS_MAX_SIZE, pvs);
	Assert(pvsSize > 0);
#endif

	Assert(MAX_PLAYERS == m_rvFriendlyPlayerPositions.Count());
	for (int i = 0; i < gpGlobals->maxClients; ++i)
	{
		// Skip self.
		if (i == GetClientIndex())
		{
			continue;
		}

		auto player = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i + 1));

		// Only teammates who are alive.
		if (!player || player->GetTeamNumber() != GetTeamNumber() || player->IsDead()
#if(1)
			)
#else // currently networking all friendlies, NEO TODO (Rain): optimise this
			// If the other player is already in our PVS, we can skip them.
			|| (engine->CheckOriginInPVS(otherPlayer->GetAbsOrigin(), pvs, pvsSize)))
#endif
		{
			continue;
		}

		Assert(player != this);
#if(0)
		if (!IsFakeClient())
		{
			DevMsg("Got here: my(edict: %i) VEC: %f %f %f -- other(edict: %i) VEC: %f %f %f\n",
				edict()->m_EdictIndex, GetAbsOrigin().x, GetAbsOrigin().y, GetAbsOrigin().z,
				player->edict()->m_EdictIndex, player->GetAbsOrigin().x, player->GetAbsOrigin().y, player->GetAbsOrigin().z);
		}
#endif

		m_rvFriendlyPlayerPositions.Set(i, player->GetAbsOrigin());
		Assert(m_rvFriendlyPlayerPositions[i].IsValid());
	}
}

void CNEO_Player::Precache( void )
{
	BaseClass::Precache();
}

void CNEO_Player::Spawn(void)
{
	ShowViewPortPanel(PANEL_SPECGUI, (GetTeamNumber() == TEAM_SPECTATOR ? true : false));

	// Should do this class update first, because most of the stuff below depends on which player class we are.
	if ((m_iNextSpawnClassChoice != -1) && (m_iNeoClass != m_iNextSpawnClassChoice))
	{
		m_iNeoClass = m_iNextSpawnClassChoice;
	}

	BaseClass::Spawn();

	SetNumAnimOverlays(NUM_LAYERS_WANTED);
	ResetAnimation();

	m_bIsPendingSpawnForThisRound = false;

	m_bLastTickInThermOpticCamo = m_bInThermOpticCamo = false;
	m_flCamoAuxLastTime = 0;

	m_bInVision = false;
	m_nVisionLastTick = 0;

	Weapon_SetZoom(false);

	SetTransmitState(FL_EDICT_PVSCHECK);

	SetPlayerTeamModel();
	SetViewOffset(VEC_VIEW_NEOSCALE(this));

	GiveLoadoutWeapon();
}

extern ConVar neo_lean_angle;

ConVar neo_lean_thirdperson_roll_lerp_scale("neo_lean_thirdperson_roll_lerp_scale", "5",
	FCVAR_REPLICATED | FCVAR_CHEAT, "Multiplier for 3rd person lean roll lerping.", true, 0.0, false, 0);

//just need to calculate lean angle in here
void CNEO_Player::Lean(void)
{
	auto vm = static_cast<CNEOPredictedViewModel*>(GetViewModel());
	if (vm)
	{
		Assert(GetBaseAnimating());
		GetBaseAnimating()->SetBoneController(0, vm->lean(this));
	}
}

void CNEO_Player::CheckVisionButtons()
{
	if (gpGlobals->tickcount - m_nVisionLastTick < TIME_TO_TICKS(0.1f))
	{
		return;
	}

	if (m_afButtonPressed & IN_VISION)
	{
		if (IsAlive())
		{
			m_nVisionLastTick = gpGlobals->tickcount;

			m_bInVision = !m_bInVision;

			if (m_bInVision)
			{
				CRecipientFilter filter;

				// NEO TODO/FIXME (Rain): optimise this loop to once per cycle instead of repeating for each client
				for (int i = 1; i <= gpGlobals->maxClients; ++i)
				{
					if (edict()->m_EdictIndex == i)
					{
						continue;
					}

					auto player = UTIL_PlayerByIndex(i);
					if (!player || !player->IsDead() || player->GetObserverMode() != OBS_MODE_IN_EYE)
					{
						continue;
					}

					if (player->GetObserverTarget() == this)
					{
						filter.AddRecipient(player);
					}
				}

				if (filter.GetRecipientCount() > 0)
				{
					static int visionToggle = CBaseEntity::PrecacheScriptSound("NeoPlayer.VisionOn");

					EmitSound_t params;
					params.m_bEmitCloseCaption = false;
					params.m_hSoundScriptHandle = visionToggle;
					params.m_pOrigin = &GetAbsOrigin();
					params.m_nChannel = CHAN_ITEM;

					EmitSound(filter, edict()->m_EdictIndex, params);
				}
			}
		}
	}
}

void CNEO_Player::PreThink(void)
{
	BaseClass::PreThink();

	if ((!GetActiveWeapon() && IsAlive()) ||
		// Whether or not we move backwards affects max speed
		((m_afButtonPressed | m_afButtonReleased) & IN_BACK))
	{
		if (GetFlags() & FL_DUCKING)
		{
			SetMaxSpeed(GetCrouchSpeed());
		}
		else if (IsWalking())
		{
			SetMaxSpeed(GetWalkSpeed());
		}
		else if (IsSprinting())
		{
			SetMaxSpeed(GetSprintSpeed());
		}
		else
		{
			SetMaxSpeed(GetNormSpeed());
		}
	}

	CheckThermOpticButtons();
	CheckVisionButtons();

	if (m_bInThermOpticCamo)
	{
		if (m_flCamoAuxLastTime == 0)
		{
			if (SuitPower_GetCurrentPercentage() >= CLOAK_AUX_COST)
			{
				m_flCamoAuxLastTime = gpGlobals->curtime;
			}
		}
		else
		{
			const float deltaTime = gpGlobals->curtime - m_flCamoAuxLastTime;
			if (deltaTime >= 1)
			{
				// Need to have at least this much spare AUX to enable.
				// This prevents AUX spam abuse where player has a sliver of AUX
				// each frame to never really run out.
				SuitPower_Drain(deltaTime * CLOAK_AUX_COST);

				if (SuitPower_GetCurrentPercentage() < CLOAK_AUX_COST)
				{
					m_bInThermOpticCamo = false;

					SuitPower_SetCharge(0);
					m_flCamoAuxLastTime = 0;
				}
				else
				{
					m_flCamoAuxLastTime = gpGlobals->curtime;
				}
			}
		}
	}
	else
	{
		m_flCamoAuxLastTime = 0;
	}

	Lean();

	// NEO HACK (Rain): Just bodging together a check for if we're allowed
	// to superjump, or if we've been airborne too long for that.
	// Ideally this should get cleaned up and moved to wherever
	// the regular engine jump does a similar check.
	bool newNetAirborneVal;
	if (IsAirborne())
	{
		m_flLastAirborneJumpOkTime = gpGlobals->curtime;
		const float deltaTime = gpGlobals->curtime - m_flLastAirborneJumpOkTime;
		const float leeway = 0.5f;
		if (deltaTime > leeway)
		{
			newNetAirborneVal = false;
			m_flLastAirborneJumpOkTime = gpGlobals->curtime;
		}
		else
		{
			newNetAirborneVal = true;
		}
	}
	else
	{
		newNetAirborneVal = false;
	}
	// Only send the network update if we actually changed state
	if (m_bHasBeenAirborneForTooLongToSuperJump != newNetAirborneVal)
	{
		m_bHasBeenAirborneForTooLongToSuperJump = newNetAirborneVal;
		NetworkStateChanged();
	}

	if (m_iNeoClass == NEO_CLASS_RECON)
	{
		if ((m_afButtonPressed & IN_JUMP) && (m_nButtons & IN_SPEED))
		{
			if (IsAllowedToSuperJump())
			{
				SuitPower_Drain(SUPER_JMP_COST);

				// If player holds both forward + back, only use up AUX power.
				// This movement trick replaces the original NT's trick of
				// sideways-superjumping with the intent of dumping AUX for a
				// jump setup that requires sprint jumping without the superjump.
				if (!((m_nButtons & IN_FORWARD) && (m_nButtons & IN_BACK)))
				{
					SuperJump();
				}
			}
			// Allow intentional AUX dump (see comment above)
			// even when not allowed to actually superjump.
			else if ((m_nButtons & IN_FORWARD) && (m_nButtons & IN_BACK))
			{
				SuitPower_Drain(SUPER_JMP_COST);
			}
		}
	}

	static int ghostEdict = -1;
	CWeaponGhost* ghost = dynamic_cast<CWeaponGhost*>(UTIL_EntityByIndex(ghostEdict));
	if (!ghost)
	{
		auto entIter = gEntList.FirstEnt();
		while (entIter)
		{
			ghost = dynamic_cast<CWeaponGhost*>(entIter);

			if (ghost)
			{
				ghostEdict = ghost->edict()->m_EdictIndex;
				break;
			}

			entIter = gEntList.NextEnt(entIter);
		}
	}

	m_bGhostExists = (ghost != NULL);

	if (m_bGhostExists)
	{
		Assert(UTIL_IsValidEntity(ghost));
		Assert(ghostEdict == ghost->edict()->m_EdictIndex);

		if (ghost->GetAbsOrigin().IsValid())
		{
			m_vecGhostMarkerPos = ghost->GetAbsOrigin();
		}
		else
		{
			Assert(false);
		}
	}

#if(0)
	auto entIter = gEntList.FirstEnt();
	int ghosts = 0;
	while (entIter)
	{
		ghost = dynamic_cast<CWeaponGhost*>(entIter);

		if (ghost)
		{
			ghosts++;
		}

		entIter = gEntList.NextEnt(entIter);
	}
	DevMsg("Num ghosts: %i\n", ghosts);
#endif

	if (IsAlive() && GetTeamNumber() != TEAM_SPECTATOR)
	{
		UpdateNetworkedFriendlyLocations();
	}
}

ConVar sv_neo_cloak_color_r("sv_neo_cloak_color_r", "1", FCVAR_CHEAT, "Thermoptic cloak flash color (red channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_g("sv_neo_cloak_color_g", "2", FCVAR_CHEAT, "Thermoptic cloak flash color (green channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_b("sv_neo_cloak_color_b", "4", FCVAR_CHEAT, "Thermoptic cloak flash color (blue channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_radius("sv_neo_cloak_color_radius", "128", FCVAR_CHEAT, "Thermoptic cloak flash effect radius.", true, 0.0f, true, 4096.0f);
ConVar sv_neo_cloak_time("sv_neo_cloak_time", "0.1", FCVAR_CHEAT, "How long should the thermoptic flash be visible, in seconds.", true, 0.0f, true, 1.0f);
ConVar sv_neo_cloak_decay("sv_neo_cloak_decay", "0", FCVAR_CHEAT, "After the cloak time, how quickly should the flash effect disappear.", true, 0.0f, true, 1.0f);
ConVar sv_neo_cloak_exponent("sv_neo_cloak_exponent", "8", FCVAR_CHEAT, "Cloak flash lighting exponent.", true, 0.0f, false, 0.0f);

void CNEO_Player::PlayCloakSound()
{
	CRecipientFilter filter;
	filter.AddRecipientsByPAS(GetAbsOrigin());
	filter.MakeReliable();
	// NEO TODO/FIXME (Rain): optimise this loop to once per cycle instead of repeating for each client
	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		if (edict()->m_EdictIndex == i)
		{
			continue;
		}

		auto player = UTIL_PlayerByIndex(i);
		if (!player || !player->IsDead() || player->GetObserverMode() != OBS_MODE_IN_EYE)
		{
			continue;
		}

		if (player->GetObserverTarget() == this)
		{
			filter.AddRecipient(player);
		}
	}
	filter.RemoveRecipient(this); // We play clientside for ourselves

	if (filter.GetRecipientCount() > 0)
	{
		static int tocOn = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOn");
		static int tocOff = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOff");

		EmitSound_t params;
		params.m_bEmitCloseCaption = false;
		params.m_hSoundScriptHandle = (m_bInThermOpticCamo ? tocOn : tocOff);
		params.m_pOrigin = &GetAbsOrigin();
		params.m_nChannel = CHAN_VOICE;

		EmitSound(filter, edict()->m_EdictIndex, params);
	}
}

void CNEO_Player::CloakFlash()
{
	CRecipientFilter filter;
	filter.AddRecipientsByPVS(GetAbsOrigin());

	g_NEO_TE_TocFlash.r = sv_neo_cloak_color_r.GetInt();
	g_NEO_TE_TocFlash.g = sv_neo_cloak_color_g.GetInt();
	g_NEO_TE_TocFlash.b = sv_neo_cloak_color_b.GetInt();
	g_NEO_TE_TocFlash.m_vecOrigin = GetAbsOrigin() + Vector(0, 0, 4);
	g_NEO_TE_TocFlash.exponent = sv_neo_cloak_exponent.GetInt();
	g_NEO_TE_TocFlash.m_fRadius = sv_neo_cloak_color_radius.GetFloat();
	g_NEO_TE_TocFlash.m_fTime = sv_neo_cloak_time.GetFloat();
	g_NEO_TE_TocFlash.m_fDecay = sv_neo_cloak_decay.GetFloat();

	g_NEO_TE_TocFlash.Create(filter);
}

void CNEO_Player::CheckThermOpticButtons()
{
	m_bLastTickInThermOpticCamo = m_bInThermOpticCamo;

	if ((m_afButtonPressed & IN_THERMOPTIC) && IsAlive())
	{
		if (GetClass() == NEO_CLASS_SUPPORT || GetClass() == NEO_CLASS_VIP)
		{
			return;
		}

		if (SuitPower_GetCurrentPercentage() >= CLOAK_AUX_COST)
		{
			m_bInThermOpticCamo = !m_bInThermOpticCamo;

			if (m_bInThermOpticCamo)
			{
				CloakFlash();
			}
		}
	}

	if (m_bInThermOpticCamo != m_bLastTickInThermOpticCamo)
	{
		PlayCloakSound();
	}
}

void CNEO_Player::SuperJump(void)
{
	Vector forward;
	AngleVectors(EyeAngles(), &forward);

	// We don't give an upwards boost aside from regular jump
	forward.z = 0;
	
	// Flip direction if jumping backwards
	if (m_nButtons & IN_BACK)
	{
		forward = -forward;
	}

	// NEO TODO (Rain): handle underwater case
	// NEO TODO (Rain): handle ladder mounted case
	// NEO TODO (Rain): handle "mounted" use key context case

	// NEO TODO (Rain): this disabled block below is taken from trigger_push implementation.
	// Probably wanna do some of this so we handle lag compensation accordingly; needs testing.
#if(0)
#if defined( HL2_DLL )
	// HACK HACK  HL2 players on ladders will only be disengaged if the sf is set, otherwise no push occurs.
	if ( pOther->IsPlayer() && 
		pOther->GetMoveType() == MOVETYPE_LADDER )
	{
		if ( !HasSpawnFlags(SF_TRIG_PUSH_AFFECT_PLAYER_ON_LADDER) )
		{
			// Ignore the push
			return;
		}
	}
#endif

	Vector vecPush = (m_flPushSpeed * vecAbsDir);
	if ((pOther->GetFlags() & FL_BASEVELOCITY) && !lagcompensation->IsCurrentlyDoingLagCompensation())
	{
		vecPush = vecPush + pOther->GetBaseVelocity();
	}
	if (vecPush.z > 0 && (pOther->GetFlags() & FL_ONGROUND))
	{
		pOther->SetGroundEntity(NULL);
		Vector origin = pOther->GetAbsOrigin();
		origin.z += 1.0f;
		pOther->SetAbsOrigin(origin);
	}

	pOther->SetBaseVelocity(vecPush);
	pOther->AddFlag(FL_BASEVELOCITY);
#endif

	ApplyAbsVelocityImpulse(forward * neo_recon_superjump_intensity.GetFloat());
}

bool CNEO_Player::IsAllowedToSuperJump(void)
{
	if (IsCarryingGhost())
		return false;

	if (GetMoveParent())
		return false;

	// Can't superjump whilst airborne (although it is kind of cool)
	if (m_bHasBeenAirborneForTooLongToSuperJump)
		return false;

	// Only superjump if we have a reasonable jump direction in mind
	// NEO TODO (Rain): should we support sideways superjumping?
	if ((m_nButtons & (IN_FORWARD | IN_BACK)) == 0)
	{
		return false;
	}

	if (SuitPower_GetCurrentPercentage() < SUPER_JMP_COST)
		return false;

	if (SUPER_JMP_DELAY_BETWEEN_JUMPS > 0)
	{
		m_flLastSuperJumpTime = gpGlobals->curtime;
		const float deltaTime = gpGlobals->curtime - m_flLastSuperJumpTime;
		if (deltaTime > SUPER_JMP_DELAY_BETWEEN_JUMPS)
			return false;

		m_flLastSuperJumpTime = gpGlobals->curtime;
	}

	return true;
}

void CNEO_Player::PostThink(void)
{
	BaseClass::PostThink();

	if (!IsAlive())
	{
		// Undo aim zoom if just died
		if (m_bFirstDeathTick)
		{
			m_bFirstDeathTick = false;

			Weapon_SetZoom(false);
			m_bInVision = false;
		}

		return;
	}
	else
	{
		m_bFirstDeathTick = true;
	}

	auto pWep = GetActiveWeapon();
	CNEOBaseCombatWeapon* pNeoWep = NULL;

	if (pWep)
	{
		if (pWep->m_bInReload && !m_bPreviouslyReloading)
		{
			Weapon_SetZoom(false);
		}
		else if (m_afButtonPressed & IN_SPEED)
		{
			Weapon_SetZoom(false);
		}
		else if (m_afButtonPressed & IN_AIM)
		{
			pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
			// Binds hack: we want grenade secondary attack to trigger on aim (mouse button 2)
			if (pNeoWep && pNeoWep->GetNeoWepBits() & NEO_WEP_THROWABLE)
			{
				static_cast<CWeaponGrenade*>(pNeoWep)->SecondaryAttack();
			}
			else
			{
				// NEO TODO (Rain): customizations for aim pressed/released/held behavior
				//if (pNeoWep != NULL) { Weapon_AimToggle(pWep); }
			}
		}
		else if (m_afButtonReleased & IN_AIM)
		{
			Weapon_AimToggle(pWep);
		}
		m_bPreviouslyReloading = pWep->m_bInReload;

		if (m_afButtonPressed & IN_DROP)
		{
			Vector eyeForward;
			EyeVectors(&eyeForward);
			const float forwardOffset = 250.0f;
			eyeForward *= forwardOffset;
			Weapon_Drop(pWep, NULL, &eyeForward);
		}
	}

#if(0)
	int iMoveX = LookupPoseParameter("move_x");
	int iMoveY = LookupPoseParameter("move_y");

	if (iMoveX != -1 && iMoveY != -1)
	{
		float speedScaleX = clamp((GetAbsVelocity().x / NEO_ASSAULT_NORM_SPEED), 0, 1);
		float speedScaleY = clamp((GetAbsVelocity().y / NEO_ASSAULT_NORM_SPEED), 0, 1);

		SetPoseParameter(iMoveX, speedScaleX);
		SetPoseParameter(iMoveY, speedScaleY);

		//DevMsg("Setspeed %f , %f\n", speedScaleX, speedScaleY);
	}
#endif

	Vector eyeForward;
	this->EyeVectors(&eyeForward, NULL, NULL);
	Assert(eyeForward.IsValid());
	m_pPlayerAnimState->Update(eyeForward[YAW], eyeForward[PITCH]);
}

void CNEO_Player::PlayerDeathThink()
{
	BaseClass::PlayerDeathThink();
}

void CNEO_Player::Weapon_AimToggle(CNEOBaseCombatWeapon* pNeoWep)
{
	if (!pNeoWep)
	{
		return;
	}

	if (IsAllowedToZoom(pNeoWep))
	{
		if (pNeoWep->IsReadyToAimIn())
		{
			const bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
			Weapon_SetZoom(showCrosshair);
		}
		else
		{
			Weapon_SetZoom(false);
		}
	}
}

void CNEO_Player::Weapon_AimToggle(CBaseCombatWeapon *pWep)
{
	// NEO TODO/HACK: Not all neo weapons currently inherit
	// through a base neo class, so we can't static_cast!!
	Weapon_AimToggle(dynamic_cast<CNEOBaseCombatWeapon*>(pWep));
}

void CNEO_Player::Weapon_SetZoom(const bool bZoomIn)
{
	const float zoomSpeedSecs = 0.25f;

	ShowCrosshair(bZoomIn);
	
	if (bZoomIn)
	{
		const int zoomAmount = 30;
		SetFOV((CBaseEntity*)this, GetDefaultFOV() - zoomAmount, zoomSpeedSecs);
	}
	else
	{
		SetFOV((CBaseEntity*)this, GetDefaultFOV(), zoomSpeedSecs);
	}

	m_bInAim = bZoomIn;
}

void CNEO_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	PlayerAnimEvent_t animEvent;
	if (!PlayerAnimToPlayerAnimEvent(playerAnim, animEvent))
	{
		DevWarning("SRV Tried to get unknown PLAYER_ANIM %d\n", playerAnim);
	}
	else
	{
		m_pPlayerAnimState->DoAnimationEvent(animEvent);
	}
	// Stopping; animations are handled by m_pPlayerAnimState->Update.
	// Should clean up this unused code later.
	return;

	/*

	auto activeWep = GetActiveWeapon();

	const char *pszAnimPrefix = (activeWep) ? activeWep->GetAnimPrefix() : "";
#define MAX_WEAPON_PREFIX_LEN 18	// "Crouch_Walk_Upper_" == 18
#define MAX_WEAPON_SUFFIX_LEN 6	// "pistol" == 6
#define NONE_STR ""
	char pszUpperAnim[MAX_WEAPON_PREFIX_LEN + MAX_WEAPON_SUFFIX_LEN + 1] = NONE_STR;
	char pszReloadAnim[MAX_WEAPON_PREFIX_LEN + MAX_WEAPON_SUFFIX_LEN + 1] = NONE_STR;

	int iLowerSequence = -1;
	int iUpperSequence = -1;

	if (idealActivity == ACT_NEO_JUMP)
	{
		iLowerSequence = LookupSequence("Jump");
	}
	else if (idealActivity == ACT_NEO_MOVE_RUN)
	{
		iLowerSequence = LookupSequence("Run_lower");

		const char *pszLayeredSequence = "Run_Upper_%s";
		V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
		iUpperSequence = LookupSequence(pszUpperAnim);
	}
	else if (idealActivity == ACT_NEO_MOVE_WALK)
	{
		iLowerSequence = LookupSequence("walk_lower");

		const char *pszLayeredSequence = "Walk_Upper_%s";
		V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
		iUpperSequence = LookupSequence(pszUpperAnim);
	}
	else if (idealActivity == ACT_NEO_IDLE_STAND)
	{
		iLowerSequence = LookupSequence("Idle_lower");

		const char *pszLayeredSequence = "Idle_Upper_%s";
		V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
		iUpperSequence = LookupSequence(pszUpperAnim);
	}
	else if (idealActivity == ACT_NEO_IDLE_CROUCH)
	{
		iLowerSequence = LookupSequence("Crouch_Idle_Lower");

		const char *pszLayeredSequence = "Crouch_Idle_Upper_%s";
		V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
		iUpperSequence = LookupSequence(pszUpperAnim);
	}
	else if (idealActivity == ACT_NEO_MOVE_CROUCH)
	{
		iLowerSequence = LookupSequence("Crouch_walk_lower");

		const char *pszLayeredSequence = "Crouch_Walk_Upper_%s";
		V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
		iUpperSequence = LookupSequence(pszUpperAnim);
	}
	else if (idealActivity == ACT_NEO_ATTACK)
	{
		if (GetFlags() & FL_DUCKING)
		{
			if (speed > 0)
			{
				iLowerSequence = LookupSequence("Crouch_walk_lower");

				const char *pszLayeredSequence = "Crouch_Walk_Shoot_%s";
				V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
				iUpperSequence = LookupSequence(pszUpperAnim);
			}
			else
			{
				iLowerSequence = LookupSequence("Crouch_Idle_Lower");

				const char *pszLayeredSequence = "Crouch_Idle_Shoot_%s";
				V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
				iUpperSequence = LookupSequence(pszUpperAnim);
			}
		}
		else
		{
			if (speed > 0)
			{
				if (speed > GetWalkSpeed())
				{
					iLowerSequence = LookupSequence("Run_lower");

					const char *pszLayeredSequence = "Run_Shoot_%s";
					V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
					iUpperSequence = LookupSequence(pszUpperAnim);
				}
				else
				{
					iLowerSequence = LookupSequence("walk_lower");

					const char *pszLayeredSequence = "Walk_Shoot_%s";
					V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
					iUpperSequence = LookupSequence(pszUpperAnim);
				}
			}
			else
			{
				iLowerSequence = LookupSequence("Idle_lower");

				const char *pszLayeredSequence = "Idle_Shoot_%s";
				V_sprintf_safe(pszUpperAnim, pszLayeredSequence, pszAnimPrefix);
				iUpperSequence = LookupSequence(pszUpperAnim);
			}
		}
	}

	if (iLowerSequence == -1)
	{
		//Assert(false);
		iLowerSequence = 0;
	}

	const bool bReloadAnimPlayingNow = ((bStartedReloading) || (activeWep && activeWep->m_bInReload));

	// Handle lower body animation
	if (GetSequence() != iLowerSequence || !SequenceLoops())
	{
		//DevMsg("Setting lower seq: %i\n", iLowerSequence);
		SetSequence(iLowerSequence);
		ResetSequence(iLowerSequence);
	}

	// Handle upper body animation
	if (iUpperSequence != -1)
	{
		if (!IsValidSequence(iUpperSequence))
		{
			Assert(false);
			Warning("CNEO_Player::SetAnimation: !IsValidSequence %i: (\"%s\")\n",
				iUpperSequence, pszUpperAnim);
		}
		else
		{
			const int iAimLayer = AddGestureSequence(iUpperSequence, true);
			SetLayerWeight(iAimLayer, 0.5f);
#if(0)
			if (!pAnimOverlay_Fire->IsActive() || pAnimOverlay_Fire->IsAbandoned())
			{
				pAnimOverlay_Fire->KillMe();
			}
			pAnimOverlay_Fire->Init(this);
			pAnimOverlay_Fire->m_nSequence = iUpperAimSequence;
			pAnimOverlay_Fire->StudioFrameAdvance(0.1, this);

			int seq = AddGestureSequence(iUpperAimSequence, true);
			if (IsValidSequence(seq))
			{
				SetLayerBlendIn(seq, 0.1f);
				SetLayerBlendOut(seq, 0.1f);
			}
#endif
		}
	}

	if (bReloadAnimPlayingNow)
	{
		const char *pszLayeredSequence = "Reload_%s";
		V_sprintf_safe(pszReloadAnim, pszLayeredSequence, pszAnimPrefix);
		Assert(!FStrEq(pszReloadAnim, NONE_STR));

		const int iUpperReloadSequence = LookupSequence(pszReloadAnim);

		const int iReloadLayer = AddGestureSequence(iUpperReloadSequence, true);
		SetLayerWeight(iReloadLayer, 0.5f);

#if(0)
		int seq = AddGestureSequence(iUpperReloadSequence, true);
		if (IsValidSequence(seq))
		{
			bool reloading = bReloadAnimPlayingNow;
			float layerCycle = GetLayerCycle(RELOADSEQUENCE_LAYER);
			int seq = iUpperReloadSequence;
			UpdateLayerSequenceGeneric(this, GetModelPtr(), RELOADSEQUENCE_LAYER, reloading, layerCycle, seq, false);
		}

		if (!IsValidSequence(iUpperReloadSequence))
		{
			Assert(false);
			Warning("CNEO_Player::SetAnimation: !IsValidSequence %i: (\"%s\")\n",
				iUpperReloadSequence, pszReloadAnim);
		}
		else
		{

			pAnimOverlay_Reload->Init(this);
			pAnimOverlay_Reload->m_nSequence = iUpperReloadSequence;
			pAnimOverlay_Reload->StudioFrameAdvance(0.1, this);

			int seq = AddGestureSequence(iUpperReloadSequence, true);
			if (IsValidSequence(seq))
			{
				SetLayerBlendIn(seq, 0.1f);
				SetLayerBlendOut(seq, 0.1f);
			}
		}
#endif
	}

#if(0)
	static CSequenceTransitioner transitioner;
	transitioner->RemoveAll();
	transitioner->CheckForSequenceChange(GetModelPtr(), GetSequence(), false, true);
	transitioner->UpdateCurrent(GetModelPtr(), GetSequence(), GetCycle(), 1.0f, gpGlobals->curtime);
#endif
	*/
}

// Purpose: Suicide, but cancel the point loss.
void CNEO_Player::SoftSuicide(void)
{
	if (IsDead())
	{
		Assert(false); // This should never get called on a dead client
		return;
	}

	m_fNextSuicideTime = gpGlobals->curtime;

	CommitSuicide();

	// HL2DM code will decrement, so we cancel it here
	IncrementFragCount(1);

	// Gamerules event will decrement, so we cancel it here
	m_iXP.GetForModify() += 1;
}

bool CNEO_Player::HandleCommand_JoinTeam( int team )
{
	const bool isAllowedToJoin = ProcessTeamSwitchRequest(team);

	if (isAllowedToJoin)
	{
		SetPlayerTeamModel();

		UTIL_ClientPrintAll(HUD_PRINTTALK, "%s1 joined team %s2\n",
			GetPlayerName(), GetTeam()->GetName());
	}

	return isAllowedToJoin;
}

bool CNEO_Player::ClientCommand( const CCommand &args )
{
	if (FStrEq(args[0], "playerstate_reverse"))
	{
		if (ShouldRunRateLimitedCommand(args))
		{
			// Player is reversing their HUD team join choice,
			// so allow instantly switching teams again.
			m_flNextTeamChangeTime = 0;
		}
		return true;
	}

	return BaseClass::ClientCommand(args);
}

void CNEO_Player::CreateViewModel( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );

	if ( GetViewModel( index ) )
	{
		return;
	}

	CNEOPredictedViewModel *vm = ( CNEOPredictedViewModel * )CreateEntityByName( "neo_predicted_viewmodel" );
	if ( vm )
	{
		vm->SetAbsOrigin( GetAbsOrigin() );
		vm->SetOwner( this );
		vm->SetIndex( index );
		DispatchSpawn( vm );
		vm->FollowEntity( this, false );
		m_hViewModel.Set( index, vm );
	}
	else
	{
		Warning("CNEO_Player::CreateViewModel: Failed to create neo_predicted_viewmodel\n");
		return;
	}
}

bool CNEO_Player::BecomeRagdollOnClient( const Vector &force )
{
	return BaseClass::BecomeRagdollOnClient(force);
}

void CNEO_Player::Event_Killed( const CTakeDamageInfo &info )
{
	// Drop all weapons
	Vector vecForward, pVecThrowDir;
	EyeVectors(&vecForward);
	VMatrix zRot;
	int numExplosivesDropped = 0;
	const int maxExplosivesToDrop = 1;
	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		auto pWep = m_hMyWeapons[i].Get();
		if (pWep)
		{
			auto pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
			if (pNeoWep && pNeoWep->IsExplosive())
			{
				if (++numExplosivesDropped > maxExplosivesToDrop)
				{
					continue;
				}
			}

			// Nowhere in particular; just drop it.
			MatrixBuildRotateZ(zRot, random->RandomFloat(-60.0f, 60.0f));
			Vector3DMultiply(zRot, vecForward, pVecThrowDir);
			pVecThrowDir.z = random->RandomFloat(-0.5f, 0.5f);
			VectorNormalize(pVecThrowDir);
			Assert(pVecThrowDir.IsValid());
			pWep->Drop(pVecThrowDir);
			pWep->SetRemoveable(false);
			Weapon_Detach(pWep);
		}
	}

	ShowViewPortPanel(PANEL_SPECGUI, true);

	BaseClass::Event_Killed(info);
}

float CNEO_Player::GetReceivedDamageScale(CBaseEntity* pAttacker)
{
	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	default:
		Assert(false);
		return BaseClass::GetReceivedDamageScale(pAttacker);
	}
}

bool CNEO_Player::WantsLagCompensationOnEntity( const CBasePlayer *pPlayer,
	const CUserCmd *pCmd,
	const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const
{
	return BaseClass::WantsLagCompensationOnEntity(pPlayer, pCmd,
		pEntityTransmitBits);
}

void CNEO_Player::FireBullets ( const FireBulletsInfo_t &info )
{
	BaseClass::FireBullets(info);
}

bool CNEO_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon,
	int viewmodelindex )
{
	ShowCrosshair(false);

	return BaseClass::Weapon_Switch(pWeapon, viewmodelindex);
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we can switch to the given weapon. Override 
//          from the base HL2MP player to let us swap to weapons with no ammo.
// Input  : pWeapon -
//-----------------------------------------------------------------------------
bool CNEO_Player::Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon)
{
    CBasePlayer *pPlayer = (CBasePlayer *)this;
#if !defined(CLIENT_DLL)
    IServerVehicle *pVehicle = pPlayer->GetVehicle();
#else
    IClientVehicle *pVehicle = pPlayer->GetVehicle();
#endif
    if (pVehicle && !pPlayer->UsingStandardWeaponsInVehicle())
        return false;

    // NEO: Needs to be commented out to let us swap to empty weapons
    // if ( !pWeapon->HasAnyAmmo() && !GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) )
    // 	return false;

    if (!pWeapon->CanDeploy())
        return false;

    if (GetActiveWeapon())
    {
        if (!GetActiveWeapon()->CanHolster())
            return false;
    }

    return true;
}

bool CNEO_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	// We already have a weapon in this slot
	if (Weapon_GetSlot(pWeapon->GetSlot()))
	{
		return false;
	}
	else if (GetClass() == NEO_CLASS_RECON)
	{
		auto pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWeapon);
		// Recons can't carry a PZ
		if ((pNeoWep) && (pNeoWep->GetNeoWepBits() & NEO_WEP_PZ))
		{
			return false;
		}
	}

	return BaseClass::BumpWeapon(pWeapon);
}

void CNEO_Player::ChangeTeam( int iTeam )
{
	const bool isAllowedToChange = ProcessTeamSwitchRequest(iTeam);

	if (isAllowedToChange)
	{
		SetPlayerTeamModel();
	}
}

void CNEO_Player::SetPlayerTeamModel( void )
{
	CNEOModelManager *modelManager = CNEOModelManager::Instance();
	if (!modelManager)
	{
		Assert(false);
		Warning("Failed to get Neo model manager\n");
		return;
	}

	const char *model = modelManager->GetPlayerModel(
		static_cast<NeoSkin>(GetSkin()),
		static_cast<NeoClass>(GetClass()),
		GetTeamNumber());

	if (!*model)
	{
		Assert(false);
		Warning("Failed to find model string for Neo player\n");
		return;
	}

	SetModel(model);
	SetPlaybackRate(1.0f);
	ResetAnimation();

	DevMsg("Set model: %s\n", model);

	//SetupPlayerSoundsByModel(model); // TODO

#if(0)
	const int modelIndex = modelinfo->GetModelIndex(model);
	const model_t *modelType = modelinfo->GetModel(modelIndex);
	MDLHandle_t modelCache = modelinfo->GetCacheHandle(modelType);
#endif
}

void CNEO_Player::PickupObject( CBaseEntity *pObject,
	bool bLimitMassAndSize )
{
	BaseClass::PickupObject(pObject, bLimitMassAndSize);
}

void CNEO_Player::PlayStepSound( Vector &vecOrigin,
	surfacedata_t *psurface, float fvol, bool force )
{
	BaseClass::PlayStepSound(vecOrigin, psurface, fvol, force);
}

bool CNEO_Player::IsCarryingGhost(void) const
{
	return GetNeoWepWithBits(this, NEO_WEP_GHOST) != NULL;
}

// Is the player allowed to drop a weapon of this type?
bool CNEO_Player::IsAllowedToDrop(CBaseCombatWeapon *pWep)
{
	if (!pWep)
	{
		return false;
	}

	// We can static_cast if it's guaranteed that all weapons will be Neotokyo type.
	// Allowing HL2DM weapons, for now.
#define SUPPORT_NON_NEOTOKYO_WEAPONS 1 // NEO FIXME (Rain): knife is not yet a neo inherited weapon

#if SUPPORT_NON_NEOTOKYO_WEAPONS
	auto pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
	if (!pNeoWep)
	{
		if (dynamic_cast<CWeaponKnife*>(pWep))
		{
			return false;
		}
		// This was not a Neotokyo weapon. Don't know what it is, but allow dropping.
		else
		{
			return true;
		}
	}
#else
	Assert(dynamic_cast<CNEOBaseCombatWeapon*>(pWep));
	auto pNeoWep = static_cast<CNEOBaseCombatWeapon*>(pWep);
#endif

	const auto wepBits = pNeoWep->GetNeoWepBits();
	Assert(wepBits > NEO_WEP_INVALID);

	const auto unallowedDrops = (NEO_WEP_KNIFE | NEO_WEP_PROX_MINE | NEO_WEP_THROWABLE);
	return ((wepBits & unallowedDrops) == 0);
}

void CNEO_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon,
	const Vector *pvecTarget, const Vector *pVelocity )
{
	if (!IsDead() && pWeapon)
	{
		if (!IsAllowedToDrop(pWeapon))
		{
			// NEO TODO (Rain): No need for server to tell
			// client to play a sound effect, move this to client.
			{
#if (0)
				CRecipientFilter filter;
				filter.AddRecipient(this);

				EmitSound_t sound;
				sound.m_nChannel = CHAN_ITEM;
				// NEO TODO (Rain): Find appropriate sound for denying drop.
				// Possibly the original NT "can't +use" sound.
				sound.m_pSoundName = "player/CPneutral.wav";
				sound.m_SoundLevel = SNDLVL_30dB;
				sound.m_flVolume = VOL_NORM;

				// NEO HACK/FIXME (Rain): This should get early precached
				// as part of our regular sound scripts on this::Precache()
				PrecacheScriptSound(sound.m_pSoundName);

				EmitSound(filter, edict()->m_EdictIndex, sound);
#endif
			}

			return;
		}

		auto neoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWeapon);
		if (neoWep)
		{
			if (neoWep->GetNeoWepBits() & NEO_WEP_SUPA7)
			{
				Assert(dynamic_cast<CWeaponSupa7*>(neoWep));
				static_cast<CWeaponSupa7*>(neoWep)->ClearDelayedInputs();
			}
		}
	}
	else
	{
		auto activeWep = GetActiveWeapon();
		if (activeWep)
		{
			// If player has held down an attack key since the previous frame
			if (((m_nButtons & IN_ATTACK) && (!(m_afButtonPressed & IN_ATTACK))) ||
				((m_nButtons & IN_ATTACK2) && (!(m_afButtonPressed & IN_ATTACK2))))
			{
				// Drop a grenade if it's primed.
				if (activeWep == Weapon_OwnsThisType("weapon_grenade"))
				{
					NEODropPrimedFragGrenade(this, GetActiveWeapon());
					return;
				}
				// Drop a smoke if it's primed.
				else if (activeWep == Weapon_OwnsThisType("weapon_smokegrenade"))
				{
					NEODropPrimedSmokeGrenade(this, activeWep);
					return;
				}
			}
		}
	}

	BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
}

void CNEO_Player::UpdateOnRemove( void )
{
	BaseClass::UpdateOnRemove();
}

void CNEO_Player::DeathSound( const CTakeDamageInfo &info )
{
	BaseClass::DeathSound(info);
}

// NEO TODO (Rain): this is mostly copied from hl2mp code,
// should rewrite to get rid of the gotos, and deal with any
// cases of multiple spawn overlaps at the same time.
#define TELEFRAG_ON_OVERLAPPING_SPAWN 0
CBaseEntity* CNEO_Player::EntSelectSpawnPoint( void )
{
#if TELEFRAG_ON_OVERLAPPING_SPAWN
	edict_t		*player = edict();
#endif

	CBaseEntity *pSpot = NULL;
	CBaseEntity *pLastSpawnPoint = g_pLastSpawn;
	const char *pSpawnpointName = "info_player_start";

	if (NEORules()->IsTeamplay())
	{
		if (GetTeamNumber() == TEAM_JINRAI)
		{
			pSpawnpointName = "info_player_attacker";
			pLastSpawnPoint = g_pLastJinraiSpawn;
		}
		else if (GetTeamNumber() == TEAM_NSF)
		{
			pSpawnpointName = "info_player_defender";
			pLastSpawnPoint = g_pLastNSFSpawn;
		}

		if (gEntList.FindEntityByClassname(NULL, pSpawnpointName) == NULL)
		{
			pSpawnpointName = "info_player_start";
			pLastSpawnPoint = g_pLastSpawn;
		}
	}

	pSpot = pLastSpawnPoint;
	// Randomize the start spot
	for (int i = random->RandomInt(1, 5); i > 0; i--)
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnpointName);
	if (!pSpot)  // skip over the null point
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnpointName);

	CBaseEntity *pFirstSpot = pSpot;

	do
	{
		if (pSpot)
		{
			// check if pSpot is valid
			if (g_pGameRules->IsSpawnPointValid(pSpot, this))
			{
				if (pSpot->GetLocalOrigin() == vec3_origin)
				{
					pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnpointName);
					continue;
				}

				// if so, go to pSpot
				goto ReturnSpot;
			}
		}
		// increment pSpot
		pSpot = gEntList.FindEntityByClassname(pSpot, pSpawnpointName);
	} while (pSpot != pFirstSpot); // loop if we're not back to the start

	// we haven't found a place to spawn yet, so kill any guy at the first spawn point and spawn there
	if (pSpot)
	{
#if TELEFRAG_ON_OVERLAPPING_SPAWN
		CBaseEntity *ent = NULL;
		for (CEntitySphereQuery sphere(pSpot->GetAbsOrigin(), 128); (ent = sphere.GetCurrentEntity()) != NULL; sphere.NextEntity())
		{
			// if ent is a client, kill em (unless they are ourselves)
			if (ent->IsPlayer() && !(ent->edict() == player))
				ent->TakeDamage(CTakeDamageInfo(GetContainingEntity(INDEXENT(0)), GetContainingEntity(INDEXENT(0)), 300, DMG_GENERIC));
		}
#endif
		goto ReturnSpot;
	}

	if (!pSpot)
	{
		pSpot = gEntList.FindEntityByClassname(pSpot, "info_player_start");

		if (pSpot)
			goto ReturnSpot;
	}

ReturnSpot:

	if (NEORules()->IsTeamplay())
	{
		if (GetTeamNumber() == TEAM_JINRAI)
		{
			g_pLastJinraiSpawn = pSpot;
		}
		else if (GetTeamNumber() == TEAM_NSF)
		{
			g_pLastNSFSpawn = pSpot;
		}
	}

	g_pLastSpawn = pSpot;

	m_flSlamProtectTime = gpGlobals->curtime + 0.5;

	return pSpot;
}

bool CNEO_Player::StartObserverMode(int mode)
{
	return BaseClass::StartObserverMode(mode);
}

void CNEO_Player::StopObserverMode()
{
	BaseClass::StopObserverMode();
}

bool CNEO_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	return BaseClass::CanHearAndReadChatFrom(pPlayer);
}

void CNEO_Player::PickDefaultSpawnTeam(void)
{
	if (!GetTeamNumber())
	{
		if (!NEORules()->IsTeamplay())
		{
			ProcessTeamSwitchRequest(TEAM_SPECTATOR);
		}
		else
		{
			ProcessTeamSwitchRequest(TEAM_SPECTATOR);

#if(0) // code for random team selection
			CTeam *pJinrai = g_Teams[TEAM_JINRAI];
			CTeam *pNSF = g_Teams[TEAM_NSF];

			if (!pJinrai || !pNSF)
			{
				ProcessTeamSwitchRequest(random->RandomInt(TEAM_JINRAI, TEAM_NSF));
			}
			else
			{
				if (pJinrai->GetNumPlayers() > pNSF->GetNumPlayers())
				{
					ProcessTeamSwitchRequest(TEAM_NSF);
				}
				else if (pNSF->GetNumPlayers() > pJinrai->GetNumPlayers())
				{
					ProcessTeamSwitchRequest(TEAM_JINRAI);
				}
				else
				{
					ProcessTeamSwitchRequest(random->RandomInt(TEAM_JINRAI, TEAM_NSF));
				}
			}
#endif
		}

		if (!GetModelPtr())
		{
			SetPlayerTeamModel();
		}
	}
}

bool CNEO_Player::ProcessTeamSwitchRequest(int iTeam)
{
	if (!GetGlobalTeam(iTeam) || iTeam == 0)
	{
		Warning("HandleCommand_JoinTeam( %d ) - invalid team index.\n", iTeam);
		return false;
	}

	// NEO TODO (Rain): add server cvars
#define TEAM_CHANGE_SUICIDE true
#define TEAM_CHANGE_INTERVAL 5.0f

	const bool justJoined = (GetTeamNumber() == TEAM_UNASSIGNED);

	// Player bots should initially join a player team.
	// Note that we can't do a ->IsBot check here, because the bot has not
	// received its fakeclient flags yet at this point. Hence using the
	// m_bNextClientIsFakeClient workaround.
	if (justJoined && NEORules()->m_bNextClientIsFakeClient && !IsHLTV())
	{
		Assert(gpGlobals->curtime >= m_flNextTeamChangeTime);
		iTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
	}
	// Limit team join spam, unless this is a newly joined player
	else if (!justJoined && m_flNextTeamChangeTime > gpGlobals->curtime)
	{
		// Except for spectators, who are allowed to join a team as soon as they wish
		if (GetTeamNumber() != TEAM_SPECTATOR)
		{
			char szWaitTime[5];
			V_sprintf_safe(szWaitTime, "%i", MAX(1, RoundFloatToInt(m_flNextTeamChangeTime - gpGlobals->curtime)));
			ClientPrint(this, HUD_PRINTTALK, "Please wait %s1 seconds before switching teams again.", szWaitTime);
			return false;
		}
	}

	if (iTeam == TEAM_SPECTATOR)
	{
		if (!mp_allowspectators.GetInt())
		{
			if (justJoined)
			{
				AssertMsg(false, "Client just joined, but was denied default team join!");
				return ProcessTeamSwitchRequest(RandomInt(TEAM_JINRAI, TEAM_NSF));
			}

			ClientPrint(this, HUD_PRINTTALK, "#Cannot Be Spectator");

			return false;
		}

		if (TEAM_CHANGE_SUICIDE)
		{
			// Unassigned implies we just joined.
			if (!justJoined && !IsDead())
			{
				SoftSuicide();
			}
		}

		// Default to free fly camera if there's nobody to spectate
		if (justJoined || GetNumOtherPlayersConnected(this) == 0)
		{
			StartObserverMode(OBS_MODE_ROAMING);
		}
		else
		{
			StartObserverMode(m_iObserverLastMode);
		}

		State_Transition(STATE_OBSERVER_MODE);
	}
	else if (iTeam == TEAM_JINRAI || iTeam == TEAM_NSF)
	{
		if (TEAM_CHANGE_SUICIDE && !justJoined && GetTeamNumber() != TEAM_SPECTATOR && !IsDead())
		{
			SoftSuicide();
		}

		StopObserverMode();

		State_Transition(STATE_ACTIVE);
	}
	else
	{
		// Client should not be able to reach this
		Assert(false);

#define SWITCH_FAIL_MSG "Team switch failed; unrecognized Neotokyo team specified."
		ClientPrint(this, HUD_PRINTTALK, SWITCH_FAIL_MSG);
		Warning(SWITCH_FAIL_MSG);

		return false;
	}

	m_flNextTeamChangeTime = gpGlobals->curtime + TEAM_CHANGE_INTERVAL;
	
	RemoveAllItems(true);
	ShowCrosshair(false);

	if (iTeam == TEAM_JINRAI || iTeam == TEAM_NSF)
	{
		SetPlayerTeamModel();
	}

	// We're skipping over HL2MP player because we don't care about
	// deathmatch rules or Combine/Rebels model stuff.
	CHL2_Player::ChangeTeam(iTeam, false, justJoined);

	return true;
}

void GiveDet(CNEO_Player* pPlayer)
{
	const char* detpackClassname = "weapon_remotedet";
	if (!pPlayer->Weapon_OwnsThisType(detpackClassname))
	{
		EHANDLE pent = CreateEntityByName(detpackClassname);
		if (pent == NULL)
		{
			Assert(false);
			Warning("Failed to spawn %s at loadout!\n", detpackClassname);
		}
		else
		{
			pent->SetLocalOrigin(pPlayer->GetLocalOrigin());
			pent->AddSpawnFlags(SF_NORESPAWN);

			auto pWeapon = dynamic_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pent);
			if (pWeapon)
			{
				const int detXpCost = pWeapon->GetNeoWepXPCost(pPlayer->GetClass());
				// Cost of -1 XP means no XP cost.
				const bool canHaveDet = (detXpCost < 0 || pPlayer->m_iXP >= detXpCost);

				pWeapon->SetSubType(0);
				if (canHaveDet)
				{
					DispatchSpawn(pent);

					if (pent != NULL && !(pent->IsMarkedForDeletion()))
					{
						pent->Touch(pPlayer);
					}
				}
				else
				{
					UTIL_Remove(pWeapon);
				}
			}
		}
	}
}

void CNEO_Player::GiveDefaultItems(void)
{
	CBasePlayer::GiveAmmo(50, "AR2");

	CBasePlayer::GiveAmmo(50, "Pistol");
	CBasePlayer::GiveAmmo(30, "AMMO_10G_SHELL");
	CBasePlayer::GiveAmmo(50, "AMMO_PRI");
	CBasePlayer::GiveAmmo(50, "AMMO_SMAC");
	CBasePlayer::GiveAmmo(1, "AMMO_DETPACK");

	const bool supportsGetKnife = true;

	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_milso");
		GiveDet(this);
		Weapon_Switch(Weapon_OwnsThisType("weapon_milso"));
		break;
	case NEO_CLASS_ASSAULT:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_tachi");
		GiveNamedItem("weapon_grenade");
		Weapon_Switch(Weapon_OwnsThisType("weapon_tachi"));
		break;
	case NEO_CLASS_SUPPORT:
		if (supportsGetKnife) { GiveNamedItem("weapon_knife"); }
		GiveNamedItem("weapon_kyla");
		GiveNamedItem("weapon_smokegrenade");
		Weapon_Switch(Weapon_OwnsThisType("weapon_kyla"));
		break;
	default:
		GiveNamedItem("weapon_knife");
		break;
	}
}

void CNEO_Player::GiveLoadoutWeapon(void)
{
	if (IsObserver() || IsDead())
	{
		return;
	}

	const char *szWep = GetWeaponByLoadoutId(m_iLoadoutWepChoice);
#if DEBUG
	DevMsg("Loadout slot: %i (\"%s\")\n", m_iLoadoutWepChoice, szWep);
#endif

	// If I already own this type don't create one
	const int wepSubType = 0;
	if (Weapon_OwnsThisType(szWep, wepSubType))
	{
		return;
	}

	EHANDLE pEnt;
	pEnt = CreateEntityByName(szWep);

	if (!pEnt)
	{
		Assert(false);
		Warning("NULL Ent in GiveLoadoutWeapon!\n");
		return;
	}

	pEnt->SetLocalOrigin(GetLocalOrigin());
	pEnt->AddSpawnFlags(SF_NORESPAWN);

	CNEOBaseCombatWeapon *pNeoWeapon = dynamic_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pEnt);
	if (pNeoWeapon)
	{
		if (neo_sv_ignore_wep_xp_limit.GetBool() || (m_iXP >= pNeoWeapon->GetNeoWepXPCost(GetClass())))
		{
			pNeoWeapon->SetSubType(wepSubType);

			DispatchSpawn(pEnt);

			if (pEnt != NULL && !(pEnt->IsMarkedForDeletion()))
			{
				pEnt->Touch(this);
				Weapon_Switch(Weapon_OwnsThisType(szWep));
			}
		}
		else
		{
			if (pEnt != NULL && !(pEnt->IsMarkedForDeletion()))
			{
				UTIL_Remove(pEnt);
			}
		}
	}
}

// NEO TODO
void CNEO_Player::GiveAllItems(void)
{
	// NEO TODO (Rain): our own ammo types
	CBasePlayer::GiveAmmo(45, "SMG1");
	CBasePlayer::GiveAmmo(1, "grenade");
	CBasePlayer::GiveAmmo(6, "Buckshot");
	CBasePlayer::GiveAmmo(6, "357");
	CBasePlayer::GiveAmmo(150, "AR2");

	CBasePlayer::GiveAmmo(255, "Pistol");
	CBasePlayer::GiveAmmo(30, "AMMO_10G_SHELL");
	CBasePlayer::GiveAmmo(150, "AMMO_PRI");
	CBasePlayer::GiveAmmo(150, "AMMO_SMAC");
	CBasePlayer::GiveAmmo(1, "AMMO_DETPACK");

	GiveNamedItem("weapon_tachi");
	GiveNamedItem("weapon_zr68s");
	Weapon_Switch(Weapon_OwnsThisType("weapon_zr68s"));
}

// Purpose: For Neotokyo, we could use this engine method
// to setup class specific armor, abilities, etc.
void CNEO_Player::EquipSuit(bool bPlayEffects)
{
	//MDLCACHE_CRITICAL_SECTION();

	BaseClass::EquipSuit();
}

void CNEO_Player::RemoveSuit(void)
{
	BaseClass::RemoveSuit();
}

void CNEO_Player::SendTestMessage(const char *message)
{
	V_memcpy(m_pszTestMessage.GetForModify(), message, sizeof(m_pszTestMessage));

	m_bShowTestMessage = true;
}

void CNEO_Player::SetTestMessageVisible(bool visible)
{
	m_bShowTestMessage = visible;
}

void CNEO_Player::StartAutoSprint(void)
{
	BaseClass::StartAutoSprint();
}

void CNEO_Player::StartSprinting(void)
{
	if (GetClass() != NEO_CLASS_RECON && m_HL2Local.m_flSuitPower < 10)
	{
		return;
	}

	if (IsCarryingGhost())
	{
		return;
	}

	BaseClass::StartSprinting();

	SetMaxSpeed(GetSprintSpeed());
}

void CNEO_Player::StopSprinting(void)
{
	BaseClass::StopSprinting();

	SetMaxSpeed(GetNormSpeed());
}

void CNEO_Player::InitSprinting(void)
{
	BaseClass::InitSprinting();
}

bool CNEO_Player::CanSprint(void)
{
	if (m_iNeoClass == NEO_CLASS_SUPPORT)
	{
		return false;
	}

	return BaseClass::CanSprint();
}

void CNEO_Player::EnableSprint(bool bEnable)
{
	BaseClass::EnableSprint(bEnable);

	SetMaxSpeed(GetSprintSpeed());
}

void CNEO_Player::StartWalking(void)
{
	SetMaxSpeed(GetWalkSpeed());
	m_fIsWalking = true;
}

void CNEO_Player::StopWalking(void)
{
	SetMaxSpeed(GetNormSpeed());
	m_fIsWalking = false;
}

float CNEO_Player::GetCrouchSpeed_WithActiveWepEncumberment(void) const
{
	return GetCrouchSpeed() * GetActiveWeaponSpeedScale();
}

float CNEO_Player::GetNormSpeed_WithActiveWepEncumberment(void) const
{
	return GetNormSpeed() * GetActiveWeaponSpeedScale();
}

float CNEO_Player::GetWalkSpeed_WithActiveWepEncumberment(void) const
{
	return GetWalkSpeed() * GetActiveWeaponSpeedScale();
}

float CNEO_Player::GetSprintSpeed_WithActiveWepEncumberment(void) const
{
	return GetSprintSpeed() * GetActiveWeaponSpeedScale();
}

float CNEO_Player::GetCrouchSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetCrouchSpeed() * pNeoWep->GetSpeedScale();
}

float CNEO_Player::GetNormSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetNormSpeed() * pNeoWep->GetSpeedScale();
}

float CNEO_Player::GetWalkSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetWalkSpeed() * pNeoWep->GetSpeedScale();
}

float CNEO_Player::GetSprintSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const
{
	Assert(pNeoWep);
	return GetSprintSpeed() * pNeoWep->GetSpeedScale();
}

float CNEO_Player::GetCrouchSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_CROUCH_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_CROUCH_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_CROUCH_SPEED * GetBackwardsMovementPenaltyScale();
	default:
		return NEO_BASE_CROUCH_SPEED * GetBackwardsMovementPenaltyScale();
	}
}

float CNEO_Player::GetNormSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_NORM_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_NORM_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_NORM_SPEED * GetBackwardsMovementPenaltyScale();
	default:
		return NEO_BASE_NORM_SPEED * GetBackwardsMovementPenaltyScale();
	}
}

float CNEO_Player::GetWalkSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_WALK_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_WALK_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_WALK_SPEED * GetBackwardsMovementPenaltyScale();
	default:
		return NEO_BASE_WALK_SPEED * GetBackwardsMovementPenaltyScale();
	}
}

float CNEO_Player::GetSprintSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	default:
		return NEO_BASE_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	}
}

float CNEO_Player::GetActiveWeaponSpeedScale() const
{
	// NEO TODO (Rain): change to static cast once all weapons are guaranteed to derive from the class
	auto pWep = dynamic_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	return (pWep ? pWep->GetSpeedScale() : 1.0f);
}

const Vector CNEO_Player::GetPlayerMins(void) const
{
	return VEC_DUCK_HULL_MIN_SCALED(this);
}

const Vector CNEO_Player::GetPlayerMaxs(void) const
{
	return VEC_DUCK_HULL_MAX_SCALED(this);
}

extern ConVar sv_turbophysics;

void CNEO_Player::InitVCollision(const Vector& vecAbsOrigin, const Vector& vecAbsVelocity)
{
	// Cleanup any old vphysics stuff.
	VPhysicsDestroyObject();

	// in turbo physics players dont have a physics shadow
	if (sv_turbophysics.GetBool())
		return;

	CPhysCollide* pModel = PhysCreateBbox(VEC_HULL_MIN_SCALED(this), VEC_HULL_MAX_SCALED(this));
	CPhysCollide* pCrouchModel = PhysCreateBbox(VEC_DUCK_HULL_MIN_SCALED(this), VEC_DUCK_HULL_MAX_SCALED(this));

	SetupVPhysicsShadow(vecAbsOrigin, vecAbsVelocity, pModel, "player_stand", pCrouchModel, "player_crouch");
}

extern ConVar sv_neo_wep_dmg_modifier;

void CNEO_Player::ModifyFireBulletsDamage(CTakeDamageInfo* dmgInfo)
{
	dmgInfo->SetDamage(dmgInfo->GetDamage() * sv_neo_wep_dmg_modifier.GetFloat());
}
