#include "cbase.h"
#include "weapon_supa7.h"
#include "neo_gamerules.h"
#ifdef CLIENT_DLL
#include "prediction.h"
#endif
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSupa7, DT_WeaponSupa7)

BEGIN_NETWORK_TABLE(CWeaponSupa7, DT_WeaponSupa7)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bSlugDelayed)),
	RecvPropBool(RECVINFO(m_bSlugLoaded)),
	RecvPropBool(RECVINFO(m_bWeaponRaised)),
	RecvPropBool(RECVINFO(m_bShellInChamber)),
	RecvPropFloat(RECVINFO(m_flNextReload)),
	RecvPropInt(RECVINFO(m_nTubeCount)),
	RecvPropInt(RECVINFO(m_nTubeSlugs)),
	RecvPropInt(RECVINFO(m_nChamber)),
	RecvPropInt(RECVINFO(m_nInsert)),
	RecvPropTime(RECVINFO(m_flInsertCreditTime)),
	RecvPropTime(RECVINFO(m_flRackFeedTime)),
	RecvPropBool(RECVINFO(m_bSlugLoad)),
#else
	SendPropBool(SENDINFO(m_bSlugDelayed)),
	SendPropBool(SENDINFO(m_bSlugLoaded)),
	SendPropBool(SENDINFO(m_bWeaponRaised)),
	SendPropBool(SENDINFO(m_bShellInChamber)),
	SendPropFloat(SENDINFO(m_flNextReload)),
	SendPropInt(SENDINFO(m_nTubeCount), 4, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nTubeSlugs), 8, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nChamber), 2, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_nInsert), 2, SPROP_UNSIGNED),
	SendPropTime(SENDINFO(m_flInsertCreditTime)),
	SendPropTime(SENDINFO(m_flRackFeedTime)),
	SendPropBool(SENDINFO(m_bSlugLoad)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSupa7)
	DEFINE_PRED_FIELD(m_bSlugDelayed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bSlugLoaded, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bWeaponRaised, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bShellInChamber, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flNextReload, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nTubeCount, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nTubeSlugs, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nChamber, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nInsert, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flInsertCreditTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flRackFeedTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bSlugLoad, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_supa7, CWeaponSupa7);

PRECACHE_WEAPON_REGISTER(weapon_supa7);

#ifdef CLIENT_DLL
ConVar cl_neo_supa7_prefer_slugs("cl_neo_supa7_prefer_slugs", "0", FCVAR_ARCHIVE | FCVAR_USERINFO,
	"Whether the Supa-7 starts a round loaded with slugs. Set by the freeze-time alt-fire and reload presses.", true, false, true, true);
#endif

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponSupa7)
	DEFINE_FIELD(m_bSlugDelayed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bSlugLoaded, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bWeaponRaised, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bShellInChamber, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flNextReload, FIELD_FLOAT),
	DEFINE_FIELD(m_nTubeCount, FIELD_INTEGER),
	DEFINE_FIELD(m_nTubeSlugs, FIELD_INTEGER),
	DEFINE_FIELD(m_nChamber, FIELD_INTEGER),
	DEFINE_FIELD(m_nInsert, FIELD_INTEGER),
	DEFINE_FIELD(m_flInsertCreditTime, FIELD_TIME),
	DEFINE_FIELD(m_flRackFeedTime, FIELD_TIME),
	DEFINE_FIELD(m_bSlugLoad, FIELD_BOOLEAN),
END_DATADESC()
#endif

#if(0)
// Might be unsuitable for shell based shotgun? Should sort out the acttable at some point.
NEO_IMPLEMENT_ACTTABLE(CWeaponSupa7)
#else
#ifdef GAME_DLL
acttable_t	CWeaponSupa7::m_acttable[] =
{
	{ ACT_HL2MP_IDLE, ACT_HL2MP_IDLE_SHOTGUN, false },
	{ ACT_HL2MP_RUN, ACT_HL2MP_RUN_SHOTGUN, false },
	{ ACT_HL2MP_IDLE_CROUCH, ACT_HL2MP_IDLE_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RANGE_ATTACK, ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN, false },
	{ ACT_HL2MP_GESTURE_RELOAD, ACT_HL2MP_GESTURE_RELOAD_SHOTGUN, false },
	{ ACT_HL2MP_JUMP, ACT_HL2MP_JUMP_SHOTGUN, false },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_SHOTGUN, false },
};
IMPLEMENT_ACTTABLE(CWeaponSupa7);
#endif
#endif

