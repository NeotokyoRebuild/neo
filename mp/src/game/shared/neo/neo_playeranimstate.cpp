#include "cbase.h"
#include "base_playeranimstate.h"

#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"

#include "neo_playeranimstate.h"

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "bone_setup.h"
#include "interpolatedvar.h"
#else
#include "neo_player.h"
#endif

#define DEFAULT_IDLE_NAME "Idle_Upper_"
#define DEFAULT_CROUCH_IDLE_NAME "Crouch_Idle_Upper_"
#define DEFAULT_CROUCH_WALK_NAME "Crouch_Walk_Upper_"
#define DEFAULT_WALK_NAME "Walk_Upper_"
#define DEFAULT_RUN_NAME "Run_Upper_"

#define DEFAULT_FIRE_IDLE_NAME "Idle_Shoot_"
#define DEFAULT_FIRE_CROUCH_NAME "Crouch_Idle_Shoot_"
#define DEFAULT_FIRE_CROUCH_WALK_NAME "Crouch_Walk_Shoot_"
#define DEFAULT_FIRE_WALK_NAME "Walk_Shoot_"
#define DEFAULT_FIRE_RUN_NAME "Run_Shoot_"

// ------------------------------------------------------------------------- //
// CSDKPlayerAnimState declaration.
// ------------------------------------------------------------------------- //
class CNEOPlayerAnimState : public CBasePlayerAnimState, public INEOPlayerAnimState
{
public:
	DECLARE_CLASS(CNEOPlayerAnimState, CBasePlayerAnimState);
	friend INEOPlayerAnimState *CreatePlayerAnimState(CBaseAnimatingOverlay *pEntity,
		INEOPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences);

	CNEOPlayerAnimState();
	~CNEOPlayerAnimState();

	virtual void DoAnimationEvent(PlayerAnimEvent_t event, int nData);
	virtual bool IsThrowingGrenade();
	virtual int CalcAimLayerSequence(float *flCycle, float *flAimSequenceWeight, bool bForceIdle);
	virtual void ClearAnimationState();
	virtual bool CanThePlayerMove();
	virtual float GetCurrentMaxGroundSpeed();
	virtual Activity CalcMainActivity();
	virtual void DebugShowAnimState(int iStartLine);
	virtual void ComputeSequences(CStudioHdr *pStudioHdr);
	virtual void ClearAnimationLayers();

	void InitNEO(CBaseAnimatingOverlay *pPlayer, INEOPlayerAnimStateHelpers *pHelpers,
		LegAnimType_t legAnimType, bool bUseAimSequences);

protected:
	int CalcFireLayerSequence(PlayerAnimEvent_t event);
	void ComputeFireSequence(CStudioHdr *pStudioHdr);

	void ComputeReloadSequence(CStudioHdr *pStudioHdr);
	int CalcReloadLayerSequence();

	bool IsOuterGrenadePrimed();
	void ComputeGrenadeSequence(CStudioHdr *pStudioHdr);
	int CalcGrenadePrimeSequence();
	int CalcGrenadeThrowSequence();
	int GetOuterGrenadeThrowCounter();

	const char* GetWeaponSuffix();
	bool HandleJumping();

	void UpdateLayerSequenceGeneric(CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled,
		float &flCurCycle, int &iSequence, bool bWaitAtEnd);

private:
	// Current state variables.
	bool m_bJumping;			// Set on a jump event.
	float m_flJumpStartTime;
	bool m_bFirstJumpFrame;

	// Aim sequence plays reload while this is on.
	bool m_bReloading;
	float m_flReloadCycle;
	int m_iReloadSequence;

	// This is set to true if ANY animation is being played in the fire layer.
	bool m_bFiring;						// If this is on, then it'll continue the fire animation in the fire layer
	// until it completes.
	int m_iFireSequence;				// (For any sequences in the fire layer, including grenade throw).
	float m_flFireCycle;

	// These control grenade animations.
	bool m_bThrowingGrenade;
	bool m_bPrimingGrenade;
	float m_flGrenadeCycle;
	int m_iGrenadeSequence;
	int m_iLastThrowGrenadeCounter;	// used to detect when the guy threw the grenade.

	INEOPlayerAnimStateHelpers *m_pHelpers;
};

