#include "cbase.h"
#include "weapon_neobasecombatweapon.h"
#include "particle_parse.h"

#include "in_buttons.h"

#ifdef GAME_DLL
#include "player.h"

extern ConVar weaponstay;
#endif

#ifdef CLIENT_DLL
#include "dlight.h"
#include "iefx.h"
#include "c_te_effect_dispatch.h"
#endif

#include "basecombatweapon_shared.h"

#include <initializer_list>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( NEOBaseCombatWeapon, DT_NEOBaseCombatWeapon )

BEGIN_NETWORK_TABLE( CNEOBaseCombatWeapon, DT_NEOBaseCombatWeapon )
#ifdef CLIENT_DLL
	RecvPropTime(RECVINFO(m_flSoonestAttack)),
	RecvPropTime(RECVINFO(m_flLastAttackTime)),
	RecvPropFloat(RECVINFO(m_flAccuracyPenalty)),
	RecvPropInt(RECVINFO(m_nNumShotsFired)),
	RecvPropBool(RECVINFO(m_bRoundChambered)),
	RecvPropBool(RECVINFO(m_bRoundBeingChambered)),
#else
	SendPropTime(SENDINFO(m_flSoonestAttack)),
	SendPropTime(SENDINFO(m_flLastAttackTime)),
	SendPropFloat(SENDINFO(m_flAccuracyPenalty)),
	SendPropInt(SENDINFO(m_nNumShotsFired)),
	SendPropBool(SENDINFO(m_bRoundChambered)),
	SendPropBool(SENDINFO(m_bRoundBeingChambered)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOBaseCombatWeapon)
	DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bRoundChambered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bRoundBeingChambered, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neobasecombatweapon, CNEOBaseCombatWeapon);

#ifdef GAME_DLL
BEGIN_DATADESC( CNEOBaseCombatWeapon )
	DEFINE_FIELD(m_flSoonestAttack, FIELD_TIME),
	DEFINE_FIELD(m_flLastAttackTime, FIELD_TIME),
	DEFINE_FIELD(m_flAccuracyPenalty, FIELD_FLOAT),
	DEFINE_FIELD(m_nNumShotsFired, FIELD_INTEGER),
	DEFINE_FIELD(m_bRoundChambered, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bRoundBeingChambered, FIELD_BOOLEAN),
END_DATADESC()
#endif

const char *GetWeaponByLoadoutId(int id)
{
	if (id < 0 || id >= NEO_WEP_LOADOUT_ID_COUNT)
	{
		Assert(false);
		return "";
	}

	const char* weps[] = {
		"weapon_mpn",
		"weapon_srm",
		"weapon_srm_s",
		"weapon_jitte",
		"weapon_jittescoped",
		"weapon_zr68c",
		"weapon_zr68s",
		"weapon_zr68l",
		"weapon_mx",
		"weapon_pz",
		"weapon_supa7",
		"weapon_m41",
		"weapon_m41l",
	};

	COMPILE_TIME_ASSERT(NEO_WEP_LOADOUT_ID_COUNT == ARRAYSIZE(weps));

	return weps[id];
}

CNEOBaseCombatWeapon::CNEOBaseCombatWeapon( void )
{
}

void CNEOBaseCombatWeapon::Precache()
{
	BaseClass::Precache();

	if ((GetNeoWepBits() & NEO_WEP_SUPPRESSED))
		PrecacheParticleSystem("ntr_muzzle_source");
}

void CNEOBaseCombatWeapon::Spawn()
{
	// If this fires, either the enum bit mask has overflowed,
	// this derived gun has no valid NeoBitFlags set,
	// or we are spawning an instance of this base class for some reason.
	Assert(GetNeoWepBits() != NEO_WEP_INVALID); 

	BaseClass::Spawn();

	m_iClip1 = GetWpnData().iMaxClip1;
	m_iPrimaryAmmoCount = GetWpnData().iDefaultClip1;

	m_iClip2 = GetWpnData().iMaxClip2;
	m_iSecondaryAmmoCount = GetWpnData().iDefaultClip2;

#ifdef GAME_DLL
	AddSpawnFlags(SF_NORESPAWN);
#endif
}

