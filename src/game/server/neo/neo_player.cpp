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

#include "neo_player_shared.h"

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
SendPropEHandle(SENDINFO(m_hDroppedJuggernautItem)),

SendPropBool(SENDINFO(m_bInThermOpticCamo)),
SendPropBool(SENDINFO(m_bLastTickInThermOpticCamo)),
SendPropBool(SENDINFO(m_bInVision)),
SendPropBool(SENDINFO(m_bHasBeenAirborneForTooLongToSuperJump)),
SendPropBool(SENDINFO(m_bShowTestMessage)),
SendPropBool(SENDINFO(m_bInAim)),
SendPropBool(SENDINFO(m_bIneligibleForLoadoutPick)),
SendPropBool(SENDINFO(m_bCarryingGhost)),

SendPropTime(SENDINFO(m_flCamoAuxLastTime)),
SendPropInt(SENDINFO(m_nVisionLastTick)),
SendPropTime(SENDINFO(m_flJumpLastTime)),

SendPropTime(SENDINFO(m_flNextPingTime)),

SendPropString(SENDINFO(m_pszTestMessage)),

SendPropArray(SendPropInt(SENDINFO_ARRAY(m_rfAttackersScores)), m_rfAttackersScores),
SendPropArray(SendPropFloat(SENDINFO_ARRAY(m_rfAttackersAccumlator), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rfAttackersAccumlator),
SendPropArray(SendPropInt(SENDINFO_ARRAY(m_rfAttackersHits)), m_rfAttackersHits),

SendPropInt(SENDINFO(m_NeoFlags), 4, SPROP_UNSIGNED),
SendPropString(SENDINFO(m_szNeoName)),
SendPropString(SENDINFO(m_szNeoClantag)),
SendPropString(SENDINFO(m_szNeoCrosshair)),
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
DEFINE_FIELD(m_flJumpLastTime, FIELD_TIME),
DEFINE_FIELD(m_flNextPingTime, FIELD_TIME),

DEFINE_FIELD(m_pszTestMessage, FIELD_STRING),

DEFINE_FIELD(m_rfAttackersScores, FIELD_CUSTOM),
DEFINE_FIELD(m_rfAttackersAccumlator, FIELD_CUSTOM),
DEFINE_FIELD(m_rfAttackersHits, FIELD_CUSTOM),

DEFINE_FIELD(m_NeoFlags, FIELD_CHARACTER),

DEFINE_FIELD(m_szNeoName, FIELD_STRING),
DEFINE_FIELD(m_szNeoClantag, FIELD_STRING),
DEFINE_FIELD(m_szNeoCrosshair, FIELD_STRING),
DEFINE_FIELD(m_szNameDupePos, FIELD_INTEGER),
DEFINE_FIELD(m_bClientWantNeoName, FIELD_BOOLEAN),

// Inputs
DEFINE_INPUTFUNC(FIELD_STRING, "SetPlayerModel", InputSetPlayerModel),
DEFINE_INPUTFUNC(FIELD_VOID, "RefillAmmo", InputRefillAmmo),

END_DATADESC()

BEGIN_ENT_SCRIPTDESC(CNEO_Player, CHL2MP_Player, "NEO Player")
END_SCRIPTDESC();

static constexpr int SHOWMENU_STRLIMIT = 512;

CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;
CNEOGameRulesProxy* neoGameRules;
extern CBaseEntity *g_pLastSpawn;

extern ConVar sv_neo_ignore_wep_xp_limit;
extern ConVar sv_neo_clantag_allow;
extern ConVar sv_neo_dev_test_clantag;
extern ConVar sv_stickysprint;
extern ConVar sv_neo_dev_loadout;

ConVar sv_neo_can_change_classes_anytime("sv_neo_can_change_classes_anytime", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "Can players change classes at any moment, even mid-round?",
	true, 0.0f, true, 1.0f);
ConVar sv_neo_change_suicide_player("sv_neo_change_suicide_player", "0", FCVAR_REPLICATED, "Kill the player if they change the team and they're alive.", true, 0.0f, true, 1.0f);
ConVar sv_neo_change_threshold_interval("sv_neo_change_threshold_interval", "0.25", FCVAR_REPLICATED, "The interval threshold limit in seconds before the player is allowed to change team.", true, 0.0f, true, 1000.0f);
ConVar sv_neo_dm_max_class_dur("sv_neo_dm_max_class_dur", "10", FCVAR_REPLICATED, "The time in seconds when the player can change class on respawn during deathmatch.", true, 0.0f, true, 60.0f);
ConVar sv_neo_warmup_godmode("sv_neo_warmup_godmode", "0", FCVAR_REPLICATED, "If enabled, everyone is invincible on idle and warmup.", true, 0.0f, true, 1.0f);

void CNEO_Player::RequestSetClass(int newClass)
{
	if (newClass < 0 || newClass >= NEO_CLASS_ENUM_COUNT)
	{
		Assert(false);
		return;
	}

	const bool bIsTypeDM = (NEORules()->GetGameType() == NEO_GAME_TYPE_TDM || NEORules()->GetGameType() == NEO_GAME_TYPE_DM);
	const NeoRoundStatus status = NEORules()->GetRoundStatus();
	if (IsDead() || sv_neo_can_change_classes_anytime.GetBool() ||
		(!m_bIneligibleForLoadoutPick && NEORules()->GetRemainingPreRoundFreezeTime(false) > 0) ||
		(bIsTypeDM && !m_bIneligibleForLoadoutPick && GetAliveDuration() < sv_neo_dm_max_class_dur.GetFloat()) ||
		(status == NeoRoundStatus::Idle || status == NeoRoundStatus::Warmup || status == NeoRoundStatus::Countdown))
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
		NEO_WEP_ZR68_S | NEO_WEP_BALC
#ifdef INCLUDE_WEP_PBK
		| NEO_WEP_PBK56S
#endif
		;

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
	const int iLoadoutClass = sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : classChosen;

	const char *pszWepName = (IN_BETWEEN_AR(0, iLoadoutClass, NEO_LOADOUT__COUNT) && IN_BETWEEN_AR(0, loadoutNumber, MAX_WEAPON_LOADOUTS)) ?
		CNEOWeaponLoadout::s_LoadoutWeapons[iLoadoutClass][loadoutNumber].info.m_szWeaponEntityName :
		"";

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

	auto *pNeoWeapon = assert_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pEnt);
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

	if (!sv_neo_ignore_wep_xp_limit.GetBool() &&
			loadoutNumber+1 > CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(m_iXP,
				sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : classChosen))
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

	if (player->IsAlive() && NEORules()->GetGameType() == NEO_GAME_TYPE_TUT)
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

extern void respawn(CBaseEntity* pEdict, bool fCopyCorpse);
void SetSkin(const CCommand &command)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());
	if (!player)
	{
		return;
	}

	if (player->IsAlive() && NEORules()->GetGameType() == NEO_GAME_TYPE_TUT)
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

	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TUT && NEORules()->GetForcedWeapon() >= 0)
	{
		respawn(player, false);
	}
}

void SetLoadout(const CCommand &command)
{
	auto player = static_cast<CNEO_Player*>(UTIL_GetCommandClient());
	if (!player)
	{
		return;
	}

	if (player->IsAlive() && NEORules()->GetGameType() == NEO_GAME_TYPE_TUT)
	{
		return;
	}

	if (command.ArgC() == 2)
	{
		int loadoutNumber = atoi(command.ArgV()[1]);
		player->RequestSetLoadout(loadoutNumber);
	}

	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TUT)
	{
		respawn(player, false);
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
	m_iNeoClass = NEORules()->GetForcedClass() >= 0 ? NEORules()->GetForcedClass() : NEO_CLASS_ASSAULT;
	m_iNeoSkin = NEORules()->GetForcedSkin() >= 0 ? NEORules()->GetForcedSkin() : NEO_SKIN_FIRST;
	m_iNeoStar = NEO_DEFAULT_STAR;
	m_iXP.GetForModify() = 0;
	V_memset(m_szNeoName.GetForModify(), 0, sizeof(m_szNeoName));
	m_szNeoNameHasSet = false;
	V_memset(m_szNeoClantag.GetForModify(), 0, sizeof(m_szNeoClantag));
	V_memset(m_szNeoCrosshair.GetForModify(), 0, sizeof(m_szNeoCrosshair));

	m_bInThermOpticCamo = m_bInVision = false;
	m_bHasBeenAirborneForTooLongToSuperJump = false;
	m_bInAim = false;
	m_bCarryingGhost = false;
	m_bInLean = NEO_LEAN_NONE;

	m_iLoadoutWepChoice = NEORules()->GetForcedWeapon() >= 0 ? NEORules()->GetForcedWeapon() : 0;
	m_iNextSpawnClassChoice = -1;

	m_bShowTestMessage = false;
	V_memset(m_pszTestMessage.GetForModify(), 0, sizeof(m_pszTestMessage));

	m_flCamoAuxLastTime = 0;
	m_nVisionLastTick = 0;
	m_flLastAirborneJumpOkTime = 0;
	m_flLastSuperJumpTime = 0;
	m_botThermOpticCamoDisruptedTimer.Invalidate();

	m_bFirstDeathTick = true;
	m_bCorpseSet = false;
	m_bPreviouslyReloading = false;
	m_bLastTickInThermOpticCamo = false;
	m_bIsPendingSpawnForThisRound = false;
	m_bAllowGibbing = true;

	m_flNextTeamChangeTime = gpGlobals->curtime + 0.5f;

	m_NeoFlags = 0;

	memset(m_szNeoNameWDupeIdx, 0, sizeof(m_szNeoNameWDupeIdx));
	m_szNeoNameWDupeIdxNeedUpdate = true;
	m_szNameDupePos = 0;
	
	m_flNextPingTime = 0;
}

