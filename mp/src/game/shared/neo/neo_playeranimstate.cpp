#include "cbase.h"
#include "base_playeranimstate.h"
#include "weapon_grenade.h"
#include "weapon_smokegrenade.h"
#include "weapon_detpack.h"

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
	virtual void ClearAnimationFlags();
	virtual void UpdateAnimationFlags();
	virtual void UpdateGrenadeAnimationFlags(CWeaponGrenade* grenadeWeapon);
	virtual void UpdateSmokeGrenadeAnimationFlags(CWeaponSmokeGrenade* grenadeWeapon);
	virtual void UpdateDetpackAnimationFlags(CWeaponDetpack* detpackWeapon);
	virtual void UpdateBaseWeaponAnimationFlags(CBaseCombatWeapon* currentWeapon);
	virtual bool CanThePlayerMove();
	virtual float GetCurrentMaxGroundSpeed();
	virtual Activity CalcMainActivity();
	virtual void DebugShowAnimState(int iStartLine);
	virtual void ComputeSequences(CStudioHdr *pStudioHdr);
	virtual void ClearAnimationLayers();

	void InitNEO(CBaseAnimatingOverlay *pPlayer, INEOPlayerAnimStateHelpers *pHelpers,
		LegAnimType_t legAnimType, bool bUseAimSequences);

	// If this returns true, then it will blend the current aim layer sequence with an idle aim layer
	// sequence based on how fast the character is moving, so it doesn't play the upper-body run at
	// full speed if he's moving really slowly.
	//
	// We return false on this for animations that don't have blends.
	virtual bool ShouldBlendAimSequenceToIdle() OVERRIDE;

	virtual bool ShouldBlendReloadSequenceToIdle();

protected:
	virtual void ComputeAimSequence() OVERRIDE;

	virtual bool ShouldResetMainSequence(int iCurrentSequence, int iNewSequence) OVERRIDE;

	int CalcFireLayerSequence(PlayerAnimEvent_t event);
	void ComputeFireSequence(CStudioHdr *pStudioHdr);

	void ComputeReloadSequence(CStudioHdr *pStudioHdr);
	int CalcReloadLayerSequence();

	bool IsOuterGrenadePrimed();
	void ComputeGrenadeSequence(CStudioHdr *pStudioHdr);
	int CalcGrenadePrimeSequence();
	int CalcGrenadeThrowSequence();
	int GetOuterGrenadeThrowCounter();

	void ComputeDetpackSequence(CStudioHdr* pStudioHdr);

	const char* GetWeaponSuffix();
	bool IsAirborne();

	void UpdateLayerSequenceGeneric(CStudioHdr *pStudioHdr, int iLayer, bool &bEnabled,
		float &flCurCycle, int &iSequence, bool bWaitAtEnd);

private:
	void UpdateReloadSequenceLayers(float flCycle, int iFirstLayer, bool bForceIdle,
		CSequenceTransitioner* pTransitioner, float flWeightScale);

private:
	// Aim sequence plays reload while this is on.
	bool m_bReloading;
	float m_flReloadCycle;
	int m_iReloadSequence;

	bool m_bFreshJump;

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

	// Detpack Animations
	bool m_bThrowingDetpack;
	bool m_bTriggeringDetpack;
	float m_flDetpackCycle;
	int m_iDetpackSequence;

	INEOPlayerAnimStateHelpers *m_pHelpers;

	CSequenceTransitioner m_ReloadSequenceTransitioner;
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
	// Base weapon reloading
	m_bReloading = false;
	m_flReloadCycle = 0;
	m_iReloadSequence = 0;

	m_bFreshJump = false;

	// Base weapon firing
	m_bFiring = false;
	m_iFireSequence = 0;
	m_flFireCycle = 0;

	// Grenade prime and firing
	m_bPrimingGrenade = false;
	m_bThrowingGrenade = false;
	m_flGrenadeCycle = 0;
	m_iGrenadeSequence = 0;
	m_iLastThrowGrenadeCounter = 0;

	// Detpack throwing and triggering
	m_bThrowingDetpack = false;
	m_bTriggeringDetpack = false;
	m_flDetpackCycle = 0;
	m_iDetpackSequence = 0;

	m_pHelpers = NULL;
}