// Viewmodel frames of the shipped team models, which play at this rate
static constexpr float VM_FRAMES_PER_SECOND = 30.0f;
// reloadinsert: the hatch closes behind the round, which is then in the tube
static constexpr float INSERT_ROUND_IN_TUBE_TIME = 12.0f / VM_FRAMES_PER_SECOND;
// reloadend: the pump is forward again with the next round chambered
static constexpr float RACK_PUMP_HOME_TIME = 21.0f / VM_FRAMES_PER_SECOND;
// reloadpump: the rack after the first insert of a reload; the pump is home with that round chambered
static constexpr const char *VM_SEQ_PUMP_AFTER_INSERT = "reloadpump";
static constexpr float PUMP_AFTER_INSERT_HOME_TIME = 17.0f / VM_FRAMES_PER_SECOND;
// The shell shot without the pump clicks of the shipped shot sound
static constexpr const char *SND_LAST_SHELL_SHOT = "weapon_murata.single_nopump";

CWeaponSupa7::CWeaponSupa7(void)
{
	m_bReloadsSingly = true;
	m_bSlugDelayed = false;
	m_bSlugLoaded = false;
	m_bWeaponRaised = false;
	m_bShellInChamber = true;
	m_flNextReload = 0.0f;
	m_nTubeCount = 0;
	m_nTubeSlugs = 0;
	m_nChamber = SUPA7_CHAMBER_EMPTY;
	m_nInsert = SUPA7_INSERT_NONE;
	m_flInsertCreditTime = 0.0f;
	m_flRackFeedTime = 0.0f;
	m_bSlugLoad = false;

	m_fMinRange1 = 0.0;
	m_fMaxRange1 = 500;
	m_fMinRange2 = 0.0;
	m_fMaxRange2 = 200;

	m_weaponSeeds = {
		"supapx",
		"supapy",
		"suparx",
		"supary",
	};
}

// Purpose: Override so only reload one shell at a time
bool CWeaponSupa7::StartReload(void)
{
	// the tube takes shells behind a chambered slug
	if (m_bSlugLoaded && !IsTubeEnabled())
		return false;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_bSlugDelayed)
	{
		if (m_iSecondaryAmmoCount <= 0)
		{
			m_bSlugDelayed = false;
		}
		else
		{
			return false;
		}
	}

	if (m_iPrimaryAmmoCount <= 0 && !IsSlugLoadActive())
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	if (!TubePhasesOnStart())
	{
		return false;
	}

	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	SetShotgunShellVisible(true);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
	m_flNextReload = gpGlobals->curtime + SequenceDuration();

	m_bInReload = true;
	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::StartReloadSlug(void)
{
	if (IsTubeEnabled())
	{
		return TubePhasesOnStartSlug();
	}

	if (m_bSlugLoaded)
		return false;

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (m_iSecondaryAmmoCount <= 0)
	{
		return false;
	}

	if (m_iClip1 >= GetMaxClip1())
		return false;

	if (!m_bInReload)
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
		m_bInReload = true;
	}

	m_bSlugDelayed = true;

	SetShotgunShellVisible(true);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
	m_flNextReload = gpGlobals->curtime + SequenceDuration();

	return true;
}

