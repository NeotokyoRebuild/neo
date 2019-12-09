#include "cbase.h"
#include "neo_player.h"


#include "neo_predicted_viewmodel.h"
#include "in_buttons.h"
#include "neo_gamerules.h"
#include "team.h"

#include "engine/IEngineSound.h"
#include "SoundEmitterSystem/isoundemittersystembase.h"

#include "ilagcompensationmanager.h"
#include "datacache/imdlcache.h"

#include "neo_model_manager.h"
#include "neo_player_shared.h"

#include "weapon_ghost.h"

#include "shareddefs.h"
#include "inetchannelinfo.h"
#include "eiface.h"

#include "neo_player_shared.h"

#include "sequence_Transitioner.h"

#include "neo_te_tocflash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void DropPrimedFragGrenade(CNEO_Player *pPlayer, CBaseCombatWeapon *pGrenade);

LINK_ENTITY_TO_CLASS(player, CNEO_Player);

IMPLEMENT_SERVERCLASS_ST(CNEO_Player, DT_NEO_Player)
SendPropInt(SENDINFO(m_iNeoClass)),
SendPropInt(SENDINFO(m_iNeoSkin)),
SendPropInt(SENDINFO(m_iXP)),
SendPropInt(SENDINFO(m_iCapTeam), 3),
SendPropInt(SENDINFO(m_iGhosterTeam)),
SendPropInt(SENDINFO(m_iLoadoutWepChoice)),

SendPropBool(SENDINFO(m_bGhostExists)),
SendPropBool(SENDINFO(m_bInThermOpticCamo)),
SendPropBool(SENDINFO(m_bIsAirborne)),
SendPropBool(SENDINFO(m_bHasBeenAirborneForTooLongToSuperJump)),
SendPropBool(SENDINFO(m_bShowTestMessage)),

SendPropString(SENDINFO(m_pszTestMessage)),

SendPropVector(SENDINFO(m_vecGhostMarkerPos), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT),

SendPropArray(SendPropVector(SENDINFO_ARRAY(m_rvFriendlyPlayerPositions), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rvFriendlyPlayerPositions),
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Player)
DEFINE_FIELD(m_iNeoClass, FIELD_INTEGER),
DEFINE_FIELD(m_iNeoSkin, FIELD_INTEGER),
DEFINE_FIELD(m_iXP, FIELD_INTEGER),
DEFINE_FIELD(m_iCapTeam, FIELD_INTEGER),
DEFINE_FIELD(m_iGhosterTeam, FIELD_INTEGER),
DEFINE_FIELD(m_iLoadoutWepChoice, FIELD_INTEGER),

DEFINE_FIELD(m_bGhostExists, FIELD_BOOLEAN),
DEFINE_FIELD(m_bInThermOpticCamo, FIELD_BOOLEAN),
DEFINE_FIELD(m_bIsAirborne, FIELD_BOOLEAN),
DEFINE_FIELD(m_bHasBeenAirborneForTooLongToSuperJump, FIELD_BOOLEAN),
DEFINE_FIELD(m_bShowTestMessage, FIELD_BOOLEAN),

DEFINE_FIELD(m_pszTestMessage, FIELD_STRING),

DEFINE_FIELD(m_vecGhostMarkerPos, FIELD_VECTOR),

DEFINE_FIELD(m_rvFriendlyPlayerPositions, FIELD_CUSTOM),
END_DATADESC()

CBaseEntity *g_pLastJinraiSpawn, *g_pLastNSFSpawn;
extern CBaseEntity *g_pLastSpawn;