CNEOPlayerAnimState::~CNEOPlayerAnimState()
{
	delete m_pHelpers;
}

void CNEOPlayerAnimState::InitNEO(CBaseAnimatingOverlay *pPlayer,
	INEOPlayerAnimStateHelpers *pHelpers, LegAnimType_t legAnimType, bool bUseAimSequences)
{
	CModAnimConfig config;
	config.m_flMaxBodyYawDegrees = NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES;
	config.m_LegAnimType = legAnimType;
	config.m_bUseAimSequences = bUseAimSequences;

	m_pHelpers = pHelpers;

	BaseClass::Init(pPlayer, config);
}

void CNEOPlayerAnimState::ClearAnimationState()
{
	ClearAnimationFlags();
	BaseClass::ClearAnimationState();
}

void CNEOPlayerAnimState::ClearAnimationFlags()
{
	m_bFiring = false;
	m_bReloading = false;

	m_bPrimingGrenade = m_bThrowingGrenade = false;
	m_bThrowingDetpack = m_bTriggeringDetpack = false;
	m_iLastThrowGrenadeCounter = GetOuterGrenadeThrowCounter();
}


void CNEOPlayerAnimState::UpdateAnimationFlags()
{
	CWeaponGrenade* grenadeWeapon = dynamic_cast<CWeaponGrenade*>(m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon());
	if (grenadeWeapon != nullptr)
	{ // Current weapon is a grenade
		UpdateGrenadeAnimationFlags(grenadeWeapon);
		return;
	}

	CWeaponSmokeGrenade* smokeGrenadeWeapon = dynamic_cast<CWeaponSmokeGrenade*>(m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon());
	if (smokeGrenadeWeapon != nullptr) 
	{ // Current weapon is a smoke grenade
		UpdateSmokeGrenadeAnimationFlags(smokeGrenadeWeapon);
		return;
	}

	CWeaponDetpack* detpackWeapon = dynamic_cast<CWeaponDetpack*>(m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon());
	if (detpackWeapon != nullptr)
	{ // Current weapon is a detpack
		UpdateDetpackAnimationFlags(detpackWeapon);
		return;
	}

	CBaseCombatWeapon* currentWeapon = dynamic_cast<CBaseCombatWeapon*>(m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon());
	if (currentWeapon != nullptr) 
	{ // Current weapon is a firearm/melee weapon
		UpdateBaseWeaponAnimationFlags(currentWeapon);
		return;
	}
	// Character hands are empty?
	ClearAnimationFlags();
	return;
}

void CNEOPlayerAnimState::UpdateGrenadeAnimationFlags(CWeaponGrenade* grenadeWeapon)
{
	m_bFiring = false;
	m_bReloading = false;
	m_bThrowingGrenade = false;

	if (!m_bThrowingGrenade && m_flGrenadeCycle < 1)
	{ // Grenade was recently thrown, continue playing throw animation
		return;
	}

	if (m_bPrimingGrenade && !grenadeWeapon->IsPrimed())
	{ // Play throw sequence if grenade was primed but now isn't, grenade was thrown (unless user switched to a different grenade after priming a grenade, need to keep track of grenade names? NEO FIXME)
		m_bPrimingGrenade = false;
		m_bThrowingGrenade = true;
		m_iGrenadeSequence = CalcGrenadePrimeSequence(); // This should be ThrowSequence but ThrowSequence returns 0 NEO FIXME
		m_flGrenadeCycle = 0;
		return;
	}

	if (m_bPrimingGrenade && grenadeWeapon->IsPrimed())
	{// Still priming grenade
		return;
	}

	m_bPrimingGrenade = grenadeWeapon->IsPrimed();
	if (m_bPrimingGrenade)
	{ // Begin prime sequence
		//m_iGrenadeSequence = CalcGrenadePrimeSequence(); // Can add a grenade priming animation here, there is no such animation in the orignal game NEO TODO
		m_flGrenadeCycle = 0;
		return;
	}
}