void CNEOBaseCombatWeapon::Equip(CBaseCombatCharacter* pOwner)
{
	BaseClass::Equip(pOwner);
#ifndef CLIENT_DLL
	NEO_WEP_BITS_UNDERLYING_TYPE weapon = GetNeoWepBits();
	if (weapon & (NEO_WEP_KYLA | NEO_WEP_MILSO | NEO_WEP_TACHI))
		SetParentAttachment("SetParentAttachment", "pistol", false);
	else if (weapon & NEO_WEP_FRAG_GRENADE)
	{
		SetLocalOrigin(Vector(-1.5, 5, -1.5));
		SetLocalAngles(QAngle(0, 0, 90));
		SetParentAttachment("SetParentAttachment", "defusekit", true);
	}
	else if (weapon & NEO_WEP_SMOKE_GRENADE)
	{
		SetLocalOrigin(Vector(-1.5, 5, -3.5));
		SetLocalAngles(QAngle(0, 0, 90));
		SetParentAttachment("SetParentAttachment", "defusekit", true);
	}
	else if (weapon & NEO_WEP_DETPACK)
	{
		SetLocalOrigin(Vector(3, 1, -9));
		SetLocalAngles(QAngle(-50, -30, 90));
		SetParentAttachment("SetParentAttachment", "defusekit", true);
	}
	else if (weapon & NEO_WEP_KNIFE)
	{
		return;
	}
	else
		SetParentAttachment("SetParentAttachment", "primary", false);
#endif
}


bool CNEOBaseCombatWeapon::Reload( void )
{
	return BaseClass::Reload();

#if(0)
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );
	if (!pOwner)
	{
		return false;
	}

	if (pOwner->m_afButtonPressed & IN_RELOAD)
	{
		return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
	}

#ifdef CLIENT_DLL
	if (!ClientWantsAutoReload())
	{
		return false;
	}
#else
	if (!ClientWantsAutoReload(pOwner))
	{
		return false;
	}
#endif

	return DefaultReload( GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD );
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CNEOBaseCombatWeapon::FinishReload(void)
{
	CBaseCombatCharacter* pOwner = GetOwner();

	if (pOwner)
	{
		// If I use primary clips, reload primary
		if (UsesClipsForAmmo1())
		{
			int primary = MIN(GetMaxClip1(), m_iPrimaryAmmoCount);
			m_iClip1 = primary;
			m_iPrimaryAmmoCount -= primary;
		}

		// If I use secondary clips, reload secondary
		if (UsesClipsForAmmo2())
		{
			int secondary = MIN(GetMaxClip2() - m_iClip2, m_iSecondaryAmmoCount);
			m_iClip2 += secondary;
			m_iSecondaryAmmoCount -= secondary;
		}

		if (m_bReloadsSingly)
		{
			m_bInReload = false;
		}
	}
}

bool CNEOBaseCombatWeapon::CanBeSelected(void)
{
	if (GetWeaponFlags() & ITEM_FLAG_NOAUTOSWITCHEMPTY)
	{
		return true;
	}

	return BaseClass::CanBeSelected();
}

bool CNEOBaseCombatWeapon::Deploy(void)
{
	const bool ret = BaseClass::Deploy();

	if (ret)
	{
		AddEffects(EF_BONEMERGE);

#ifdef DEBUG
		CNEO_Player* pOwner = NULL;
		if (GetOwner())
		{
			pOwner = dynamic_cast<CNEO_Player*>(GetOwner());
			Assert(pOwner);
		}
#else
		auto pOwner = static_cast<CNEO_Player*>(GetOwner());
#endif

		if (pOwner)
		{
			if (pOwner->GetFlags() & FL_DUCKING)
			{
				pOwner->SetMaxSpeed(pOwner->GetCrouchSpeed_WithWepEncumberment(this));
			}
			else if (pOwner->IsWalking())
			{
				pOwner->SetMaxSpeed(pOwner->GetWalkSpeed_WithWepEncumberment(this));
			}
			else if (pOwner->IsSprinting())
			{
				pOwner->SetMaxSpeed(pOwner->GetSprintSpeed_WithWepEncumberment(this));
			}
			else
			{
				pOwner->SetMaxSpeed(pOwner->GetNormSpeed_WithWepEncumberment(this));
			}
		}
	}

	return ret;
}

