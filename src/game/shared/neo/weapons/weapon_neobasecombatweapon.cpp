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
#include "prediction.h"
#include "hud_crosshair.h"
#include "ui/neo_hud_crosshair.h"
#include "model_types.h"
#include "c_neo_player.h"
#else
#include "items.h"
#endif // CLIENT_DLL

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
	SendPropExclude("DT_BaseAnimating", "m_nSequence"),
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

ConVar sv_neo_accuracy_penalty_scale("sv_neo_accuracy_penalty_scale", "1.0", FCVAR_REPLICATED, "Scales the accuracy penalty per shot.", true, 0.0f, true, 2.0f);
ConVar sv_neo_dynamic_viewkick("sv_neo_dynamic_viewkick", "0", FCVAR_REPLICATED, "Enables view kick scaling based on current inaccuracy.", true, 0.0f, true, 1.0f);

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

// TODO: This lookup could be more efficient with sequential IDs a la SDK,
// but we'll probably move this stuff to the weapon scripts anyway.
static const WeaponHandlingInfo_t handlingTable[] = {
	{NEO_WEP_AA13,
		{{VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_JITTE,
		{{VECTOR_CONE_3DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_3DEGREES}},
		{-0.5, -0.25, -0.6, 0.6},
	},
	{NEO_WEP_JITTE_S,
		{{VECTOR_CONE_3DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_3DEGREES}},
		{-0.5, -0.25, -0.6, 0.6},
	},
	{NEO_WEP_KYLA,
		{{VECTOR_CONE_5DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_2DEGREES, VECTOR_CONE_4DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{10.5, 6.0, -0.75, 0.0, -0.5, 0.5},
	},
	{NEO_WEP_M41,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}}
	},
	{NEO_WEP_M41_L,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{1.0, 2.5, -0.5, 0.0, -0.5, 0.5},
	},
	{NEO_WEP_M41_S,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}}
	},
	{NEO_WEP_MILSO,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_2DEGREES, VECTOR_CONE_4DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_MPN,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{1.0, 1.0, -0.5, -0.25, -0.6, 0.6},
	},
	{NEO_WEP_MPN_S,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{1.0, 1.0, -0.5, -0.25, -0.6, 0.6},
	},
	{NEO_WEP_MX,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_MX_S,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_PZ,
		{{VECTOR_CONE_2DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_1DEGREES / 2, VECTOR_CONE_2DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
		{1.0, 0.0, -0.25, -0.75, -0.6, 0.6},
	},
#ifdef INCLUDE_WEP_PBK
	{NEO_WEP_PBK56S,
		{{VECTOR_CONE_2DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_1DEGREES / 2, VECTOR_CONE_2DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
		{1.0, 0.0, -0.25, -0.75, -0.6, 0.6},
	},
#endif
	{NEO_WEP_SMAC,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_SRM,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_SRM_S,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_SRS,
		{{VECTOR_CONE_7DEGREES, VECTOR_CONE_20DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_7DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{1.0, 1.5, -0.5, 0.0, -0.5, 0.5},
	},
	{NEO_WEP_SUPA7,
		{
			{VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES},
			{VECTOR_CONE_4DEGREES, VECTOR_CONE_10DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_3DEGREES}
		},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_TACHI,
		{{VECTOR_CONE_5DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_4DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_ZR68_C,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_ZR68_L,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_2DEGREES}},
		{0.0, 0.0, 0.0, 0.0},
		{1.0, 2.5, -0.5, 0.0, -0.5, 0.5},
	},
	{NEO_WEP_ZR68_S,
		{{VECTOR_CONE_4DEGREES, VECTOR_CONE_7DEGREES, VECTOR_CONE_PRECALCULATED, VECTOR_CONE_3DEGREES}},
		{0.15, 0.25, -0.4, 0.4},
	},
	{NEO_WEP_BALC,
		{{VECTOR_CONE_6DEGREES, VECTOR_CONE_6DEGREES, VECTOR_CONE_6DEGREES, VECTOR_CONE_6DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
};

CNEOBaseCombatWeapon::CNEOBaseCombatWeapon( void )
{
}

void CNEOBaseCombatWeapon::Precache()
{
	BaseClass::Precache();

	if (!(GetNeoWepBits() & NEO_WEP_SUPPRESSED))
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

	m_weaponHandling = handlingTable[0];
	for (const auto& handling: handlingTable)
	{
		if (handling.weaponID & GetNeoWepBits())
		{
			m_weaponHandling = handling;
			break;
		}
	}

#ifdef GAME_DLL
	AddSpawnFlags(SF_NORESPAWN);
#else
	SetNextClientThink(gpGlobals->curtime + TICK_INTERVAL);
#endif // GAME_DLL
}

void CNEOBaseCombatWeapon::Activate(void)
{
#ifdef CLIENT_DLL
	BaseClass::Activate();	// Not being called client side atm, just call baseclass in case it does
#else
	CBaseAnimating::Activate(); // Skip CBaseCombatWeapon::Activate();

	if (GetOwnerEntity())
		return;

	if (g_pGameRules->IsAllowedToSpawn(this) == false)
	{
		UTIL_Remove(this);
		return;
	}

	VPhysicsInitNormal(SOLID_BBOX, GetSolidFlags() | FSOLID_TRIGGER, false);

	if (HasSpawnFlags(SF_ITEM_START_CONSTRAINED))
	{
		//Constrain the weapon in place (copied from CItem::Spawn )
		IPhysicsObject* pReferenceObject, * pAttachedObject;

		pReferenceObject = g_PhysWorldObject;
		pAttachedObject = VPhysicsGetObject();

		if (pReferenceObject && pAttachedObject)
		{
			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState(pReferenceObject, pAttachedObject);

			fixed.constraint.forceLimit = lbs2kg(10000);
			fixed.constraint.torqueLimit = lbs2kg(10000);

			m_pConstraint = physenv->CreateFixedConstraint(pReferenceObject, pAttachedObject, NULL, fixed);

			m_pConstraint->SetGameData((void*)this);
		}
	}
#endif
}

#ifdef CLIENT_DLL
void CNEOBaseCombatWeapon::ClientThink()
{
	if (GetOwner() && m_flTemperature > 0)
	{
		constexpr int DESIRED_TEMPERATURE_WHEN_HELD = 0;
		m_flTemperature = max(DESIRED_TEMPERATURE_WHEN_HELD, m_flTemperature - (TICK_INTERVAL / THERMALS_OBJECT_COOL_TIME));
	}
	else if (m_flTemperature < 1)
	{
		constexpr int DESIRED_TEMPERATURE_WITHOUT_OWNER = 1;
		m_flTemperature = min(DESIRED_TEMPERATURE_WITHOUT_OWNER, m_flTemperature + (TICK_INTERVAL / THERMALS_OBJECT_COOL_TIME));
	}
	SetNextClientThink(gpGlobals->curtime + TICK_INTERVAL);
}
#endif // CLIENT_DLL


void CNEOBaseCombatWeapon::Equip(CBaseCombatCharacter* pOwner)
{
	BaseClass::Equip(pOwner);
	auto neoOwner = static_cast<CNEO_Player*>(pOwner);
	if (neoOwner->m_bInThermOpticCamo)
	{
		AddEffects(EF_NOSHADOW);
	}
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
			if (pOwner->GetFlags() & FL_DUCKING || pOwner->IsWalking())
			{
				pOwner->SetMaxSpeed(pOwner->GetCrouchSpeed_WithWepEncumberment(this));
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

float CNEOBaseCombatWeapon::GetAccuracyPenaltyDecay()
{
	auto penaltyDecay = 1.2f;
	auto *pOwner = GetOwner();
	if (pOwner && pOwner->GetFlags() & FL_DUCKING)
	{
		penaltyDecay += 1.0f;
	}
	return penaltyDecay;
}

void CNEOBaseCombatWeapon::UpdateInaccuracy()
{
	CNEO_Player *pOwner = static_cast<CNEO_Player *>(ToBasePlayer(GetOwner()));
	if (!pOwner)
		return;

	if (pOwner->IsAirborne())
	{
		m_flAccuracyPenalty += gpGlobals->frametime * 3.0;
	}

	if (!pOwner->IsInAim() && pOwner->GetAbsVelocity().Length2D() > 5.0)
	{
		m_flAccuracyPenalty += gpGlobals->frametime * 2.0;
	}

	m_flAccuracyPenalty -= gpGlobals->frametime * GetAccuracyPenaltyDecay();
	m_flAccuracyPenalty = clamp(m_flAccuracyPenalty, 0.0f, GetMaxAccuracyPenalty());
}

void CNEOBaseCombatWeapon::ItemPreFrame(void)
{
	UpdateInaccuracy();
}

// Handles lowering the weapon view model when character is sprinting
void CNEOBaseCombatWeapon::ProcessAnimationEvents()
{
	if (GetNeoWepBits() & NEO_WEP_THROWABLE)
	{
		return;
	}

	CNEO_Player* pOwner = static_cast<CNEO_Player*>(ToBasePlayer(GetOwner()));
	if (!pOwner)
	{
		return;
	}

	const auto next = [this](const int activity, const float nextAttackDelay = 0.2) {
		SendWeaponAnim(activity);
		if (GetNeoWepBits() & NEO_WEP_THROWABLE)
		{
			return;
		}
		m_flNextPrimaryAttack = max(gpGlobals->curtime + nextAttackDelay, m_flNextPrimaryAttack);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	};

	// NEO JANK (Adam) Why do we have to bombard the zr68l viewmodel with SendWeaponAnim(ACT_VM_IDLE_LOWERED) during sprint to make it act normally? Breakpoint in SendWeaponAnim isn't triggered by anything else after this animation is sent while sprinting
	const bool loweredCheck = GetNeoWepBits() & NEO_WEP_ZR68_L ? true : !m_bLowered;
	if (loweredCheck && !m_bInReload && !m_bRoundBeingChambered &&
		(pOwner->IsSprinting() || pOwner->GetMoveType() == MOVETYPE_LADDER))
	{
		m_bLowered = true;
		next(ACT_VM_IDLE_LOWERED);
	}
	else if (m_bLowered && !(pOwner->IsSprinting() || pOwner->GetMoveType() == MOVETYPE_LADDER))
	{
		m_bLowered = false;
		next(ACT_VM_IDLE);
	}
	else if (m_bLowered && m_bRoundBeingChambered)
	{ // For bolt action weapons
		m_bLowered = false;
		next(ACT_VM_PULLBACK, 1.2f);
	}

	else if (m_bLowered && gpGlobals->curtime > m_flNextPrimaryAttack)
	{
		SetWeaponIdleTime(gpGlobals->curtime + 0.2);
		m_flNextPrimaryAttack = max(gpGlobals->curtime + 0.2, m_flNextPrimaryAttack);
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	}
}

void CNEOBaseCombatWeapon::ItemPostFrame(void)
{
	CNEO_Player* pOwner = static_cast<CNEO_Player*>(ToBasePlayer(GetOwner()));
	if (!pOwner)
		return;

	ProcessAnimationEvents();

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

	if (!bFired && (pOwner->m_afButtonPressed & IN_ATTACK))
	{
		if (!IsMeleeWeapon() &&
			((UsesClipsForAmmo1() && m_iClip1 <= 0) || (!UsesClipsForAmmo1() && m_iPrimaryAmmoCount <= 0)))
		{
			DryFire();
			m_flLastAttackTime = gpGlobals->curtime - 3.f;
		}
		else if (pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false)
		{
			// This weapon doesn't fire underwater
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			m_flLastAttackTime = gpGlobals->curtime - 3.f;
			return;
		}
	}

	if (!bFired && (pOwner->m_nButtons & IN_ATTACK) && (m_flNextPrimaryAttack <= gpGlobals->curtime))
	{
		// Clip empty? Or out of ammo on a no-clip weapon?
		if (!IsMeleeWeapon() &&
			((UsesClipsForAmmo1() && m_iClip1 <= 0) || (!UsesClipsForAmmo1() && m_iPrimaryAmmoCount <= 0)))
		{
			if (!(GetNeoWepBits() & NEO_WEP_SRS))
			{
				DryFire();
			}
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
		if (m_flTimeWeaponIdle <= gpGlobals->curtime)
		{
			WeaponIdle();
		}
		if (m_flLastAttackTime + 3.f < gpGlobals->curtime)
		{
			ReloadOrSwitchWeapons();
		}
	}
}

const WeaponSpreadInfo_t &CNEOBaseCombatWeapon::GetSpreadInfo()
{
	Assert(m_weaponHandling.weaponID & GetNeoWepBits());
	return m_weaponHandling.spreadInfo[0];
}

const Vector &CNEOBaseCombatWeapon::GetBulletSpread(void)
{
	auto pOwner = ToNEOPlayer(GetOwner());

	// We lerp from very accurate to inaccurate over time
	static Vector cone;
	auto weaponSpread = GetSpreadInfo();
	if (pOwner && pOwner->IsInAim())
	{
		VectorLerp(
			weaponSpread.minSpreadAim,
			weaponSpread.maxSpreadAim,
			m_flAccuracyPenalty,
			cone);
	}
	else
	{
		VectorLerp(
			weaponSpread.minSpreadHip,
			weaponSpread.maxSpreadHip,
			m_flAccuracyPenalty,
			cone);
	}

	return cone;
}

void CNEOBaseCombatWeapon::AddViewKick()
{
	auto owner = ToNEOPlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	auto kickInfo = m_weaponHandling.kickInfo;
	float punchScale = sv_neo_dynamic_viewkick.GetBool() ? m_flAccuracyPenalty : 1.0f;

	QAngle viewPunch;
	viewPunch.x = SharedRandomFloat(m_weaponSeeds.punchX, kickInfo.minX * punchScale, kickInfo.maxX * punchScale);
	viewPunch.y = SharedRandomFloat(m_weaponSeeds.punchY, kickInfo.minY * punchScale, kickInfo.maxY * punchScale);
	viewPunch.z = 0;

	owner->ViewPunch(viewPunch);

	#ifdef CLIENT_DLL

	if (!prediction->IsFirstTimePredicted())
	{
		return;
	}

	auto recoilInfo = m_weaponHandling.recoilInfo;
	float recoilScale = owner->IsInAim() ? recoilInfo.adsFactor : recoilInfo.hipFactor;
	if (!recoilScale)
	{
		return;
	}

	QAngle viewAngles;
	engine->GetViewAngles(viewAngles);

	viewAngles.x += SharedRandomFloat(m_weaponSeeds.recoilX, recoilInfo.minX * recoilScale, recoilInfo.maxX * recoilScale);
	viewAngles.y += SharedRandomFloat(m_weaponSeeds.recoilY, recoilInfo.minY * recoilScale, recoilInfo.maxY * recoilScale);

	engine->SetViewAngles(viewAngles);

	#endif
}

void CNEOBaseCombatWeapon::DryFire()
{
	WeaponSound(EMPTY);
	SendWeaponAnim(ACT_VM_DRYFIRE);
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFastestDryRefireTime(); // SequenceDuration();
}

void CNEOBaseCombatWeapon::PrimaryAttack(void)
{
	if (ShootingIsPrevented())
	{
		return;
	}
	if (gpGlobals->curtime < m_flSoonestAttack)
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
			DryFire();
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
	auto pPlayer = static_cast<CNEO_Player*>(GetOwner());

	if (!pPlayer)
	{
		return;
	}

	if (!(GetNeoWepBits() & NEO_WEP_SUPPRESSED))
	{
		pPlayer->DoMuzzleFlash();
#ifdef GAME_DLL
		if (pPlayer->m_bInThermOpticCamo)
		{
			pPlayer->CloakFlash(0.1);
		}
#endif // GAME_DLL
	}

	SendWeaponAnim(GetPrimaryAttackActivity());
	SetWeaponIdleTime(gpGlobals->curtime + 2.0);

	// player "shoot" animation
	pPlayer->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

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
	info.m_iTracerFreq = 0;

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

	m_flAccuracyPenalty = min(
		GetMaxAccuracyPenalty(),
		m_flAccuracyPenalty + GetAccuracyPenalty() * sv_neo_accuracy_penalty_scale.GetFloat()
	);

	//Add our view kick in
	pOwner->ViewPunchReset();
	AddViewKick();
}

void CNEOBaseCombatWeapon::SecondaryAttack()
{
	if (!ShootingIsPrevented())
	{
		BaseClass::SecondaryAttack();
	}
}

Activity CNEOBaseCombatWeapon::GetPrimaryAttackActivity()
{
	if (m_nNumShotsFired < 1)
		return ACT_VM_PRIMARYATTACK;

	if (m_nNumShotsFired < 2)
		return ACT_VM_RECOIL1;

	if (m_nNumShotsFired < 3)
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

bool CNEOBaseCombatWeapon::CanBePickedUpByClass(int classId)
{
	return true;
}

#ifdef CLIENT_DLL
void CNEOBaseCombatWeapon::ProcessMuzzleFlashEvent()
{
	C_BasePlayer *owner = GetPlayerOwner();
	if (!owner)
		return; // If using a view model in first person, muzzle flashes are not processed until the player drops their weapon. In that case, do not play a muzzle flash effect. Need to change how this is calculated if we want to allow dropped weapons to cook off for example

	C_BasePlayer *localPlayer = UTIL_PlayerByIndex(GetLocalPlayerIndex());
	if (!localPlayer)
		return;

	// bIsVisible in function calling ProcessMuzzleFlashEvent is set to true even though this weapon may not be drawn? Probably same reason for the same check in C_BaseCombatWeapon::DrawModel
	if (localPlayer->IsObserver() && localPlayer->GetObserverMode() == OBS_MODE_IN_EYE && localPlayer->GetObserverTarget() == owner)
		return;

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
	el->radius = random->RandomInt(32, 64);
	el->decay = el->radius / 0.05f;
	el->die = gpGlobals->curtime + 0.05f;
	el->color.r = 255;
	el->color.g = 192;
	el->color.b = 64;
	el->color.exponent = 5;

	// Muzzle flash particle
	ParticleProp()->Create("ntr_muzzle_source", PATTACH_POINT_FOLLOW, iAttachment);
}

void CNEOBaseCombatWeapon::DrawCrosshair()
{
	auto *player = C_NEO_Player::GetLocalNEOPlayer();
	if (!player)
	{
		return;
	}

	// NEO TODO (nullsystem): Put some X on crosshair if aiming at teammate, see comment
	// in C_BaseCombatWeapon::DrawCrosshair
	// EX: bool bOnTarget = (m_iState == WEAPON_IS_ONTARGET);

	auto *crosshair = GET_HUDELEMENT(CHudCrosshair);
	if (!crosshair)
	{
		return;
	}

	if (GetWpnData().iconCrosshair)
	{
		crosshair->SetCrosshair(GetWpnData().iconCrosshair, crosshair->m_crosshairInfo.color);
	}
	else
	{
		crosshair->ResetCrosshair();
	}
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

void CNEOBaseCombatWeapon::ThirdPersonSwitch(bool bThirdPerson)
{
	UpdateVisibility();
	if (!bThirdPerson)
	{
		SetModel(GetViewModel());
	}
}


int CNEOBaseCombatWeapon::RestoreData(const char* context, int slot, int type)
{
	int val = BaseClass::RestoreData(context, slot, type);
	if (ShouldDrawLocalPlayerViewModel() && GetModelIndex() != modelinfo->GetModelIndex(STRING(GetModelName())))
	{ // NEOHACK (Adam) This restore data method converts the local players' weapon model back to a world model if the player was in third person with this weapon equipped, regardless of whether they are back in first person or not. Need to set it back to a viewmodel (Must an entity have the same model on server and client? If so we need to refactor how we handle view models)
		SetModel(GetViewModel());
	}
	return val;
}

extern ConVar mat_neo_toc_test;
#ifdef GLOWS_ENABLE
extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
int CNEOBaseCombatWeapon::DrawModel(int flags)
{
#ifdef GLOWS_ENABLE
	auto pTargetPlayer = glow_outline_effect_enable.GetBool() ? C_NEO_Player::GetLocalNEOPlayer() : C_NEO_Player::GetVisionTargetNEOPlayer();
#else
	auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
#endif // GLOWS_ENABLE
	if (!pTargetPlayer)
	{
		Assert(false);
		return BaseClass::DrawModel(flags);
	}

	if (GetOwner() == pTargetPlayer && ShouldDrawLocalPlayerViewModel())
		return 0;

	if (flags & STUDIO_IGNORE_NEO_EFFECTS || !(flags & STUDIO_RENDER))
	{
		return BaseClass::DrawModel(flags);
	}

	auto pOwner = static_cast<C_NEO_Player *>(GetOwner());
	bool inThermalVision = pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT;
	int ret = 0;
	
	if (inThermalVision && (!pOwner || (pOwner && !pOwner->IsCloaked())))
	{
		IMaterial* pass = materials->FindMaterial("dev/thermal_weapon_model", TEXTURE_GROUP_MODEL);
		modelrender->ForcedMaterialOverride(pass);
		ret |= BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
		return ret;
	}

	if ((pOwner && pOwner->IsCloaked()) && !inThermalVision)
	{
		mat_neo_toc_test.SetValue(pOwner->GetCloakFactor());
		IMaterial* pass = materials->FindMaterial("models/player/toc", TEXTURE_GROUP_CLIENT_EFFECTS);
		modelrender->ForcedMaterialOverride(pass);
		ret |= BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
		return ret;
	}
	else
	{
		return BaseClass::DrawModel(flags);
	}
}

RenderGroup_t CNEOBaseCombatWeapon::GetRenderGroup()
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());
	if (pPlayer)
	{
		return pPlayer->IsCloaked() ? RENDER_GROUP_TRANSLUCENT_ENTITY : RENDER_GROUP_OPAQUE_ENTITY;
	}

	return BaseClass::GetRenderGroup();
}

bool CNEOBaseCombatWeapon::UsesPowerOfTwoFrameBufferTexture()
{
	auto pPlayer = static_cast<C_NEO_Player*>(GetOwner());
	if (pPlayer)
	{
		return pPlayer->IsCloaked();
	}

	return BaseClass::UsesPowerOfTwoFrameBufferTexture();
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

#ifdef GAME_DLL
void CNEOBaseCombatWeapon::Use(CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
{
	auto* neoPlayer = ToNEOPlayer(pActivator);

	if (neoPlayer && neoPlayer->Weapon_CanSwitchTo(this) && CanBePickedUpByClass(neoPlayer->GetClass()))
	{
		neoPlayer->Weapon_DropSlot(GetSlot());
		neoPlayer->Weapon_Equip(this);

		RemoveEffects(EF_BONEMERGE);
	}

	BaseClass::Use(pActivator, pCaller, useType, value);
}
#endif