void CNEOPlayerAnimState::UpdateSmokeGrenadeAnimationFlags(CWeaponSmokeGrenade* grenadeWeapon)
{
	m_bFiring = false;
	m_bReloading = false;
	m_bThrowingGrenade = false;

	if (!m_bThrowingGrenade && m_flGrenadeCycle < 1)
	{ // Grenade was recently thrown, continue playing throw animation
		return;
	}

	if (m_bPrimingGrenade && !grenadeWeapon->IsPrimed())
	{ // Play throw sequence if grenade was primed but now isn't, grenade was thrown (unless user switched to a different grenade after priming a grenade, need to keep track of grenade names? NEO FIXME)
		m_bPrimingGrenade = false;
		m_bThrowingGrenade = true;
		m_iGrenadeSequence = CalcGrenadePrimeSequence(); // This should be ThrowSequence but ThrowSequence returns 0 NEO FIXME
		m_flGrenadeCycle = 0;
		return;
	}

	if (m_bPrimingGrenade && grenadeWeapon->IsPrimed())
	{// Still priming grenade
		return;
	}

	m_bPrimingGrenade = grenadeWeapon->IsPrimed();
	if (m_bPrimingGrenade)
	{ // Begin prime sequence
		//m_iGrenadeSequence = CalcGrenadePrimeSequence(); // Can add a grenade priming animation here, there is no such animation in the orignal game NEO TODO
		m_flGrenadeCycle = 0;
		return;
	}
}

void CNEOPlayerAnimState::UpdateDetpackAnimationFlags(CWeaponDetpack* detpackWeapon)
{
	if (detpackWeapon->m_bRemoteHasBeenTriggered)
	{ // detpack triggering
		if (!m_bTriggeringDetpack)
		{
			m_flDetpackCycle = 0;
			m_iDetpackSequence = CalcSequenceIndex("Idle_Shoot_detremote");
			m_bTriggeringDetpack = true;
		}
		m_bThrowingDetpack = false;
		return;
	}

	if (detpackWeapon->m_bWantsToThrowThisDetpack && !detpackWeapon->m_fDrawbackFinished)
	{ // detpack throwing
		if (!m_bThrowingDetpack)
		{ // first tick
			m_flDetpackCycle = 0;
			m_iDetpackSequence = CalcSequenceIndex("Idle_Shoot_detpack");
			m_bThrowingDetpack = true;
		}
		m_bTriggeringDetpack = false;
		return;
	}
}

void CNEOPlayerAnimState::UpdateBaseWeaponAnimationFlags(CBaseCombatWeapon* currentWeapon)
{
	m_bFiring = !((gpGlobals->curtime - currentWeapon->m_flNextPrimaryAttack.Get()) > 0.5f);
	m_bReloading = currentWeapon->m_bInReload.Get();
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
		m_bFiring = (m_iFireSequence > 0);
	}
	else if (event == PLAYERANIMEVENT_JUMP)
	{
		m_bFreshJump = true;
	}
	else if (event == PLAYERANIMEVENT_RELOAD)
	{
		m_iReloadSequence = CalcReloadLayerSequence();
		if (m_iReloadSequence > 0)
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
		return 0;
	}

	auto pWeapon = m_pHelpers->NEOAnim_GetActiveWeapon();
	if (!pWeapon)
	{
		return 0;
	}

	// First, look for Reload_<weapon name>.
	char szName[512];
	Q_snprintf(szName, sizeof(szName), "Reload_%s", pSuffix);

	int iReloadSequence = CalcSequenceIndex(szName);// m_pOuter->LookupSequence(szName);
	Assert(iReloadSequence > 0);
	if (iReloadSequence > 0)
	{
		return iReloadSequence;
	}

	// Fall back to Reload_M4.
	iReloadSequence = CalcSequenceIndex("Reload_M4");
	Assert(iReloadSequence > 0);
	return iReloadSequence;
}

void CNEOPlayerAnimState::UpdateLayerSequenceGeneric(CStudioHdr *pStudioHdr,
	int iLayer, bool &bEnabled, float &flCurCycle, int &iSequence, bool bWaitAtEnd)
{
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
			return;
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
	if (m_bThrowingGrenade)
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bThrowingGrenade,
			m_flGrenadeCycle, m_iGrenadeSequence, false);
	}
	if (m_bPrimingGrenade) 
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bPrimingGrenade,
			m_flGrenadeCycle, m_iGrenadeSequence, true);
	}
	bool grenadeBeingThrown = m_flGrenadeCycle < 1;
	if (!m_bThrowingGrenade && grenadeBeingThrown)
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, grenadeBeingThrown,
			m_flGrenadeCycle, m_iGrenadeSequence, false);
	}
}