float CNEOBaseCombatWeapon::GetFireRate()
{
	return GetHL2MPWpnData().m_flCycleTime;
}

float CNEOBaseCombatWeapon::GetPenetration() const
{
	return GetWpnData().m_flPenetration;
}

#ifdef CLIENT_DLL
bool CNEOBaseCombatWeapon::Holster(CBaseCombatWeapon* pSwitchingTo)
{
#ifdef DEBUG
	CNEO_Player* pOwner = NULL;
	if (GetOwner())
	{
		pOwner = dynamic_cast<CNEO_Player*>(GetOwner());
		Assert(pOwner);
	}
#else
	auto pOwner = static_cast<CNEO_Player*>(GetOwner());
#endif

	if (pOwner)
	{
		pOwner->Weapon_SetZoom(false);
	}

	return BaseClass::Holster(pSwitchingTo);
}

void CNEOBaseCombatWeapon::ItemHolsterFrame(void)
{ // Overrides the base class behaviour of reloading the weapon after its been holstered for 3 seconds
	return;
}
#endif

void CNEOBaseCombatWeapon::CheckReload(void)
{
	if (!m_bInReload && UsesClipsForAmmo1() && m_iClip1 == 0 && GetOwner() && !ClientWantsAutoReload(GetOwner()))
	{
		return;
	}

	BaseClass::CheckReload();
}

// Handles lowering the weapon view model when character is sprinting
void CNEOBaseCombatWeapon::ProcessAnimationEvents(void)
{
	CNEO_Player* pOwner = static_cast<CNEO_Player*>(ToBasePlayer(GetOwner()));
	if (!pOwner)
		return;

	if (!m_bLowered && (pOwner->IsSprinting()) && !m_bInReload && !m_bRoundBeingChambered)
	{
		m_bLowered = true;
		SendWeaponAnim(ACT_VM_IDLE_LOWERED);
		m_flNextPrimaryAttack = max(gpGlobals->curtime + 0.2, m_flNextPrimaryAttack);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	}
	else if (m_bLowered && !(pOwner->IsSprinting()))
	{
		m_bLowered = false;
		SendWeaponAnim(ACT_VM_IDLE);
		m_flNextPrimaryAttack = max(gpGlobals->curtime + 0.2, m_flNextPrimaryAttack);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	}
	else if (m_bLowered && m_bRoundBeingChambered)
	{ // For bolt action weapons
		m_bLowered = false;
		SendWeaponAnim(ACT_VM_PULLBACK);
		m_flNextPrimaryAttack = max(gpGlobals->curtime + 0.2, m_flNextPrimaryAttack);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	}

	if (m_bLowered)
	{
		if (gpGlobals->curtime > m_flNextPrimaryAttack)
		{
			SendWeaponAnim(ACT_VM_IDLE_LOWERED);
			m_flNextPrimaryAttack = max(gpGlobals->curtime + 0.2, m_flNextPrimaryAttack);
			m_flNextSecondaryAttack = m_flNextPrimaryAttack;
		}
	}
}