// Purpose: Override so only reload one shell at a time
bool CWeaponSupa7::Reload(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Assert(false);
		Warning("ERROR: Supa7 Reload called incorrectly!\n");
	}

	auto pOwner = static_cast<CNEO_Player*>(GetOwner());

	if (pOwner == NULL)
		return false;

	if (m_iPrimaryAmmoCount <= 0 && !IsSlugLoadActive())
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	if (IsTubeFull())
	{
		return false;
	}

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_RELOAD);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
	StartInsert(IsSlugLoadActive() ? SUPA7_INSERT_SLUG : SUPA7_INSERT_SHELL);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	constexpr float TIME_BETWEEN_SHELLS_LOADED = 0.5f;
	// ProposeNextAttack(gpGlobals->curtime + TIME_BETWEEN_SHELLS_LOADED);
	m_flNextReload = gpGlobals->curtime + TIME_BETWEEN_SHELLS_LOADED;

	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::ReloadSlug(void)
{
	// First, flip this bool to ensure we can't get stuck in the m_bSlugDelayed state
	m_bSlugDelayed = false;

	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Assert(false);
		Warning("ERROR: Supa7 ReloadSlug called incorrectly!\n");
	}

	auto pOwner = static_cast<CNEO_Player*>(GetOwner());

	if (pOwner == NULL)
		return false;

	if (m_iSecondaryAmmoCount <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	if (IsTubeFull())
	{
		return false;
	}

	// the tube takes the slug only once it is in
	if (!IsTubeEnabled())
	{
		FillClipSlug();
		m_bShellInChamber = false;
	}

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_RELOAD);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
	StartInsert(SUPA7_INSERT_SLUG);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	// ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
	m_flNextReload = gpGlobals->curtime + SequenceDuration();

	return true;
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FinishReload(void)
{
	SetShotgunShellVisible(false);

	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bInReload = false;

	if (TubePhasesOnFinish())
	{
		return;
	}

	// Finish reload animation
	if (!m_bShellInChamber)
	{
		SendWeaponAnim(ACT_SHOTGUN_PUMP);
		WeaponSound(SPECIAL2);
		m_bShellInChamber = true;
	}
	else
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);
	}

	pOwner->m_flNextAttack = gpGlobals->curtime;
	// ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FillClip(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	// Add them to the clip
	if (m_iPrimaryAmmoCount > 0)
	{
		if (Clip1() < GetMaxClip1())
		{
			m_iClip1++;
			m_iPrimaryAmmoCount -= 1;
		}
	}
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FillClipSlug(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	// Add them to the clip
	if (m_iSecondaryAmmoCount > 0 && !m_bSlugLoaded)
	{
		m_iClip1++;
		m_iSecondaryAmmoCount -= 1;
		m_bSlugLoaded = true;
		m_bSlugDelayed = false;
	}
}

void CWeaponSupa7::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	// Only the player fires this way so we can cast
	auto pPlayer = static_cast<CNEO_Player*>(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (!TubePhasesCanFire())
	{
		return;
	}

	const bool bLastShotWithoutPump = IsLastShotWithoutPump();
	int numBullets = GetNEOWpnData().m_iBullets;
	Vector bulletSpread = GetBulletSpread();
	int ammoType = m_iPrimaryAmmoType;
	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	// Change the firing characteristics and sound if a slug was loaded
	if (m_bSlugLoaded)
	{
		numBullets = 1;
		ammoType = m_iSecondaryAmmoType;
		WeaponSound(WPN_DOUBLE);
		if (!bLastShotWithoutPump)
		{
			WeaponSound(SPECIAL2);
		}
	}
	else if (bLastShotWithoutPump)
	{
		EmitShotSoundPiece(SND_LAST_SHELL_SHOT);
	}
	else
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE);
	}
#ifdef GAME_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);
#endif

	FireBulletsInfo_t info(numBullets, vecSrc, vecAiming, bulletSpread, MAX_TRACE_LENGTH, ammoType);
	if (m_bSlugLoaded)
	{
		constexpr float NEO_SLUG_PENETRATION_VALUE = 32.f;
		info.m_flPenetration = NEO_SLUG_PENETRATION_VALUE;
	}
	info.m_pAttacker = pPlayer;
	info.m_iTracerFreq = 0;

	pPlayer->DoMuzzleFlash();

	if (bLastShotWithoutPump)
	{
		// the shipped recoil clips are the shots without the pump stroke
		SendWeaponAnim(m_bSlugLoaded ? ACT_VM_RECOIL1 : ACT_VM_RECOIL2);
	}
	else
	{
		SendWeaponAnim(m_bSlugLoaded ? ACT_VM_PRIMARYATTACK : ACT_VM_SECONDARYATTACK);
	}
	m_bSlugLoaded = false;

	// Don't fire again until fire animation has completed
	ProposeNextAttack(gpGlobals->curtime + GetFireRate());
	m_iClip1 -= 1;
	if (m_iClip1 == 0)
	{
		m_bShellInChamber = false;
	}
	TubePhasesOnShot();

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	// Fire the bullets, and force the first shot to be perfectly accurate
	pPlayer->FireBullets(info);

	if (!m_iClip1 && m_iPrimaryAmmoCount <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	AddViewKick();
}

void CWeaponSupa7::SecondaryAttack(void)
{
	if (ShootingIsPrevented())
		return;

	StartReloadSlug();
}