CNEO_Player::~CNEO_Player( void )
{
}

void CNEO_Player::Precache( void )
{
	BaseClass::Precache();
}

extern ConVar bot_changeclass;
void CNEO_Player::Spawn(void)
{
	int teamNumber = GetTeamNumber();
	ShowViewPortPanel(PANEL_SPECGUI, teamNumber == TEAM_SPECTATOR);

	if (teamNumber == TEAM_JINRAI || teamNumber == TEAM_NSF)
	{
		if (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze)
		{
			AddFlag(FL_GODMODE);
			AddNeoFlag(NEO_FL_FREEZETIME);
		}

		if (NEORules()->IsRoundOn())
		{ // NEO TODO (Adam) should people who were spectating at the start of the match and potentially have unfair information about the enemy team be allowed to join the game?
			m_bSpawnedThisRound = true;
		}

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
	m_iBotDetectableBleedingInjuryEvents = 0;
	m_flCamoAuxLastTime = 0;

	m_bInVision = false;
	m_nVisionLastTick = 0;
	m_bInLean = NEO_LEAN_NONE;
	m_bCorpseSet = false;
	m_bAllowGibbing = true;
	m_bIneligibleForLoadoutPick = false;

	static_assert(_ARRAYSIZE(m_rfAttackersScores) == MAX_PLAYERS);
	static_assert(_ARRAYSIZE(m_rfAttackersAccumlator) == MAX_PLAYERS);
	static_assert(_ARRAYSIZE(m_rfAttackersHits) == MAX_PLAYERS);
	for (int i = 0; i < MAX_PLAYERS; ++i)
	{
		m_rfAttackersScores.GetForModify(i) = 0;
		m_rfAttackersAccumlator.GetForModify(i) = 0.0f;
		m_rfAttackersHits.GetForModify(i) = 0;
	}

	m_flRanOutSprintTime = 0.0f;
	m_flNextPingTime = 0.0f;

	Weapon_SetZoom(false);

	SetTransmitState(FL_EDICT_PVSCHECK);

	StopWaterDeathSounds();

	SetPlayerTeamModel();
	if (teamNumber == TEAM_JINRAI || teamNumber == TEAM_NSF)
	{
		if (IsFakeClient())
		{
			const int maxLoadoutChoice = CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(m_iXP,
					sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : m_iNeoClass.Get()) - 1;
			m_iLoadoutWepChoice = RandomInt(MAX(0, maxLoadoutChoice - 3), maxLoadoutChoice);
		}
		GiveLoadoutWeapon();
		SetViewOffset(VEC_VIEW_NEOSCALE(this));
	}

	if (teamNumber == TEAM_UNASSIGNED && gpGlobals->eLoadType != MapLoad_Background)
	{
		int forcedTeam = NEORules()->GetForcedTeam();
		if (NEORules()->GetForcedTeam() < 1) // don't let this loop infinitely if forcedTeam set to TEAM_UNASSIGNED
		{
			engine->ClientCommand(this->edict(), "teammenu");
			return;
		}
		ChangeTeam(forcedTeam);

		if (NEORules()->GetForcedClass() < 0)
		{
			engine->ClientCommand(this->edict(), "classmenu");
			return;
		}
		m_iNeoClass = NEORules()->GetForcedClass();

		if (NEORules()->GetForcedWeapon() < 0)
		{
			engine->ClientCommand(this->edict(), "loadoutmenu");
			return;
		}
		m_iLoadoutWepChoice = NEORules()->GetForcedWeapon();
		respawn(this, false);
	}
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
	if ((m_nButtons & IN_LEAN_LEFT) && !(m_nButtons & IN_LEAN_RIGHT || IsSprinting()))
	{
		m_bInLean = NEO_LEAN_LEFT;
	}
	else if ((m_nButtons & IN_LEAN_RIGHT) && !(m_nButtons & IN_LEAN_LEFT || IsSprinting()))
	{
		m_bInLean = NEO_LEAN_RIGHT;
	}
}

extern ConVar neo_ghost_bhopping;
void CNEO_Player::CalculateSpeed(void)
{
	float speed = GetNormSpeed();

	if (auto pNeoWep = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon()))
	{
		speed *= pNeoWep->GetSpeedScale();
	}

	if (GetFlags() & FL_DUCKING)
	{
		speed *= NEO_CROUCH_MODIFIER;
	}

	if (IsSprinting())
	{
		switch (m_iNeoClass) {
			case NEO_CLASS_RECON:
				speed *= NEO_RECON_SPRINT_MODIFIER;
				break;
			case NEO_CLASS_ASSAULT:
			case NEO_CLASS_VIP:
			case NEO_CLASS_JUGGERNAUT:
				speed *= NEO_ASSAULT_SPRINT_MODIFIER;
				break;
			case NEO_CLASS_SUPPORT:
				speed *= NEO_SUPPORT_SPRINT_MODIFIER; // Should never happen
				break;
			default:
				break;
		}
	}

	if (IsInAim())
	{
		speed *= NEO_AIM_MODIFIER;
	}

	if (m_nButtons & IN_WALK)
	{
		speed = MIN(GetFlags() & FL_DUCKING ? NEO_CROUCH_WALK_SPEED : NEO_WALK_SPEED, speed);
	}

	Vector absoluteVelocity = GetAbsVelocity();
	absoluteVelocity.z = 0.f;
	float currentSpeed = absoluteVelocity.Length();

	if (((!neo_ghost_bhopping.GetBool() && m_bCarryingGhost) || m_iNeoClass == NEO_CLASS_JUGGERNAUT) && GetMoveType() == MOVETYPE_WALK && currentSpeed > speed)
	{
		float overSpeed = currentSpeed - speed;
		absoluteVelocity.NormalizeInPlace();
		absoluteVelocity *= -overSpeed;
		ApplyAbsVelocityImpulse(absoluteVelocity);
	}
	speed = MAX(speed, 55);

	// Slowdown after jumping
	if (m_iNeoClass != NEO_CLASS_RECON)
	{
		const float timeSinceJumping = gpGlobals->curtime - m_flJumpLastTime;
		constexpr float SLOWDOWN_TIME = 1.15f;
		if (timeSinceJumping < SLOWDOWN_TIME)
		{
			speed = MAX(75, speed * (1 - ((SLOWDOWN_TIME - timeSinceJumping) * 1.75)));
		}
	}
	SetMaxSpeed(speed);
}

void CNEO_Player::HandleSpeedChangesLegacy()
{
	int buttonsChanged = m_afButtonPressed | m_afButtonReleased;

	bool bCanSprint = CanSprint();
	bool bIsSprinting = IsSprinting();
#ifdef NEO
	constexpr float MOVING_SPEED_MINIMUM = 0.5f; // NEOTODO (Adam) This is the same value as defined in cbaseanimating, should we be using the same value? Should we import it here?
	bool bWantSprint = (bCanSprint && IsSuitEquipped() && (m_nButtons & IN_SPEED) && GetLocalVelocity().IsLengthGreaterThan(MOVING_SPEED_MINIMUM));
#else
	bool bWantSprint = ( bCanSprint && IsSuitEquipped() && (m_nButtons & IN_SPEED) && (m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)));
#endif
#ifdef NEO
	if ( bIsSprinting != bWantSprint && ((m_nButtons & IN_SPEED) || buttonsChanged & IN_SPEED))
#else
	if ( bIsSprinting != bWantSprint && (buttonsChanged & IN_SPEED)  )
#endif
	{
		// If someone wants to sprint, make sure they've pressed the button to do so. We want to prevent the
		// case where a player can hold down the sprint key and burn tiny bursts of sprint as the suit recharges
		// We want a full debounce of the key to resume sprinting after the suit is completely drained
		if ( bWantSprint )
		{
			if ( sv_stickysprint.GetBool() )
			{
				StartAutoSprint();
			}
			else
			{
				StartSprinting();
			}
		}
		else
		{
			if ( !sv_stickysprint.GetBool() )
			{
				StopSprinting();
			}
			// Reset key, so it will be activated post whatever is suppressing it.
			m_nButtons &= ~IN_SPEED;
		}
	}

#ifdef NEO
	if (bIsSprinting && GetLocalVelocity().IsLengthLessThan(MOVING_SPEED_MINIMUM))
#else
	if (bIsSprinting && !(m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)))
#endif
	{
		StopSprinting();
	}

	bool bIsWalking = IsWalking();
	// have suit, pressing button, not sprinting or ducking
	bool bWantWalking;

	if( IsSuitEquipped() )
	{
		bWantWalking = (m_nButtons & IN_WALK) && !IsSprinting();
	}
	else
	{
		bWantWalking = true;
	}

	if( bIsWalking != bWantWalking )
	{
		if ( bWantWalking )
		{
			StartWalking();
		}
		else
		{
			StopWalking();
		}
	}
}

