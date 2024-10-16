#include "cbase.h"
#include "neo_player.h"

#include "neo_predicted_viewmodel.h"
#include "neo_predicted_viewmodel_muzzleflash.h"
#include "in_buttons.h"
#include "neo_gamerules.h"
#include "team.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"
#include "datacache/imdlcache.h"

#include "neo_model_manager.h"
#include "gib.h"
#include "world.h"

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

#include "neo_weapon_loadout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(player, CNEO_Player);

IMPLEMENT_SERVERCLASS_ST(CNEO_Player, DT_NEO_Player)
SendPropInt(SENDINFO(m_iNeoClass)),
SendPropInt(SENDINFO(m_iNeoSkin)),
SendPropInt(SENDINFO(m_iNeoStar)),
SendPropInt(SENDINFO(m_iXP)),
SendPropInt(SENDINFO(m_iLoadoutWepChoice)),
SendPropInt(SENDINFO(m_iNextSpawnClassChoice)),
SendPropInt(SENDINFO(m_bInLean)),

SendPropBool(SENDINFO(m_bInThermOpticCamo)),
SendPropBool(SENDINFO(m_bLastTickInThermOpticCamo)),
SendPropBool(SENDINFO(m_bInVision)),
SendPropBool(SENDINFO(m_bHasBeenAirborneForTooLongToSuperJump)),
SendPropBool(SENDINFO(m_bShowTestMessage)),
SendPropBool(SENDINFO(m_bInAim)),
SendPropBool(SENDINFO(m_bIneligibleForLoadoutPick)),

SendPropTime(SENDINFO(m_flCamoAuxLastTime)),
SendPropInt(SENDINFO(m_nVisionLastTick)),

SendPropString(SENDINFO(m_pszTestMessage)),