// Purpose: Public accessor for resetting the reload, and any "delayed" vars, to
// make sure a freshly picked up supa can never act on its own against user will.
void CWeaponSupa7::ClearDelayedInputs(void)
{
	m_bFireOnEmpty = false;
	m_bSlugDelayed = false;

	// Explicitly networking to DT_LocalActiveWeaponData.
	// This helps with some edge cases where the client disagrees
	// about the current m_bInReload state with server if the gun
	// is dropped instantly after starting a reload.
	// Ideally we'd always predict it, but this should be fairly
	// low bandwith cost (only networked to current gun owner)
	// for not having to do a larger rewrite.
	m_bInReload.GetForModify() = false;

	TubePhasesOnDrop();

	SetShotgunShellVisible(false);
}

// Purpose: Override so shotgun can do multiple reloads in a row
void CWeaponSupa7::ItemPostFrame(void)
{
	auto pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}
	ProcessAnimationEvents();
	TubePhasesOnFrame();
	TubePhasesOnFreezeTime();

	if (m_bInReload)
	{
		TubePhasesOnLoadInput();

		// If I'm primary firing and have one round stop reloading
		if ((pOwner->m_nButtons & IN_ATTACK) && (m_iClip1 >= 1) && !IsRackPending())
		{
			// OG cues a shotgun rack when beginning a reload from a completely empty clip. You can however fire without racking the shotgun, and it will instead rack the next time you reload (if you don't cancel it by shooting again)
			// This is a slight nerf to the supa where the gun has to be racked before it is fired
			FinishReload();
		}
		else if (m_flNextReload <= gpGlobals->curtime)
		{
			// If we're supposed to have a slug loaded, load it
			if (m_bSlugDelayed)
			{
				if (!ReloadSlug())
				{
					m_bSlugDelayed.GetForModify() = false;
				}
			}
			else if (!TubePhasesOnNextInsert())
			{
				// If out of ammo end reload
				if ((m_iPrimaryAmmoCount <= 0 || m_bSlugLoaded || m_iClip1 >= GetMaxClip1()))
				{
					FinishReload();
				}
				else
				{
					if (m_bWeaponRaised)
					{ // delays filling the clip until the first reload animation is finished, forcing the entire reload animation to be finished before a shell is loaded into the clip
						FillClip();
					}
					m_bWeaponRaised = true;
					Reload();
				}
			}
			return;
		}
	}
	else
	{
		m_bWeaponRaised = false;
		SetShotgunShellVisible(false);
	}

	// Shotgun uses same timing and ammo for secondary attack
	if (pOwner->m_nButtons & IN_ATTACK2 && (m_flNextPrimaryAttack <= gpGlobals->curtime) && m_iSecondaryAmmoCount)
	{
		// If the firing button was just pressed, reset the firing time
		if (pOwner->m_afButtonPressed & IN_ATTACK2)
		{
			ProposeNextAttack(gpGlobals->curtime);
		}
		SecondaryAttack();
	}
	else if (pOwner->m_nButtons & IN_ATTACK && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		if ((m_iClip1 <= 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !m_iPrimaryAmmoCount))
		{
			DryFire();
		}
		else if (TubePhasesNeedRack())
		{
			StartRack();
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == WL_Eyes && !m_bFiresUnderwater)
		{
			WeaponSound(EMPTY);
			ProposeNextAttack(gpGlobals->curtime + GetFastestDryRefireTime());
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer* pPlayer = ToBasePlayer(GetOwner());
			if (pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK)
			{
				ProposeNextAttack(gpGlobals->curtime);
			}
			PrimaryAttack();
		}
	}

	if (!(pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime && pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		if (!StartReload())
		{
			TubePhasesOnReloadPress();
		}
	}
	else if (!(pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime && pOwner->m_nButtons & IN_ATTACK2 && UsesClipsForAmmo1() && !m_bInReload)
	{
		StartReloadSlug();
	}
	else
	{
		// no fire buttons down
		m_bFireOnEmpty = false;

		if (!HasAnyAmmo() && m_flNextPrimaryAttack < gpGlobals->curtime)
		{
			// weapon isn't useable, switch.
			if (!(GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY) && pOwner->SwitchToNextBestWeapon(this))
			{
				ProposeNextAttack(gpGlobals->curtime + 0.3);
				return;
			}
		}

		if (!m_bInReload)
		{
			WeaponIdle();
		}
	}
}

// The draw at spawn keeps the gun busy; the freeze-time load choice applies during it
void CWeaponSupa7::ItemBusyFrame(void)
{
	TubePhasesOnFreezeTime();
	BaseClass::ItemBusyFrame();
}

bool CWeaponSupa7::IsTubeEnabled() const
{
	return NEORules() && NEORules()->IsReloadExperimental();
}

bool CWeaponSupa7::IsTubeFull() const
{
	return IsTubeEnabled() && m_nTubeCount >= TubeCapacity();
}

// Returns whether the shell reload may start
bool CWeaponSupa7::TubePhasesOnStart()
{
	if (!IsTubeEnabled())
	{
		return true;
	}

	SyncTube();
	if (IsTubeFull())
	{
		return false;
	}

	m_nInsert = SUPA7_INSERT_NONE;
	return true;
}

// Alt-fire with no reload running starts one that loads slugs
bool CWeaponSupa7::TubePhasesOnStartSlug()
{
	if (m_bInReload || m_iSecondaryAmmoCount <= 0 || IsInFreezeTime())
	{
		return false;
	}

	m_bSlugLoad = true;
	if (!StartReload())
	{
		m_bSlugLoad = false;
		return false;
	}
	return true;
}

// Alt-fire makes the rest of the load slugs, from the round in the hand on; a reload press makes it shells again
void CWeaponSupa7::TubePhasesOnLoadInput()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!IsTubeEnabled() || !pOwner)
	{
		return;
	}

	if ((pOwner->m_nButtons & IN_ATTACK2) && m_iSecondaryAmmoCount > 0)
	{
		m_bSlugLoad = true;
	}
	else if ((pOwner->m_afButtonPressed & IN_RELOAD) && m_iPrimaryAmmoCount > 0)
	{
		m_bSlugLoad = false;
	}
}