void CNEOPlayerAnimState::ComputeDetpackSequence(CStudioHdr* pStudioHdr)
{
	if (m_bThrowingDetpack)
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bThrowingDetpack,
			m_flDetpackCycle, m_iDetpackSequence, false);
	}
	if (m_bTriggeringDetpack)
	{
		UpdateLayerSequenceGeneric(pStudioHdr, GRENADESEQUENCE_LAYER, m_bTriggeringDetpack,
			m_flDetpackCycle, m_iDetpackSequence, false);
	}
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
	UpdateLayerSequenceGeneric(pStudioHdr, RELOADSEQUENCE_LAYER, m_bReloading,
		m_flReloadCycle, m_iReloadSequence, false);
}

int CNEOPlayerAnimState::CalcAimLayerSequence(float *flCycle,
	float *flAimSequenceWeight, bool bForceIdle)
{
	const char* pSuffix = GetWeaponSuffix();
	if (!pSuffix)
	{
		return 0;
	}
	
	if (bForceIdle)
	{
		switch (GetCurrentMainSequenceActivity())
		{
		case ACT_NEO_IDLE_CROUCH:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_IDLE_NAME, pSuffix);

		default:
			return CalcSequenceIndex("%s%s", DEFAULT_IDLE_NAME, pSuffix);
		}
	}
	else
	{
		switch (GetCurrentMainSequenceActivity())
		{
		case ACT_NEO_MOVE_RUN:
			return CalcSequenceIndex("%s%s", DEFAULT_RUN_NAME, pSuffix);

		case ACT_RUNTOIDLE:
			Assert(false);
		case ACT_IDLETORUN:
			Assert(false);
		case ACT_NEO_MOVE_WALK:
			return CalcSequenceIndex("%s%s", DEFAULT_WALK_NAME, pSuffix);

		case ACT_NEO_IDLE_CROUCH:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_IDLE_NAME, pSuffix);

		case ACT_NEO_MOVE_CROUCH:
			return CalcSequenceIndex("%s%s", DEFAULT_CROUCH_WALK_NAME, pSuffix);

		case ACT_NEO_IDLE_STAND:
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

	// NEO TODO (Rain): cleanup once done with debug
	int res;

	switch (GetCurrentMainSequenceActivity())
	{
	case ACT_PLAYER_RUN_FIRE:
		Assert(false);
	case ACT_NEO_MOVE_RUN:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_RUN_NAME, pSuffix);
		break;

	case ACT_PLAYER_WALK_FIRE:
		Assert(false);
	case ACT_NEO_MOVE_WALK:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_WALK_NAME, pSuffix);
		break;

	case ACT_PLAYER_CROUCH_FIRE:
		Assert(false);
	case ACT_NEO_IDLE_CROUCH:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_CROUCH_NAME, pSuffix);
		break;

	case ACT_PLAYER_CROUCH_WALK_FIRE:
		Assert(false);
	case ACT_NEO_MOVE_CROUCH:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_CROUCH_WALK_NAME, pSuffix);
		break;

	default:
	case ACT_PLAYER_IDLE_FIRE:
		res = CalcSequenceIndex("%s%s", DEFAULT_FIRE_IDLE_NAME, pSuffix);
		break;
	}

	//DevMsg("Res: %d\n", res);

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

	if (currentActivity == ACT_NEO_MOVE_WALK || currentActivity == ACT_NEO_IDLE_STAND)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetWalkSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_NEO_MOVE_RUN)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_SPRINT)
	{
		Assert(false);
		return static_cast<CNEO_Player*>(m_pOuter)->GetSprintSpeed_WithActiveWepEncumberment();
	}
	else if (currentActivity == ACT_NEO_MOVE_CROUCH)
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetCrouchSpeed_WithActiveWepEncumberment();
	}
	else
	{
		return static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed();
	}
}

bool CNEOPlayerAnimState::IsAirborne()
{
	Assert(m_pOuter);
	Assert(m_pOuter->MyCombatCharacterPointer());
	return static_cast<CNEO_Player*>(m_pOuter->MyCombatCharacterPointer())->IsAirborne();
}