SendPropArray(SendPropVector(SENDINFO_ARRAY(m_rvFriendlyPlayerPositions), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rvFriendlyPlayerPositions),
SendPropArray(SendPropInt(SENDINFO_ARRAY(m_rfAttackersScores)), m_rfAttackersScores),
SendPropArray(SendPropFloat(SENDINFO_ARRAY(m_rfAttackersAccumlator), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rfAttackersAccumlator),
SendPropArray(SendPropInt(SENDINFO_ARRAY(m_rfAttackersHits)), m_rfAttackersHits),

SendPropInt(SENDINFO(m_NeoFlags), 4, SPROP_UNSIGNED),
SendPropString(SENDINFO(m_szNeoName)),
SendPropInt(SENDINFO(m_szNameDupePos)),
SendPropBool(SENDINFO(m_bClientWantNeoName)),

SendPropTime(SENDINFO(m_flDeathTime)),
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Player)
DEFINE_FIELD(m_iNeoClass, FIELD_INTEGER),
DEFINE_FIELD(m_iNeoSkin, FIELD_INTEGER),
DEFINE_FIELD(m_iNeoStar, FIELD_INTEGER),
DEFINE_FIELD(m_iXP, FIELD_INTEGER),
DEFINE_FIELD(m_iLoadoutWepChoice, FIELD_INTEGER),
DEFINE_FIELD(m_bIneligibleForLoadoutPick, FIELD_BOOLEAN),
DEFINE_FIELD(m_iNextSpawnClassChoice, FIELD_INTEGER),
DEFINE_FIELD(m_bInLean, FIELD_INTEGER),

DEFINE_FIELD(m_bInThermOpticCamo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bLastTickInThermOpticCamo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInVision, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHasBeenAirborneForTooLongToSuperJump, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShowTestMessage, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInAim, FIELD_BOOLEAN),

DEFINE_FIELD(m_flCamoAuxLastTime, FIELD_TIME),
DEFINE_FIELD(m_nVisionLastTick, FIELD_TICK),

DEFINE_FIELD(m_pszTestMessage, FIELD_STRING),

DEFINE_FIELD(m_rvFriendlyPlayerPositions, FIELD_CUSTOM),
DEFINE_FIELD(m_rfAttackersScores, FIELD_CUSTOM),
DEFINE_FIELD(m_rfAttackersAccumlator, FIELD_CUSTOM),
DEFINE_FIELD(m_rfAttackersHits, FIELD_CUSTOM),

DEFINE_FIELD(m_NeoFlags, FIELD_CHARACTER),

DEFINE_FIELD(m_szNeoName, FIELD_STRING),
DEFINE_FIELD(m_szNameDupePos, FIELD_INTEGER),
DEFINE_FIELD(m_bClientWantNeoName, FIELD_BOOLEAN),
END_DATADESC()

#define SHOWMENU_STRLIMIT (512)

CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;
CNEOGameRulesProxy* neoGameRules;
extern CBaseEntity *g_pLastSpawn;

extern ConVar neo_sv_ignore_wep_xp_limit;

ConVar sv_neo_can_change_classes_anytime("sv_neo_can_change_classes_anytime", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Can players change classes at any moment, even mid-round?",
	true, 0.0f, true, 1.0f);
ConVar sv_neo_change_suicide_player("sv_neo_change_suicide_player", "0", FCVAR_REPLICATED, "Kill the player if they change the team and they're alive.", true, 0.0f, true, 1.0f);
ConVar sv_neo_change_threshold_interval("sv_neo_change_threshold_interval", "0.25", FCVAR_REPLICATED, "The interval threshold limit in seconds before the player is allowed to change team.", true, 0.0f, true, 1000.0f);
ConVar neo_sv_dm_max_class_dur("neo_sv_dm_max_class_dur", "10", FCVAR_REPLICATED, "The time in seconds when the player can change class on respawn during deathmatch.", true, 0.0f, true, 60.0f);

void CNEO_Player::RequestSetClass(int newClass)
{
	if (newClass < 0 || newClass >= NEO_CLASS_ENUM_COUNT)
	{
		Assert(false);
		return;
	}

	const NeoRoundStatus status = NEORules()->GetRoundStatus();
	if (IsDead() || sv_neo_can_change_classes_anytime.GetBool() ||
		(!m_bIneligibleForLoadoutPick && NEORules()->GetRemainingPreRoundFreezeTime(false) > 0) ||
		(NEORules()->GetGameType() == NEO_GAME_TYPE_TDM && !m_bIneligibleForLoadoutPick &&
				GetAliveDuration() < neo_sv_dm_max_class_dur.GetFloat()) ||
		(status == NeoRoundStatus::Idle || status == NeoRoundStatus::Warmup))
	{
		m_iNeoClass = newClass;
		m_iNextSpawnClassChoice = -1;

		SetPlayerTeamModel();
		SetViewOffset(VEC_VIEW_NEOSCALE(this));
		InitSprinting();
		RemoveAllItems(false);
		GiveDefaultItems();
		m_HL2Local.m_cloakPower = CloakPower_Cap();
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
	const NeoRoundStatus roundStatus = NEORules()->GetRoundStatus();
	bool canChangeImmediately = ((roundStatus != NeoRoundStatus::RoundLive) && (roundStatus != NeoRoundStatus::PostRound)) || !IsAlive();

	if (canChangeImmediately)
	{
		m_iNeoSkin = newSkin;

		SetPlayerTeamModel();
	}
	//else NEOTODO Set for next spawn
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

void CNEO_Player::RequestSetStar(int newStar)
{
	const int team = GetTeamNumber();
	if (team != TEAM_JINRAI && team != TEAM_NSF)
	{
		return;
	}
	if (newStar < STAR_NONE || newStar > STAR_FOXTROT)
	{
		return;
	}
	if (newStar == m_iNeoStar)
	{
		return;
	}

	m_iNeoStar.GetForModify() = newStar;
}

bool CNEO_Player::RequestSetLoadout(int loadoutNumber)
{
	int classChosen = m_iNextSpawnClassChoice.Get() != -1 ? m_iNextSpawnClassChoice.Get() : m_iNeoClass.Get();
	const char *pszWepName = CNEOWeaponLoadout::GetLoadoutWeaponEntityName(classChosen, loadoutNumber, false);

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

	if (!neo_sv_ignore_wep_xp_limit.GetBool() && loadoutNumber+1 > CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(m_iXP, classChosen, false))
	{
		DevMsg("Insufficient XP for %s\n", pszWepName);
		result = false;
	}

	if (result)
	{
		m_iLoadoutWepChoice = loadoutNumber;
		GiveLoadoutWeapon();
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

		if (NEORules()->GetGameType() == NEO_GAME_TYPE_VIP && player->m_iNeoClass == NEO_CLASS_VIP)
		{
			return;
		}
		else
		{
			nextClass = clamp(nextClass, NEO_CLASS_RECON, NEO_CLASS_SUPPORT);
		}

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

CON_COMMAND_F(joinstar, "Join star", FCVAR_USERINFO)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());
	if (player && args.ArgC() == 2)
	{
		const int star = atoi(args.ArgV()[1]);
		player->RequestSetStar(star);
	}
}

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
	m_iNeoStar = NEO_DEFAULT_STAR;
	m_iXP.GetForModify() = 0;
	V_memset(m_szNeoName.GetForModify(), 0, sizeof(m_szNeoName));
	m_szNeoNameHasSet = false;

	m_bInThermOpticCamo = m_bInVision = false;
	m_bHasBeenAirborneForTooLongToSuperJump = false;
	m_bInAim = false;
	m_bInLean = NEO_LEAN_NONE;

	m_iLoadoutWepChoice = 0;
	m_iNextSpawnClassChoice = -1;

	m_bShowTestMessage = false;
	V_memset(m_pszTestMessage.GetForModify(), 0, sizeof(m_pszTestMessage));

	ZeroFriendlyPlayerLocArray();

	m_flCamoAuxLastTime = 0;
	m_nVisionLastTick = 0;
	m_flLastAirborneJumpOkTime = 0;
	m_flLastSuperJumpTime = 0;

	m_bFirstDeathTick = true;
	m_bCorpseSpawned = false;
	m_bPreviouslyReloading = false;
	m_bLastTickInThermOpticCamo = false;
	m_bIsPendingSpawnForThisRound = false;

	m_flNextTeamChangeTime = gpGlobals->curtime + 0.5f;

	m_NeoFlags = 0;

	memset(m_szNeoNameWDupeIdx, 0, sizeof(m_szNeoNameWDupeIdx));
	m_szNeoNameWDupeIdxNeedUpdate = true;
	m_szNameDupePos = 0;

	m_iDmgMenuCurPage = 0;
	m_iDmgMenuNextPage = 0;
}

CNEO_Player::~CNEO_Player( void )
{
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
	int teamNumber = GetTeamNumber();
	ShowViewPortPanel(PANEL_SPECGUI, teamNumber == TEAM_SPECTATOR);

	if (teamNumber == TEAM_JINRAI || teamNumber == TEAM_NSF)
	{
		State_Transition(STATE_ACTIVE);
	}
	
	// Should do this class update first, because most of the stuff below depends on which player class we are.
	if ((m_iNextSpawnClassChoice != -1) && (m_iNeoClass != m_iNextSpawnClassChoice))
	{
		m_iNeoClass = m_iNextSpawnClassChoice;
	}

	BaseClass::Spawn();

	m_HL2Local.m_cloakPower = CloakPower_Cap();

	m_bIsPendingSpawnForThisRound = false;

	m_bLastTickInThermOpticCamo = m_bInThermOpticCamo = false;
	m_flCamoAuxLastTime = 0;

	m_bInVision = false;
	m_nVisionLastTick = 0;
	m_bInLean = NEO_LEAN_NONE;
	m_bCorpseSpawned = false;
	m_bIneligibleForLoadoutPick = false;

	for (int i = 0; i < m_rfAttackersScores.Count(); ++i)
	{
		m_rfAttackersScores.Set(i, 0);
	}
	for (int i = 0; i < m_rfAttackersAccumlator.Count(); ++i)
	{
		m_rfAttackersAccumlator.Set(i, 0.0f);
	}
	for (int i = 0; i < m_rfAttackersHits.Count(); ++i)
	{
		m_rfAttackersHits.Set(i, 0);
	}

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
	if (m_iNeoClass == NEO_CLASS_VIP)
		return;

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

void CNEO_Player::CheckLeanButtons()
{
	if (!IsAlive())
	{
		return;
	}
	
	m_bInLean = NEO_LEAN_NONE;
	if ((m_nButtons & IN_LEAN_LEFT) && !(m_nButtons & IN_LEAN_RIGHT))
	{
		m_bInLean = NEO_LEAN_LEFT;
	}
	else if ((m_nButtons & IN_LEAN_RIGHT) && !(m_nButtons & IN_LEAN_LEFT))
	{
		m_bInLean = NEO_LEAN_RIGHT;
	}
}

void CNEO_Player::PreThink(void)
{
	BaseClass::PreThink();

	if (!m_bInThermOpticCamo)
	{
		CloakPower_Update();
	}

	float speed = GetNormSpeed();
	static constexpr float DUCK_WALK_SPEED_MODIFIER = 0.75;
	if (m_nButtons & IN_DUCK)
	{
		speed *= DUCK_WALK_SPEED_MODIFIER;
	}
	if (m_nButtons & IN_WALK)
	{
		speed *= DUCK_WALK_SPEED_MODIFIER;
	}
	if (IsSprinting() && !IsAirborne())
	{
		static constexpr float RECON_SPRINT_SPEED_MODIFIER = 0.75;
		static constexpr float OTHER_CLASSES_SPRINT_SPEED_MODIFIER = 0.6;
		speed /= m_iNeoClass == NEO_CLASS_RECON ? RECON_SPRINT_SPEED_MODIFIER : OTHER_CLASSES_SPRINT_SPEED_MODIFIER;
	}
	if (m_bInAim.Get())
	{
		static constexpr float AIM_SPEED_MODIFIER = 0.6;
		speed *= AIM_SPEED_MODIFIER;
	}
	if (auto pNeoWep = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon()))
	{
		speed *= pNeoWep->GetSpeedScale();
	}

	if (!IsAirborne() && m_iNeoClass != NEO_CLASS_RECON)
	{
		const float deltaTime = gpGlobals->curtime - m_flLastAirborneJumpOkTime;
		const float leeway = 1.0f;
		if (deltaTime < leeway)
		{
			speed = (speed / 2) + (deltaTime / 2 * (speed));
		}
	}
	SetMaxSpeed(MAX(speed, 56));

	CheckThermOpticButtons();
	CheckVisionButtons();

	if (m_bInThermOpticCamo)
	{
		if (m_flCamoAuxLastTime == 0)
		{
			if (m_HL2Local.m_cloakPower >= CLOAK_AUX_COST)
			{
				m_flCamoAuxLastTime = gpGlobals->curtime;
			}
		}
		else
		{
			const float deltaTime = gpGlobals->curtime - m_flCamoAuxLastTime;
			if (deltaTime >= 1)
			{
				// Need to have at least this much spare camo to enable.
				// This prevents camo spam abuse where player has a sliver of camo
				// each frame to never really run out.
				CloakPower_Drain(deltaTime * CLOAK_AUX_COST);

				if (m_HL2Local.m_cloakPower <= 0.1f)
				{
					m_bInThermOpticCamo = false;
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

	if (IsAlive())
	{
		Lean();
	}

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
		if (!player || (player->IsDead() && player->GetObserverMode() == OBS_MODE_NONE))
		{
			continue;
		}

		static constexpr float MAX_CLOAK_DISTANCE = 50.0f;
		const auto dir = EyePosition() - player->GetAbsOrigin();
		const float distance = dir.Length();
		if (distance < MAX_CLOAK_DISTANCE)
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

		if (m_HL2Local.m_cloakPower >= CLOAK_AUX_COST)
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
	if (!IsSprinting())
		return false;

	if (IsCarryingGhost())
		return false;

	if (GetMoveParent())
		return false;

	if (IsPlayerUnderwater())
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
			m_bInLean = NEO_LEAN_NONE;
		}

		return;
	}
	else
	{
		m_bFirstDeathTick = true;
	}

	CheckLeanButtons();

	if (auto *pNeoWep = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon()))
	{
		const bool clientAimHold = ClientWantsAimHold(this);
		if (pNeoWep->m_bInReload && !m_bPreviouslyReloading)
		{
			Weapon_SetZoom(false);
		}
		else if (CanSprint() && m_afButtonPressed & IN_SPEED)
		{
			Weapon_SetZoom(false);
		}
		else if (m_afButtonPressed & IN_AIM)
		{
			if (!CanSprint() || !(m_nButtons & IN_SPEED))
			{
				Weapon_AimToggle(pNeoWep, clientAimHold ? NEO_TOGGLE_FORCE_AIM : NEO_TOGGLE_DEFAULT);
			}
		}
		else if (clientAimHold && (m_afButtonReleased & IN_AIM))
		{
			Weapon_AimToggle(pNeoWep, NEO_TOGGLE_FORCE_UN_AIM);
		}
		m_bPreviouslyReloading = pNeoWep->m_bInReload;

		if (m_afButtonPressed & IN_DROP)
		{
			Vector eyeForward;
			EyeVectors(&eyeForward);
			const float forwardOffset = 250.0f;
			eyeForward *= forwardOffset;
			Weapon_Drop(pNeoWep, NULL, &eyeForward);
		}
	}
}

void CNEO_Player::PlayerDeathThink()
{
	if (m_nButtons & ~IN_SCORE)
	{
		m_bEnterObserver = true;
	}
	BaseClass::PlayerDeathThink();
}

void CNEO_Player::Weapon_AimToggle(CNEOBaseCombatWeapon *pNeoWep, const NeoWeponAimToggleE toggleType)
{
	if (IsAllowedToZoom(pNeoWep))
	{
		if (toggleType != NEO_TOGGLE_FORCE_UN_AIM)
		{
			const bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
			Weapon_SetZoom(showCrosshair);
		}
		else if (toggleType != NEO_TOGGLE_FORCE_AIM)
		{
			Weapon_SetZoom(false);
		}
	}
}

void CNEO_Player::SetNameDupePos(const int dupePos)
{
	m_szNameDupePos = dupePos;
	m_szNeoNameWDupeIdxNeedUpdate = true;
}

int CNEO_Player::NameDupePos() const
{
	return m_szNameDupePos;
}

const char *CNEO_Player::GetNeoPlayerName(const CNEO_Player *viewFrom) const
{
	const bool nameFetchWantNeoName = (viewFrom) ? viewFrom->m_bClientWantNeoName : m_bClientWantNeoName;

	const int dupePos = m_szNameDupePos;
	if (nameFetchWantNeoName && m_szNeoName.Get()[0] != '\0')
	{
		const char *neoName = m_szNeoName.Get();
		if (dupePos > 0)
		{
			if (m_szNeoNameWDupeIdxNeedUpdate)
			{
				m_szNeoNameWDupeIdxNeedUpdate = false;
				V_snprintf(m_szNeoNameWDupeIdx, sizeof(m_szNeoNameWDupeIdx), "%s (%d)", neoName, dupePos);
			}
			return m_szNeoNameWDupeIdx;
		}
		return neoName;
	}

	const char *stndName = const_cast<CNEO_Player *>(this)->GetPlayerName();
	if (nameFetchWantNeoName && dupePos > 0)
	{
		if (m_szNeoNameWDupeIdxNeedUpdate)
		{
			m_szNeoNameWDupeIdxNeedUpdate = false;
			V_snprintf(m_szNeoNameWDupeIdx, sizeof(m_szNeoNameWDupeIdx), "%s (%d)", stndName, dupePos);
		}
		return m_szNeoNameWDupeIdx;
	}
	return stndName;
}

const char *CNEO_Player::GetNeoPlayerNameDirect() const
{
	return m_szNeoNameHasSet ? m_szNeoName.Get() : NULL;
}

void CNEO_Player::SetNeoPlayerName(const char *newNeoName)
{
	// NEO NOTE (nullsystem): Generally it's never NULL but just incase
	if (newNeoName)
	{
		V_memcpy(m_szNeoName.GetForModify(), newNeoName, sizeof(m_szNeoName)-1);
		m_szNeoNameHasSet = true;
	}
}

void CNEO_Player::SetClientWantNeoName(const bool b)
{
	m_bClientWantNeoName = b;
}

void CNEO_Player::Weapon_SetZoom(const bool bZoomIn)
{
	ShowCrosshair(bZoomIn);
	
	const int fov = GetDefaultFOV();
	SetFOV(static_cast<CBaseEntity *>(this), bZoomIn ? NeoAimFOV(fov, GetActiveWeapon()) : fov, NEO_ZOOM_SPEED);
	m_bInAim = bZoomIn;
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
			GetNeoPlayerName(), GetTeam()->GetName());
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
	else if (FStrEq(args[0], "menuselect"))
	{
		if (args.ArgC() < 2)
		{
			return true;
		}
		const int slot = atoi(args[1]);

		if (m_iDmgMenuCurPage > 0)
		{
			// menuselect being used by damage stats
			switch (slot)
			{
			case 1: // Dismiss
			{
				m_iDmgMenuCurPage = 0;
				m_iDmgMenuNextPage = 0;
				break;
			}
			case 2: // Next-page
			{
				char infoStr[SHOWMENU_STRLIMIT];
				int infoStrSize = 0;
				bool showMenu = false;
				m_iDmgMenuCurPage = m_iDmgMenuNextPage;
				m_iDmgMenuNextPage = SetDmgListStr(infoStr, sizeof(infoStr), m_iDmgMenuNextPage, &infoStrSize, &showMenu, NULL);
				if (showMenu) ShowDmgInfo(infoStr, infoStrSize);
				break;
			}
			default:
				break;
			}
		}

		return true;
	}

	return BaseClass::ClientCommand(args);
}

void CNEO_Player::CreateViewModel( int index )
{
	Assert( index >= 0 && index < MAX_VIEWMODELS );
	Assert(MUZZLE_FLASH_VIEW_MODEL_INDEX != index && MUZZLE_FLASH_VIEW_MODEL_INDEX < MAX_VIEWMODELS);

	if ( !GetViewModel( index ) )
	{
		CNEOPredictedViewModel* vm = (CNEOPredictedViewModel*)CreateEntityByName("neo_predicted_viewmodel");
		if (vm)
		{
			vm->SetAbsOrigin(GetAbsOrigin());
			vm->SetOwner(this);
			vm->SetIndex(index);
			DispatchSpawn(vm);
			vm->FollowEntity(this, false);
			m_hViewModel.Set(index, vm);
		}
	}

	if ( !GetViewModel(MUZZLE_FLASH_VIEW_MODEL_INDEX))
	{
		auto* vmm = (CNEOPredictedViewModelMuzzleFlash*)CreateEntityByName("neo_predicted_viewmodel_muzzleflash");
		if (!vmm)
		{
			Assert(false);
			return;
		}
		
		CBaseViewModel* vm = GetViewModel();
		if (!vm)
		{
			Assert(false);
			return;
		}

		vmm->SetAbsOrigin(vm->GetAbsOrigin());
		vmm->SetOwner(this);
		vmm->SetParent(vm);
		DispatchSpawn(vmm);
		m_hViewModel.Set(MUZZLE_FLASH_VIEW_MODEL_INDEX, vmm);
	}
}

bool CNEO_Player::BecomeRagdollOnClient( const Vector &force )
{
	return BaseClass::BecomeRagdollOnClient(force);
}

void CNEO_Player::ShowDmgInfo(char* infoStr, int infoStrSize)
{
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	// UserMessage has limits, so need to send more than once if it
	// doesn't fit in one UserMessage packet
	static const int MAX_INFOSTR_PER_UMSG = 240;
	int infoStrOffset = 0;
	int infoStrOffsetMax = MAX_INFOSTR_PER_UMSG;
	while (infoStrSize > 0)
	{
		char tmpCh = '\0';
		if (infoStrOffsetMax < SHOWMENU_STRLIMIT)
		{
			tmpCh = infoStr[infoStrOffsetMax];
			infoStr[infoStrOffsetMax] = '\0';
		}
		UserMessageBegin(filter, "ShowMenu");
		{
			WRITE_SHORT(static_cast<short>(m_iDmgMenuNextPage ? 3 : 1)); // Amount of options
			WRITE_CHAR(static_cast<char>(60)); // 60s timeout
			WRITE_BYTE(static_cast<unsigned int>(infoStrSize > MAX_INFOSTR_PER_UMSG));
			WRITE_STRING(infoStr + infoStrOffset);
		}
		MessageEnd();
		if (infoStrOffsetMax < SHOWMENU_STRLIMIT)
		{
			infoStr[infoStrOffsetMax] = tmpCh;
		}
		infoStrOffset += MAX_INFOSTR_PER_UMSG;
		infoStrOffsetMax += MAX_INFOSTR_PER_UMSG;
		infoStrSize -= MAX_INFOSTR_PER_UMSG;
	}
}

int CNEO_Player::SetDmgListStr(char* infoStr, const int infoStrMax, const int playerIdxStart,
		int *infoStrSize, bool *showMenu, const CTakeDamageInfo *info) const
{
	Assert(infoStrSize != NULL);
	Assert(showMenu != NULL);
	*showMenu = false;
	static const int TITLE_LEN = 30; // Rough-approximate
	static const int POSTFIX_LEN = 15 + 12;
	static const int TOTALLINE_LEN = 64; // Rough-approximate
	static int FILLSTR_END = SHOWMENU_STRLIMIT - POSTFIX_LEN - TOTALLINE_LEN - 2;
	memset(infoStr, 0, infoStrMax);
	Q_snprintf(infoStr, infoStrMax, "Damage infos (Round %d):\n", NEORules()->roundNumber());
	if (info)
	{
		// If CTakeDamageInfo pointer given, then also show killed by information
		auto* neoAttacker = dynamic_cast<CNEO_Player*>(info->GetAttacker());
		if (neoAttacker && neoAttacker->entindex() != entindex())
		{
			char killByLine[SHOWMENU_STRLIMIT];
			auto* killedWithInflictor = info->GetInflictor();
			const bool inflictorIsPlayer = killedWithInflictor ? !Q_strcmp(killedWithInflictor->GetDebugName(), "player") : false;
			auto killedWithName = killedWithInflictor ? (inflictorIsPlayer ? neoAttacker->m_hActiveWeapon->GetPrintName() : killedWithInflictor->GetDebugName()) : "";
			if (!Q_strcmp(killedWithName, "neo_grenade_frag")) { killedWithName = "Frag Grenade"; }
			if (!Q_strcmp(killedWithName, "neo_deployed_detpack")) { killedWithName = "Remote Detpack"; }
			KillerLineStr(killByLine, sizeof(killByLine), neoAttacker, this, killedWithName);
			Q_strncat(infoStr, killByLine, infoStrMax, COPY_ALL_CHARACTERS);
		}
	}
	Q_strncat(infoStr, " \n", infoStrMax, COPY_ALL_CHARACTERS);
	int infoStrLen = Q_strlen(infoStr);
	const int thisIdx = entindex();
	int nextPage = 0;
	for (int pIdx = playerIdxStart; pIdx <= gpGlobals->maxClients; ++pIdx)
	{
		if (pIdx == thisIdx)
		{
			continue;
		}

		auto* neoAttacker = dynamic_cast<CNEO_Player*>(UTIL_PlayerByIndex(pIdx));
		if (!neoAttacker || neoAttacker->IsHLTV())
		{
			continue;
		}

		const AttackersTotals attackerInfo = {
			.dealtDmgs = neoAttacker->GetAttackersScores(thisIdx),
			.dealtHits = neoAttacker->GetAttackerHits(thisIdx),
			.takenDmgs = GetAttackersScores(pIdx),
			.takenHits = GetAttackerHits(pIdx),
		};
		const char *dmgerName = neoAttacker->GetNeoPlayerName(this);
#define DEBUG_SHOW_ALL (0)
#if DEBUG_SHOW_ALL
		if (dmgerName)
#else
		if (dmgerName && (attackerInfo.dealtDmgs > 0 || attackerInfo.takenDmgs > 0))
#endif
		{
			*showMenu = true;
			const char *dmgerClass = GetNeoClassName(neoAttacker->GetClass());
			static char infoLine[SHOWMENU_STRLIMIT];
			const int infoLineLen = DmgLineStr(infoLine, sizeof(infoLine),
					dmgerName, dmgerClass, attackerInfo);
			if ((infoStrLen + infoLineLen) >= FILLSTR_END)
			{
				// Truncate for this page
				nextPage = pIdx;
				break;
			}

			Q_strncat(infoStr, infoLine, infoStrMax, COPY_ALL_CHARACTERS);
			infoStrLen += infoLineLen;
		}
	}

	if (!*showMenu)
	{
		return 0;
	}

	const AttackersTotals attackersTotals = GetAttackersTotals();
	static char totalInfoLine[SHOWMENU_STRLIMIT];
	memset(totalInfoLine, 0, sizeof(totalInfoLine));
	Q_snprintf(totalInfoLine, sizeof(totalInfoLine), " \nTOTAL: Dealt: %d in %d hits | Taken: %d in %d hits\n",
			attackersTotals.dealtDmgs, attackersTotals.dealtHits,
			attackersTotals.takenDmgs, attackersTotals.takenHits);
	Q_strncat(infoStr, totalInfoLine, infoStrMax, COPY_ALL_CHARACTERS);
	Q_strncat(infoStr, "->1. Dismiss", infoStrMax, COPY_ALL_CHARACTERS);
	if (nextPage > 0)
	{
		Q_strncat(infoStr, "\n->2. Next", infoStrMax, COPY_ALL_CHARACTERS);
	}
	*infoStrSize = Q_strlen(infoStr);

	return nextPage;
}

void CNEO_Player::StartShowDmgStats(const CTakeDamageInfo* info)
{
	char infoStr[SHOWMENU_STRLIMIT];
	int infoStrSize = 0;
	bool showMenu = false;
	m_iDmgMenuCurPage = 1;
	m_iDmgMenuNextPage = SetDmgListStr(infoStr, sizeof(infoStr), m_iDmgMenuCurPage, &infoStrSize, &showMenu, info);
	if (showMenu) ShowDmgInfo(infoStr, infoStrSize);

	// Notify the client to print it out in the console
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	UserMessageBegin(filter, "DamageInfo");
	{
		short attackerIdx = 0;
		auto* neoAttacker = info ? dynamic_cast<CNEO_Player*>(info->GetAttacker()) : NULL;
		const char* killedWithName = "";
		if (neoAttacker && neoAttacker->entindex() != entindex())
		{
			attackerIdx = static_cast<short>(neoAttacker->entindex());
			auto* killedWithInflictor = info->GetInflictor();
			const bool inflictorIsPlayer = killedWithInflictor ? !Q_strcmp(killedWithInflictor->GetDebugName(), "player") : false;
			killedWithName = killedWithInflictor ? (inflictorIsPlayer ? neoAttacker->m_hActiveWeapon->GetPrintName() : killedWithInflictor->GetDebugName()) : "";
			if (!Q_strcmp(killedWithName, "neo_grenade_frag")) { killedWithName = "Frag Grenade"; }
			if (!Q_strcmp(killedWithName, "neo_deployed_detpack")) { killedWithName = "Remote Detpack"; }
		}
		WRITE_SHORT(attackerIdx);
		WRITE_STRING(killedWithName);
	}
	MessageEnd();
}

void CNEO_Player::Event_Killed( const CTakeDamageInfo &info )
{
	// Calculate force for weapon drop
	Vector forceVector = CalcDamageForceVector(info);

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
			Weapon_DropOnDeath(pWep, forceVector, info.GetAttacker());
		}
	}

	// Handle Corpse and Gibs
	if (!m_bCorpseSpawned) // Event_Killed can be called multiple times, only spawn the dead model once
	{
		SpawnDeadModel(info);
	}

	if (!IsBot() && !IsHLTV())
	{
		StartShowDmgStats(&info);
	}

	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TDM)
	{
		GetGlobalTeam(NEORules()->GetOpposingTeam(this))->AddScore(1);
	}

	BaseClass::Event_Killed(info);
}

