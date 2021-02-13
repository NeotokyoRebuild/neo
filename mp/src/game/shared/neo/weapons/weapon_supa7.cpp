#include "cbase.h"
#include "weapon_supa7.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(WeaponSupa7, DT_WeaponSupa7)

BEGIN_NETWORK_TABLE(CWeaponSupa7, DT_WeaponSupa7)
#ifdef CLIENT_DLL
	RecvPropBool(RECVINFO(m_bNeedPump)),
	RecvPropBool(RECVINFO(m_bDelayedFire1)),
	RecvPropBool(RECVINFO(m_bDelayedFire2)),
	RecvPropBool(RECVINFO(m_bDelayedReload)),
	RecvPropBool(RECVINFO(m_bSlugDelayed)),
	RecvPropBool(RECVINFO(m_bSlugLoaded)),
#else
	SendPropBool(SENDINFO(m_bNeedPump)),
	SendPropBool(SENDINFO(m_bDelayedFire1)),
	SendPropBool(SENDINFO(m_bDelayedFire2)),
	SendPropBool(SENDINFO(m_bDelayedReload)),
	SendPropBool(SENDINFO(m_bSlugDelayed)),
	SendPropBool(SENDINFO(m_bSlugLoaded)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponSupa7)
	DEFINE_PRED_FIELD(m_bNeedPump, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bDelayedFire1, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bDelayedFire2, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bDelayedReload, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bSlugDelayed, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bSlugLoaded, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

//NEO_IMPLEMENT_ACTTABLE(CWeaponSupa7)

LINK_ENTITY_TO_CLASS(weapon_supa7, CWeaponSupa7);

PRECACHE_WEAPON_REGISTER(weapon_supa7);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponSupa7)
	DEFINE_FIELD(m_bNeedPump, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bDelayedFire1, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bDelayedFire2, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bDelayedReload, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bSlugDelayed, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bSlugLoaded, FIELD_BOOLEAN),
END_DATADESC()
#endif

extern ConVar sk_auto_reload_time;
extern ConVar sk_plr_num_shotgun_pellets;

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

// Purpose: Only update next attack time if it's further away in the future.
inline void CWeaponSupa7::ProposeNextAttack(const float flNextAttackProposal)
{
	if (m_flNextPrimaryAttack < flNextAttackProposal)
	{
		m_flNextPrimaryAttack = flNextAttackProposal;
	}
}

CWeaponSupa7::CWeaponSupa7(void)
{
	m_bReloadsSingly = true;

	m_bNeedPump = false;
	m_bDelayedFire1 = false;
	m_bDelayedFire2 = false;
	m_bSlugDelayed = false;
	m_bSlugLoaded = false;

	m_fMinRange1 = 0.0;
	m_fMaxRange1 = 500;
	m_fMinRange2 = 0.0;
	m_fMaxRange2 = 200;
}

// Purpose: Override so only reload one shell at a time
bool CWeaponSupa7::StartReload(void)
{
	if (m_bNeedPump)
		return false;

	if (m_bSlugLoaded || m_bSlugDelayed)
		return false;

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	// (brekiy) seems redundant to me
#if(0)
	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;
#endif
	SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);

	// Make shotgun shell visible
	SetBodygroup(1, 0);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());

	m_bInReload = true;
	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::StartReloadSlug(void)
{
	if (m_bNeedPump)
		return false;

	if (m_bSlugLoaded)
		return false;

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
	{
		return false;
	}

	if (!m_bInReload)
	{
		SendWeaponAnim(ACT_SHOTGUN_RELOAD_START);
		m_bInReload = true;
	}

	if (m_iClip1 >= GetMaxClip1())
		return false;

	m_bSlugDelayed = true;

	// Make shotgun shell visible
	SetBodygroup(1, 0);

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
		Warning("ERROR: Supa7 Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	int j = MIN(1, pOwner->GetAmmoCount(m_iPrimaryAmmoType));

	if (j <= 0)
		return false;

	FillClip();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim(ACT_VM_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());

	return true;
}

// Purpose: Start loading a slug, avoid overriding the default reload functionality due to secondary ammo
bool CWeaponSupa7::ReloadSlug(void)
{
	// Check that StartReload was called first
	if (!m_bInReload)
	{
		Warning("ERROR: Supa7 Reload called incorrectly!\n");
	}

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return false;

	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) <= 0)
		return false;

	if (m_iClip1 >= GetMaxClip1())
		return false;

	FillClipSlug();
	// Play reload on different channel as otherwise steals channel away from fire sound
	WeaponSound(RELOAD);
	SendWeaponAnim(ACT_VM_RELOAD);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
	return true;
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FinishReload(void)
{
	// Make shotgun shell invisible
	SetBodygroup(1, 1);

	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bInReload = false;

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_RELOAD_FINISH);

	pOwner->m_flNextAttack = gpGlobals->curtime;
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FillClip(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	// Add them to the clip
	if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) > 0)
	{
		if (Clip1() < GetMaxClip1())
		{
			m_iClip1++;
			pOwner->RemoveAmmo(1, m_iPrimaryAmmoType);
		}
	}
}