INEOPlayerAnimState *CreatePlayerAnimState(CBaseAnimatingOverlay *pEntity,
	INEOPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences)
{
	CNEOPlayerAnimState *pRet = new CNEOPlayerAnimState;
	pRet->InitNEO(pEntity, pHelpers, legAnimType, bUseAimSequences);
	Assert(pRet);
	return pRet;
}

class CNEOPlayerAnimStateHelpers : public INEOPlayerAnimStateHelpers
{
public:
	CNEOPlayerAnimStateHelpers(CNEO_Player* player) : m_pPlayer(player)
	{
		Assert(m_pPlayer);
	}

	virtual CBaseCombatWeapon* NEOAnim_GetActiveWeapon()
	{
		return m_pPlayer->GetActiveWeapon();
	}

	virtual bool NEOAnim_CanMove()
	{
		if ((m_pPlayer->GetNeoFlags() & FL_FROZEN) || (m_pPlayer->GetFlags() & FL_FROZEN))
		{
			return false;
		}
		return true;
	}

private:
	CNEO_Player* m_pPlayer;
};

INEOPlayerAnimStateHelpers* CreateAnimStateHelpers(CNEO_Player* pPlayer)
{
	CNEOPlayerAnimStateHelpers* pRet = new CNEOPlayerAnimStateHelpers(pPlayer);
	Assert(pRet);
	return pRet;
}

// ------------------------------------------------------------------------- //
// CNEOPlayerAnimState implementation.
// ------------------------------------------------------------------------- //
CNEOPlayerAnimState::CNEOPlayerAnimState()
{
	m_pOuter = NULL;
	m_bReloading = false;
}

CNEOPlayerAnimState::~CNEOPlayerAnimState()
{
	delete m_pHelpers;
}

void CNEOPlayerAnimState::InitNEO(CBaseAnimatingOverlay *pPlayer,
	INEOPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences)
{
	CModAnimConfig config;
	config.m_flMaxBodyYawDegrees = 90;
	config.m_LegAnimType = legAnimType;
	config.m_bUseAimSequences = bUseAimSequences;

	m_pHelpers = pHelpers;

	BaseClass::Init(pPlayer, config);
}

void CNEOPlayerAnimState::ClearAnimationState()
{
	m_bJumping = false;
	m_bFiring = false;
	m_bReloading = false;
	m_bThrowingGrenade = m_bPrimingGrenade = false;
	m_iLastThrowGrenadeCounter = GetOuterGrenadeThrowCounter();

	BaseClass::ClearAnimationState();
}

void CNEOPlayerAnimState::DoAnimationEvent(PlayerAnimEvent_t event, int nData)
{
	Assert(event != PLAYERANIMEVENT_THROW_GRENADE);

	if (event == PLAYERANIMEVENT_FIRE_GUN_PRIMARY ||
		event == PLAYERANIMEVENT_FIRE_GUN_SECONDARY)
	{
		// Regardless of what we're doing in the fire layer, restart it.
		m_flFireCycle = 0;
		m_iFireSequence = CalcFireLayerSequence(event);
		m_bFiring = m_iFireSequence != -1;
	}
	else if (event == PLAYERANIMEVENT_JUMP)
	{
		// Play the jump animation.
		m_bJumping = true;
		m_bFirstJumpFrame = true;
		m_flJumpStartTime = gpGlobals->curtime;
	}
	else if (event == PLAYERANIMEVENT_RELOAD)
	{
		m_iReloadSequence = CalcReloadLayerSequence();
		if (m_iReloadSequence != -1)
		{
			m_bReloading = true;
			m_flReloadCycle = 0;
		}
	}
	else
	{
		Assert(!"CNEOPlayerAnimState::DoAnimationEvent");
	}
}

float g_flThrowGrenadeFraction = 0.25;
bool CNEOPlayerAnimState::IsThrowingGrenade()
{
	if (m_bThrowingGrenade)
	{
		// An animation event would be more appropriate here.
		return m_flGrenadeCycle < g_flThrowGrenadeFraction;
	}
	else
	{
		bool bThrowPending = (m_iLastThrowGrenadeCounter != GetOuterGrenadeThrowCounter());
		return bThrowPending || IsOuterGrenadePrimed();
	}
}