void CNEO_Player::Weapon_DropOnDeath(CBaseCombatWeapon* pWeapon, Vector velocity, CBaseEntity *pAttacker)
{
	CNEOBaseCombatWeapon* pNeoWeapon = static_cast<CNEOBaseCombatWeapon*>(pWeapon);
	if (!pNeoWeapon)
		return;
	if (!pNeoWeapon->GetWpnData().m_bDropOnDeath)
		return;
#if(0) // Remove this #if to enable dropping of live grenades if killed while primed

	// If attack button was held down when player died, drop a live grenade. NEOTODO (Adam) Add delay between pressing an attack button and the pin being fully pulled out. If pin not out when dead do not drop a live grenade
	if (GetActiveWeapon() == pNeoWeapon && pNeoWeapon->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE) {
		if (((m_nButtons & IN_ATTACK) && (!(m_afButtonPressed & IN_ATTACK))) ||
			((m_nButtons & IN_ATTACK2) && (!(m_afButtonPressed & IN_ATTACK2))))
		{
			auto pWeaponFrag = static_cast<CWeaponGrenade*>(pNeoWeapon);
			pWeaponFrag->ThrowGrenade(this, false, pAttacker);
			return;
		}
	}
	if (GetActiveWeapon() == pNeoWeapon && pNeoWeapon->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE) {
		if (((m_nButtons & IN_ATTACK) && (!(m_afButtonPressed & IN_ATTACK))) ||
			((m_nButtons & IN_ATTACK2) && (!(m_afButtonPressed & IN_ATTACK2))))
		{
			auto pWeaponSmoke = static_cast<CWeaponSmokeGrenade*>(pNeoWeapon);
			pWeaponSmoke->ThrowGrenade(this, false, pAttacker);
			return;
		}
	}
#endif
	
	if (pWeapon->IsEffectActive( EF_BONEMERGE ))
	{
		//int iBIndex = LookupBone("valveBiped.Bip01_R_Hand"); NEOTODO (Adam) Should mimic the behaviour in basecombatcharacter that places the weapon such that the bones used in the bonemerge overlap
		Vector vFacingDir = BodyDirection2D();
		pWeapon->SetAbsOrigin(Weapon_ShootPosition() + vFacingDir);
	}

	pWeapon->AddEffects(EF_BONEMERGE);
	Vector playerVelocity = vec3_origin;
	GetVelocity(&playerVelocity, NULL);
	
	if (VPhysicsGetObject())
	{
		playerVelocity += velocity * VPhysicsGetObject()->GetInvMass();
	}
	else {
		playerVelocity += velocity;
	}
	pWeapon->Drop(playerVelocity);
	Weapon_Detach(pWeapon);
}