// Freeze time: alt-fire loads the gun with slugs, a reload press with shells, and the client keeps the choice for
// later freeze times
void CWeaponSupa7::TubePhasesOnFreezeTime()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!IsTubeEnabled() || !pOwner)
	{
		return;
	}

#ifdef GAME_DLL
	if (!IsInFreezeTime())
	{
		m_bFreezePreferenceApplied = false;
		return;
	}

	if (!m_bFreezePreferenceApplied)
	{
		m_bFreezePreferenceApplied = true;
		const char *pszPrefer = engine->GetClientConVarValue(pOwner->entindex(), "cl_neo_supa7_prefer_slugs");
		if (pszPrefer && *pszPrefer && V_atoi(pszPrefer) != 0)
		{
			ConvertLoad(SUPA7_INSERT_SLUG);
		}
	}
#else
	if (!IsInFreezeTime())
	{
		return;
	}
#endif

	Supa7Insert_t load = SUPA7_INSERT_NONE;
	if (pOwner->m_afButtonPressed & IN_ATTACK2)
	{
		load = SUPA7_INSERT_SLUG;
	}
	else if (pOwner->m_afButtonPressed & IN_RELOAD)
	{
		load = SUPA7_INSERT_SHELL;
	}

	if (load == SUPA7_INSERT_NONE)
	{
		return;
	}

	ConvertLoad(load);
#ifdef CLIENT_DLL
	if (pOwner->IsLocalPlayer() && prediction->IsFirstTimePredicted())
	{
		cl_neo_supa7_prefer_slugs.SetValue(load == SUPA7_INSERT_SLUG);
	}
#endif
}

// A round still in the hand goes back to the reserve; an empty chamber is racked before the next shot.
// Returns whether the tube finished the reload
bool CWeaponSupa7::TubePhasesOnFinish()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!IsTubeEnabled() || !pOwner)
	{
		return false;
	}

	CancelInsert();
	m_bSlugLoad = false;
	SyncTube();
	if (!IsChamberLive() && m_nTubeCount > 0)
	{
		StartRack();
	}
	else
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);
	}
	pOwner->m_flNextAttack = gpGlobals->curtime;
	return true;
}

bool CWeaponSupa7::TubePhasesCanFire()
{
	if (!IsTubeEnabled())
	{
		return true;
	}

	SyncTube();
	return IsChamberLive();
}

// The fire animation's pump ejects the hull and feeds the tube's last-loaded round; the tube then sets the clip
void CWeaponSupa7::TubePhasesOnShot()
{
	if (!IsTubeEnabled())
	{
		return;
	}

	m_nChamber = SUPA7_CHAMBER_HULL;
	if (m_nTubeCount <= 0 && UsesPumpAfterInsert())
	{
		// the last shot leaves the action closed on its hull; the reload's first round is racked over it
		UpdateClipFromTube();
		return;
	}
	FeedChamber();
}