// Purpose: Play finish reload anim and fill clip
void CWeaponSupa7::FillClipSlug(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;
	// Add them to the clip
	if (pOwner->GetAmmoCount(m_iSecondaryAmmoType) > 0 && !m_bSlugLoaded)
	{
		m_iClip1++;
		pOwner->RemoveAmmo(1, m_iSecondaryAmmoType);
		m_bSlugLoaded = true;
		m_bSlugDelayed = false;
	}
}

// Purpose: Play weapon pump anim
void CWeaponSupa7::Pump(void)
{
	CBaseCombatCharacter *pOwner = GetOwner();

	if (pOwner == NULL)
		return;

	m_bNeedPump = false;

	if (m_bDelayedReload)
	{
		m_bDelayedReload = false;
		if (m_bSlugDelayed)
		{
			StartReloadSlug();
		}
		else {
			StartReload();
		}
	}

	WeaponSound(SPECIAL1);

	// Finish reload animation
	SendWeaponAnim(ACT_SHOTGUN_PUMP);

	pOwner->m_flNextAttack = gpGlobals->curtime + SequenceDuration();
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
}

void CWeaponSupa7::DryFire(void)
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);

	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
}

void CWeaponSupa7::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	pPlayer->ViewPunchReset();

	// MUST call sound before removing a round from the clip of a CMachineGun
	WeaponSound(SINGLE);

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(ACT_VM_PRIMARYATTACK);

	// Don't fire again until fire animation has completed
	ProposeNextAttack(gpGlobals->curtime + SequenceDuration());
	m_iClip1 -= 1;

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	Vector vecSrc = pPlayer->Weapon_ShootPosition();
	Vector vecAiming = pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

	int numBullets = 7;
	Vector bulletSpread = GetBulletSpread();
	int ammoType = m_iPrimaryAmmoType;
	if (m_bSlugLoaded) {
		numBullets = 1;
		bulletSpread *= 0.5;
		ammoType = m_iSecondaryAmmoType;
		m_bSlugLoaded = false;
	}
	FireBulletsInfo_t info(numBullets, vecSrc, vecAiming, bulletSpread, MAX_TRACE_LENGTH, ammoType);
	info.m_pAttacker = pPlayer;

	// Fire the bullets, and force the first shot to be perfectly accurate
	pPlayer->FireBullets(info);

	if (!m_iClip1 && pPlayer->GetAmmoCount(m_iPrimaryAmmoType) <= 0)
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
	return;
}