void CNEOBaseCombatWeapon::ItemPostFrame(void)
{
	CNEO_Player* pOwner = static_cast<CNEO_Player*>(ToBasePlayer(GetOwner()));
	if (!pOwner)
		return;

	UpdateAutoFire();

	//Track the duration of the fire
	//FIXME: Check for IN_ATTACK2 as well?
	//FIXME: What if we're calling ItemBusyFrame?
	m_fFireDuration = (pOwner->m_nButtons & IN_ATTACK) ? (m_fFireDuration + gpGlobals->frametime) : 0.0f;

	if (UsesClipsForAmmo1())
	{
		CheckReload();
	}

	bool bFired = false;

	// Secondary attack has priority
	if ((pOwner->m_nButtons & IN_ATTACK2) && CanPerformSecondaryAttack())
	{
		if (UsesSecondaryAmmo() && m_iSecondaryAmmoCount <= 0)
		{
			if (m_flNextEmptySoundTime < gpGlobals->curtime)
			{
				WeaponSound(EMPTY);
				m_flNextSecondaryAttack = m_flNextEmptySoundTime = gpGlobals->curtime + 0.5;
			}
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bAltFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			// FIXME: This isn't necessarily true if the weapon doesn't have a secondary fire!
			// For instance, the crossbow doesn't have a 'real' secondary fire, but it still 
			// stops the crossbow from firing on the 360 if the player chooses to hold down their
			// zoom button. (sjb) Orange Box 7/25/2007
#if !defined(CLIENT_DLL)
			if (!IsX360() || !ClassMatches("weapon_crossbow"))
#endif
			{
				bFired = ShouldBlockPrimaryFire();
			}

			SecondaryAttack();

			// Secondary ammo doesn't have a reload animation
			if (UsesClipsForAmmo2())
			{
				// reload clip2 if empty
				if (m_iClip2 < 1)
				{
					m_iSecondaryAmmoCount -= 1;
					m_iClip2 = m_iClip2 + 1;
				}
			}
		}
	}

	if (!bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if (!IsMeleeWeapon() &&
			((UsesClipsForAmmo1() && m_iClip1 <= 0) || (!UsesClipsForAmmo1() && m_iPrimaryAmmoCount <= 0)))
		{
			if (m_bRoundChambered) // bolt action rifles can have this value set to false, prevents empty clicking when holding the attack button when looking through scope to prevent bolting/reloading
				HandleFireOnEmpty();
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			//NOTENOTE: There is a bug with this code with regards to the way machine guns catch the leading edge trigger
			//			on the player hitting the attack key.  It relies on the gun catching that case in the same frame.
			//			However, because the player can also be doing a secondary attack, the edge trigger may be missed.
			//			We really need to hold onto the edge trigger and only clear the condition when the gun has fired its
			//			first shot.  Right now that's too much of an architecture change -- jdw

			// If the firing button was just pressed, or the alt-fire just released, reset the firing time
			if ((pOwner->m_afButtonPressed & IN_ATTACK) || (pOwner->m_afButtonReleased & IN_ATTACK2))
			{
				m_flNextPrimaryAttack = gpGlobals->curtime;
			}

			PrimaryAttack();

			if (AutoFiresFullClip())
			{
				m_bFiringWholeClip = true;
			}

#ifdef CLIENT_DLL
			pOwner->SetFiredWeapon(true);
#endif
		}
	}

	// -----------------------
	//  Reload pressed / Clip Empty
	//  Can only start the Reload Cycle after the firing cycle
	if ((pOwner->m_nButtons & IN_RELOAD) && m_flNextPrimaryAttack <= gpGlobals->curtime && UsesClipsForAmmo1() && !m_bInReload)
	{
		// reload when reload is pressed, or if no buttons are down and weapon is empty.
		Reload();
		m_fFireDuration = 0.0f;
	}

	// -----------------------
	//  No buttons down
	// -----------------------
	if (!(((pOwner->m_nButtons & IN_ATTACK) && !(pOwner->IsSprinting())) || (pOwner->m_nButtons & IN_ATTACK2) || (CanReload() && pOwner->m_nButtons & IN_RELOAD)))
	{
		// no fire buttons down or reloading
		if (!ReloadOrSwitchWeapons() && (m_bInReload == false))
		{
			WeaponIdle();
		}
	}
}

ConVar sv_neo_wep_acc_penalty_scale("sv_neo_wep_acc_penalty_scale", "7.5", FCVAR_REPLICATED,
	"Temporary global neo wep accuracy penalty scaler.", true, 0.01, true, 9999.0);

ConVar sv_neo_wep_cone_min_scale("sv_neo_wep_cone_min_scale", "0.01", FCVAR_REPLICATED,
	"Temporary global neo wep bloom min cone scaler.", true, 0.01, true, 10.0);

ConVar sv_neo_wep_cone_max_scale("sv_neo_wep_cone_max_scale", "0.7", FCVAR_REPLICATED,
	"Temporary global neo wep bloom max cone scaler.", true, 0.01, true, 10.0);