void CNEO_Player::SpawnDeadModel(const CTakeDamageInfo& info)
{
	m_bCorpseSpawned = true;

	int deadModelType = -1;
	if (info.GetDamageType() & (1 << 6)) //info.GetDamage() >= 100
	{ // Died to blast damage, remove all limbs
		deadModelType = 0;
	}
	else
	{ // Set model based on last hitgroup
		switch (LastHitGroup())
		{
		case 1: // Head
			deadModelType = 1;
			break;
		case 4: // Left Arm
			deadModelType = 2;
			break;
		case 5: // Right Arm
			deadModelType = 4;
			break;
		case 6: // Left Leg
			deadModelType = 3;
			break;
		case 7: // Right Leg
			deadModelType = 5;
			break;
		}
	}

	if (deadModelType == -1)
		return;

	CNEOModelManager* modelManager = CNEOModelManager::Instance();
	if (!modelManager)
	{
		Assert(false);
		Warning("Failed to get Neo model manager\n");
		return;
	}
	switch (deadModelType)
	{
	case 0: // Spawn all gibs
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_HEAD));
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_LARM));
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_LLEG));
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_RARM));
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_RLEG));
		SetPlayerCorpseModel(deadModelType);
		break;
	case 1: // head
		SetPlayerCorpseModel(deadModelType);
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_HEAD));
		break;
	case 2: // left arm
		SetPlayerCorpseModel(deadModelType);
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_LARM));
		break;
	case 3: // left leg
		SetPlayerCorpseModel(deadModelType);
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_LLEG));
		break;
	case 4: // right arm
		SetPlayerCorpseModel(deadModelType);
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_RARM));
		break;
	case 5: // right leg
		SetPlayerCorpseModel(deadModelType);
		CGib::SpawnSpecificGibs(this, 1, 750, 1500, modelManager->GetGibModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NEO_GIB_LIMB_RLEG));
		break;
	default:
		break;
	}
}