void CNEO_Player::RequestSetClass(int newClass)
{
	bool canChangeImmediately = true; // TODO

	if (canChangeImmediately)
	{
		m_iNeoClass = newClass;

		SetPlayerTeamModel();

		InitSprinting();
	}
	else
	{
		// TODO set for next spawn
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

static inline bool IsNeoPrimary(CNEOBaseCombatWeapon *pNeoWep)
{
	if (!pNeoWep)
	{
		return false;
	}

	const int bits = pNeoWep->GetNeoWepBits();
	const int primaryBits = NEO_WEP_AA13 | NEO_WEP_GHOST | NEO_WEP_JITTE | NEO_WEP_JITTE_S |
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

	if (m_iXP < pNeoWeapon->GetNeoWepXPCost(GetClass()))
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

// NEO FIXME/HACK (Rain): bots don't properly set their fakeclient flag currently,
// making IsFakeClient and IsBot return false. This is an ugly hack to get bots
// joining teams. We cannot trust this player input (and it's slow), so it really
// should be fixed properly.
inline bool Hack_IsBot(CNEO_Player *player)
{
	if (!player)
	{
		return false;
	}

	const char *name = player->GetPlayerInfo()->GetName();

	return (strlen(name) == 5 && name[0] == 'B' && name[1] == 'o' && name[2] == 't');
}

static inline int GetNumOtherPlayersConnected(CNEO_Player *asker)
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

	m_bInLeanLeft = false;
	m_bInLeanRight = false;
	m_bGhostExists = false;
	m_bInThermOpticCamo = false;
	m_bIsAirborne = false;
	m_bHasBeenAirborneForTooLongToSuperJump = false;

	m_leanPosTargetOffset = vec3_origin;

	m_iCapTeam = TEAM_UNASSIGNED;
	m_iGhosterTeam = TEAM_UNASSIGNED;
	m_iLoadoutWepChoice = 0;

	m_bShowTestMessage = false;
	V_memset(m_pszTestMessage.GetForModify(), 0, sizeof(m_pszTestMessage));

	m_vecGhostMarkerPos = vec3_origin;

	ZeroFriendlyPlayerLocArray();
}

CNEO_Player::~CNEO_Player( void )
{
}

int CNEO_Player::GetSkin() const
{
	return m_iNeoSkin;
}

int CNEO_Player::GetClass() const
{
	return m_iNeoClass;
}

inline void CNEO_Player::ZeroFriendlyPlayerLocArray(void)
{
	for (int i = 0; i < m_rvFriendlyPlayerPositions.Count(); i++)
	{
		m_rvFriendlyPlayerPositions.Set(i, Vector(0, 0, 0));
	}
	NetworkStateChanged();
}

void CNEO_Player::UpdateNetworkedFriendlyLocations()
{
	const int pvsMaxSize = (engine->GetClusterCount() / 8) + 1;
	Assert(pvsMaxSize > 0);

	// NEO HACK/FIXME (Rain): we should stack allocate instead
	unsigned char *pvs = new unsigned char[pvsMaxSize];

	const int cluster = engine->GetClusterForOrigin(GetAbsOrigin());
	const int pvsSize = engine->GetPVSForCluster(cluster, pvsMaxSize, pvs);
	Assert(pvsSize > 0);

	for (int i = 1; i <= gpGlobals->maxClients; i++)
	{
		CNEO_Player *otherPlayer = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(i));

		vec_t zeroPos[3] = { 0, 0, 0 };

		// Look for valid players that aren't us
		if (!otherPlayer || otherPlayer == this)
		{
			m_rvFriendlyPlayerPositions.Set(i, vec3_origin);
			m_rvFriendlyPlayerPositions.GetForModify(i).CopyToArray(zeroPos);
			continue;
		}

		// Only players in our team, and are alive
		else if (otherPlayer->GetTeamNumber() != GetTeamNumber() || otherPlayer->IsDead())
		{
			m_rvFriendlyPlayerPositions.Set(i, vec3_origin);
			m_rvFriendlyPlayerPositions.GetForModify(i).CopyToArray(zeroPos);
			continue;
		}

		// If the other player is already in our PVS, we can skip them.
		else if (engine->CheckOriginInPVS(otherPlayer->GetAbsOrigin(), pvs, pvsSize))
		{
#if(0) // currently networking all friendlies, NEO TODO (Rain): optimise this
			m_rvFriendlyPlayerPositions.Set(i, vec3_origin);
			m_rvFriendlyPlayerPositions.GetForModify(i).CopyToArray(zeroPos);
			continue;
#endif
		}

		vec_t absPos[3] = { otherPlayer->GetAbsOrigin().x, otherPlayer->GetAbsOrigin().y, otherPlayer->GetAbsOrigin().z };

		m_rvFriendlyPlayerPositions.Set(i, otherPlayer->GetAbsOrigin());
		m_rvFriendlyPlayerPositions.GetForModify(i).CopyToArray(absPos);
	}

	delete[] pvs;
}

void CNEO_Player::Precache( void )
{
	BaseClass::Precache();
}

void CNEO_Player::Spawn(void)
{
	BaseClass::Spawn();

	ShowCrosshair(false);

	m_leanPosTargetOffset = VEC_VIEW;

	SetTransmitState(FL_EDICT_ALWAYS);

	m_bIsAirborne = (!(GetFlags() & FL_ONGROUND));

	GiveLoadoutWeapon();
}

bool CNEO_Player::IsAirborne(void) const
{
	return m_bIsAirborne;
}

extern ConVar neo_lean_angle;
ConVar neo_lean_thirdperson_roll_lerp_scale("neo_lean_thirdperson_roll_lerp_scale", "5",
	FCVAR_REPLICATED | FCVAR_CHEAT, "Multiplier for 3rd person lean roll lerping.", true, 0.0, false, 0);

void CNEO_Player::DoThirdPersonLean(void)
{
	CNEOPredictedViewModel *vm = static_cast<CNEOPredictedViewModel*>(GetViewModel());

	if (!vm)
	{
		return;
	}

	int leanDir = vm->CalcLean(this);

	CBaseAnimating *anim = GetBaseAnimating();
	Assert(anim);
	//int pelvisBone = anim->LookupBone("ValveBiped.Bip01_Pelvis");
	//Assert(pelvisBone != -1);

	const float startRot = GetBoneController(0);

	const float ultimateRot =
		(leanDir & IN_LEAN_LEFT) ? -neo_lean_angle.GetFloat() :
		(leanDir & IN_LEAN_RIGHT) ? neo_lean_angle.GetFloat() :
		0;

	float lerpedRot;

	const float leniency = 0.5f;
	// We're close enough to target, snap to it so we don't drift when lerping towards zero.
	if (fabs(startRot - ultimateRot) < leniency)
	{
		lerpedRot = ultimateRot;
	}
	else
	{
		lerpedRot = Lerp(gpGlobals->frametime * neo_lean_thirdperson_roll_lerp_scale.GetFloat(), startRot, ultimateRot);
	}

#if(0)
	static float lastRot = 0;
	if (lastRot != lerpedRot)
	{
		DevMsg("New lean target; leaning %f --> %f\n", startRot, lerpedRot);
		lastRot = lerpedRot;
	}
#endif

	anim->SetBoneController(0, lerpedRot);
}

void CNEO_Player::PreThink(void)
{
	BaseClass::PreThink();

	DoThirdPersonLean();

	// NEO HACK (Rain): Just bodging together a check for if we're allowed
	// to superjump, or if we've been airborne too long for that.
	// Ideally this should get cleaned up and moved to wherever
	// the regular engine jump does a similar check.
	bool newNetAirborneVal;
	if (IsAirborne())
	{
		static float lastAirborneJumpOkTime = gpGlobals->curtime;
		const float deltaTime = gpGlobals->curtime - lastAirborneJumpOkTime;
		const float leeway = 0.5f;
		if (deltaTime > leeway)
		{
			newNetAirborneVal = false;
			lastAirborneJumpOkTime = gpGlobals->curtime;
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
	static CWeaponGhost* ghost = dynamic_cast<CWeaponGhost*>(UTIL_EntityByIndex(ghostEdict));
	if (!ghost)
	{
		ghost = dynamic_cast<CWeaponGhost*>(UTIL_EntityByIndex(ghostEdict));
		if (!ghost)
		{
			auto entIter = gEntList.FirstEnt();
			while (entIter)
			{
				ghost = dynamic_cast<CWeaponGhost*>(entIter);

				if (ghost)
				{
					ghostEdict = ghost->entindex();
					break;
				}

				entIter = gEntList.NextEnt(entIter);
			}
		}
	}

	m_bGhostExists = (ghost != NULL);

	if (m_bGhostExists)
	{
		if (ghost)
		{
			m_vecGhostMarkerPos = ghost->GetAbsOrigin();
			auto owner = ghost->GetPlayerOwner();
			if (owner)
			{
				m_iGhosterTeam = owner->GetTeamNumber();
			}
			else
			{
				m_iGhosterTeam = TEAM_UNASSIGNED;
			}
		}
		else
		{
			Assert(false);
			m_bGhostExists = false;
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

	CheckThermOpticButtons();
}

ConVar sv_neo_cloak_color_r("sv_neo_cloak_color_r", "1", FCVAR_CHEAT, "Thermoptic cloak flash color (red channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_g("sv_neo_cloak_color_g", "2", FCVAR_CHEAT, "Thermoptic cloak flash color (green channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_b("sv_neo_cloak_color_b", "4", FCVAR_CHEAT, "Thermoptic cloak flash color (blue channel).", true, 0.0f, true, 255.0f);
ConVar sv_neo_cloak_color_radius("sv_neo_cloak_color_radius", "64", FCVAR_CHEAT, "Thermoptic cloak flash effect radius.", true, 0.0f, true, 4096.0f);
ConVar sv_neo_cloak_time("sv_neo_cloak_time", "0.1", FCVAR_CHEAT, "How long should the thermoptic flash be visible, in seconds.", true, 0.0f, true, 1.0f);
ConVar sv_neo_cloak_decay("sv_neo_cloak_decay", "0", FCVAR_CHEAT, "After the cloak time, how quickly should the flash effect disappear.", true, 0.0f, true, 1.0f);
ConVar sv_neo_cloak_exponent("sv_neo_cloak_exponent", "4", FCVAR_CHEAT, "Cloak flash lighting exponent.", true, 0.0f, false, 0.0f);

inline void CNEO_Player::CheckThermOpticButtons()
{
	if (m_afButtonPressed & IN_THERMOPTIC)
	{
		if (IsAlive())
		{
			m_bInThermOpticCamo = !m_bInThermOpticCamo;

			CRecipientFilter filter;
			filter.AddRecipientsByPVS(GetAbsOrigin());

			if (m_bInThermOpticCamo)
			{
				// Do cloak flash
				g_NEO_TE_TocFlash.r = sv_neo_cloak_color_r.GetInt();
				g_NEO_TE_TocFlash.g = sv_neo_cloak_color_g.GetInt();
				g_NEO_TE_TocFlash.b = sv_neo_cloak_color_b.GetInt();
				g_NEO_TE_TocFlash.m_vecOrigin = GetAbsOrigin();
				g_NEO_TE_TocFlash.exponent = sv_neo_cloak_exponent.GetInt();
				g_NEO_TE_TocFlash.m_fRadius = sv_neo_cloak_color_radius.GetFloat();
				g_NEO_TE_TocFlash.m_fTime = sv_neo_cloak_time.GetFloat();
				g_NEO_TE_TocFlash.m_fDecay = sv_neo_cloak_decay.GetFloat();

				g_NEO_TE_TocFlash.Create(filter);
			}

			// Play cloak sound
			static int tocOn = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOn");
			static int tocOff = CBaseEntity::PrecacheScriptSound("NeoPlayer.ThermOpticOff");

			EmitSound_t tocSoundParams;
			tocSoundParams.m_bEmitCloseCaption = false;
			tocSoundParams.m_hSoundScriptHandle = (m_bInThermOpticCamo ? tocOn : tocOff);
			tocSoundParams.m_pOrigin = &GetAbsOrigin();

			EmitSound(filter, edict()->m_EdictIndex, tocSoundParams);
		}
	}
}

inline void CNEO_Player::SuperJump(void)
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
		const float thisTime = gpGlobals->curtime;
		static float lastSuperJumpTime = thisTime;
		const float deltaTime = thisTime - lastSuperJumpTime;
		if (deltaTime > SUPER_JMP_DELAY_BETWEEN_JUMPS)
			return false;

		lastSuperJumpTime = thisTime;
	}

	return true;
}

void CNEO_Player::PostThink(void)
{
	BaseClass::PostThink();

	CBaseCombatWeapon *pWep = GetActiveWeapon();

	if (pWep)
	{
		static bool previouslyReloading = false;

		if (pWep->m_bInReload)
		{
			if (!previouslyReloading)
			{
				Weapon_SetZoom(false);
			}
		}
		else
		{
			if (m_afButtonReleased & IN_AIM)
			{
				Weapon_AimToggle(pWep);
			}
		}

		previouslyReloading = pWep->m_bInReload;
	}

	if (m_afButtonPressed & IN_DROP)
	{
		Vector eyeForward;
		EyeVectors(&eyeForward);
		const float forwardOffset = 250.0f;
		eyeForward *= forwardOffset;

		Weapon_Drop(GetActiveWeapon(), NULL, &eyeForward);
	}

#if(0)
	if (m_iCapTeam != TEAM_UNASSIGNED)
	{
		const int resetTime = 11;
		static float lastTime = gpGlobals->curtime;
		float dTime = gpGlobals->curtime - lastTime;
		if (dTime >= resetTime)
		{
			m_iCapTeam = TEAM_UNASSIGNED;
			lastTime = gpGlobals->curtime;

			NEORules()->RestartGame();

			return;
		}
	}
#endif

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
}

void CNEO_Player::PlayerDeathThink()
{
	BaseClass::PlayerDeathThink();
}

void CNEO_Player::Weapon_AimToggle( CBaseCombatWeapon *pWep )
{
	// NEO TODO/HACK: Not all neo weapons currently inherit
	// through a base neo class, so we can't static_cast!!
	auto neoCombatWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
	if (!neoCombatWep)
	{
		return;
	}
	else if (!IsAllowedToZoom(neoCombatWep))
	{
		return;
	}

	bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
	Weapon_SetZoom(showCrosshair);
}

inline void CNEO_Player::Weapon_SetZoom(bool bZoomIn)
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
}

void UpdateLayerSequenceGeneric(CNEO_Player *pPlayer, CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd)
{
	if (!bEnabled)
		return;

	// Increment the fire sequence's cycle.
	flCurCycle += pPlayer->GetSequenceCycleRate(pStudioHdr, iSequence) * gpGlobals->frametime;
	if (flCurCycle > 1)
	{
		if (bWaitAtEnd)
		{
			flCurCycle = 1;
		}
		else
		{
			// Not firing anymore.
			bEnabled = false;
			iSequence = 0;
			return;
		}
	}

	// Now dump the state into its animation layer.
	CAnimationLayer *pLayer = pPlayer->GetAnimOverlay(iLayer);

	pLayer->m_flCycle = flCurCycle;
	pLayer->m_nSequence = iSequence;

	pLayer->m_flPlaybackRate = 1.0;
	pLayer->m_flWeight = 1.0f;
	pLayer->m_nOrder = iLayer;
}

// NEO FIXME (Rain): need to implement neo animstate for smoothed gestures!
// See CSS code for basic idea.
void CNEO_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
#if(0) // These should live in animstate
#define AIMSEQUENCE_LAYER		1	// Aim sequence uses layers 0 and 1 for the weapon idle animation (needs 2 layers so it can blend).
#define NUM_AIMSEQUENCE_LAYERS	4	// Then it uses layers 2 and 3 to blend in the weapon run/walk/crouchwalk animation.

#define FIRESEQUENCE_LAYER		(AIMSEQUENCE_LAYER+NUM_AIMSEQUENCE_LAYERS)
#define RELOADSEQUENCE_LAYER	(FIRESEQUENCE_LAYER + 1)
#define GRENADESEQUENCE_LAYER	(RELOADSEQUENCE_LAYER + 1)
#define NUM_LAYERS_WANTED		(GRENADESEQUENCE_LAYER + 1)

	auto pAnimOverlay_Fire = GetAnimOverlay(FIRESEQUENCE_LAYER);
	auto pAnimOverlay_Reload = GetAnimOverlay(RELOADSEQUENCE_LAYER);
	Assert(pAnimOverlay_Fire);
	Assert(pAnimOverlay_Reload);
#endif

	float speed;

	if (GetFlags() & (FL_FROZEN | FL_ATCONTROLS))
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}
	else
	{
		speed = GetAbsVelocity().Length2D();
	}

	Activity idealActivity = ACT_NEO_MOVE_RUN;

	const bool bStartedReloading = (playerAnim == PLAYER_RELOAD);

	// This could stand to be redone. Why is playerAnim abstracted from activity? (sjb)
	if (playerAnim == PLAYER_DIE)
	{
		if (m_lifeState == LIFE_ALIVE)
		{
			return;
		}
	}
	else if (playerAnim == PLAYER_ATTACK1)
	{
		if (GetActivity() == ACT_NEO_HOVER ||
			GetActivity() == ACT_NEO_SWIM ||
			GetActivity() == ACT_NEO_DIE)
		{
			idealActivity = GetActivity();
		}
		else
		{
			idealActivity = ACT_NEO_ATTACK;
		}
	}
	else if (playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK || bStartedReloading)
	{
		if (GetFlags() & FL_DUCKING)
		{
			if (speed > 0)
			{
				idealActivity = ACT_NEO_MOVE_CROUCH;
			}
			else
			{
				idealActivity = ACT_NEO_IDLE_CROUCH;
			}
		}
		else
		{
			if (speed > 0)
			{
				if (speed > GetWalkSpeed())
				{
					idealActivity = ACT_NEO_MOVE_RUN;
				}
				else
				{
					idealActivity = ACT_NEO_MOVE_WALK;
				}
			}
			else
			{
				idealActivity = ACT_NEO_IDLE_STAND;
			}
		}
	}

	if (!(GetFlags() & FL_ONGROUND))	// Still jumping
	{
		idealActivity = ACT_NEO_JUMP;
	}
	else if (m_afButtonPressed & IN_JUMP) // Started jumping now
	{
		idealActivity = ACT_NEO_JUMP;
	}

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

	SetActivity(idealActivity);

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
	}

	return isAllowedToJoin;
}

bool CNEO_Player::ClientCommand( const CCommand &args )
{
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
	BaseClass::Event_Killed(info);
}

int CNEO_Player::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	return BaseClass::OnTakeDamage(inputInfo);
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

bool CNEO_Player::BumpWeapon( CBaseCombatWeapon *pWeapon )
{
	int bumpedSlot = pWeapon->GetSlot();

	CBaseCombatWeapon *currentSlotWep = Weapon_GetSlot(bumpedSlot);

	// We already have a weapon in this slot
	if (currentSlotWep)
	{
		return false;
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

inline bool CNEO_Player::IsCarryingGhost(void)
{
#ifdef DEBUG
	auto baseWep = GetWeapon(NEO_WEAPON_PRIMARY_SLOT);
	if (!baseWep)
	{
		return false;
	}

	auto wep = dynamic_cast<CNEOBaseCombatWeapon*>(baseWep);
	if (!wep)
	{
		//Assert(false); // FIXME
	}
#else
	//auto wep = static_cast<CNEOBaseCombatWeapon*>(GetWeapon(NEO_WEAPON_PRIMARY_SLOT));
	auto wep = dynamic_cast<CNEOBaseCombatWeapon*>(GetWeapon(NEO_WEAPON_PRIMARY_SLOT));
#endif
	return (wep && wep->IsGhost());
}

// Is the player allowed to drop a weapon of this type?
inline bool CNEO_Player::IsAllowedToDrop(CBaseCombatWeapon *pWep)
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
		// This was not a Neotokyo weapon. Don't know what it is, but allow dropping.
		return true;
	}
#else
	Assert(dynamic_cast<CNEOBaseCombatWeapon*>(pWep));
	auto pNeoWep = static_cast<CNEOBaseCombatWeapon*>(pWep);
#endif

	const int wepBits = pNeoWep->GetNeoWepBits();

	Assert(wepBits > NEO_WEP_INVALID);

	const int unallowedDrops = (NEO_WEP_DETPACK | NEO_WEP_FRAG_GRENADE |
		NEO_WEP_KNIFE | NEO_WEP_PROX_MINE | NEO_WEP_SMOKE_GRENADE);

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
	const bool suicide = true;
	const float teamChangeInterval = 5.0f;

	const bool justJoined = (GetTeamNumber() == TEAM_UNASSIGNED);

	// Player bots should initially join a player team
	if (justJoined && Hack_IsBot(this) && !IsHLTV())
	{
		iTeam = RandomInt(TEAM_JINRAI, TEAM_NSF);
	}
	// Limit team join spam, unless this is a newly joined player
	else if (!justJoined && m_flNextTeamChangeTime > gpGlobals->curtime)
	{
		// Except for spectators, who are allowed to join a team as soon as they wish
		if (GetTeamNumber() != TEAM_SPECTATOR)
		{
			ClientPrint(this, HUD_PRINTTALK, "Please wait before switching teams again.");
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

		if (suicide)
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
		if (suicide)
		{
			if (!justJoined && GetTeamNumber() != TEAM_SPECTATOR && !IsDead())
			{
				SoftSuicide();
			}
		}

		StopObserverMode();

		State_Transition(STATE_ACTIVE);
	}
	else
	{
		// Client should not be able to reach this
		Assert(false);

		ClientPrint(this, HUD_PRINTTALK, "Team switch failed, unrecognized Neotokyo team specified.");

		return false;
	}

	m_flNextTeamChangeTime = gpGlobals->curtime + teamChangeInterval;
	
	RemoveAllItems(true);
	ShowCrosshair(false);

	if (iTeam == TEAM_JINRAI || iTeam == TEAM_NSF)
	{
		SetPlayerTeamModel();
		GiveAllItems();
	}

	// We're skipping over HL2MP player because we don't care about
	// deathmatch rules or Combine/Rebels model stuff.
	CHL2_Player::ChangeTeam(iTeam, false, justJoined);

	return true;
}

void CNEO_Player::GiveDefaultItems(void)
{
	//EquipSuit();

	CBasePlayer::GiveAmmo(150, "Pistol");
	CBasePlayer::GiveAmmo(150, "AR2");
	CBasePlayer::GiveAmmo(30, "AMMO_10G_SHELL");

	const bool supportsGetKnife = true;

	switch (GetClass())
	{
	case NEO_CLASS_RECON:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_milso");
		Weapon_Switch(Weapon_OwnsThisType("weapon_milso"));
		break;
	case NEO_CLASS_ASSAULT:
		GiveNamedItem("weapon_knife");
		GiveNamedItem("weapon_tachi");
		Weapon_Switch(Weapon_OwnsThisType("weapon_tachi"));
		break;
	case NEO_CLASS_SUPPORT:
		if (supportsGetKnife) { GiveNamedItem("weapon_knife"); }
		GiveNamedItem("weapon_kyla");
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
		if (m_iXP >= pNeoWeapon->GetNeoWepXPCost(GetClass()))
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
				UTIL_Remove((CBaseEntity*)pEnt);
			}
		}
	}
}

