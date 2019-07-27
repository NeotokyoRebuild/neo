#include "cbase.h"
#include "weapon_hl2mpbasehlmpcombatweapon.h"
#include "neo_player.h"
#include "globalstate.h"
#include "game.h"
#include "neo_gamerules.h"
#include "hl2mp_player_shared.h"
#include "neo_predicted_viewmodel.h"
#include "in_buttons.h"
#include "neo_gamerules.h"
#include "KeyValues.h"
#include "team.h"
#include "weapon_hl2mpbase.h"
#include "grenade_satchel.h"
#include "eventqueue.h"
#include "gamestats.h"

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

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void DropPrimedFragGrenade(CNEO_Player *pPlayer, CBaseCombatWeapon *pGrenade);

LINK_ENTITY_TO_CLASS(player, CNEO_Player);

IMPLEMENT_SERVERCLASS_ST(CNEO_Player, DT_NEO_Player)
SendPropInt(SENDINFO(m_iNeoClass)),
SendPropInt(SENDINFO(m_iNeoSkin)),

SendPropInt(SENDINFO(m_iCapTeam), 3),

SendPropBool(SENDINFO(m_bShowTestMessage)),
SendPropString(SENDINFO(m_pszTestMessage)),

SendPropVector(SENDINFO(m_vecGhostMarkerPos)),
SendPropInt(SENDINFO(m_iGhosterTeam)),
SendPropBool(SENDINFO(m_bGhostExists)),
SendPropBool(SENDINFO(m_bInThermOpticCamo)),

SendPropArray(SendPropVector(SENDINFO_ARRAY(m_rvFriendlyPlayerPositions), -1, SPROP_COORD_MP_LOWPRECISION | SPROP_CHANGES_OFTEN, MIN_COORD_FLOAT, MAX_COORD_FLOAT), m_rvFriendlyPlayerPositions),
END_SEND_TABLE()

BEGIN_DATADESC(CNEO_Player)
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

ConCommand setclass("setclass", SetClass, "Set class", FCVAR_USERINFO);
ConCommand setskin("SetVariant", SetSkin, "Set skin", FCVAR_USERINFO);

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

	m_bInLeanLeft = false;
	m_bInLeanRight = false;
	m_bGhostExists = false;
	m_bInThermOpticCamo = false;

	m_leanPosTargetOffset = vec3_origin;

	m_iCapTeam = TEAM_UNASSIGNED;
	m_iGhosterTeam = TEAM_UNASSIGNED;

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
		CNEO_Player *otherPlayer = (CNEO_Player*)UTIL_PlayerByIndex(i);

		vec_t zeroPos[3] = { 0, 0, 0 };

		// Look for valid players that aren't us
		if (!otherPlayer || otherPlayer == this)
		{
			m_rvFriendlyPlayerPositions.Set(i, vec3_origin);
			m_rvFriendlyPlayerPositions.GetForModify(i).CopyToArray(zeroPos);
			continue;
		}

		// Only players in our team
		else if (otherPlayer->GetTeamNumber() != GetTeamNumber())
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
}

extern ConVar neo_lean_angle;
extern ConVar neo_lean_thirdperson_roll_lerp_scale("neo_lean_thirdperson_roll_lerp_scale", "5", FCVAR_REPLICATED | FCVAR_CHEAT, "Multiplier for 3rd person lean roll lerping.", true, 0.0, false, 0);
void CNEO_Player::DoThirdPersonLean(void)
{
	CNEOPredictedViewModel *vm = (CNEOPredictedViewModel*)GetViewModel();

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

	static int ghostEdict = -1;
	auto ent = UTIL_EntityByIndex(ghostEdict);
	bool ghostIsValid = (ent != NULL);
	if (!ghostIsValid)
	{
		auto entIter = gEntList.FirstEnt();
		while (entIter)
		{
			auto ghost = dynamic_cast<CWeaponGhost*>(entIter);

			if (ghost)
			{
				ghostEdict = ghost->entindex();
				ghostIsValid = true;
				break;
			}

			entIter = gEntList.NextEnt(entIter);
		}
	}

	if (ghostIsValid)
	{
		auto ghost = dynamic_cast<CWeaponGhost*>(UTIL_EntityByIndex(ghostEdict));
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
			ghostIsValid = false;
		}
	}

	m_bGhostExists = ghostIsValid;

	if (IsAlive() && GetTeamNumber() != TEAM_SPECTATOR)
	{
		UpdateNetworkedFriendlyLocations();
	}

	CheckThermOpticButtons();
}

inline void CNEO_Player::CheckThermOpticButtons()
{
	if (m_afButtonPressed & IN_THERMOPTIC)
	{
		if (IsAlive())
		{
			m_bInThermOpticCamo = !m_bInThermOpticCamo;
		}
	}
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
}

void CNEO_Player::PlayerDeathThink()
{
	BaseClass::PlayerDeathThink();
}

void CNEO_Player::Weapon_AimToggle( CBaseCombatWeapon *pWep )
{
	if (!IsAllowedToZoom(pWep))
	{
		return;
	}

	bool showCrosshair = (m_Local.m_iHideHUD & HIDEHUD_CROSSHAIR) == HIDEHUD_CROSSHAIR;
	Weapon_SetZoom(showCrosshair);
}

// NEO TODO (Rain): mirror clientside so we can predict
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