#if 0
void CNEO_Player::HandleSpeedChanges( CMoveData *mv )
{
	int nChangedButtons = mv->m_nButtons ^ mv->m_nOldButtons;

	bool bJustPressedSpeed = !!( nChangedButtons & IN_SPEED );

#ifdef NEO
	constexpr float MOVING_SPEED_MINIMUM = 0.5f; // NEOTODO (Adam) This is the same value as defined in cbaseanimating, should we be using the same value? Should we import it here?
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) && GetLocalVelocity().IsLengthGreaterThan(MOVING_SPEED_MINIMUM));
#else
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) );
#endif

#ifdef NEO
	const bool bWantsToChangeSprinting = ( IsSprinting() != bWantSprint ) && ((mv->m_nButtons & IN_SPEED) || (( nChangedButtons & IN_SPEED ) != 0));
#else
	const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && ( nChangedButtons & IN_SPEED ) != 0;
#endif

	bool bSprinting = IsSprinting();
	if ( bWantsToChangeSprinting )
	{
		if ( bWantSprint )
		{
#ifdef NEO
			if ( m_HL2Local.m_flSuitPower < SPRINT_START_MIN )
#else
			if ( m_HL2Local.m_flSuitPower < 10.0f )
#endif
			{
#ifndef NEO
				if ( bJustPressedSpeed )
				{
					CPASAttenuationFilter filter( this );
					filter.UsePredictionRules();
					EmitSound( filter, entindex(), "HL2Player.SprintNoPower" );
				}
#endif
			}
			else
			{
				bSprinting = true;
			}
		}
		else
		{
			bSprinting = false;
		}
	}

	if ( m_HL2Local.m_flSuitPower < 0.01 )
	{
		bSprinting = false;
	}

	bool bWantWalking;

	if ( IsSuitEquipped() )
	{
		bWantWalking = ( mv->m_nButtons & IN_WALK ) && !bSprinting && !( mv->m_nButtons & IN_DUCK );
	}
	else
	{
		bWantWalking = true;
	}

	if ( bWantWalking )
	{
		bSprinting = false;
	}

	m_HL2Local.m_bNewSprinting = bSprinting;

	if ( bSprinting )
	{
#ifndef NEO
		if ( bJustPressedSpeed )
		{
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "HL2Player.SprintStart" );
		}
#endif
		mv->m_flClientMaxSpeed = GetSprintSpeed_WithActiveWepEncumberment();
	}
	else if ( bWantWalking )
	{
		mv->m_flClientMaxSpeed = GetCrouchSpeed_WithActiveWepEncumberment();
	}
	else
	{
		mv->m_flClientMaxSpeed = GetNormSpeed_WithActiveWepEncumberment();
	}

	mv->m_flMaxSpeed = MaxSpeed();
}
#endif

void CNEO_Player::PreThink(void)
{
	SpectatorTakeoverPlayerPreThink();

	BaseClass::PreThink();

	if (m_HL2Local.m_flSuitPower <= 0.0f && IsSprinting())
	{
		m_flRanOutSprintTime = gpGlobals->curtime;
		StopSprinting();
	}

	if (!m_bInThermOpticCamo)
	{
		CloakPower_Update();
	}

	CalculateSpeed();

	CheckThermOpticButtons();
	CheckVisionButtons();
	CheckPingButton(this);

	if (m_bInThermOpticCamo)
	{
		if (m_flCamoAuxLastTime == 0)
		{
			if (m_HL2Local.m_cloakPower >= MIN_CLOAK_AUX)
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
					SetCloakState(false);
					m_flCamoAuxLastTime = 0;
					PlayCloakSound(false);
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

	if (IsAlive() || m_vecLean != vec3_origin)
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

	if (m_iNeoClass == NEO_CLASS_RECON &&
		(m_afButtonPressed & IN_JUMP) && (m_nButtons & IN_SPEED) &&
		IsAllowedToSuperJump())
	{
		SuitPower_Drain(SUPER_JMP_COST);
		bool forward = m_nButtons & IN_FORWARD;
		bool backward = m_nButtons & IN_BACK;
		if (forward xor backward)
		{
			SuperJump();
		}
	}
}

void CNEO_Player::PlayCloakSound(bool removeLocalPlayer)
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
	if (removeLocalPlayer)
	{
		filter.RemoveRecipient(this); // We play clientside for ourselves
	}

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

		// for emulating bot visibility of cloak initiation flash
		m_botThermOpticCamoDisruptedTimer.Start(0.5f);
	}
}

void CNEO_Player::SetCloakState(bool state)
{
	m_bInThermOpticCamo = state;

	void (CBaseCombatWeapon:: * setShadowState)(int) = m_bInThermOpticCamo ? &CBaseCombatWeapon::AddEffects 
																		   : &CBaseCombatWeapon::RemoveEffects;

	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		if (CBaseCombatWeapon* weapon = GetWeapon(i))
		{
			(weapon->*setShadowState)(EF_NOSHADOW);
		}
	}
}

void CNEO_Player::CloakFlash(float time)
{
	CRecipientFilter filter;
	filter.AddRecipientsByPVS(GetAbsOrigin());

	g_NEO_TE_TocFlash.r = 64;
	g_NEO_TE_TocFlash.g = 64;
	g_NEO_TE_TocFlash.b = 255;
	g_NEO_TE_TocFlash.m_vecOrigin = GetAbsOrigin();
	g_NEO_TE_TocFlash.exponent = 10;
	g_NEO_TE_TocFlash.m_fRadius = 96;
	g_NEO_TE_TocFlash.m_fTime = time ? time : 0.5;
	g_NEO_TE_TocFlash.m_fDecay = 192;

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

		if (!m_bInThermOpticCamo && m_HL2Local.m_cloakPower >= MIN_CLOAK_AUX)
		{
			SetCloakState( true );
			CloakFlash();
		}
		else
		{
			SetCloakState( false );
		}
	}

	if (m_bInThermOpticCamo != m_bLastTickInThermOpticCamo)
	{
		PlayCloakSound();
	}
}

//-----------------------------------------------------------------------------
// Purpose: return true if given target cant be seen because of fog
//-----------------------------------------------------------------------------
bool CNEO_Player::IsHiddenByFog(CBaseEntity* target) const
{
	if (!target)
		return false;

	return RandomFloat() < GetFogObscuredRatio(target);
}