int CNEOPlayerAnimState::CalcReloadLayerSequence()
{
	const char *pSuffix = GetWeaponSuffix();
	if (!pSuffix)
	{
		return -1;
	}

	auto pWeapon = m_pHelpers->NEOAnim_GetActiveWeapon();
	if (!pWeapon)
	{
		return -1;
	}

	// First, look for reload_<weapon name>.
	char szName[512];
	Q_snprintf(szName, sizeof(szName), "reload_%s", pSuffix);
	int iReloadSequence = m_pOuter->LookupSequence(szName);
	if (iReloadSequence != -1)
	{
		return iReloadSequence;
	}
	
	// NEO NOTE (Rain): this is imported from SDK codebase.
	// Figure out if we need to do something here, or can we remove it.
#if(0) // SDK TODO
	// Ok, look for generic categories.. pistol, shotgun, rifle, etc.
	if ( pWeapon->GetSDKWpnData().m_WeaponType == WEAPONTYPE_PISTOL )
	{
	Q_snprintf( szName, sizeof( szName ), "reload_pistol" );
	iReloadSequence = m_pOuter->LookupSequence( szName );
	if ( iReloadSequence != -1 )
	return iReloadSequence;
	}
#endif

	// Fall back to reload_m4.
	// NEO FIXME (Rain): get a sensible NT fallback value (Pistol/ZR68C?)
	iReloadSequence = CalcSequenceIndex("reload_m4");
	if (iReloadSequence > 0)
	{
		return iReloadSequence;
	}
	
	return -1;
}

void CNEOPlayerAnimState::UpdateLayerSequenceGeneric(CStudioHdr *pStudioHdr,
	int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd)
{
#ifdef GAME_DLL
	Assert(false); // currently not expecting to do anything here serverside
#else
	if (!bEnabled)
	{
		return;
	}

	// Increment the fire sequence's cycle.
	flCurCycle += m_pOuter->GetSequenceCycleRate(pStudioHdr, iSequence) * gpGlobals->frametime;
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
	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay(iLayer);

	pLayer->m_flCycle = flCurCycle;
	pLayer->m_nSequence = iSequence;

	pLayer->m_flPlaybackRate = 1.0;
	pLayer->m_flWeight = 1.0f;
	pLayer->m_nOrder = iLayer;
#endif
}

// We don't have a concept of cooking grenades, so checking for the actual moment of tossing it.
bool CNEOPlayerAnimState::IsOuterGrenadePrimed()
{
	auto player = static_cast<CNEO_Player*>(m_pOuter->MyCombatCharacterPointer());
	if (player)
	{
		auto vm = player->GetViewModel();
		if (vm)
		{
			return (vm->GetSequenceActivity(vm->GetSequence()) == ACT_VM_THROW);
		}
	}
	return false;
}

void CNEOPlayerAnimState::ComputeGrenadeSequence(CStudioHdr *pStudioHdr)
{
#ifdef CLIENT_DLL
	if (m_bThrowingGrenade)
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bThrowingGrenade,
			m_flGrenadeCycle, m_iGrenadeSequence, false);
	}
	else
	{
		// Priming the grenade isn't an event.. we just watch the player for it.
		// Also play the prime animation first if he wants to throw the grenade.
		bool bThrowPending = (m_iLastThrowGrenadeCounter != GetOuterGrenadeThrowCounter());
		if (IsOuterGrenadePrimed() || bThrowPending)
		{
			if (!m_bPrimingGrenade)
			{
				// If this guy just popped into our PVS, and he's got his grenade primed, then
				// let's assume that it's all the way primed rather than playing the prime
				// animation from the start.
				if (TimeSinceLastAnimationStateClear() < 0.4f)
					m_flGrenadeCycle = 1;
				else
					m_flGrenadeCycle = 0;

				m_iGrenadeSequence = CalcGrenadePrimeSequence();
			}

			m_bPrimingGrenade = true;
			UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bPrimingGrenade,
				m_flGrenadeCycle, m_iGrenadeSequence, true);

			// If we're waiting to throw and we're done playing the prime animation...
			if (bThrowPending && m_flGrenadeCycle == 1)
			{
				m_iLastThrowGrenadeCounter = GetOuterGrenadeThrowCounter();

				// Now play the throw animation.
				m_iGrenadeSequence = CalcGrenadeThrowSequence();
				if (m_iGrenadeSequence != -1)
				{
					// Configure to start playing 
					m_bThrowingGrenade = true;
					m_bPrimingGrenade = false;
					m_flGrenadeCycle = 0;
				}
			}
		}
		else
		{
			m_bPrimingGrenade = false;
		}
	}
#endif
}

int CNEOPlayerAnimState::CalcGrenadePrimeSequence()
{
	return CalcSequenceIndex("Idle_Shoot_Gren1");
}