// Purpose: Override so shotgun can do multiple reloads in a row
void CWeaponSupa7::ItemPostFrame(void)
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
	{
		return;
	}

	if (m_bNeedPump)
	{
		if (pOwner->m_nButtons & IN_RELOAD)
		{
			m_bDelayedReload = true;
			return;
		}
		else if (pOwner->m_nButtons & IN_ATTACK2)
		{
			m_bDelayedReload = true;
			m_bSlugDelayed = true;
			return;
		}
	}

	if (m_bInReload)
	{
		// If I'm primary firing and have one round stop reloading and fire
		if ((pOwner->m_nButtons & IN_ATTACK) && (m_iClip1 >= 1) && !m_bNeedPump)
		{
			m_bInReload = false;
			m_bNeedPump = false;
			m_bDelayedFire1 = true;
		}
		// If I'm secondary firing and am not already trying to load a slug queue one
		else if ((pOwner->m_nButtons & IN_ATTACK2) && (m_iClip1 < GetMaxClip1()) && !m_bNeedPump && !m_bSlugDelayed)
		{
			m_bSlugDelayed = true;
			m_bDelayedFire2 = true;
		}
		else if (m_flNextPrimaryAttack <= gpGlobals->curtime)
		{
			// If out of ammo end reload
			if (pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 || m_bSlugLoaded || m_iClip1 >= GetMaxClip1())
			{
				FinishReload();
				return;
			}
			else
			{
				// If we're supposed to have a slug loaded, load it
				if (m_bSlugDelayed)
				{
					ReloadSlug();
					return;
				}
				else
				{
					Reload();
					return;
				}
			}
		}
	}
	else
	{
		// Make shotgun shell invisible
		SetBodygroup(1, 1);
	}

	if ((m_bNeedPump) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		Pump();
		return;
	}

	// Shotgun uses same timing and ammo for secondary attack
	if ((m_bDelayedFire2 || pOwner->m_nButtons & IN_ATTACK2) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		m_bDelayedFire2 = false;

		if (pOwner->GetAmmoCount(m_iSecondaryAmmoType))
		{
			// If the firing button was just pressed, reset the firing time
			if (pOwner->m_afButtonPressed & IN_ATTACK2)
			{
				ProposeNextAttack(gpGlobals->curtime);
			}
			SecondaryAttack();
		}
	}
	else if ((m_bDelayedFire1 || pOwner->m_nButtons & IN_ATTACK) && m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		m_bDelayedFire1 = false;
		if ((m_iClip1 <= 0 && UsesClipsForAmmo1()) || (!UsesClipsForAmmo1() && !pOwner->GetAmmoCount(m_iPrimaryAmmoType)))
		{
			if (!pOwner->GetAmmoCount(m_iPrimaryAmmoType))
			{
				DryFire();
			}
			else
			{
				StartReload();
			}
		}
		// Fire underwater?
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			WeaponSound(EMPTY);
			ProposeNextAttack(gpGlobals->curtime + GetFastestDryRefireTime());
			return;
		}
		else
		{
			// If the firing button was just pressed, reset the firing time
			CBasePlayer *pPlayer = ToBasePlayer(GetOwner());
			if (pPlayer && pPlayer->m_afButtonPressed & IN_ATTACK)
			{
				ProposeNextAttack(gpGlobals->curtime);
			}
			PrimaryAttack();
		}
	}

	if (pOwner->m_nButtons & IN_RELOAD && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		StartReload();
	}
	else if (pOwner->m_nButtons & IN_ATTACK2 && UsesClipsForAmmo1() && !m_bInReload)
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
		else
		{
			// weapon is useable. Reload if empty and weapon has waited as long as it has to after firing
			if (m_iClip1 <= 0 && !(GetWeaponFlags() & ITEM_FLAG_NOAUTORELOAD) && m_flNextPrimaryAttack < gpGlobals->curtime)
			{
				if (StartReload())
				{
					// if we've successfully started to reload, we're done
					return;
				}
			}
		}

		WeaponIdle();
		return;
	}
}

void CWeaponSupa7::AddViewKick(void)
{
	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	if (pPlayer == NULL)
		return;

	QAngle punch;
	punch.Init(SharedRandomFloat("supapax", -2, -1), SharedRandomFloat("supapay", -1, 1), 0);
	pPlayer->ViewPunch(punch);
}

#if(0)
void CWeaponSupa7::ItemHolsterFrame(void)
{
	// Must be player held
	if (GetOwner() && GetOwner()->IsPlayer() == false)
		return;

	// We can't be active
	if (GetOwner()->GetActiveWeapon() == this)
		return;

	// If it's been longer than three seconds, reload
	if ((gpGlobals->curtime - m_flHolsterTime) > sk_auto_reload_time.GetFloat())
	{
		// Reset the timer
		m_flHolsterTime = gpGlobals->curtime;

		if (GetOwner() == NULL)
			return;

		if (m_iClip1 == GetMaxClip1())
			return;

		// Just load the clip with no animations
		int ammoFill = MIN((GetMaxClip1() - m_iClip1), GetOwner()->GetAmmoCount(GetPrimaryAmmoType()));

		GetOwner()->RemoveAmmo(ammoFill, GetPrimaryAmmoType());
		m_iClip1 += ammoFill;
	}
}
#endif

#if(0)
void CWeaponSupa7::WeaponIdle( void )
{
	//Only the player fires this way so we can cast
	CBasePlayer *pPlayer = GetOwner()

	if ( pPlayer == NULL )
		return;

	//If we're on a target, play the new anim
	if ( pPlayer->IsOnTarget() )
	{
		SendWeaponAnim( ACT_VM_IDLE_ACTIVE );
	}
}
#endif