//-----------------------------------------------------------------------------
// Purpose: return 0-1 ratio where zero is not obscured, and 1 is completely obscured
//-----------------------------------------------------------------------------
float CNEO_Player::GetFogObscuredRatio(CBaseEntity* target) const
{
	if (!target)
		return 0.0f;

	auto targetPlayer = ToNEOPlayer(target);
	if (targetPlayer == nullptr)
	{
		// If it's not a player, this cloaking logic doesn't apply, so it is not obscured
		return 0.0f;
	}

	if (!targetPlayer->GetBotPerceivedCloakState())
	{
		// Target is not cloaked, so not obscured
		return 0.0f;
	}

	// --- Base Detection Chance (Per Tick) ---
	// This is the baseline chance of detection in ideal conditions (stationary, healthy, class-agnostic bot).
	// Aiming for ~5% detection
	float detectionChance = 0.05f * gpGlobals->interval_per_tick;

	// --- Multipliers for Detection Chance ---
	// Multipliers > 1.0 increase detection likelihood. Multipliers < 1.0 decrease it.

	// Target Movement Multipliers
	constexpr float MULT_TARGET_WALKING = 20.0f;   // Walking increases detection chance by 20x
	constexpr float MULT_TARGET_RUNNING = 50.0f;   // Running increases detection chance by 50x (very high risk)

	// Bot's State Multipliers (How the observer's state affects its perception)
	constexpr float MULT_MY_PLAYER_MOVING = 0.5f;  // Observer moving: 50% less chance to detect (detectionChance *= 0.5)
	constexpr float MULT_SUPPORT_BOT_VISION = 0.6; // Support bot: 40% less chance to detect (detectionChance *= 0.6)
	constexpr float MIN_ASSAULT_DETECTION_CHANCE_PER_TICK = 0.20f; // 20% detection per tick (very high)

	// Per Bleeding Injury Target Percentage Increase (Estimate of client-side bleeding decals)
	constexpr float MULT_BLEEDING_INJURY_EVENT_FACTOR = 0.05f;

	// Distance Multipliers (How distance affects detection)
	// These define ranges where detection scales.
	constexpr float DISTANCE_MAX_DETECTION_SQ = 100.0f * 100.0f;  // Max detection effect at 100 units (100^2)
	constexpr float DISTANCE_MIN_DETECTION_SQ = 6000.0f * 6000.0f; // Min detection effect at 6000 units (6000^2)

	constexpr float DISTANCE_MULT_CLOSE = 5.0f; // Multiplier when very close (e.g., within 100 units)
	constexpr float DISTANCE_MULT_FAR = 0.01f;    // Multiplier when very far (e.g., beyond 3000 units)

	// --- Helper Lambdas for Movement ---
	constexpr auto isMoving = [](const CNEO_Player* player, float tolerance = 10.0f) {
		return !player->GetAbsVelocity().IsZero(tolerance);
		};
	// Defined a clear threshold for 'running' velocity.
	constexpr auto isRunning = [](const CNEO_Player* player, float runSpeedThreshold = 200.0f) {
		return player->GetAbsVelocity().LengthSqr() > (runSpeedThreshold * runSpeedThreshold);
		};

	bool myPlayerIsMoving = isMoving(this); // Observer (me) is moving
	bool targetIsMoving = isMoving(targetPlayer);
	bool targetIsRunning = isRunning(targetPlayer);

	// --- Apply Multipliers to Base Detection Chance ---

	// Player Movement Impact
	if (targetIsRunning) // Running is the most severe penalty
	{
		detectionChance *= MULT_TARGET_RUNNING;
	}
	else if (targetIsMoving) // Walking/strafing
	{
		detectionChance *= MULT_TARGET_WALKING;
	}

	// Bot Movement Impact
	if (myPlayerIsMoving)
	{
		detectionChance *= MULT_MY_PLAYER_MOVING;
	}

	// Distance Impact
	const Vector& myPos = GetAbsOrigin(); // TODO: May need GetBot()->GetPosition() equivalent
	float currentRangeSq = (target->GetAbsOrigin() - myPos).LengthSqr(); // TODO: May need known.GetLastKnownPosition() equivalent

	float distanceMultiplier;
	if (currentRangeSq <= DISTANCE_MAX_DETECTION_SQ) // Very close range
	{
		distanceMultiplier = DISTANCE_MULT_CLOSE;
	}
	else if (currentRangeSq >= DISTANCE_MIN_DETECTION_SQ) // Very far range
	{
		distanceMultiplier = DISTANCE_MULT_FAR;
	}
	else // Interpolate between max and min detection effects
	{
		// Alpha: 1.0 when at DISTANCE_MAX_DETECTION_SQ, 0.0 when at DISTANCE_MIN_DETECTION_SQ
		float alpha = 1.0f - ((currentRangeSq - DISTANCE_MAX_DETECTION_SQ) / (DISTANCE_MIN_DETECTION_SQ - DISTANCE_MAX_DETECTION_SQ));
		distanceMultiplier = DISTANCE_MULT_FAR * (1.0f - alpha) + DISTANCE_MULT_CLOSE * alpha;
	}
	detectionChance *= distanceMultiplier;

	// Class-specific Bot Perception
	if (GetClass() == NEO_CLASS_SUPPORT)
	{
		detectionChance *= MULT_SUPPORT_BOT_VISION;
	}

	// Injured Target Impact
	detectionChance *= 1.0f + (MULT_BLEEDING_INJURY_EVENT_FACTOR * targetPlayer->GetBotDetectableBleedingInjuryEvents());

	// Assault class motion vision
	if (GetClass() == NEO_CLASS_ASSAULT && targetIsMoving)
	{
		detectionChance = Max(detectionChance, MIN_ASSAULT_DETECTION_CHANCE_PER_TICK);
	}

	// Ensure the final detection chance is within valid bounds [0, 1] (as a ratio)
	detectionChance = Clamp(detectionChance, 0.0f, 1.0f);

	// Convert detection chance to obscured ratio (invert: high detection = low obscured ratio)
	float obscuredRatio = 1.0f - detectionChance;

	return obscuredRatio;
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

	float boostIntensity = GetPlayerMaxSpeed();
	if (m_nButtons & (IN_MOVELEFT | IN_MOVERIGHT))
	{
		constexpr float sideWaysNerf = 0.70710678118; // 1 / sqrt(2);
		boostIntensity *= sideWaysNerf;
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

	ApplyAbsVelocityImpulse(forward * boostIntensity);
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
	if ((m_nButtons & (IN_FORWARD | IN_BACK | IN_MOVELEFT | IN_MOVERIGHT)) == 0)
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

		auto observerMode = GetObserverMode();
		if (observerMode == OBS_MODE_CHASE || observerMode == OBS_MODE_IN_EYE)
		{
			auto target = GetObserverTarget();
			if (!IsValidObserverTarget(target))
			{
				auto nextTarget = FindNextObserverTarget(false);
				if (nextTarget && nextTarget != target)
				{
					SetObserverTarget(nextTarget);
				}
			}
		}

		if ((observerMode == OBS_MODE_DEATHCAM || observerMode == OBS_MODE_NONE) && gpGlobals->curtime >= (GetDeathTime() + DEATH_ANIMATION_TIME))
		{ // We switch observer mode to none to view own body in third person so assume should still be changing observer targets
			auto target = GetObserverTarget();
			if (!IsValidObserverTarget(target))
			{
				auto nextTarget = FindNextObserverTarget(false);
				if (nextTarget && nextTarget != target)
				{
					SetObserverTarget(nextTarget);
				}
			}
			SetObserverMode(OBS_MODE_IN_EYE);
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
		else if (clientAimHold ? (m_nButtons & IN_AIM && !IsInAim()) : m_afButtonPressed & IN_AIM)
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
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_TUT)
	{	// no respawning in TUT gamemode if team class and weapon enforced, NEO TODO (Adam) look at this later
		return;
	}
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

const char *CNEO_Player::GetNeoClantag() const
{
	if (!sv_neo_clantag_allow.GetBool())
	{
		return "";
	}
#ifdef DEBUG
	const char *overrideClantag = sv_neo_dev_test_clantag.GetString();
	if (overrideClantag && overrideClantag[0])
	{
		return overrideClantag;
	}
#endif
	return m_szNeoClantag.Get();
}

const char *CNEO_Player::InternalGetNeoPlayerName(const CNEO_Player *viewFrom) const
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

const char *CNEO_Player::GetNeoPlayerName(const CNEO_Player *viewFrom) const
{
	if (viewFrom && viewFrom->m_bClientStreamermode && viewFrom->entindex() != entindex())
	{
		[[maybe_unused]] uchar32 u32Out;
		bool bError = false;
		const char *pSzName = InternalGetNeoPlayerName(viewFrom);
		const int iSize = Q_UTF8ToUChar32(pSzName, u32Out, bError);
		if (!bError) V_memcpy(gStreamerModeNames[entindex()], pSzName, iSize);
		return gStreamerModeNames[entindex()];
	}
	return InternalGetNeoPlayerName(viewFrom);
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
	if (bZoomIn == m_bInAim)
	{
		return;
	}

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
	if (NEORules()->GetRoundStatus() != NeoRoundStatus::Pause)
	{
		m_iXP.GetForModify() += 1;
	}
}

bool CNEO_Player::HandleCommand_JoinTeam( int team )
{
	const bool isAllowedToJoin = ProcessTeamSwitchRequest(team);

	if (isAllowedToJoin)
	{
		SetPlayerTeamModel();

		CRecipientFilter filterNonStreamers;
		CRecipientFilter filterStreamers;
		filterNonStreamers.MakeReliable();
		filterStreamers.MakeReliable();
		for (int i = 1; i <= gpGlobals->maxClients; ++i)
		{
			auto neoPlayer = static_cast<CNEO_Player *>(UTIL_PlayerByIndex(i));
			if (neoPlayer)
			{
				if (neoPlayer->m_bClientStreamermode)
				{
					filterStreamers.AddRecipient(neoPlayer);
				}
				else
				{
					filterNonStreamers.AddRecipient(neoPlayer);
				}
			}
		}
		UTIL_ClientPrintFilter(filterNonStreamers, HUD_PRINTTALK,
							   "%s1 joined team %s2\n", GetNeoPlayerName(), GetTeam()->GetName());
		UTIL_ClientPrintFilter(filterStreamers, HUD_PRINTTALK, "Player joined team %s1\n", GetTeam()->GetName());
	}

	if (isAllowedToJoin && NEORules()->GetForcedClass() >= 0 && NEORules()->GetForcedWeapon() >= 0)
	{
		respawn(this, false);
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

		switch (m_eMenuSelectType)
		{
		case MENU_SELECT_TYPE_PAUSE:
			m_eMenuSelectType = MENU_SELECT_TYPE_NONE;
			if (slot == PAUSE_MENU_SELECT_SHORT || slot == PAUSE_MENU_SELECT_LONG)
			{
				// When a pause requested, this will activate either:
				// - If during freezetime, immediate pausing
				// - If during live-round, after round finishes pausing
				NEORules()->m_iPausingTeam = GetTeamNumber();
				NEORules()->m_iPausingRound = NEORules()->roundNumber() + 1;
				NEORules()->m_flPauseDur = (slot == PAUSE_MENU_SELECT_SHORT) ? 60.0f : 180.0f;

				char szPauseMsg[128];
				V_sprintf_safe(szPauseMsg, "Pause requested by %s. Pausing the match %s for %s.",
							   (GetTeamNumber() == TEAM_JINRAI) ? "Jinrai" : "NSF",
							   (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze) ?
								   "immediately" : "when the round ends",
							   (slot == PAUSE_MENU_SELECT_SHORT) ? "1 minute" : "3 minutes");
				UTIL_ClientPrintAll(HUD_PRINTTALK, szPauseMsg);
			}
			break;
		default:
			break;
		}

		return true;
	}
	else if (FStrEq(args[0], "spectatortakeoverplayer"))
	{
		if (args.ArgC() < 2)
		{
			return false;
		}

		int targetIndex = V_atoi(args[1]);
		CBaseEntity* pTarget = UTIL_PlayerByUserId(targetIndex);
		CNEO_Player* pPlayerToTakeover = ToNEOPlayer(pTarget);

		if (pPlayerToTakeover)
		{
			SpectatorTryReplacePlayer(pPlayerToTakeover);
		}
		else
		{
			return false;
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
			vm->AddEffects(EF_NODRAW);
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
	if (GetClass() != NEO_CLASS_JUGGERNAUT)
	{
		return BaseClass::BecomeRagdollOnClient(force);
	}
	else
	{
		return false;
	}
}

void CNEO_Player::StartShowDmgStats(const CTakeDamageInfo *info)
{
	// Notify the client to both print out in console and show UI stats
	CSingleUserRecipientFilter filter(this);
	filter.MakeReliable();

	UserMessageBegin(filter, "DamageInfo");
	{
		short attackerIdx = 0;
		auto *neoAttacker = info ? ToNEOPlayer(info->GetAttacker()) : nullptr;
		auto *killedWithInflictor = info ? info->GetInflictor() : nullptr;
		const char *killedWithName = "";
		if (neoAttacker)
		{
			attackerIdx = static_cast<short>(neoAttacker->entindex());

			if (killedWithInflictor)
			{
				const bool inflictorIsPlayer = (ToNEOPlayer(killedWithInflictor) != nullptr);
				if (inflictorIsPlayer)
				{
					const auto *neoActiveWep = static_cast<CNEOBaseCombatWeapon *>(neoAttacker->GetActiveWeapon());
					if (neoAttacker->entindex() == entindex())
					{
						// NEO NOTE (nullsystem): This is a suicide kill, only explosive weapons makes sense
						// Otherwise this could either be "kill" command or by map but still registers as player (EX: leech in ntre_rogue_ctg)
						// For now we cannot tell EX: leech from kill comamnd so just leave it blank for that here
						killedWithName = (neoActiveWep && (neoActiveWep->GetNeoWepBits() & NEO_WEP_EXPLOSIVE)) ? neoActiveWep->GetPrintName() : "";
					}
					else
					{
						killedWithName = (neoActiveWep) ? neoActiveWep->GetPrintName() : "";
					}
				}
				else
				{
					killedWithName = killedWithInflictor->GetDebugName();
				}
			}
			if (!Q_strcmp(killedWithName, "neo_grenade_frag")) { killedWithName = "Frag Grenade"; }
			if (!Q_strcmp(killedWithName, "neo_deployed_detpack")) { killedWithName = "Remote Detpack"; }
		}
		else if (killedWithInflictor)
		{
			// Set it to NEO_ENVIRON_KILLED to indicate the map killed the player
			attackerIdx = NEO_ENVIRON_KILLED;
			killedWithName = killedWithInflictor->GetDebugName();
		}
		WRITE_SHORT(attackerIdx);
		WRITE_STRING(killedWithName);
	}
	MessageEnd();
}

void CNEO_Player::AddPoints(int score, bool bAllowNegativeScore)
{
	if (m_hSpectatorTakeoverPlayerTarget.Get())
	{
		if (score >= 0)
		{
			// Don't penalize possessed/bot for takeover player's actions
			m_hSpectatorTakeoverPlayerTarget->AddPoints(score, false);
			return;
		}
		// If a player made a mistake while taking over another player, continue to penalize them
	}

	// Positive score always adds
	if (score < 0)
	{
		if (!bAllowNegativeScore)
		{
			if (m_iXP < 0)		// Can't go more negative
				return;

			if (-score > m_iXP)	// Will this go negative?
			{
				score = -m_iXP;		// Sum will be 0
			}
		}
	}

	int oldRank = GetRank(m_iXP);
	m_iXP += score;
	//pl.frags = m_iFrags; NEO TODO (Adam) Is this actually used anywhere? should we include a xp field in CPlayerState ?
	int newRank = GetRank(m_iXP);
	if (oldRank == newRank)
	{
		return;
	}

	IGameEvent* event = gameeventmanager->CreateEvent("player_rankchange");
	if (event)
	{
		event->SetInt("userid", GetUserID());
		event->SetInt("oldrank", oldRank);
		event->SetInt("newrank", newRank);

		gameeventmanager->FireEvent(event);
	}
}

void CNEO_Player::Event_Killed( const CTakeDamageInfo &info )
{
	if (!m_bForceServerRagdoll && GetClass() != NEO_CLASS_JUGGERNAUT)
	{
		CreateRagdollEntity();
	}

	StopWaterDeathSounds();

	Weapon_DropAllOnDeath(info);

	if (GetClass() == NEO_CLASS_JUGGERNAUT)
	{
		SpawnJuggernautPostDeath();
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

	// Handle Corpse and Gibs
	if (!m_bCorpseSet) // Event_Killed can be called multiple times, only set the dead model and spawn gibs once
	{
		SetDeadModel(info);
	}

	RestorePlayerFromSpectatorTakeover();
}

void CNEO_Player::Weapon_DropAllOnDeath( const CTakeDamageInfo &info )
{
	// Drop all weapons except the active weapon
	const Vector damageForce = CalcDamageForceVector(info);
	int iExplosivesDropped = 0;
	constexpr int MAX_EXPLOSIVES_DROPPED = 1;
	CNEOBaseCombatWeapon* pActiveWeapon = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon());
	for (int i = 0; i < MAX_WEAPONS; ++i)
	{
		auto pNeoWeapon = static_cast<CNEOBaseCombatWeapon*>(m_hMyWeapons[i].Get());
		if (!pNeoWeapon)
		{
			continue;
		}

		if (pNeoWeapon == pActiveWeapon)
		{
			continue;
		}

		if (pNeoWeapon->IsExplosive() && ++iExplosivesDropped > MAX_EXPLOSIVES_DROPPED)
		{
			UTIL_Remove(pNeoWeapon);
			continue;
		}

		// Nowhere in particular; just drop it.
		Weapon_DropOnDeath(pNeoWeapon, damageForce);
	}

	// Same as above but for the active weapon
	if (pActiveWeapon)
	{
		if (pActiveWeapon->IsExplosive() && ++iExplosivesDropped > MAX_EXPLOSIVES_DROPPED)
		{
			UTIL_Remove(pActiveWeapon);
		}
		else
		{
			Weapon_DropOnDeath(pActiveWeapon, damageForce);
		}
	}
}

void CNEO_Player::Weapon_DropOnDeath(CNEOBaseCombatWeapon* pNeoWeapon, Vector damageForce)
{
	if (!pNeoWeapon->GetWpnData().m_bDropOnDeath)
	{ // Can't drop this weapon on death, remove it
		UTIL_Remove(pNeoWeapon);
		return;
	}
	
	if (pNeoWeapon->IsEffectActive( EF_BONEMERGE ))
	{
		//int iBIndex = LookupBone("valveBiped.Bip01_R_Hand"); NEOTODO (Adam) Should mimic the behaviour in basecombatcharacter that places the weapon such that the bones used in the bonemerge overlap
		Vector vFacingDir = BodyDirection2D();
		pNeoWeapon->SetAbsOrigin(Weapon_ShootPosition() + vFacingDir);
	}
	else
	{
		pNeoWeapon->AddEffects(EF_BONEMERGE);
	}

	Vector playerVelocity = vec3_origin;
	GetVelocity(&playerVelocity, nullptr);
	pNeoWeapon->Drop(playerVelocity);
	Weapon_Detach(pNeoWeapon);

	if (pNeoWeapon->VPhysicsGetObject())
	{
		constexpr float DAMAGE_FORCE_SCALE = 0.1f;
		pNeoWeapon->VPhysicsGetObject()->ApplyForceCenter(damageForce * DAMAGE_FORCE_SCALE);
	}
}

void CNEO_Player::SetDeadModel(const CTakeDamageInfo& info)
{
	m_bCorpseSet = true;

	int deadModelType = -1;

	if (!m_bAllowGibbing || m_bForceServerRagdoll) // Prevent gibbing if a custom player model has been set via I/O or the ragdoll is serverside
	{
		return;
	}

	if (info.GetDamageType() & DMG_BLAST)
	{
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

	if (deadModelType == 0)
	{
		for (int i = 0; i < NEO_GIB_LIMB__ENUM_COUNT; i++)
		{
			CGib::SpawnSpecificGibs(this, 1, 10, 1000, modelManager->GetGibModel((NeoSkin)GetSkin(), (NeoClass)GetClass(), GetTeamNumber(), NeoGibLimb(i)));
		}
		UTIL_BloodSpray(info.GetDamagePosition(), info.GetDamageForce(), BLOOD_COLOR_RED, 10, FX_BLOODSPRAY_ALL);
	}
	else
	{
		CGib::SpawnSpecificGibs(this, 1, 10, 1000, modelManager->GetGibModel((NeoSkin)GetSkin(), (NeoClass)GetClass(), GetTeamNumber(), NeoGibLimb(deadModelType-1)));
		UTIL_BloodSpray(info.GetDamagePosition(), info.GetDamageForce(), BLOOD_COLOR_RED, 10, FX_BLOODSPRAY_GORE | FX_BLOODSPRAY_DROPS);
	}
	SetPlayerCorpseModel(deadModelType);
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
	const char* model = modelManager->GetCorpseModel((NeoSkin)GetSkin(), (NeoClass)GetClass(), GetTeamNumber(), NeoGib(type));

	if (!*model)
	{
		Assert(false);
		Warning("Failed to find model string for Neo player corpse\n");
		return;
	}

	if (m_hRagdoll)
	{
		m_hRagdoll->SetModel(model);
	}
}

float CNEO_Player::GetReceivedDamageScale(CBaseEntity* pAttacker)
{
	if ((NEORules()->GetGameType() == NEO_GAME_TYPE_TDM || NEORules()->GetGameType() == NEO_GAME_TYPE_DM)
			&& m_aliveTimer.IsLessThen(5.f) && !m_bIneligibleForLoadoutPick)
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
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_DAMAGE_MODIFIER * BaseClass::GetReceivedDamageScale(pAttacker);
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

	if (!((static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon()))->GetNeoWepBits() & NEO_WEP_SUPPRESSED))
	{
		// cloak disruption from unsuppressed weapons
		m_botThermOpticCamoDisruptedTimer.Start(0.5f);
	}
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
	const auto activeWeapon = GetActiveWeapon();
	if (activeWeapon == pWeapon)
	{
		return false;
	}

	if (activeWeapon)
	{
		activeWeapon->StopWeaponSound(RELOAD_NPC);
	}

	ShowCrosshair(false);
	Weapon_SetZoom(false);
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
	
	auto neoWeapon = assert_cast<CNEOBaseCombatWeapon*>(pWeapon);
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

// Input to set the player's model (not skin)
void CNEO_Player::InputSetPlayerModel( inputdata_t & inputdata )
{
	const char* modelpath = inputdata.value.String();

	if (modelpath && *modelpath)
	{
		PrecacheModel(modelpath);

		SetModel(modelpath);

		m_bAllowGibbing = false; // Disallow gibs or else the corpse will turn into whatever the player picked as their skin
	}
}

void CNEO_Player::InputRefillAmmo( inputdata_t & inputdata)
{
	CBaseCombatWeapon* pWeapon = GetActiveWeapon();

	if (pWeapon)
	{
		pWeapon->SetPrimaryAmmoCount(pWeapon->GetDefaultClip1());
		if (pWeapon->UsesSecondaryAmmo())
		{
			pWeapon->SetSecondaryAmmoCount(pWeapon->GetDefaultClip2());
		}
	}
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

	if (!pWeapon)
	{
		BaseClass::Weapon_Drop(pWeapon, pvecTarget, pVelocity);
		return;
	}

	auto neoWeapon = assert_cast<CNEOBaseCombatWeapon*>(pWeapon);
	if (IsDead() && neoWeapon->CanDrop())
	{
		if (auto* activeWep = static_cast<CNEOBaseCombatWeapon*>(GetActiveWeapon()))
		{
			// If player has held down an attack key since the previous frame
			if (((m_nButtons & IN_ATTACK) && (!(m_afButtonPressed & IN_ATTACK))) ||
				((m_nButtons & IN_ATTACK2) && (!(m_afButtonPressed & IN_ATTACK2))))
			{
				// Drop a grenade if it's primed.
				if (activeWep->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE)
				{
					auto grenade = assert_cast<CWeaponGrenade*>(activeWep);
					grenade->ThrowGrenade(this);
					grenade->DecrementAmmo(this);
					return;
				}
				// Drop a smoke if it's primed.
				else if (activeWep->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE)
				{
					auto grenade = assert_cast<CWeaponSmokeGrenade*>(activeWep);
					grenade->ThrowGrenade(this);
					grenade->DecrementAmmo(this);
					return;
				}
			}
		}
	}
	else if (!neoWeapon->CanDrop())
	{
		return; // Return early to not drop weapon
	}

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

	// NEO TODO (nullsystem): Teamplay vs non-teamplay
	// info_player_deathmatch is from HL2MP, but maps can utilize HL2MP and this entity anyway
	const bool bIsTeamplay = NEORules()->IsTeamplay();
	if (!bIsTeamplay)
	{
		pSpawnpointName = "info_player_deathmatch";
		pLastSpawnPoint = g_pLastSpawn;
	}
	else
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

bool CNEO_Player::ModeWantsSpectatorGUI(int iMode)
{ 
	return iMode != OBS_MODE_FIXED;
}

bool CNEO_Player::CanHearAndReadChatFrom( CBasePlayer *pPlayer )
{
	return BaseClass::CanHearAndReadChatFrom(pPlayer);
}

void CNEO_Player::PickDefaultSpawnTeam(void)
{
	if (!GetTeamNumber())
	{
		ProcessTeamSwitchRequest(TEAM_UNASSIGNED);
		if (!GetModelPtr())
		{
			SetPlayerTeamModel();
		}
	}
}

extern bool RespawnWithRet(CBaseEntity* pEdict, bool fCopyCorpse);
bool CNEO_Player::ProcessTeamSwitchRequest(int iTeam)
{
	if (!GetGlobalTeam(iTeam))
	{
		Warning("HandleCommand_JoinTeam( %d ) - invalid team index.\n", iTeam);
		return false;
	}

	const bool justJoined = (GetTeamNumber() == TEAM_UNASSIGNED);

	// Limit team join spam, unless this is a newly joined player
	if (!justJoined && m_flNextTeamChangeTime > gpGlobals->curtime)
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
	bool changedTeams = false;
	bool spawnImmediately = false;
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

		// StartObserverMode and State_Transition will check whether we are on the Spectator team to decide whether to enforce certain camera modes, easier if we switch teams beforehand
		CHL2_Player::ChangeTeam(iTeam, false, justJoined);
		changedTeams = true;

		State_Transition(STATE_OBSERVER_MODE);
		// Default to free fly camera if there's nobody to spectate
		if (justJoined || GetNumOtherPlayersConnected(this) == 0)
		{
			StartObserverMode(OBS_MODE_ROAMING);
		}
		else
		{
			StartObserverMode(OBS_MODE_IN_EYE);
		}
	}
	else if (iTeam == TEAM_UNASSIGNED)
	{
		SetObserverMode(OBS_MODE_FIXED);
		State_Transition(STATE_OBSERVER_MODE);
	}
	else if (iTeam == TEAM_JINRAI || iTeam == TEAM_NSF)
	{
		if (!justJoined && GetTeamNumber() != TEAM_SPECTATOR && !IsDead())
		{
			if (suicidePlayerIfAlive || NEORules()->CanChangeTeamClassLoadoutWhenAlive())
			{
				SoftSuicide();
			}
			else if (!suicidePlayerIfAlive)
			{
				ClientPrint(this, HUD_PRINTCENTER, "You can't switch teams while you are alive.");
				return false;
			}
		}

		CHL2_Player::ChangeTeam(iTeam, false, justJoined);
		changedTeams = true;
		
		// Spawn the player immediately if its a single life game mode
		spawnImmediately = !NEORules()->CanRespawnAnyTime() && NEORules()->FPlayerCanRespawn(this);
		if (!spawnImmediately)
		{
			if (NEORules()->CanRespawnAnyTime() || IsFakeClient())
			{ // Stop observer mode so we spawn in anyway after a short delay, bots crash when transitioning to observer mode
				StopObserverMode();
			}
			else
			{ // If we're not allowed to be in the current observer mode (because of mp_forcecamera for example), this will give us a new observer mode.
				SetObserverMode(GetObserverMode());
				State_Transition(STATE_OBSERVER_MODE);
			}
		}
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

	if (!changedTeams)
	{
		// We're skipping over HL2MP player because we don't care about
		// deathmatch rules or Combine/Rebels model stuff.
		CHL2_Player::ChangeTeam(iTeam, false, justJoined);
	}

	if (spawnImmediately)
	{ // Spawn after all the items are removed
		bool success = RespawnWithRet(this, false);
		if (!success)
		{ // shouldn't fail if NEORules()->FPlayerCanRespawn(this) returns true
			Assert(false);
			SetObserverMode(GetObserverMode());
			State_Transition(STATE_OBSERVER_MODE);
		}
	}

	NEORules()->m_bThinkCheckClantags = true;

	return true;
}

int CNEO_Player::GetAttackersScores(const int attackerIdx) const
{
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_DM || NEORules()->GetGameType() == NEO_GAME_TYPE_TDM)
	{
		return m_rfAttackersScores.Get(attackerIdx);
	}
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

		auto* neoAttacker = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(pIdx));
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
		if (sv_neo_warmup_godmode.GetBool() &&
				(NEORules()->GetRoundStatus() == NeoRoundStatus::Idle ||
				 NEORules()->GetRoundStatus() == NeoRoundStatus::Warmup ||
				 NEORules()->GetRoundStatus() == NeoRoundStatus::Countdown))
		{
			return 0;
		}

		// Checking because attacker might be prop or world
		if (auto *attacker = ToNEOPlayer(info.GetAttacker()))
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

					if (sv_neo_mirror_teamdamage_immunity.GetBool())
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

				if (bIsTeamDmg && sv_neo_teamdamage_kick.GetBool() && NEORules()->GetRoundStatus() == NeoRoundStatus::RoundLive)
				{
					attacker->m_iTeamDamageInflicted += iDamage;
				}

				if (info.GetDamageType() & (DMG_BULLET | DMG_SLASH | DMG_BUCKSHOT)) {
					++m_iBotDetectableBleedingInjuryEvents;
				}
			}
		}
	}
	return BaseClass::OnTakeDamage_Alive(info);
}

void GiveDet(CNEO_Player* pPlayer)
{
	constexpr const char* detpackClassname = "weapon_remotedet";
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

			auto pWeapon = assert_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pent);
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
	case NEO_CLASS_JUGGERNAUT:
		GiveNamedItem("weapon_balc");
		Weapon_Switch(Weapon_OwnsThisType("weapon_balc"));
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
	if (!(status == NeoRoundStatus::Idle || status == NeoRoundStatus::Warmup || status == NeoRoundStatus::Countdown) &&
		(IsObserver() || IsDead() || m_bIneligibleForLoadoutPick
		 || m_aliveTimer.IsGreaterThen(sv_neo_time_alive_until_cant_change_loadout.GetFloat())))
	{
		return;
	}

	const int iLoadoutClass = sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : m_iNeoClass.Get();
	const char *szWep = (IN_BETWEEN_AR(0, iLoadoutClass, NEO_LOADOUT__COUNT) && IN_BETWEEN_AR(0, m_iLoadoutWepChoice, MAX_WEAPON_LOADOUTS)) ?
			CNEOWeaponLoadout::s_LoadoutWeapons[iLoadoutClass][m_iLoadoutWepChoice].info.m_szWeaponEntityName :
			"";
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

	pEnt->SetLocalOrigin(GetLocalOrigin());
	pEnt->AddSpawnFlags(SF_NORESPAWN);

	CNEOBaseCombatWeapon *pNeoWeapon = assert_cast<CNEOBaseCombatWeapon*>((CBaseEntity*)pEnt);
	if (pNeoWeapon)
	{
		if (sv_neo_ignore_wep_xp_limit.GetBool() ||
				m_iLoadoutWepChoice+1 <= CNEOWeaponLoadout::GetNumberOfLoadoutWeapons(m_iXP,
					sv_neo_dev_loadout.GetBool() ? NEO_LOADOUT_DEV : m_iNeoClass.Get()))
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

	m_flRanOutSprintTime = 0.0f;
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

float CNEO_Player::CloakPower_Get(void) const
{
	return m_HL2Local.m_cloakPower;
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
		return NEO_RECON_CROUCH_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_CROUCH_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_CROUCH_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_CROUCH_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_CROUCH_SPEED;
	default:
		return (NEO_BASE_SPEED * NEO_CROUCH_MODIFIER);
	}
}

float CNEO_Player::GetNormSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_BASE_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_BASE_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_BASE_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_BASE_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_BASE_SPEED;
	default:
		return NEO_BASE_SPEED;
	}
}