int CNEOPlayerAnimState::CalcGrenadeThrowSequence()
{
	// We don't have a prime sequence, so we throw on the prime;
	// don't need to return anything here.
	return 0;
}

int CNEOPlayerAnimState::GetOuterGrenadeThrowCounter()
{
	CNEO_Player *pPlayer = dynamic_cast<CNEO_Player*>(m_pOuter);
	if (!pPlayer)
	{
		return 0;
	}

	return 0;
	//return pPlayer->m_iThrowGrenadeCounter; // NEO FIXME (Rain): unimplemented
}

void CNEOPlayerAnimState::ComputeReloadSequence(CStudioHdr *pStudioHdr)
{
#ifdef CLIENT_DLL
	UpdateLayerSequenceGeneric(pStudioHdr, RELOADSEQUENCE_LAYER, m_bReloading,
		m_flReloadCycle, m_iReloadSequence, false);
#else
	// Server doesn't bother with different fire sequences.
#endif
}

int CNEOPlayerAnimState::CalcAimLayerSequence(float *flCycle,
	float *flAimSequenceWeight, bool bForceIdle)
{
	const char *pSuffix = GetWeaponSuffix();
	if (!pSuffix)
	{
		return 0;
	}
	
	if (bForceIdle)
	{
		switch (GetCurrentMainSequenceActivity())
		{
		case ACT_CROUCHIDLE:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_IDLE_NAME, pSuffix);

		default:
			return CalcSequenceIndex("%s%s", DEFAULT_IDLE_NAME, pSuffix);
		}
	}
	else
	{
		switch (GetCurrentMainSequenceActivity())
		{
		case ACT_RUN:
			return CalcSequenceIndex("%s%s", DEFAULT_RUN_NAME, pSuffix);

		case ACT_WALK:
		case ACT_RUNTOIDLE:
		case ACT_IDLETORUN:
			return CalcSequenceIndex("%s%s", DEFAULT_WALK_NAME, pSuffix);

		case ACT_CROUCHIDLE:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_IDLE_NAME, pSuffix);

		case ACT_RUN_CROUCH:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_WALK_NAME, pSuffix);

		case ACT_IDLE:
		default:
			return CalcSequenceIndex("%s%s", DEFAULT_IDLE_NAME, pSuffix);
		}
	}
}

// NEO TODO/FIXME (Rain): suffix/prefix confusion on naming.
// I *think* this is talking about the same value as the
// WeaponData.anim_prefix in weapon scripts.
//
// If that is not the case, we might have to implement
// GetWpnData.m_szAnimExtension on Neo base wep.
const char *CNEOPlayerAnimState::GetWeaponSuffix()
{
	// Figure out the weapon suffix.
	auto pWeapon = m_pHelpers->NEOAnim_GetActiveWeapon();
	if (!pWeapon)
	{
		return "Pistol";
	}

	const char *pSuffix = pWeapon->GetWpnData().szAnimationPrefix;

	return pSuffix;
}

int CNEOPlayerAnimState::CalcFireLayerSequence(PlayerAnimEvent_t event)
{
	// Figure out the weapon suffix.
	auto pWeapon = m_pHelpers->NEOAnim_GetActiveWeapon();
	if (!pWeapon)
	{
		return 0;
	}
	
	const char *pSuffix = GetWeaponSuffix();
	if (!pSuffix)
	{
		return 0;
	}

	// Don't rely on their weapon here because the player has usually switched to their 
	// pistol or rifle by the time the PLAYERANIMEVENT_THROW_GRENADE message gets to the client.
	if (event == PLAYERANIMEVENT_THROW_GRENADE)
	{
		pSuffix = "Gren";
	}

	int res;

	switch (GetCurrentMainSequenceActivity())
	{
	case ACT_PLAYER_RUN_FIRE:
	case ACT_RUN:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_RUN_NAME, pSuffix);
		break;

	case ACT_PLAYER_WALK_FIRE:
	case ACT_WALK:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_WALK_NAME, pSuffix);
		break;

	case ACT_PLAYER_CROUCH_FIRE:
	case ACT_CROUCHIDLE:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_CROUCH_NAME, pSuffix);
		break;

	case ACT_PLAYER_CROUCH_WALK_FIRE:
	case ACT_RUN_CROUCH:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_CROUCH_WALK_NAME, pSuffix);
		break;

	default:
	case ACT_PLAYER_IDLE_FIRE:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_IDLE_NAME, pSuffix);
		break;
	}

	DevMsg("Res: %d\n", res);

	return res;
}