// A rack or insert cut off by the drop does nothing for the next owner
void CWeaponSupa7::TubePhasesOnDrop()
{
	if (!IsTubeEnabled())
	{
		return;
	}

	CancelInsert();
	m_flRackFeedTime = 0.0f;
	m_bSlugLoad = false;
}

void CWeaponSupa7::TubePhasesOnFrame()
{
	if (!IsTubeEnabled())
	{
		return;
	}

	SyncTube();
	RunInsert();
	RunRack();
}

// The next round of a reload; the reload ends when the reserve of the type it loads runs out.
// Returns whether the tube handled the step
bool CWeaponSupa7::TubePhasesOnNextInsert()
{
	if (!IsTubeEnabled())
	{
		return false;
	}

	m_nInsert = SUPA7_INSERT_NONE;
	if (!IsChamberLive() && m_nTubeCount > 0 && UsesPumpAfterInsert())
	{
		StartPumpAfterInsert();
		return true;
	}

	const int iReserve = m_bSlugLoad ? m_iSecondaryAmmoCount : m_iPrimaryAmmoCount;
	if (iReserve <= 0 || IsTubeFull() || !Reload())
	{
		FinishReload();
	}
	return true;
}

// Rounds in the tube and none in the chamber: the trigger racks one up first
bool CWeaponSupa7::TubePhasesNeedRack() const
{
	return IsTubeEnabled() && !IsChamberLive() && !IsRackPending();
}

// With nothing to load, a reload press racks up a round for an empty chamber
void CWeaponSupa7::TubePhasesOnReloadPress()
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!IsTubeEnabled() || !pOwner || !(pOwner->m_afButtonPressed & IN_RELOAD))
	{
		return;
	}

	if (!IsChamberLive() && m_nTubeCount > 0)
	{
		StartRack();
	}
}

// Credited rounds stay in the tube; a round still in the hand and a rack before its pump is home do nothing
void CWeaponSupa7::TubePhasesOnHolster()
{
	if (!IsTubeEnabled())
	{
		return;
	}

	CancelInsert();
	m_flRackFeedTime = 0.0f;
	m_bSlugDelayed = false;
	m_bSlugLoad = false;
}

// Rebuild the tube from the clip count when something set the count directly (a new or picked-up gun, a give):
// a live round in the chamber, shells in the tube
void CWeaponSupa7::SyncTube()
{
	if (IsTubeSynced())
	{
		return;
	}

	const int iClip = Clamp(m_iClip1.Get(), 0, GetMaxClip1());
	m_nChamber = (iClip > 0) ? (m_bSlugLoaded ? SUPA7_CHAMBER_SLUG : SUPA7_CHAMBER_SHELL) : SUPA7_CHAMBER_EMPTY;
	m_nTubeCount = Max(0, iClip - SUPA7_CHAMBER_ROUNDS);
	m_nTubeSlugs = 0;
	UpdateClipFromTube();
}

void CWeaponSupa7::TubePush(Supa7Insert_t round)
{
	if (m_nTubeCount >= TubeCapacity())
	{
		return;
	}

	if (round == SUPA7_INSERT_SLUG)
	{
		m_nTubeSlugs |= (1 << m_nTubeCount);
	}
	else
	{
		m_nTubeSlugs &= ~(1 << m_nTubeCount);
	}
	m_nTubeCount += 1;
	UpdateClipFromTube();
}

// The pump ejects a hull and feeds the tube's last-loaded round; a live round stays where it is
void CWeaponSupa7::FeedChamber()
{
	if (IsChamberLive())
	{
		return;
	}

	if (m_nTubeCount <= 0)
	{
		m_nChamber = SUPA7_CHAMBER_EMPTY;
		UpdateClipFromTube();
		return;
	}

	m_nTubeCount -= 1;
	const bool bSlug = (m_nTubeSlugs & (1 << m_nTubeCount)) != 0;
	m_nTubeSlugs &= ~(1 << m_nTubeCount);
	m_nChamber = bSlug ? SUPA7_CHAMBER_SLUG : SUPA7_CHAMBER_SHELL;
	UpdateClipFromTube();
}