float CNEO_Player::GetSprintSpeed(void) const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_SPRINT_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_SPRINT_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_SPRINT_SPEED;
	case NEO_CLASS_VIP:
		return NEO_VIP_SPRINT_SPEED;
	case NEO_CLASS_JUGGERNAUT:
		return NEO_JUGGERNAUT_SPRINT_SPEED;
	default:
		return NEO_BASE_SPEED; // No generic sprint modifier; default speed.
	}
}

extern ConVar neo_ctg_ghost_beacons_when_inactive;
int CNEO_Player::ShouldTransmit(const CCheckTransmitInfo* pInfo)
{
	if (pInfo)
	{
		auto otherNeoPlayer = assert_cast<CNEO_Player*>(Instance(pInfo->m_pClientEnt));

		// If other is spectator or same team
		if (otherNeoPlayer->GetTeamNumber() == TEAM_SPECTATOR ||
#ifdef GLOWS_ENABLE
			otherNeoPlayer->IsDead() ||
#endif
			GetTeamNumber() == otherNeoPlayer->GetTeamNumber())
		{
			return FL_EDICT_ALWAYS;
		}

		// If the other player is carrying the ghost and the ghost doesn't need to be the active weapon to fetch ghost beacons
		if (neo_ctg_ghost_beacons_when_inactive.GetBool() && NEORules()->GetGhosterPlayer() == otherNeoPlayer->entindex())
		{
			return FL_EDICT_ALWAYS;
		}


		// If the other player is actively using the ghost and therefore fetching beacons
		auto otherWep = static_cast<CNEOBaseCombatWeapon*>(otherNeoPlayer->GetActiveWeapon());
		if (otherWep && otherWep->GetNeoWepBits() & NEO_WEP_GHOST &&
			static_cast<CWeaponGhost*>(otherWep)->IsPosWithinViewDistance(GetAbsOrigin()))
		{
			return FL_EDICT_ALWAYS;
		}

		// If this player is carrying the ghost (whether active or not)
		if (IsCarryingGhost())
		{
			return FL_EDICT_ALWAYS;
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

void CNEO_Player::BecomeJuggernaut()
{
	NEORules()->JuggernautActivated(this);
	if (m_iNextSpawnClassChoice = -1)
	{
		m_iNextSpawnClassChoice = GetClass(); // Don't let the player respawn as the juggernaut
	}
	RemoveFlag(FL_DUCKING);
	m_Local.m_bDucked = false;
	m_Local.m_bDucking = false;
	m_Local.m_flDucktime = 0.0f;
	m_Local.m_flDuckJumpTime = 0.0f;
	m_Local.m_flJumpTime = 0.0f;
	if (GetToggledDuckState())
	{
		ToggleDuck();
	}

#define COLOR_JGR_FADE color32{170, 170, 170, 255}
	UTIL_ScreenFade(this, COLOR_JGR_FADE, 1.0f, 0.0f, FFADE_IN);

	m_iNeoClass = NEO_CLASS_JUGGERNAUT;
	SetPlayerTeamModel();
	SetViewOffset(VEC_VIEW_NEOSCALE(this));
	InitSprinting();
	RemoveAllItems(false);
	GiveDefaultItems();
	SetHealth(GetMaxHealth());
	SuitPower_SetCharge(100);
	//SetBloodColor(DONT_BLEED); Check C_HL2MP_Player::TraceAttack
	m_HL2Local.m_cloakPower = CloakPower_Cap();
	m_bAllowGibbing = false;
	m_bInVision = false;
}

void CNEO_Player::SpawnJuggernautPostDeath()
{
	CNEO_Juggernaut* pJuggernautItem = (CNEO_Juggernaut*)CreateEntityByName("neo_juggernaut");
	pJuggernautItem->SetAbsOrigin(GetAbsOrigin());
	pJuggernautItem->SetAbsAngles(GetAbsAngles());
	pJuggernautItem->SetAbsVelocity(GetAbsVelocity());
	pJuggernautItem->m_bPostDeath = true;
	if (NEORules()->GetGameType() == NEO_GAME_TYPE_JGR)
	{
		if (NEORules()->GetRoundStatus() == NeoRoundStatus::RoundLive)
		{
			EmitSound_t soundParams;
			soundParams.m_pSoundName = "HUD.GhostPickUp";
			soundParams.m_nChannel = CHAN_USER_BASE;
			soundParams.m_bWarnOnDirectWaveReference = false;
			soundParams.m_bEmitCloseCaption = false;
			soundParams.m_SoundLevel = ATTN_TO_SNDLVL(ATTN_NONE);

			CRecipientFilter soundFilter;
			soundFilter.AddAllPlayers();
			soundFilter.MakeReliable();
			EmitSound(soundFilter, this->entindex(), soundParams);
		}

		NEORules()->m_pJuggernautPlayer = nullptr;
		NEORules()->m_pJuggernautItem = pJuggernautItem;
	}
	m_hDroppedJuggernautItem = pJuggernautItem;
	DispatchSpawn(pJuggernautItem);
}

const char *CNEO_Player::GetOverrideStepSound(const char *pBaseStepSound)
{
	if (GetClass() == NEO_CLASS_JUGGERNAUT)
	{
		if (!IsSprinting())
		{
			if (m_Local.m_nStepside)
			{
				return "JGR56.FootstepLeft";
			}
			else
			{
				return "JGR56.FootstepRight";
			}
		}
		else
		{
			if (m_Local.m_nStepside)
			{
				return "JGR56.RunFootstepLeft";
			}
			else
			{
				return "JGR56.RunFootstepRight";
			}
		}
	}

	return BaseClass::GetOverrideStepSound(pBaseStepSound);
}

// Start spectator takeover of player related code:
ConVar sv_neo_spec_replace_player_bot_enable("sv_neo_spec_replace_player_bot_enable", "1", FCVAR_NONE, "Allow spectators to take over bots.", true, 0, true, 1);
ConVar sv_neo_spec_replace_player_afk_enable("sv_neo_spec_replace_player_afk_enable", "1", FCVAR_NONE, "Allow spectators to take over AFK players.", true, 0, true, 1);
ConVar sv_neo_spec_replace_player_afk_time_sec( "sv_neo_spec_replace_player_afk_time_sec",
	"120", FCVAR_NONE,
	"Seconds of inactivity before a player is considered AFK for spectator takeover.",
	true, -1, true, 999);
ConVar sv_neo_spec_replace_player_min_exp("sv_neo_spec_replace_player_min_exp",
	"0", FCVAR_NONE,
	"Minimum experience allowed to takeover players ",
	true, -999, true, 999);

bool CNEO_Player::IsAFK() const {
    // NEO JANK GetTimeSinceLastUserCommand seems to return 0 as long as the player is connected, so use an alternative timer
    return GetTimeSinceWeaponFired() > sv_neo_spec_replace_player_afk_time_sec.GetInt();
}

void CNEO_Player::SpectatorTryReplacePlayer(CNEO_Player* pNeoPlayerToReplace)
{
	CSingleUserRecipientFilter filter(this);

	if (!IsObserver() && IsAlive())
	{
		DevWarning("A client initiating player takeover without being in observer mode might indicate server command bugs or tampering.");
		UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: Not in observer mode.");
		return;
	}

	if (m_iXP < sv_neo_spec_replace_player_min_exp.GetInt())
	{
		if (m_iXP < 0)
		{
			UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: Rankless Dogs are not authorized.");
		}
		else
		{
			UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE,
				"Shell takeover failed: Requires at least %s1 XP for authorization.",
				sv_neo_spec_replace_player_min_exp.GetString());
		}
		return;
	}

	if (!pNeoPlayerToReplace)
	{
		UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: The target is not a valid candidate.");
		return;
	}

	if (!pNeoPlayerToReplace->IsAlive())
	{
		UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: The target is dead.");
		return;
	}

	if (pNeoPlayerToReplace->GetTeamNumber() != GetTeamNumber())
	{
		UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: Target is not friendly.");
		return;
	}

	const bool bIsTargetBot = pNeoPlayerToReplace->IsBot();
	const bool bIsTargetAFK = pNeoPlayerToReplace->IsAFK();
	const bool bAllowBotTakeover = sv_neo_spec_replace_player_bot_enable.GetBool();
	const bool bAllowAfkTakeover = sv_neo_spec_replace_player_afk_enable.GetBool();

	// If no valid condition is met, determine the specific reason and inform the user.
	if (bIsTargetBot)
	{
		if (!bAllowBotTakeover)
		{
			UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: Taking over drones is disabled.");
			return;
		}
	}
	else if (bIsTargetAFK)
	{
		if (!bAllowAfkTakeover)
		{
			UTIL_ClientPrintFilter(filter, HUD_PRINTCONSOLE, "Shell takeover failed: Taking over inactive shells is disabled.");
			return;
		}
	}
	else
	{
		char timeBuf[4]; // assuming max sv_neo_spec_replace_player_afk_time_sec is 999
		int  secondsLeft = sv_neo_spec_replace_player_afk_time_sec.GetInt()
			- static_cast<int>(pNeoPlayerToReplace->GetTimeSinceWeaponFired());
		V_snprintf(timeBuf, sizeof(timeBuf), "%d", secondsLeft);
		UTIL_ClientPrintFilter(
			filter,
			HUD_PRINTCONSOLE,
			"Shell takeover failed: Shell is not considered inactive until %s1 seconds.",
			timeBuf);
		return;
	}

	m_bSpectatorTakeoverPlayerPending = true;
	m_hSpectatorTakeoverPlayerTarget = pNeoPlayerToReplace;
	ForceRespawn();

	// Send client confirmation to smooth out transition
	CBroadcastRecipientFilter updatefilter;
	updatefilter.MakeReliable();
	UserMessageBegin(updatefilter, "CSpectatorTakeoverPlayer");
		WRITE_LONG(this->GetUserID());
		WRITE_LONG(pNeoPlayerToReplace->GetUserID());
	MessageEnd();
}

void CNEO_Player::SpectatorTakeoverPlayerPreThink()
{
	if (m_hSpectatorTakeoverPlayerImpersonatingMe.Get())
	{
		// Currently being impersonated by a player, but allow waiting on events in base PreThink.
		BaseClass::PreThink();
		return;
	}

	if (m_bSpectatorTakeoverPlayerPending)
	{
		if (!IsAlive() || IsObserver())
		{
			// Something is wrong, abort the takeover.
			DevWarning("Shell takeover failed: Player state was invalid after ForceRespawn().\n");
			m_bSpectatorTakeoverPlayerPending = false;
			m_hSpectatorTakeoverPlayerTarget = nullptr;
			return;
		}

		CNEO_Player* pPlayerTakeoverTarget = m_hSpectatorTakeoverPlayerTarget.Get();

		if (pPlayerTakeoverTarget)
		{
			m_iNeoClass = pPlayerTakeoverTarget->m_iNeoClass;
			m_iNeoSkin = pPlayerTakeoverTarget->m_iNeoSkin;
			SetMaxHealth(pPlayerTakeoverTarget->GetMaxHealth());
			SetHealth(pPlayerTakeoverTarget->GetHealth());
			SetArmorValue(pPlayerTakeoverTarget->ArmorValue());
			m_HL2Local.m_cloakPower = pPlayerTakeoverTarget->m_HL2Local.m_cloakPower;
			m_HL2Local.m_flSuitPower = pPlayerTakeoverTarget->m_HL2Local.m_flSuitPower;
			m_iLoadoutWepChoice = pPlayerTakeoverTarget->m_iLoadoutWepChoice;

			m_bInThermOpticCamo = pPlayerTakeoverTarget->m_bInThermOpticCamo;
			m_bHasBeenAirborneForTooLongToSuperJump = pPlayerTakeoverTarget->m_bHasBeenAirborneForTooLongToSuperJump;
			m_bInAim = pPlayerTakeoverTarget->m_bInAim;
			Weapon_SetZoom(pPlayerTakeoverTarget->m_bInAim);
			m_bCarryingGhost = pPlayerTakeoverTarget->m_bCarryingGhost;
			m_bInLean = pPlayerTakeoverTarget->m_bInLean;
			m_iLoadoutWepChoice = pPlayerTakeoverTarget->m_iLoadoutWepChoice;
			m_iNextSpawnClassChoice = pPlayerTakeoverTarget->m_iNextSpawnClassChoice;
			m_flCamoAuxLastTime = pPlayerTakeoverTarget->m_flCamoAuxLastTime;
			m_flLastAirborneJumpOkTime = pPlayerTakeoverTarget->m_flLastAirborneJumpOkTime;
			m_flLastSuperJumpTime = pPlayerTakeoverTarget->m_flLastSuperJumpTime;
			m_botThermOpticCamoDisruptedTimer.Invalidate(); // taken over by player
			m_bFirstDeathTick = pPlayerTakeoverTarget->m_bFirstDeathTick;
			m_bCorpseSet = pPlayerTakeoverTarget->m_bCorpseSet;
			m_bPreviouslyReloading = pPlayerTakeoverTarget->m_bPreviouslyReloading;
			m_bLastTickInThermOpticCamo = pPlayerTakeoverTarget->m_bLastTickInThermOpticCamo;
			m_bIsPendingSpawnForThisRound = pPlayerTakeoverTarget->m_bIsPendingSpawnForThisRound;
			m_bAllowGibbing = pPlayerTakeoverTarget->m_bAllowGibbing;
			m_iBotDetectableBleedingInjuryEvents = pPlayerTakeoverTarget->m_iBotDetectableBleedingInjuryEvents;

			m_bInVision = pPlayerTakeoverTarget->m_bInVision;
			m_nVisionLastTick = pPlayerTakeoverTarget->m_nVisionLastTick;


			// Transfer weapons from the takeover target.
			RemoveAllItems(false);
			CBaseCombatWeapon* pTakeoverTargetActiveWeapon = pPlayerTakeoverTarget->GetActiveWeapon();
			for (int i = 0; i < MAX_WEAPONS; ++i)
			{
				CBaseCombatWeapon* pTakeoverTargetWeapon = pPlayerTakeoverTarget->GetWeapon(i);
				if (pTakeoverTargetWeapon)
				{
					pPlayerTakeoverTarget->Weapon_Detach(pTakeoverTargetWeapon);
					Weapon_Equip(pTakeoverTargetWeapon);
				}
			}

			// After giving weapons, ensure the correct active weapon is set.
			if (pTakeoverTargetActiveWeapon)
			{
				CBaseCombatWeapon* pPlayerActiveWeapon = Weapon_OwnsThisType(pTakeoverTargetActiveWeapon->GetClassname());
				if (pPlayerActiveWeapon)
				{
					Weapon_Switch(pPlayerActiveWeapon);
				}
			}

			// Teleport the takeover target's location and velocity.
			SetAbsOrigin(pPlayerTakeoverTarget->GetAbsOrigin());
			SetAbsAngles(pPlayerTakeoverTarget->GetAbsAngles());
			SetAbsVelocity(pPlayerTakeoverTarget->GetAbsVelocity());
			SetLocalAngles(pPlayerTakeoverTarget->GetLocalAngles());
			SetViewOffset(VEC_VIEW_NEOSCALE(this));
			SnapEyeAngles( pPlayerTakeoverTarget->EyeAngles() ); 
			SetPlayerTeamModel();
			InitSprinting();

			// Put the takeover target in stasis.
			pPlayerTakeoverTarget->SpectatorTakeoverPlayerInitiate(this);
		}

		m_bSpectatorTakeoverPlayerPending = false;
	}
}

void CNEO_Player::RestorePlayerFromSpectatorTakeover()
{
	if (m_hSpectatorTakeoverPlayerTarget.Get())
	{
		CNEO_Player* pPlayerTakenOver = m_hSpectatorTakeoverPlayerTarget.Get();
		m_hSpectatorTakeoverPlayerTarget->SpectatorTakeoverPlayerRevert(this);
		m_hSpectatorTakeoverPlayerTarget = nullptr;

		// If the replaced player was a human, they were AFK so kick them to spectator
		if (pPlayerTakenOver && !pPlayerTakenOver->IsBot())
		{
			pPlayerTakenOver->ChangeTeam(TEAM_SPECTATOR);
		}
	}
}

void CNEO_Player::SpectatorTakeoverPlayerInitiate(CNEO_Player* pPlayer)
{
    m_hSpectatorTakeoverPlayerImpersonatingMe = pPlayer;
    pPlayer->m_hSpectatorTakeoverPlayerTarget = this;

    // Strip all weapons and items so replaced player is truly disarmed.
    RemoveAllItems(true);

    // Silently remove player from the game.
    SetSolid(SOLID_NONE);
    AddEffects(EF_NODRAW);
    m_takedamage = DAMAGE_NO;
    m_lifeState = LIFE_DEAD;
    AddFlag(FL_NOTARGET);
}

void CNEO_Player::SpectatorTakeoverPlayerRevert(CNEO_Player* pPlayer)
{
    // Clear takeover handles
    m_hSpectatorTakeoverPlayerImpersonatingMe = nullptr;
    if (pPlayer)
    {
        pPlayer->m_hSpectatorTakeoverPlayerTarget = nullptr;
    }
}