// NEO HACK/FIXME (Rain): Doing some temporary bloom accuracy scaling here for easier testing.
// Need to clean this up later once we have good values!!
#define TEMP_WEP_STR(name) #name
#define MAKE_TEMP_WEP_BLOOM_SCALER(weapon, defval) ConVar sv_neo_##weapon##_bloom_scale(TEMP_WEP_STR(sv_neo_##weapon##_bloom_scale), #defval, FCVAR_REPLICATED, TEMP_WEP_STR(Temporary weapon bloom scaler for #weapon), true, 0.01, true, 9999.0)

MAKE_TEMP_WEP_BLOOM_SCALER(weapon_jitte,			2);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_jittescoped,		2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_kyla,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_m41,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_m41l,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_m41s,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_milso,			2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_mpn,				20.0);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_mpn_unsilenced,	4.0);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_mx,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_mx_silenced,		2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_pz,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_smac,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_srm,				2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_srm_s,			2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_tachi,			2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_zr68c,			2.5);
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_zr68s,			2.5);
#ifdef INCLUDE_WEP_PBK
MAKE_TEMP_WEP_BLOOM_SCALER(weapon_pbk56s,			2.5);
#endif

const Vector& CNEOBaseCombatWeapon::GetBulletSpread(void)
{
	static Vector cone;

	// NEO HACK/FIXME (Rain): Doing some temporary bloom accuracy scaling here for easier testing.
	// Need to clean this up later once we have good values!!
	const std::initializer_list<ConVar*> bloomScalers = {
		&sv_neo_weapon_jitte_bloom_scale,
		&sv_neo_weapon_jittescoped_bloom_scale,
		&sv_neo_weapon_kyla_bloom_scale,
		&sv_neo_weapon_m41_bloom_scale,
		&sv_neo_weapon_m41l_bloom_scale,
		&sv_neo_weapon_m41s_bloom_scale,
		&sv_neo_weapon_milso_bloom_scale,
		&sv_neo_weapon_mpn_bloom_scale,
		&sv_neo_weapon_mpn_unsilenced_bloom_scale,
		&sv_neo_weapon_mx_bloom_scale,
		&sv_neo_weapon_mx_silenced_bloom_scale,
		&sv_neo_weapon_pz_bloom_scale,
		&sv_neo_weapon_smac_bloom_scale,
		&sv_neo_weapon_srm_bloom_scale,
		&sv_neo_weapon_srm_s_bloom_scale,
		&sv_neo_weapon_tachi_bloom_scale,
		&sv_neo_weapon_zr68c_bloom_scale,
		&sv_neo_weapon_zr68s_bloom_scale,
#ifdef INCLUDE_WEP_PBK
		& sv_neo_weapon_pbk56s_bloom_scale,
#endif
	};
	float wepSpecificBloomScale = 1.0f;
	for (ConVar* scaler : bloomScalers)
	{
		if (V_strstr(scaler->GetName(), GetName()))
		{
			wepSpecificBloomScale = scaler->GetFloat();
			break;
		}
	}

	Assert(GetInnateInaccuracy() <= GetMaxAccuracyPenalty());

	const float ramp = RemapValClamped(m_flAccuracyPenalty,
		GetInnateInaccuracy(),
		GetMaxAccuracyPenalty() * sv_neo_wep_acc_penalty_scale.GetFloat(),
		0.0f,
		1.0f);

	// We lerp from very accurate to inaccurate over time
	VectorLerp(
		GetMinCone() * sv_neo_wep_cone_min_scale.GetFloat(),
		GetMaxCone() * sv_neo_wep_cone_max_scale.GetFloat() * wepSpecificBloomScale,
		ramp,
		cone);

	return cone;
}