// The counts the rest of the game reads: live rounds in the gun, and the chambered round's type
void CWeaponSupa7::UpdateClipFromTube()
{
	m_iClip1 = m_nTubeCount + (IsChamberLive() ? 1 : 0);
	m_bSlugLoaded = (m_nChamber == SUPA7_CHAMBER_SLUG);
	m_bShellInChamber = IsChamberLive();
}

// The reserve gives up the round only when it is in the tube, so an insert cut off before then costs nothing
void CWeaponSupa7::StartInsert(Supa7Insert_t round)
{
	if (!IsTubeEnabled())
	{
		return;
	}

	m_nInsert = round;
	m_flInsertCreditTime = gpGlobals->curtime + INSERT_ROUND_IN_TUBE_TIME;
}

void CWeaponSupa7::RunInsert()
{
	if (m_flInsertCreditTime <= 0.0f || gpGlobals->curtime < m_flInsertCreditTime)
	{
		return;
	}

	m_flInsertCreditTime = 0.0f;
	if (m_nTubeCount >= TubeCapacity())
	{
		return;
	}

	// the round's type is set as it enters the tube, so an input during the insert decides it
	if (m_nInsert == SUPA7_INSERT_NONE)
	{
		return;
	}

	if (m_bSlugLoad && m_iSecondaryAmmoCount > 0)
	{
		TubePush(SUPA7_INSERT_SLUG);
		m_iSecondaryAmmoCount -= 1;
	}
	else if (!m_bSlugLoad && m_iPrimaryAmmoCount > 0)
	{
		TubePush(SUPA7_INSERT_SHELL);
		m_iPrimaryAmmoCount -= 1;
	}
}

bool CWeaponSupa7::IsInFreezeTime() const
{
	auto pOwner = static_cast<CNEO_Player *>(GetOwner());
	return pOwner && (pOwner->GetNeoFlags() & NEO_FL_FREEZETIME);
}

// Sets the type of the rounds in the gun, the chamber first, then the tube in fire order, while the reserve of that
// type lasts; each replaced round goes back to its reserve
void CWeaponSupa7::ConvertLoad(Supa7Insert_t round)
{
	const bool bToSlug = (round == SUPA7_INSERT_SLUG);
	auto swapWithReserve = [this, bToSlug]()
	{
		int &iTakeFrom = bToSlug ? m_iSecondaryAmmoCount.GetForModify() : m_iPrimaryAmmoCount.GetForModify();
		if (iTakeFrom <= 0)
		{
			return false;
		}

		iTakeFrom -= 1;
		int &iGiveTo = bToSlug ? m_iPrimaryAmmoCount.GetForModify() : m_iSecondaryAmmoCount.GetForModify();
		iGiveTo += 1;
		return true;
	};

	SyncTube();
	if (IsChamberLive() && (m_nChamber == SUPA7_CHAMBER_SLUG) != bToSlug && swapWithReserve())
	{
		m_nChamber = bToSlug ? SUPA7_CHAMBER_SLUG : SUPA7_CHAMBER_SHELL;
	}

	for (int i = m_nTubeCount - 1; i >= 0; --i)
	{
		const int iBit = (1 << i);
		if (((m_nTubeSlugs & iBit) != 0) == bToSlug)
		{
			continue;
		}

		if (!swapWithReserve())
		{
			break;
		}

		m_nTubeSlugs = bToSlug ? (m_nTubeSlugs | iBit) : (m_nTubeSlugs & ~iBit);
	}
	UpdateClipFromTube();
}

void CWeaponSupa7::CancelInsert()
{
	m_nInsert = SUPA7_INSERT_NONE;
	m_flInsertCreditTime = 0.0f;
}

// The shipped reload's finishing rack, also played alone: the gun fires only once it ends
void CWeaponSupa7::StartRack()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return;
	}

	SendWeaponAnim(ACT_SHOTGUN_PUMP);
	WeaponSound(SPECIAL2);

	const float flDuration = ViewModelSequenceDuration();
	m_flRackFeedTime = gpGlobals->curtime + Min(RACK_PUMP_HOME_TIME, flDuration);
	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + flDuration);
	SetWeaponIdleTime(gpGlobals->curtime + flDuration);
}

// The model's own rack between the first insert and the rest of the reload; without it the reload ends with the
// shipped rack
bool CWeaponSupa7::UsesPumpAfterInsert() const
{
	return IsTubeEnabled() && LookupViewModelSequence(VM_SEQ_PUMP_AFTER_INSERT) != ACTIVITY_NOT_AVAILABLE;
}