Activity CNEOPlayerAnimState::CalcMainActivity()
{
	if (IsAirborne())
	{
		return ACT_NEO_JUMP;
	}

	Activity idealActivity = ACT_NEO_IDLE_STAND;

	const float flOuterSpeed = GetOuterXYSpeed();

	if (m_pOuter->GetFlags() & FL_DUCKING)
	{
		if (flOuterSpeed > MOVING_MINIMUM_SPEED)
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
		Assert(dynamic_cast<CNEO_Player*>(m_pOuter));

		if (flOuterSpeed > MOVING_MINIMUM_SPEED)
		{
			if (flOuterSpeed > static_cast<CNEO_Player*>(m_pOuter)->GetNormSpeed_WithActiveWepEncumberment())
			{
				idealActivity = ACT_NEO_MOVE_RUN;
			}
			else if (flOuterSpeed > static_cast<CNEO_Player*>(m_pOuter)->GetWalkSpeed_WithActiveWepEncumberment())
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

	return idealActivity;
}

void CNEOPlayerAnimState::DebugShowAnimState(int iStartLine)
{
	engine->Con_NPrintf(iStartLine++, "fire  : %s, cycle: %.2f\n",
		m_bFiring ? GetSequenceName(m_pOuter->GetModelPtr(), m_iFireSequence) :
		"[not firing]", m_flFireCycle);
	
	engine->Con_NPrintf(iStartLine++, "reload: %s, cycle: %.2f\n",
		m_bReloading ? GetSequenceName(m_pOuter->GetModelPtr(), m_iReloadSequence) :
		"[not reloading]", m_flReloadCycle);
	
	BaseClass::DebugShowAnimState(iStartLine);
}

void CNEOPlayerAnimState::ComputeSequences(CStudioHdr *pStudioHdr)
{
	BaseClass::ComputeSequences(pStudioHdr);
	UpdateAnimationFlags();

	ComputeFireSequence(pStudioHdr);
	ComputeReloadSequence(pStudioHdr);
	ComputeGrenadeSequence(pStudioHdr);
	ComputeDetpackSequence(pStudioHdr);
}

void CNEOPlayerAnimState::ClearAnimationLayers()
{
	VPROF("CNEOPlayerAnimState::ClearAnimationLayers");
	if (!m_pOuter)
		return;

	m_pOuter->SetNumAnimOverlays(NUM_LAYERS_WANTED);
	for (int i = 0; i < m_pOuter->GetNumAnimOverlays(); i++)
	{
		m_pOuter->GetAnimOverlay(i)->SetOrder(CBaseAnimatingOverlay::MAX_OVERLAYS);
#ifndef CLIENT_DLL
		m_pOuter->GetAnimOverlay(i)->m_fFlags = 0;
#endif
	}
}

void CNEOPlayerAnimState::ComputeFireSequence(CStudioHdr *pStudioHdr)
{
	UpdateLayerSequenceGeneric(pStudioHdr, FIRESEQUENCE_LAYER, m_bFiring,
		m_flFireCycle, m_iFireSequence, false);
}

bool CNEOPlayerAnimState::ShouldBlendAimSequenceToIdle()
{
	const Activity act = GetCurrentMainSequenceActivity();
	return (act == ACT_NEO_MOVE_RUN || act == ACT_NEO_MOVE_WALK || act == ACT_NEO_MOVE_CROUCH || act == ACT_NEO_RELOAD);
}

bool CNEOPlayerAnimState::ShouldBlendReloadSequenceToIdle()
{
	Assert(!m_pOuter || m_pOuter->MyCombatCharacterPointer());
	if (!m_pOuter || !m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon())
	{
#ifdef DEBUG
		DevWarning("Early return from ShouldBlendReloadSequenceToIdle\n");
#endif
		m_bReloading = false;
		return false;
	}

	if (m_bReloading)
	{
		m_bReloading = m_pOuter->MyCombatCharacterPointer()->GetActiveWeapon()->m_bInReload;
	}

	return (m_bReloading && ((m_pOuter)->GetFlags() & FL_DUCKING));
}

void CNEOPlayerAnimState::ComputeAimSequence()
{
	if (ShouldBlendReloadSequenceToIdle())
	{
		const float cycle = (m_pOuter->GetFlags() & FL_DUCKING) ?
#ifdef CLIENT_DLL
			m_pOuter->GetAnimOverlay(RELOADSEQUENCE_LAYER)->m_flCycle.GetRaw() : m_pOuter->GetCycle();
#else
			m_pOuter->GetAnimOverlay(RELOADSEQUENCE_LAYER)->m_flCycle.Get() : m_pOuter->GetCycle();
#endif
		UpdateReloadSequenceLayers(cycle, RELOADSEQUENCE_LAYER, true, &m_ReloadSequenceTransitioner, 1.0f);
	}
	BaseClass::ComputeAimSequence();
}

void CNEOPlayerAnimState::UpdateReloadSequenceLayers(float flCycle, int iFirstLayer,
	bool bForceIdle, CSequenceTransitioner* pTransitioner, float flWeightScale)
{
	float flReloadSequenceWeight = 1;
	int iReloadSequence = CalcReloadLayerSequence();
	if (iReloadSequence == -1)
		iReloadSequence = 0;

	// Feed the current state of the animation parameters to the sequence transitioner.
	// It will hand back either 1 or 2 animations in the queue to set, depending on whether
	// it's transitioning or not. We just dump those into the animation layers.
	pTransitioner->CheckForSequenceChange(
		m_pOuter->GetModelPtr(),
		iReloadSequence,
		true,	// force transitions on the same anim
		true	// yes, interpolate when transitioning
	);

	pTransitioner->UpdateCurrent(
		m_pOuter->GetModelPtr(),
		iReloadSequence,
		flCycle,
		GetOuter()->GetPlaybackRate(),
		gpGlobals->curtime
	);

	CAnimationLayer* pDest0 = m_pOuter->GetAnimOverlay(iFirstLayer);
	CAnimationLayer* pDest1 = m_pOuter->GetAnimOverlay(iFirstLayer + 1);

	if (pTransitioner->m_animationQueue.Count() == 1)
	{
		// If only 1 animation, then blend it in fully.
		CAnimationLayer* pSource0 = &pTransitioner->m_animationQueue[0];
		*pDest0 = *pSource0;

		pDest0->m_flWeight = 1;
		pDest1->m_flWeight = 0;
		pDest0->m_nOrder = iFirstLayer;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}
	else if (pTransitioner->m_animationQueue.Count() >= 2)
	{
		// The first one should be fading out. Fade in the new one inversely.
		CAnimationLayer* pSource0 = &pTransitioner->m_animationQueue[0];
		CAnimationLayer* pSource1 = &pTransitioner->m_animationQueue[1];

		*pDest0 = *pSource0;
		*pDest1 = *pSource1;
		Assert(pDest0->m_flWeight >= 0.0f && pDest0->m_flWeight <= 1.0f);
		pDest1->m_flWeight = 1 - pDest0->m_flWeight;	// This layer just mirrors the other layer's weight (one fades in while the other fades out).

		pDest0->m_nOrder = iFirstLayer;
		pDest1->m_nOrder = iFirstLayer + 1;

#ifndef CLIENT_DLL
		pDest0->m_fFlags |= ANIM_LAYER_ACTIVE;
		pDest1->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
	}

	pDest0->m_flWeight *= flWeightScale * flReloadSequenceWeight;
	pDest0->m_flWeight = clamp((float)pDest0->m_flWeight, 0.0f, 1.0f);

	pDest1->m_flWeight *= flWeightScale * flReloadSequenceWeight;
	pDest1->m_flWeight = clamp((float)pDest1->m_flWeight, 0.0f, 1.0f);

	pDest0->m_flCycle = pDest1->m_flCycle = flCycle;
}

bool CNEOPlayerAnimState::ShouldResetMainSequence(int iCurrentSequence, int iNewSequence)
{
	if (IsAirborne())
	{
		if (!m_bFreshJump)
		{
			// Only reset active mid-air jump sequence if we're transitioning away from that animation
			return iNewSequence != SelectWeightedSequence(TranslateActivity(ACT_NEO_JUMP));
		}
		m_bFreshJump = false;
	}

	return BaseClass::ShouldResetMainSequence(iCurrentSequence, iNewSequence);
}