// NEO TODO
void CNEO_Player::GiveAllItems(void)
{
	// NEO TODO (Rain): our own ammo types
	CBasePlayer::GiveAmmo(255, "Pistol");
	CBasePlayer::GiveAmmo(45, "SMG1");
	CBasePlayer::GiveAmmo(1, "grenade");
	CBasePlayer::GiveAmmo(6, "Buckshot");
	CBasePlayer::GiveAmmo(6, "357");

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

float CNEO_Player::GetCrouchSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_CROUCH_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_CROUCH_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_CROUCH_SPEED;
	}

	return NEO_BASE_CROUCH_SPEED;
}

float CNEO_Player::GetNormSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_NORM_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_NORM_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_NORM_SPEED;
	}

	return NEO_BASE_NORM_SPEED;
}

float CNEO_Player::GetWalkSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_WALK_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_WALK_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_WALK_SPEED;
	default:
		return NEO_BASE_WALK_SPEED;
	}
}

float CNEO_Player::GetSprintSpeed() const
{
	switch (m_iNeoClass)
	{
	case NEO_CLASS_RECON:
		return NEO_RECON_SPRINT_SPEED;
	case NEO_CLASS_ASSAULT:
		return NEO_ASSAULT_SPRINT_SPEED;
	case NEO_CLASS_SUPPORT:
		return NEO_SUPPORT_SPRINT_SPEED;
	default:
		return NEO_BASE_SPRINT_SPEED;
	}
}