void CNEO_Player::SetAnimation( PLAYER_ANIM playerAnim )
{
	int animDesired;

	float speed;

	speed = GetAbsVelocity().Length2D();

	if ( GetFlags() & ( FL_FROZEN | FL_ATCONTROLS ) )
	{
		speed = 0;
		playerAnim = PLAYER_IDLE;
	}

	Activity idealActivity = ACT_IDLE;

	if ( playerAnim == PLAYER_JUMP )
	{
		idealActivity = ACT_HOP;
	}
	else if ( playerAnim == PLAYER_DIE )
	{
		if ( m_lifeState == LIFE_ALIVE )
		{
			return;
		}
	}
	else if ( playerAnim == PLAYER_ATTACK1 )
	{
		if ( GetActivity( ) == ACT_HOVER	|| 
			 GetActivity( ) == ACT_SWIM		||
			 GetActivity( ) == ACT_HOP		||
			 GetActivity( ) == ACT_LEAP		||
			 GetActivity( ) == ACT_DIESIMPLE )
		{
			idealActivity = GetActivity( );
		}
		else
		{
			idealActivity = ACT_RANGE_ATTACK1;
		}
	}
	else if ( playerAnim == PLAYER_RELOAD )
	{
		idealActivity = ACT_RELOAD;
	}
	else if ( playerAnim == PLAYER_IDLE || playerAnim == PLAYER_WALK )
	{
		// Still jumping
		if ( !( GetFlags() & FL_ONGROUND ) && GetActivity( ) == ACT_HOP )
		{
			idealActivity = GetActivity();
		}
		else
		{
			if ( GetFlags() & FL_DUCKING )
			{
				if ( speed > 0 )
				{
					idealActivity = ACT_WALK_CROUCH;
				}
				else
				{
					idealActivity = ACT_CROUCHIDLE;
				}
			}
			else
			{
				if ( speed > 0 )
				{
					{
						idealActivity = ACT_RUN;
					}
				}
				else
				{
					idealActivity = ACT_IDLE;
				}
			}
		}
	}

	SetActivity(idealActivity);

	animDesired = SelectWeightedSequence( Weapon_TranslateActivity ( idealActivity ) );

	if (animDesired == -1)
	{
		animDesired = SelectWeightedSequence( idealActivity );

		if ( animDesired == -1 )
		{
			animDesired = 0;
		}
	}

	// Already using the desired animation?
	if ( GetSequence() == animDesired )
		return;
	
	m_flPlaybackRate = 1.0;
	ResetSequence( animDesired );
	SetCycle( 0 );
}

// Purpose: Suicide, but cancel the point loss.
void CNEO_Player::SoftSuicide(void)
{
	if (IsDead())
	{
		AssertMsg(false, "This should never get called on a dead client");
		return;
	}

	m_fNextSuicideTime = gpGlobals->curtime;

	CommitSuicide();

	IncrementFragCount(1);
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

// Is the player allowed to drop a weapon of this type?
// NEO TODO (Rain): Forbid Neo drops like knife, neo nades etc.
// once they have been implemented.
inline bool CNEO_Player::IsAllowedToDrop(CBaseCombatWeapon *pWep)
{
	if (!pWep)
	{
		return false;
	}

	const char *unallowedDrops[] = {
		"weapon_frag",
	};

	CBaseCombatWeapon *pTest = NULL;
	for (int i = 0; i < ARRAYSIZE(unallowedDrops); i++)
	{
		pTest = Weapon_OwnsThisType(unallowedDrops[i]);
		if (pWep == pTest)
		{
			return false;
		}
	}

	return true;
}

// Is the player allowed to aim zoom with a weapon of this type?
inline bool CNEO_Player::IsAllowedToZoom(CBaseCombatWeapon *pWep)
{
	if (!pWep)
	{
		return false;
	}

	// NEO TODO (Rain): this list will probably eventually become longer
	// than forbidden list; swap logic?
	const char *allowedAimZoom[] = {
		"weapon_aa13",
		"weapon_tachi",
		"weapon_zr68s",
	};

	CBaseCombatWeapon *pTest = NULL;
	for (int i = 0; i < ARRAYSIZE(allowedAimZoom); i++)
	{
		pTest = Weapon_OwnsThisType(allowedAimZoom[i]);
		if (pWep == pTest)
		{
			return true;
		}
	}

	return false;
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

	GiveNamedItem("weapon_tachi");
	GiveNamedItem("weapon_zr68s");

	Weapon_Switch(Weapon_OwnsThisType("weapon_zr68s"));
}

void CNEO_Player::GiveAllItems(void)
{
	// NEO TODO (Rain): our own ammo types
	CBasePlayer::GiveAmmo(255, "Pistol");
	CBasePlayer::GiveAmmo(45, "SMG1");
	CBasePlayer::GiveAmmo(1, "grenade");
	CBasePlayer::GiveAmmo(6, "Buckshot");
	CBasePlayer::GiveAmmo(6, "357");

	GiveNamedItem("weapon_tachi");
	GiveNamedItem("weapon_aa13");
	Weapon_Switch(Weapon_OwnsThisType("weapon_aa13"));

#if(0) // startup weps stuff
	if (m_nCyborgClass == NEO_CLASS_RECON)
	{
		GiveNamedItem("weapon_tachi");
		GiveNamedItem("weapon_ghost");
		Weapon_Switch(Weapon_OwnsThisType("weapon_tachi"));
	}
	else if (m_nCyborgClass == NEO_CLASS_ASSAULT)
	{
		GiveNamedItem("weapon_tachi");
		GiveNamedItem("weapon_ghost");
		Weapon_Switch(Weapon_OwnsThisType("weapon_tachi"));
	}
	else if (m_nCyborgClass == NEO_CLASS_SUPPORT)
	{
		
	}
#endif
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