void CNEO_Player::SetPlayerCorpseModel(int type)
{
	CNEOModelManager* modelManager = CNEOModelManager::Instance();
	if (!modelManager)
	{
		Assert(false);
		Warning("Failed to get Neo model manager\n");
		return;
	}
	const char* model = modelManager->GetCorpseModel(static_cast<NeoSkin>(GetSkin()), static_cast<NeoClass>(GetClass()), GetTeamNumber(), NeoGib(type));

	if (!*model)
	{
		Assert(false);
		Warning("Failed to find model string for Neo player corpse\n");
		return;
	}

	SetModel(model);
	SetPlaybackRate(1.0f);
}

float CNEO_Player::GetReceivedDamageScale(CBaseEntity* pAttacker)
{
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TDM && m_aliveTimer.IsLessThen(5.f))
	{ // 5 seconds of invincibility in TDM
		return 0.f;
	}

	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
	case NEO_CLASS_VIP:
		return NEO_ASSAULT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
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

void CNEO_Player::Weapon_Equip(CBaseCombatWeapon* pWeapon)
{
 	for (int i=0;i<MAX_WEAPONS;i++) 
	{
		if (!m_hMyWeapons[i]) 
		{
			m_hMyWeapons.Set( i, pWeapon );
			break;
		}
	}
	
	pWeapon->ChangeTeam( GetTeamNumber() );
	pWeapon->Equip( this );

	// Pass the lighting origin over to the weapon if we have one
	pWeapon->SetLightingOriginRelative( GetLightingOriginRelative() );
}	

