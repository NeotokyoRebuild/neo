#include "cbase.h"
#include "weapon_supa7.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSupa7, DT_WeaponSupa7)

BEGIN_NETWORK_TABLE(CWeaponSupa7, DT_WeaponSupa7)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bSlugDelayed)),
	RecvPropBool(RECVINFO(m_bSlugLoaded)),
	RecvPropBool(RECVINFO(m_bWeaponRaised)),
	RecvPropBool(RECVINFO(m_bShellInChamber)),
#else
	SendPropBool(SENDINFO(m_bSlugDelayed)),
	SendPropBool(SENDINFO(m_bSlugLoaded)),
	SendPropBool(SENDINFO(m_bWeaponRaised)),
	SendPropBool(SENDINFO(m_bShellInChamber)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSupa7)
	DEFINE_PRED_FIELD(m_bSlugDelayed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bSlugLoaded, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bWeaponRaised, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bShellInChamber, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_supa7, CWeaponSupa7);

PRECACHE_WEAPON_REGISTER(weapon_supa7);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponSupa7)
	DEFINE_FIELD(m_bSlugDelayed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bSlugLoaded, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bWeaponRaised, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bShellInChamber, FIELD_BOOLEAN),
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

CWeaponSupa7::CWeaponSupa7(void)
{
	m_bReloadsSingly = true;
	m_bSlugDelayed = false;
	m_bSlugLoaded = false;
	m_bWeaponRaised = false;
	m_bShellInChamber = true;

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
	if (m_bSlugLoaded)
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

	if (m_iPrimaryAmmoCount <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	SetShotgunShellVisible(true);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());

	m_bInReload = true;
	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::StartReloadSlug(void)
{
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

	if (m_iPrimaryAmmoCount <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_RELOAD);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	constexpr float TIME_BETWEEN_SHELLS_LOADED = 0.5f;
	ProposeNextAttack(gpGlobals->curtime + TIME_BETWEEN_SHELLS_LOADED);

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

	FillClipSlug();
	m_bShellInChamber = false;

	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(SPECIAL1);
	SendWeaponAnim(ACT_VM_RELOAD);
	pOwner->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
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
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
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

	int numBullets = 7;
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
		WeaponSound(SPECIAL2);
	}
	else
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE);
	}

	FireBulletsInfo_t info(numBullets, vecSrc, vecAiming, bulletSpread, MAX_TRACE_LENGTH, ammoType);
	if (m_bSlugLoaded)
	{
		constexpr float NEO_SLUG_PENETRATION_VALUE = 32.f;
		info.m_flPenetration = NEO_SLUG_PENETRATION_VALUE;
	}
	info.m_pAttacker = pPlayer;
	info.m_iTracerFreq = 0;

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(m_bSlugLoaded ? ACT_VM_PRIMARYATTACK : ACT_VM_SECONDARYATTACK);
	m_bSlugLoaded = false;

	// Don't fire again until fire animation has completed
	ProposeNextAttack(gpGlobals->curtime + GetFireRate());
	m_iClip1 -= 1;
	if (m_iClip1 == 0)
	{
		m_bShellInChamber = false;
	}

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	// Fire the bullets, and force the first shot to be perfectly accurate
	pPlayer->FireBullets(info);

	if (!m_iClip1 && m_iPrimaryAmmoCount <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	pPlayer->ViewPunchReset();
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

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading
		if ((pOwner->m_nButtons & IN_ATTACK) && (m_iClip1 >= 1))
		{
			// OG cues a shotgun rack when beginning a reload from a completely empty clip. You can however fire without racking the shotgun, and it will instead rack the next time you reload (if you don't cancel it by shooting again)
			// This is a slight nerf to the supa where the gun has to be racked before it is fired
			FinishReload();
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If we're supposed to have a slug loaded, load it
			if (m_bSlugDelayed)
			{
				if (!ReloadSlug())
				{
					m_bSlugDelayed.GetForModify() = false;
				}
			}
			else
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
		StartReload();
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

const WeaponSpreadInfo_t &CWeaponSupa7::GetSpreadInfo()
{
	Assert(m_weaponHandling.weaponID & GetNeoWepBits());
	return m_weaponHandling.spreadInfo[m_bSlugLoaded];
}

bool CWeaponSupa7::SlugLoaded() const
{
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