// The shot that empties the tube has no round to rack up
bool CWeaponSupa7::IsLastShotWithoutPump() const
{
	return m_nTubeCount <= 0 && UsesPumpAfterInsert();
}

// Racks the round just loaded over the hull, then the reload goes on from the loading pose
void CWeaponSupa7::StartPumpAfterInsert()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return;
	}

	SetActivity(ACT_SHOTGUN_PUMP);
	SendViewModelAnim(LookupViewModelSequence(VM_SEQ_PUMP_AFTER_INSERT));
	WeaponSound(SPECIAL2);

	const float flDuration = ViewModelSequenceDuration();
	m_flRackFeedTime = gpGlobals->curtime + Min(PUMP_AFTER_INSERT_HOME_TIME, flDuration);
	m_flNextReload = gpGlobals->curtime + flDuration;
	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(m_flRackFeedTime);
}

void CWeaponSupa7::RunRack()
{
	if (m_flRackFeedTime <= 0.0f || gpGlobals->curtime < m_flRackFeedTime)
	{
		return;
	}

	m_flRackFeedTime = 0.0f;
	FeedChamber();
}

// The viewmodel plays the sequence; the weapon entity can carry the world model on the client
float CWeaponSupa7::ViewModelSequenceDuration() const
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	CBaseViewModel *pVM = pOwner ? pOwner->GetViewModel(m_nViewModelIndex) : nullptr;
	return pVM ? pVM->SequenceDuration() : 0.0f;
}

int CWeaponSupa7::LookupViewModelSequence(const char *pszSequence) const
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	CBaseViewModel *pVM = pOwner ? pOwner->GetViewModel(m_nViewModelIndex) : nullptr;
	return pVM ? pVM->LookupSequence(pszSequence) : ACTIVITY_NOT_AVAILABLE;
}

// Played as WeaponSound plays: predicted for the owner, from the server for the rest
void CWeaponSupa7::EmitShotSoundPiece(const char *pszSound)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	CSoundParameters params;
	if (!pOwner || !GetParametersForSound(pszSound, params, NULL))
	{
		return;
	}

	CPASAttenuationFilter filter(pOwner, params.soundlevel);
	if (IsPredicted() && CBaseEntity::GetPredictionPlayer())
	{
		filter.UsePredictionRules();
	}
	EmitSound(filter, pOwner->entindex(), pszSound);
}

bool CWeaponSupa7::Holster(CBaseCombatWeapon *pSwitchingTo)
{
	TubePhasesOnHolster();
	return BaseClass::Holster(pSwitchingTo);
}

bool CWeaponSupa7::HasChamberSlot() const
{
	return IsTubeEnabled() || BaseClass::HasChamberSlot();
}

bool CWeaponSupa7::IsRoundChambered() const
{
	if (!IsTubeEnabled())
	{
		return BaseClass::IsRoundChambered();
	}

	// before the first update rebuilds the tube from a new clip count, a loaded gun has a round chambered
	return IsTubeSynced() ? IsChamberLive() : (m_iClip1 > 0);
}

int CWeaponSupa7::RoundsInMagazine() const
{
	if (!IsTubeEnabled())
	{
		return BaseClass::RoundsInMagazine();
	}

	return IsTubeSynced() ? m_nTubeCount.Get() : Max(0, m_iClip1 - SUPA7_CHAMBER_ROUNDS);
}

int CWeaponSupa7::MagazineCapacity() const
{
	return IsTubeEnabled() ? TubeCapacity() : BaseClass::MagazineCapacity();
}

const WeaponSpreadInfo_t &CWeaponSupa7::GetSpreadInfo()
{
	Assert(m_weaponHandling.weaponID & GetNeoWepBits());
	return m_weaponHandling.spreadInfo[m_bSlugLoaded];
}

// For the fire-mode letter: the type a reload is loading, otherwise the chambered round's
bool CWeaponSupa7::SlugLoaded() const
{
	if (IsTubeEnabled() && m_bInReload)
	{
		return m_bSlugLoad;
	}
	return m_bSlugLoaded;
}

void CWeaponSupa7::Drop(const Vector& vecVelocity)
{
	ClearDelayedInputs();
	CNEOBaseCombatWeapon::Drop(vecVelocity);
}

bool CWeaponSupa7::CanBePickedUpByClass(int classId)
{
	return classId != NEO_CLASS_JUGGERNAUT;
}