bool CNEOPlayerAnimState::CanThePlayerMove()
{
	return m_pHelpers->NEOAnim_CanMove();
}

float CNEOPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	const Activity currentActivity = m_pOuter->GetSequenceActivity(m_pOuter->GetSequence());

	Assert(dynamic_cast<CNEO_Player*>(m_pOuter));

	if (currentActivity == ACT_WALK || currentActivity == ACT_IDLE)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetWalkSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_RUN)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_SPRINT)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetSprintSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_RUN_CROUCH)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetCrouchSpeed_WithActiveWepEncumberment();
	}
	else
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed();
	}
}

bool CNEOPlayerAnimState::HandleJumping()
{
	if (m_bJumping)
	{
		if (m_bFirstJumpFrame)
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		if (gpGlobals->curtime - m_flJumpStartTime > 0.2f)
		{
			if (m_pOuter->GetFlags() & FL_ONGROUND)
			{
				m_bJumping = false;
				RestartMainSequence();	// Reset the animation.
			}
		}
	}

	// Are we still jumping? If so, keep playing the jump animation.
	return m_bJumping;
}

Activity CNEOPlayerAnimState::CalcMainActivity()
{
	float flOuterSpeed = GetOuterXYSpeed();

	if (HandleJumping())
	{
		return ACT_HOP;
	}
	else
	{
		Activity idealActivity = ACT_IDLE;

		if (m_pOuter->GetFlags() & FL_DUCKING)
		{
			if (flOuterSpeed > MOVING_MINIMUM_SPEED)
				idealActivity = ACT_RUN_CROUCH;
			else
				idealActivity = ACT_CROUCHIDLE;
		}
		else
		{
			Assert(dynamic_cast<CNEO_Player*>(m_pOuter));

			if (flOuterSpeed > MOVING_MINIMUM_SPEED)
			{
				if (flOuterSpeed > static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed())
				{
					idealActivity = ACT_SPRINT;
				}
				else if (flOuterSpeed > static_cast<CNEO_Player*>(m_pOuter)->GetWalkSpeed())
				{
					idealActivity = ACT_RUN;
				}
				else
				{
					idealActivity = ACT_WALK;
				}
			}
			else
			{
				idealActivity = ACT_IDLE;
			}
		}

		return idealActivity;
	}
}

void CNEOPlayerAnimState::DebugShowAnimState(int iStartLine)
{
#ifdef CLIENT_DLL
	engine->Con_NPrintf(iStartLine++, "fire  : %s, cycle: %.2f\n",
		m_bFiring ? GetSequenceName(m_pOuter->GetModelPtr(), m_iFireSequence) :
		"[not firing]", m_flFireCycle);
	
	engine->Con_NPrintf(iStartLine++, "reload: %s, cycle: %.2f\n",
		m_bReloading ? GetSequenceName(m_pOuter->GetModelPtr(), m_iReloadSequence) :
		"[not reloading]", m_flReloadCycle);
	
	BaseClass::DebugShowAnimState(iStartLine);
#endif
}

void CNEOPlayerAnimState::ComputeSequences(CStudioHdr *pStudioHdr)
{
	BaseClass::ComputeSequences(pStudioHdr);

	ComputeFireSequence(pStudioHdr);
	ComputeReloadSequence(pStudioHdr);
	ComputeGrenadeSequence(pStudioHdr);
}

void CNEOPlayerAnimState::ClearAnimationLayers()
{
	if (!m_pOuter)
	{
		return;
	}

	m_pOuter->SetNumAnimOverlays(NUM_LAYERS_WANTED);
	for (int i = 0; i < m_pOuter->GetNumAnimOverlays(); i++)
	{
		m_pOuter->GetAnimOverlay(i)->SetOrder(CBaseAnimatingOverlay::MAX_OVERLAYS);
	}
}

void CNEOPlayerAnimState::ComputeFireSequence(CStudioHdr *pStudioHdr)
{
#ifdef CLIENT_DLL
	UpdateLayerSequenceGeneric(pStudioHdr, FIRESEQUENCE_LAYER, m_bFiring,
		m_flFireCycle, m_iFireSequence, false);
#else
	// Server doesn't bother with different fire sequences.
#endif
}