bool CNEO_Player::Weapon_Switch( CBaseCombatWeapon *pWeapon,
                                 int viewmodelindex )
{
	ShowCrosshair(false);
	Weapon_SetZoom(false);
	const auto activeWeapon = GetActiveWeapon();
	if (activeWeapon)
	{
		activeWeapon->StopWeaponSound(RELOAD_NPC);
	}

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

	const auto activeWeapon = GetActiveWeapon();
	if (activeWeapon)
    {
		if (!activeWeapon->CanHolster())
			return false;
    }

    return true;
}

bool CNEO_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	auto weaponSlot = pWeapon->GetSlot();
	// Only pick up grenades if we don't have grenades of that type NEOTODO (Adam) What if we have less than the maximum of that type (i.e one smoke grenade)? Can I carry more of a grenade than I spawn with?
	if (weaponSlot == 3 && Weapon_GetPosition(weaponSlot, pWeapon->GetPosition()))
	{
		return false;
	}

	// We already have a weapon in this slot
	if (weaponSlot != 3 && Weapon_GetSlot(weaponSlot))
	{
		return false;
	}
	
	auto neoWeapon = dynamic_cast<CNEOBaseCombatWeapon*>(pWeapon);
	if(neoWeapon)
	{
		if(!neoWeapon->CanBePickedUpByClass(GetClass()))
		{
			return false;	
		}
	}

	return BaseClass::BumpWeapon(pWeapon);
}

bool CNEO_Player::Weapon_GetPosition(int slot, int position)
{
	// Check for that slot being occupied already
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (m_hMyWeapons[i].Get() != NULL)
		{
			// If the slots match, it's already occupied
			if (m_hMyWeapons[i]->GetSlot() == slot && m_hMyWeapons[i]->GetPosition() == position)
				return m_hMyWeapons[i];
		}
	}

	return NULL;
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
	//ResetAnimation();

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