void CNEOBaseCombatWeapon::PrimaryAttack(void)
{
	Assert(!ShootingIsPrevented());

	if (gpGlobals->curtime < m_flSoonestAttack)
	{
		return;
	}
	// Can't shoot again yet
	else if (gpGlobals->curtime - m_flLastAttackTime < GetFireRate())
	{
		return;
	}
	else if (m_iClip1 == 0)
	{
		if (!m_bFireOnEmpty)
		{
			CheckReload();
		}
		else
		{
			WeaponSound(EMPTY);
			SendWeaponAnim(ACT_VM_DRYFIRE);
			m_flNextPrimaryAttack = 0.2;
		}
		return;
	}

	auto pOwner = ToBasePlayer(GetOwner());

	if (!pOwner)
	{
		Assert(false);
		return;
	}
	else if (m_iClip1 == 0 && !ClientWantsAutoReload(pOwner))
	{
		return;
	}

	if (IsSemiAuto())
	{
		// Do nothing if we hold fire whilst semi auto
		if ((pOwner->m_afButtonLast & IN_ATTACK) &&
			(pOwner->m_nButtons & IN_ATTACK))
		{
			return;
		}
	}

	pOwner->ViewPunchReset();

	if ((gpGlobals->curtime - m_flLastAttackTime) > 0.5f)
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		++m_nNumShotsFired;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	// If my clip is empty (and I use clips) start reload
	if (UsesClipsForAmmo1() && !m_iClip1)
	{
		Reload();
		return;
	}

	// Only the player fires this way so we can cast
	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	pPlayer->DoMuzzleFlash();

	SendWeaponAnim(GetPrimaryAttackActivity());

	// player "shoot" animation
	pPlayer->SetAnimation(PLAYER_ATTACK1);

	FireBulletsInfo_t info;
	info.m_vecSrc = pPlayer->Weapon_ShootPosition();

	info.m_vecDirShooting = pPlayer->GetAutoaimVector(AUTOAIM_SCALE_DEFAULT);

	// To make the firing framerate independent, we may have to fire more than one bullet here on low-framerate systems, 
	// especially if the weapon we're firing has a really fast rate of fire.
	info.m_iShots = 0;
	float fireRate = GetFireRate();

	while (m_flNextPrimaryAttack <= gpGlobals->curtime)
	{
		// MUST call sound before removing a round from the clip of a CMachineGun
		WeaponSound(SINGLE, m_flNextPrimaryAttack);
		m_flNextPrimaryAttack = m_flNextPrimaryAttack + fireRate;
		info.m_iShots++;
		if (!fireRate)
			break;
	}

	// Make sure we don't fire more than the amount in the clip
	if (UsesClipsForAmmo1())
	{
		info.m_iShots = MIN(info.m_iShots, m_iClip1);
		m_iClip1 -= info.m_iShots;
	}
	else
	{
		info.m_iShots = MIN(info.m_iShots, m_iPrimaryAmmoCount);
		m_iPrimaryAmmoCount -= info.m_iShots;
	}

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = 2;

#if !defined( CLIENT_DLL )
	// Fire the bullets
	info.m_vecSpread = pPlayer->GetAttackSpread(this);
#else
	//!!!HACKHACK - what does the client want this function for? 
	info.m_vecSpread = GetActiveWeapon()->GetBulletSpread();
#endif // CLIENT_DLL

	info.m_flPenetration = GetPenetration();

	pPlayer->FireBullets(info);

	if (!m_iClip1 && m_iPrimaryAmmoCount <= 0)
	{
		// HEV suit - indicate out of ammo condition
		pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);
	}

	//Add our view kick in
	AddViewKick();

	m_flAccuracyPenalty += GetAccuracyPenalty();
}

bool CNEOBaseCombatWeapon::CanBePickedUpByClass(int classId)
{
	return true;
}

#ifdef CLIENT_DLL
void CNEOBaseCombatWeapon::ProcessMuzzleFlashEvent()
{
	if (GetPlayerOwner() == NULL)
		return; // If using a view model in first person, muzzle flashes are not processed until the player drops their weapon. In that case, do not play a muzzle flash effect. Need to change how this is calculated if we want to allow dropped weapons to cook off for example

	if ((GetNeoWepBits() & NEO_WEP_SUPPRESSED))
		return;

	int iAttachment = -1;
	if (!GetBaseAnimating())
		return;
	
	// Find the attachment point index
	iAttachment = GetBaseAnimating()->LookupAttachment("muzzle_flash");
	if (iAttachment <= 0)
	{
		Warning("Model '%s' doesn't have attachment '%s'\n", GetPrintName(), "muzzle_flash");
		return;
	}

	// Muzzle flash light
	Vector vAttachment;
	if (!GetAttachment(iAttachment, vAttachment))
		return;

	// environment light
	dlight_t* el = effects->CL_AllocDlight(LIGHT_INDEX_MUZZLEFLASH + index);
	el->origin = vAttachment;
	el->radius = random->RandomInt(64, 96);
	el->decay = el->radius / 0.1f;
	el->die = gpGlobals->curtime + 0.1f;
	el->color.r = 255;
	el->color.g = 192;
	el->color.b = 64;
	el->color.exponent = 5;

	// Muzzle flash particle
	DispatchMuzzleParticleEffect(iAttachment);
}