void CNEO_Player::Weapon_Drop( CBaseCombatWeapon *pWeapon,
	const Vector *pvecTarget, const Vector *pVelocity )
{
	m_bIneligibleForLoadoutPick = true;
	if(auto neoWeapon = dynamic_cast<CNEOBaseCombatWeapon *>(pWeapon))
	{
		if (neoWeapon->CanDrop() && IsDead() && pWeapon)
		{
			if (auto *activeWep = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon()))
			{
				// If player has held down an attack key since the previous frame
				if (((m_nButtons & IN_ATTACK) && (!(m_afButtonPressed & IN_ATTACK))) ||
					((m_nButtons & IN_ATTACK2) && (!(m_afButtonPressed & IN_ATTACK2))))
				{
					// Drop a grenade if it's primed.
					if (auto pWeaponFrag = dynamic_cast<CWeaponGrenade *>(activeWep))
					{
						pWeaponFrag->ThrowGrenade(this);
						pWeaponFrag->DecrementAmmo(this);
						return;
					}
					// Drop a smoke if it's primed.
					else if (auto pWeaponSmoke = dynamic_cast<CWeaponSmokeGrenade *>(activeWep))
					{
						pWeaponSmoke->ThrowGrenade(this);
						pWeaponSmoke->DecrementAmmo(this);
						return;
					}
				}
			}
		}
		else if (!neoWeapon->CanDrop())
		{
			return; // Return early to not drop weapon
		}
	}

	if (!pWeapon)
		return;

	pWeapon->m_bInReload = false;
	pWeapon->StopWeaponSound(RELOAD_NPC);
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
	const auto alternate = NEORules()->roundAlternate();

	if (NEORules()->IsTeamplay())
	{
		if (GetTeamNumber() == TEAM_JINRAI)
		{
			pSpawnpointName = alternate ? "info_player_attacker" : "info_player_defender";
			pLastSpawnPoint = g_pLastJinraiSpawn;
		}
		else if (GetTeamNumber() == TEAM_NSF)
		{
			pSpawnpointName = alternate ? "info_player_defender" : "info_player_attacker";
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

	const bool justJoined = (GetTeamNumber() == TEAM_UNASSIGNED);

	// Outside the below statement scope because this incorrectly triggers warnings on MSVC otherwise:
	// https://developercommunity.visualstudio.com/t/fallthrough-does-not-suppress-warning-c26819-when/1206339
	enum JoinMode {
		Random = -2,
		TeamWithLessPlayers = -1,
	};

	// Player bots should initially join a player team.
	// Note that we can't do a ->IsBot check here, because the bot has not
	// received its fakeclient flags yet at this point. Hence using the
	// m_bNextClientIsFakeClient workaround.
	if (justJoined && NEORules()->m_bNextClientIsFakeClient && !IsHLTV())
	{
		Assert(gpGlobals->curtime >= m_flNextTeamChangeTime);
		
		ConVarRef joinMode{ "bot_next_team" };

		int numJin, numNsf;
		switch (joinMode.GetInt())
		{
		case JoinMode::TeamWithLessPlayers:
			numJin = GetGlobalTeam(TEAM_JINRAI) ? GetGlobalTeam(TEAM_JINRAI)->GetNumPlayers() : 0;
			numNsf = GetGlobalTeam(TEAM_NSF) ? GetGlobalTeam(TEAM_NSF)->GetNumPlayers() : 0;
			if (numJin < numNsf)
			{
				iTeam = TEAM_JINRAI;
				break;
			}
			else if (numNsf < numJin)
			{
				iTeam = TEAM_NSF;
				break;
			}
			[[fallthrough]];
		case JoinMode::Random:
			iTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
			break;
		default:
			const auto lastGameTeam = GetNumberOfTeams() - LAST_SHARED_TEAM;
			Assert(FIRST_GAME_TEAM <= lastGameTeam);
			iTeam = Clamp(joinMode.GetInt(), FIRST_GAME_TEAM, lastGameTeam);
		}
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

	const bool suicidePlayerIfAlive = sv_neo_change_suicide_player.GetBool();
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

		if (suicidePlayerIfAlive)
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
		if (!justJoined && GetTeamNumber() != TEAM_SPECTATOR && !IsDead())
		{
			if (suicidePlayerIfAlive)
			{
				SoftSuicide();
			}
			else if (!suicidePlayerIfAlive)
			{
				ClientPrint(this, HUD_PRINTCENTER, "You can't switch teams while you are alive.");
				return false;
			}
		}

		StopObserverMode();
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

	m_flNextTeamChangeTime = gpGlobals->curtime + sv_neo_change_threshold_interval.GetFloat();
	
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

int CNEO_Player::GetAttackersScores(const int attackerIdx) const
{
	return min(m_rfAttackersScores.Get(attackerIdx), 100);
}

int CNEO_Player::GetAttackerHits(const int attackerIdx) const
{
	return m_rfAttackersHits.Get(attackerIdx);
}

AttackersTotals CNEO_Player::GetAttackersTotals() const
{
	AttackersTotals totals = {};

	const int thisIdx = entindex();
	for (int pIdx = 1; pIdx <= gpGlobals->maxClients; ++pIdx)
	{
		if (pIdx == thisIdx)
		{
			continue;
		}

		auto* neoAttacker = dynamic_cast<CNEO_Player*>(UTIL_PlayerByIndex(pIdx));
		if (!neoAttacker || neoAttacker->IsHLTV())
		{
			continue;
		}

		totals.dealtDmgs += neoAttacker->GetAttackersScores(thisIdx);
		totals.takenDmgs += GetAttackersScores(pIdx);
		totals.dealtHits += neoAttacker->GetAttackerHits(thisIdx);
		totals.takenHits += GetAttackerHits(pIdx);
	}
	return totals;
}

int	CNEO_Player::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
	if (m_takedamage != DAMAGE_EVENTS_ONLY)
	{
		// Dynamic cast, attacker might be prop/this player fallen
		if (auto *attacker = dynamic_cast<CNEO_Player *>(info.GetAttacker()))
		{
			const int attackerIdx = attacker->entindex();

			// Separate the fractional amount of damage from the whole
			const float flFractionalDamage = info.GetDamage() - floor(info.GetDamage());
			int iDamage = static_cast<int>(info.GetDamage() - flFractionalDamage);

			float flDmgAccumlator = m_rfAttackersAccumlator.Get(attackerIdx);
			flDmgAccumlator += flFractionalDamage;
			if (flDmgAccumlator >= 1.0f)
			{
				++iDamage;
				flDmgAccumlator -= 1.0f;
			}

			// Mirror team-damage
			const bool bIsTeamDmg = (attackerIdx != entindex() && attacker->GetTeamNumber() == GetTeamNumber());
			if (bIsTeamDmg)
			{
				const float flMirrorMult = NEORules()->MirrorDamageMultiplier();
				if (flMirrorMult > 0.0f)
				{
					// Attacker own-team-inflicting - Mirror the damage
					// NEO NOTE (nullsystem): DO NOT call OnTakeDamage directly as that'll crash the game, instead
					// let the gamerule handle this situation by causing the attacker to suicide when health gets
					// depliciated
					const float flInflict = iDamage * flMirrorMult;
					attacker->OnTakeDamage_Alive(CTakeDamageInfo(GetWorldEntity(), GetWorldEntity(),
																 flInflict, DMG_SLASH));
					if (attacker->m_iHealth <= 0)
					{
						attacker->m_bKilledInflicted = true;
					}

					if (neo_sv_mirror_teamdamage_immunity.GetBool())
					{
						// Give immunity to the victim and don't go through the OnTakeDamage_Alive
						return 0;
					}
				}
			}

			// Apply damages/hits numbers
			if (iDamage > 0)
			{
				m_rfAttackersScores.GetForModify(attackerIdx) += iDamage;
				m_rfAttackersAccumlator.Set(attackerIdx, flDmgAccumlator);
				m_rfAttackersHits.GetForModify(attackerIdx) += 1;

				if (bIsTeamDmg && neo_sv_teamdamage_kick.GetBool() && NEORules()->GetRoundStatus() == NeoRoundStatus::RoundLive)
				{
					attacker->m_iTeamDamageInflicted += iDamage;
				}
			}
		}
	}
	return BaseClass::OnTakeDamage_Alive(info);
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
			pent->SetAbsOrigin(pPlayer->EyePosition());
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
	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_milso");
		if (this->m_iXP >= 4) { GiveDet(this); }
		Weapon_Switch(Weapon_OwnsThisType("weapon_milso"));
		break;
	case NEO_CLASS_ASSAULT:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_tachi");
		GiveNamedItem("weapon_grenade");
		Weapon_Switch(Weapon_OwnsThisType("weapon_tachi"));
		break;
	case NEO_CLASS_SUPPORT:
		GiveNamedItem("weapon_kyla");
		GiveNamedItem("weapon_smokegrenade");
		Weapon_Switch(Weapon_OwnsThisType("weapon_kyla"));
		break;
	case NEO_CLASS_VIP:
		GiveNamedItem("weapon_milso");
		Weapon_Switch(Weapon_OwnsThisType("weapon_milso"));
		break;
	default:
		GiveNamedItem("weapon_knife");
		Weapon_Switch(Weapon_OwnsThisType("weapon_knife"));
		break;
	}
}

ConVar sv_neo_time_alive_until_cant_change_loadout("sv_neo_time_alive_until_cant_change_loadout", "25.f", FCVAR_CHEAT | FCVAR_REPLICATED, "How long after spawning changing loadouts is disabled ",
	true, 0.0f, false, 1.0f);

void CNEO_Player::GiveLoadoutWeapon(void)
{
	const NeoRoundStatus status = NEORules()->GetRoundStatus();
	if (!(status == NeoRoundStatus::Idle || status == NeoRoundStatus::Warmup) &&
		(IsObserver() || IsDead() || m_bIneligibleForLoadoutPick || m_aliveTimer.IsGreaterThen(sv_neo_time_alive_until_cant_change_loadout.GetFloat())))
	{
		return;
	}

	const char* szWep = CNEOWeaponLoadout::GetLoadoutWeaponEntityName(m_iNeoClass.Get(), m_iLoadoutWepChoice, false);
#if DEBUG
	DevMsg("Loadout slot: %i (\"%s\")\n", m_iLoadoutWepChoice.Get(), szWep);
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

	pEnt->SetAbsOrigin(EyePosition());
	pEnt->AddSpawnFlags(SF_NORESPAWN);

	CNEOBaseCombatWeapon *pNeoWeapon = dynamic_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pEnt);
	if (pNeoWeapon)
	{
		if (neo_sv_ignore_wep_xp_limit.GetBool() || m_iLoadoutWepChoice+1 <= CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(m_iXP, m_iNeoClass.Get(), false))
		{
			pNeoWeapon->SetSubType(wepSubType);

			DispatchSpawn(pEnt);

			if (pEnt != NULL && !(pEnt->IsMarkedForDeletion()))
			{
				RemoveAllItems(false);
				GiveDefaultItems();
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
	if (IsCarryingGhost())
	{
		return;
	}

	BaseClass::StartSprinting();
}

void CNEO_Player::StopSprinting(void)
{
	BaseClass::StopSprinting();
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
}

void CNEO_Player::StartWalking(void)
{
	m_fIsWalking = true;
}

void CNEO_Player::StopWalking(void)
{
	m_fIsWalking = false;
}

void CNEO_Player::CloakPower_Update(void)
{
	if (m_HL2Local.m_cloakPower < CloakPower_Cap())
	{
		float chargeRate = 0.0f;
		switch (GetClass())
		{
		case NEO_CLASS_RECON:
			chargeRate = 0.55f;
			break;
		case NEO_CLASS_ASSAULT:
			chargeRate = 0.25f;
			break;
		default:
			break;
		}
		CloakPower_Charge(chargeRate * gpGlobals->frametime);
	}
}

bool CNEO_Player::CloakPower_Drain(float flPower)
{
	m_HL2Local.m_cloakPower -= flPower;

	if (m_HL2Local.m_cloakPower < 0.0)
	{
		// Camo is depleted: clamp and fail
		m_HL2Local.m_cloakPower = 0.0;
		return false;
	}

	return true;
}

void CNEO_Player::CloakPower_Charge(float flPower)
{
	m_HL2Local.m_cloakPower += flPower;

	const float cloakCap = CloakPower_Cap();
	if (m_HL2Local.m_cloakPower > cloakCap)
	{
		// Full charge, clamp.
		m_HL2Local.m_cloakPower = cloakCap;
	}
}

float CNEO_Player::CloakPower_Cap() const
{
	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		return 13.0f;
	case NEO_CLASS_ASSAULT:
		return 8.0f;
	default:
		break;
	}
	return 0.0f;
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
	case NEO_CLASS_VIP:
		return NEO_VIP_CROUCH_SPEED * GetBackwardsMovementPenaltyScale();
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
	case NEO_CLASS_VIP:
		return NEO_VIP_NORM_SPEED * GetBackwardsMovementPenaltyScale();
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
	case NEO_CLASS_VIP:
		return NEO_VIP_WALK_SPEED * GetBackwardsMovementPenaltyScale();
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
	case NEO_CLASS_VIP:
		return NEO_VIP_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	default:
		return NEO_BASE_SPRINT_SPEED * GetBackwardsMovementPenaltyScale();
	}
}

int CNEO_Player::ShouldTransmit(const CCheckTransmitInfo* pInfo)
{
	if (auto ent = Instance(pInfo->m_pClientEnt))
	{
		if (auto *otherNeoPlayer = dynamic_cast<CNEO_Player *>(ent))
		{
			// If other is spectator or same team
			if (otherNeoPlayer->GetTeamNumber() == TEAM_SPECTATOR ||
					GetTeamNumber() == otherNeoPlayer->GetTeamNumber())
			{
				return FL_EDICT_ALWAYS;
			}

			// If the other player is actively using the ghost and therefore fetching beacons
			auto otherWep = static_cast<CNEOBaseCombatWeapon *>(otherNeoPlayer->GetActiveWeapon());
			if (otherWep && otherWep->GetNeoWepBits() & NEO_WEP_GHOST &&
					static_cast<CWeaponGhost *>(otherWep)->IsPosWithinViewDistance(GetAbsOrigin()))
			{
				return FL_EDICT_ALWAYS;
			}

			// If this player is carrying the ghost (wether active or not)
			if (IsCarryingGhost())
			{
				return FL_EDICT_ALWAYS;
			}
		}	
	}
	
	return BaseClass::ShouldTransmit(pInfo);
}

float CNEO_Player::GetActiveWeaponSpeedScale() const
{
	auto pWep = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
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