void CNEOBaseCombatWeapon::DispatchMuzzleParticleEffect(int iAttachment) {
	static constexpr char particleName[] = "ntr_muzzle_source";
	constexpr bool resetAllParticlesOnEntity = false;
	const ParticleAttachment_t iAttachType = ParticleAttachment_t::PATTACH_POINT_FOLLOW;

	CEffectData	data;

	data.m_nHitBox = GetParticleSystemIndex(particleName);
	data.m_hEntity = this;
	data.m_fFlags |= PARTICLE_DISPATCH_FROM_ENTITY;
	data.m_vOrigin = GetAbsOrigin();
	data.m_nDamageType = iAttachType;
	data.m_nAttachmentIndex = iAttachment;

	if (resetAllParticlesOnEntity)
		data.m_fFlags |= PARTICLE_DISPATCH_RESET_PARTICLES;

	CSingleUserRecipientFilter filter(UTIL_PlayerByIndex(GetLocalPlayerIndex()));
	te->DispatchEffect(filter, 0.0, data.m_vOrigin, "ParticleEffect", data);
}

static inline bool ShouldDrawLocalPlayerViewModel(void)
{
	return !C_BasePlayer::ShouldDrawLocalPlayer();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNEOBaseCombatWeapon::ShouldDraw(void)
{
	if (m_iWorldModelIndex == 0)
		return false;

	// FIXME: All weapons with owners are set to transmit in CBaseCombatWeapon::UpdateTransmitState,
	// even if they have EF_NODRAW set, so we have to check this here. Ideally they would never
	// transmit except for the weapons owned by the local player.
	if (IsEffectActive(EF_NODRAW))
		return false;

	C_BaseCombatCharacter* pOwner = GetOwner();

	// weapon has no owner, always draw it
	if (!pOwner)
		return true;

	C_BasePlayer* pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// carried by local player?
	if (pOwner == pLocalPlayer)
	{
		if (!pOwner->ShouldDraw())
		{
			// Our owner is invisible.
			// This also tests whether the player is zoomed in, in which case you don't want to draw the weapon.
			return false;
		}

		// 3rd person mode?
		if (!ShouldDrawLocalPlayerViewModel())
			return true;

		// don't draw active weapon if not in some kind of 3rd person mode, the viewmodel will do that
		return false;
	}

	// FIXME: We may want to only show active weapons on NPCs
	// These are carried by AIs; always show them
	return true;
}

int CNEOBaseCombatWeapon::DrawModel(int flags)
{
	C_BaseCombatCharacter* localPlayer = C_BasePlayer::GetLocalPlayer();
	if (GetOwner() == localPlayer && ShouldDrawLocalPlayerViewModel())
		return 0;
	
	return BaseClass::DrawModel(flags);
}
#endif

bool CNEOBaseCombatWeapon::CanDrop()
{
	return true;
}

void CNEOBaseCombatWeapon::SetPickupTouch(void)
{
#if GAME_DLL
	SetTouch(&CBaseCombatWeapon::DefaultTouch);

	// Ghosts should never get removed by deferred think tick,
	// regardless of whether other kinds of guns remain in the world.
	if (IsGhost())
	{
		return;
	}

	if (!weaponstay.GetBool())
	{
		BaseClass::SetPickupTouch();
		return;
	}

	// If we previously scheduled a removal, but the cvar was changed before it fired,
	// cancel that scheduled removal.
	if (this->m_pfnThink == &CBaseEntity::SUB_Remove)
	{
		SetNextThink(TICK_NEVER_THINK);
		SetThink(NULL);
	}
#endif
}
