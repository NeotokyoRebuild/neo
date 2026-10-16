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
#include "neo_crosshair.h"
#include "model_types.h"
#include "c_neo_player.h"
#include "in_main.h"
#else
#include "items.h"
#include "neo_gamerules.h"
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
	RecvPropBool(RECVINFO(m_bTriggerReset)),
	RecvPropTime(RECVINFO(m_flReloadReadyTime)),
	RecvPropInt(RECVINFO(m_nReloadPhase)),
	RecvPropBool(RECVINFO(m_bChamberEmpty)),
	RecvPropBool(RECVINFO(m_bReloadDry)),
	RecvPropInt(RECVINFO(m_nReloadTable)),
	RecvPropTime(RECVINFO(m_flReloadStart)),
	RecvPropInt(RECVINFO(m_nReloadSoundsDone)),
	RecvPropInt(RECVINFO(m_spawnflags)),
#else
	SendPropTime(SENDINFO(m_flSoonestAttack)),
	SendPropTime(SENDINFO(m_flLastAttackTime)),
	SendPropFloat(SENDINFO(m_flAccuracyPenalty)),
	SendPropInt(SENDINFO(m_nNumShotsFired)),
	SendPropBool(SENDINFO(m_bTriggerReset)),
	SendPropTime(SENDINFO(m_flReloadReadyTime)),
	SendPropInt(SENDINFO(m_nReloadPhase), 3, SPROP_UNSIGNED),
	SendPropBool(SENDINFO(m_bChamberEmpty)),
	SendPropBool(SENDINFO(m_bReloadDry)),
	SendPropInt(SENDINFO(m_nReloadTable), RELOAD_PHASE_TABLE_BITS),
	SendPropTime(SENDINFO(m_flReloadStart)),
	SendPropInt(SENDINFO(m_nReloadSoundsDone), RELOAD_PHASE_ROWS_MAX * RELOAD_SOUNDS_PER_ROW, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_spawnflags)),
	SendPropExclude("DT_BaseAnimating", "m_nSequence"),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOBaseCombatWeapon)
	DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bTriggerReset, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flReloadReadyTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nReloadPhase, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bChamberEmpty, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_bReloadDry, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nReloadTable, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_flReloadStart, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),
	DEFINE_PRED_FIELD(m_nReloadSoundsDone, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neobasecombatweapon, CNEOBaseCombatWeapon);

#ifdef GAME_DLL
BEGIN_DATADESC( CNEOBaseCombatWeapon )
	DEFINE_FIELD(m_flSoonestAttack, FIELD_TIME),
	DEFINE_FIELD(m_flLastAttackTime, FIELD_TIME),
	DEFINE_FIELD(m_flAccuracyPenalty, FIELD_FLOAT),
	DEFINE_FIELD(m_nNumShotsFired, FIELD_INTEGER),
	DEFINE_FIELD(m_bTriggerReset, FIELD_BOOLEAN),
	DEFINE_FIELD(m_flReloadReadyTime, FIELD_TIME),
	DEFINE_FIELD(m_nReloadPhase, FIELD_INTEGER),
	DEFINE_FIELD(m_bChamberEmpty, FIELD_BOOLEAN),
	DEFINE_FIELD(m_bReloadDry, FIELD_BOOLEAN),
	DEFINE_FIELD(m_nReloadTable, FIELD_INTEGER),
	DEFINE_FIELD(m_flReloadStart, FIELD_TIME),
	DEFINE_FIELD(m_nReloadSoundsDone, FIELD_INTEGER),
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
	// NOTE: NEO_WEP_INVALID must be the first item!
	{NEO_WEP_INVALID,
		{{vec3_invalid, vec3_invalid, vec3_invalid, vec3_invalid}},
		{0.0, 0.0, 0.0, 0.0},
	},

	{NEO_WEP_AA13,
		{{VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES, VECTOR_CONE_5DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
	},
	{NEO_WEP_BALC,
		{{VECTOR_CONE_2DEGREES, VECTOR_CONE_6DEGREES, VECTOR_CONE_1DEGREES, VECTOR_CONE_3DEGREES}},
		{0.25, 0.5, -0.6, 0.6},
		{1.0, 0.0, -0.25, -0.75, -0.6, 0.6},
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
};

CNEOBaseCombatWeapon::CNEOBaseCombatWeapon( void )
{
	m_bTriggerReset = true;
	m_nReloadPhase = RELOAD_PHASE_NONE;
	m_bChamberEmpty = false;
	m_bReloadDry = false;
	m_nReloadTable = -1;
	m_flReloadStart = 0.0f;
	m_nReloadSoundsDone = 0;
}

const CNEOWeaponInfo &CNEOBaseCombatWeapon::GetNEOWpnData() const
{
	const FileWeaponInfo_t *pWeaponInfo = &GetWpnData();
	const CNEOWeaponInfo *pNEOInfo;

#ifdef _DEBUG
	pNEOInfo = dynamic_cast< const CNEOWeaponInfo* >( pWeaponInfo );
	Assert( pNEOInfo );
#else
	pNEOInfo = static_cast< const CNEOWeaponInfo* >( pWeaponInfo );
#endif

	return *pNEOInfo;
}

void CNEOBaseCombatWeapon::Precache()
{
	BaseClass::Precache();
}

const char *CNEOBaseCombatWeapon::GetViewModel( int ) const
{
	auto owner = GetOwner();

	if (!owner)
	{
		return GetWpnData().szViewModel;
	}

	return owner->GetTeamNumber() == TEAM_JINRAI ?
			   GetWpnData().szViewModel : GetNEOWpnData().szViewModel2;
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

	// This case is used for handling guns for which handling doesn't make sense,
	// for example the ghost, knife, grenades... Typically it is not used but the bot logic
	// way want to know the spread of the ghost etc, so the invalid first slot is used to handle those.
	m_weaponHandling = handlingTable[0];
	AssertMsg(m_weaponHandling.weaponID == NEO_WEP_INVALID, "Expected the first m_weaponHandling item to be NEO_WEP_INVALID");
	// Assuming this wasn't one of the above-mentioned "invalid" weapon handling entries, this is where we get the real one.
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

	if (NEORules()->IsCyberspace())
	{
		SetRenderMode(kRenderTransTexture);
		m_nRenderFX = kRenderFxDistort;
	}
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

void CNEOBaseCombatWeapon::Detach()
{
	BaseClass::Detach();

	RemoveEffects(EF_BONEMERGE | EF_BONEMERGE_FASTCULL);
	SetParent(nullptr);
	SetOwnerEntity(nullptr);
	SetAbsAngles(vec3_angle);
	SetMoveType(MOVETYPE_VPHYSICS);
	RemoveSolidFlags(FSOLID_NOT_SOLID);
	SetCollisionGroup(COLLISION_GROUP_WEAPON);
}

#ifdef CLIENT_DLL
void CNEOBaseCombatWeapon::ClientThink()
{
	const float temperatureChange = GetOwner() && m_flTemperature < THERMALS_OBJECT_TEMPERATURE_HELD ? TICK_INTERVAL * THERMALS_OBJECT_COOL_RATE : -TICK_INTERVAL * THERMALS_OBJECT_COOL_RATE;
	m_flTemperature = Max(THERMALS_OBJECT_MIN_TEMPERATURE, Min(THERMALS_OBJECT_MAX_TEMPERATURE, m_flTemperature + temperatureChange));

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
	if (HasReloadPhases())
	{
		return ReloadWithPhases();
	}

	// an empty weapon reloads with its slide held back until the release, where the model has that reload
	if (UsesClipsForAmmo1() && m_iClip1 <= 0 && SelectWeightedSequence(ACT_VM_RELOAD_EMPTY) != ACTIVITY_NOT_AVAILABLE)
	{
		if (!DefaultReload(GetMaxClip1(), GetMaxClip2(), ACT_VM_RELOAD_EMPTY))
		{
			return false;
		}

		// others see the reload, as for a reload with rounds left
		ToHL2MPPlayer(GetOwner())->DoAnimationEvent(PLAYERANIMEVENT_RELOAD);
		return true;
	}

	if (!BaseClass::Reload())
	{
		return false;
	}

	// The magazine loads at the gun's last reload click, which the script gives as a time before the reload animation ends;
	// the animation and the wait to fire still run to the end
	m_flReloadReadyTime = Max(gpGlobals->curtime, m_flNextPrimaryAttack - GetNEOWpnData().m_flReloadReadyBeforeEnd);

	return true;

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

// Each reload sound piece's file starts this long before its first transient
static constexpr float RELOAD_SOUND_LEAD = 0.02f;

// The shipped reload sequence, the dry reload with every phase
static constexpr const char *VM_SEQ_RELOAD = "reload";
// A pistol's dry reload (the Milso ships it, the Tachi's is built)
static constexpr const char *VM_SEQ_RELOAD_EMPTY = "reloadempty";

// The reload sequence for the phase credited last, dry or partial; nullptr: nothing left to play
// (sequences other than the shipped reload carry no activity, so they are picked by name)
static const char *ReloadSequenceName(int iPhaseDone, bool bDry, bool bFromDraw)
{
	switch (iPhaseDone)
	{
	case RELOAD_PHASE_NONE:
		// a pistol's dry reload ends with the slide release; magazine guns have no reloadempty and play the reload
		return bDry ? VM_SEQ_RELOAD_EMPTY : "reload_partial";
	case RELOAD_PHASE_DROPPED:
		// after a switch the magazine is out: the draw itself brings the fresh one up out of view
		if (bFromDraw)
		{
			return bDry ? "draw_resume_drop_dry" : "draw_resume_drop_partial";
		}
		return bDry ? "reload_from_drop_dry" : "reload_from_drop_partial";
	case RELOAD_PHASE_SEATED:
		return bDry ? "reload_from_seat_dry" : "reload_from_seat_partial";
	case RELOAD_PHASE_SLAPPED:
		return bDry ? "reload_from_slap_dry" : nullptr;
	default:
		return nullptr;
	}
}

bool CNEOBaseCombatWeapon::HasReloadPhases() const
{
	return GetNEOWpnData().m_iNumReloadTables > 0;
}

// A reload is dry when it has to chamber a round with the bolt
bool CNEOBaseCombatWeapon::IsReloadDry() const
{
	return !GetNEOWpnData().m_bChamberedRound || m_bChamberEmpty;
}

// The phase that completes a reload: the bolt on a dry one, the last magazine phase otherwise
ReloadPhase_t CNEOBaseCombatWeapon::LastReloadPhase(bool bDry) const
{
	// the full reload of that kind lists every phase it needs (a pistol's slide release is only in reloadempty)
	const CNEOWeaponInfo &info = GetNEOWpnData();
	int iTable = bDry ? info.FindReloadTable(VM_SEQ_RELOAD_EMPTY) : -1;
	if (iTable < 0)
	{
		iTable = info.FindReloadTable(VM_SEQ_RELOAD);
	}
	if (iTable < 0)
	{
		return RELOAD_PHASE_NONE;
	}

	ReloadPhase_t last = RELOAD_PHASE_NONE;
	const ReloadPhaseTable_t &table = info.m_reloadTables[iTable];
	for (int i = 0; i < table.iNumRows; ++i)
	{
		if (table.rows[i].phase == RELOAD_PHASE_BOLTED && !bDry)
		{
			continue;
		}
		last = Max(last, table.rows[i].phase);
	}
	return last;
}

// An interrupted reload has phases credited and phases left to do
bool CNEOBaseCombatWeapon::IsReloadPending() const
{
	return m_nReloadPhase != RELOAD_PHASE_NONE && m_nReloadPhase < LastReloadPhase(IsReloadDry());
}

// Start, or resume after a switch, a reload whose phases the weapon script lists
bool CNEOBaseCombatWeapon::ReloadWithPhases()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner || m_bInReload || pOwner->GetNextAttack() > gpGlobals->curtime)
	{
		return false;
	}

	const CNEOWeaponInfo &info = GetNEOWpnData();
	if (!IsReloadPending())
	{
		m_nReloadPhase = RELOAD_PHASE_NONE;
		if (m_iPrimaryAmmoCount <= 0)
		{
			return false;
		}

		if (m_iClip1 == 0)
		{
			CBasePlayer *pPlayer = ToBasePlayer(pOwner);
			if (pPlayer && !(pPlayer->m_nButtons & IN_RELOAD) && !ClientWantsAutoReload(pOwner))
			{
				return false;
			}
			m_bChamberEmpty = info.m_bChamberedRound;
		}

		// a full magazine with a round in the chamber has nothing to gain
		const int iFull = GetMaxClip1() + (IsReloadDry() ? 0 : 1);
		if (m_iClip1 >= iFull)
		{
			return false;
		}
	}

	m_bReloadDry = IsReloadDry();
	int iSequence = -1;
	const int iTable = SelectReloadTable(false, &iSequence);
	if (iTable < 0)
	{
		Warning("%s: ReloadPhases has no \"%s\" sequence\n", GetClassname(), VM_SEQ_RELOAD);
		return false;
	}

	StartReloadSequence(iTable, iSequence, ResumeGestureCycle());
	return true;
}

// Credit every phase of the playing reload sequence whose time has come
void CNEOBaseCombatWeapon::RunReloadPhases()
{
	if (!m_bInReload || m_nReloadTable < 0)
	{
		return;
	}

	const ReloadPhaseTable_t &table = GetNEOWpnData().m_reloadTables[m_nReloadTable];
	const float flElapsed = gpGlobals->curtime - m_flReloadStart;

	// Sound pieces play on their own times, before or after their phase's credit
	// (the pieces of phases credited before this sequence started are marked done when it starts)
	for (int i = 0; i < table.iNumRows; ++i)
	{
		for (int j = 0; j < RELOAD_SOUNDS_PER_ROW; ++j)
		{
			const ReloadSoundPiece_t &piece = table.rows[i].sounds[j];
			const int iBit = 1 << (i * RELOAD_SOUNDS_PER_ROW + j);
			if (piece.szName[0] && !(m_nReloadSoundsDone & iBit) && flElapsed >= piece.flAt - RELOAD_SOUND_LEAD)
			{
				m_nReloadSoundsDone |= iBit;
				EmitReloadSoundPiece(piece.szName);
			}
		}
	}

	for (int i = 0; i < table.iNumRows; ++i)
	{
		const ReloadPhaseRow_t &row = table.rows[i];
		if (row.phase <= m_nReloadPhase || (row.phase == RELOAD_PHASE_BOLTED && !m_bReloadDry))
		{
			continue;
		}
		if (flElapsed < row.flCredit)
		{
			break;
		}
		CreditReloadPhase(row.phase);
	}
}

// The phase table, and the viewmodel sequence (-1: the reload activity), for the reload to play now.
// A model without the sequence a phase calls for plays the shipped reload, crediting only the phases left
int CNEOBaseCombatWeapon::SelectReloadTable(bool bFromDraw, int *pSequence) const
{
	const CNEOWeaponInfo &info = GetNEOWpnData();
	*pSequence = -1;

	const char *pszName = ReloadSequenceName(m_nReloadPhase, m_bReloadDry, bFromDraw);
	if (pszName && V_strcmp(pszName, VM_SEQ_RELOAD) != 0)
	{
		const int iTable = info.FindReloadTable(pszName);
		const int iSequence = (iTable >= 0) ? LookupReloadSequence(pszName) : -1;
		if (iSequence >= 0)
		{
			*pSequence = iSequence;
			return iTable;
		}
	}

	if (bFromDraw)
	{
		return -1;
	}
	return info.FindReloadTable(VM_SEQ_RELOAD);
}

// Play a reload sequence from its start; its phase table credits the phases left from now on
// Where the full reload's gesture should start for a reload resumed after a switch: the time the last completed phase
// counts in the full reload, as a fraction of it (0 for a fresh reload)
float CNEOBaseCombatWeapon::ResumeGestureCycle() const
{
	if (m_nReloadPhase == RELOAD_PHASE_NONE)
	{
		return 0.0f;
	}

	const CNEOWeaponInfo &info = GetNEOWpnData();
	const int iTable = info.FindReloadTable(VM_SEQ_RELOAD);
	const int iSequence = LookupReloadSequence(VM_SEQ_RELOAD);
	if (iTable < 0 || iSequence < 0)
	{
		return 0.0f;
	}

	const float flDuration = ReloadSequenceDuration(iSequence);
	const ReloadPhaseTable_t &table = info.m_reloadTables[iTable];
	for (int i = 0; i < table.iNumRows; ++i)
	{
		if (table.rows[i].phase == m_nReloadPhase && flDuration > 0.0f)
		{
			return Clamp(table.rows[i].flCredit / flDuration, 0.0f, 1.0f);
		}
	}
	return 0.0f;
}

// Reload sequences are looked up on the owner's viewmodel: on the client the weapon entity carries the world model,
// so a lookup there would fail and the client would predict a different sequence than the server plays
CBaseViewModel *CNEOBaseCombatWeapon::ReloadViewModel() const
{
	CBasePlayer *pOwner = ToBasePlayer(GetOwner());
	return pOwner ? pOwner->GetViewModel(m_nViewModelIndex) : nullptr;
}

int CNEOBaseCombatWeapon::LookupReloadSequence(const char *pszName) const
{
	CBaseViewModel *pVM = ReloadViewModel();
	return pVM ? pVM->LookupSequence(pszName) : -1;
}

float CNEOBaseCombatWeapon::ReloadSequenceDuration(int iSequence) const
{
	CBaseViewModel *pVM = ReloadViewModel();
	return pVM ? pVM->SequenceDuration(iSequence) : 0.0f;
}

// Stop the reload gesture others see, for a reload that ended or was cut short
void CNEOBaseCombatWeapon::EndReloadGesture()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (pOwner && pOwner->IsPlayer())
	{
		static_cast<CNEO_Player *>(pOwner)->DoAnimationEvent(PLAYERANIMEVENT_RELOAD_END);
	}
}

void CNEOBaseCombatWeapon::StartReloadSequence(int iTable, int iSequence, float flGestureStartCycle)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	m_nReloadTable = iTable;

	// the phases already credited, and a partial reload's bolt, play no sound
	const ReloadPhaseTable_t &table = GetNEOWpnData().m_reloadTables[iTable];
	int iSoundsDone = 0;
	for (int i = 0; i < table.iNumRows; ++i)
	{
		const ReloadPhase_t phase = table.rows[i].phase;
		if (phase <= m_nReloadPhase || (phase == RELOAD_PHASE_BOLTED && !m_bReloadDry))
		{
			iSoundsDone |= ((1 << RELOAD_SOUNDS_PER_ROW) - 1) << (i * RELOAD_SOUNDS_PER_ROW);
		}
	}
	m_nReloadSoundsDone = iSoundsDone;
	if (!HasReloadSoundPieces())
	{
		WeaponSound(RELOAD_NPC);
	}

	if (iSequence >= 0)
	{
		SetSequence(iSequence);
		SendViewModelAnim(iSequence);
	}
	else
	{
		SendWeaponAnim(ACT_VM_RELOAD);
	}
	// the reload gesture other players see (SetAnimation(PLAYER_RELOAD) does nothing under NEO)
	if (pOwner && pOwner->IsPlayer())
	{
		// the gesture's start cycle travels in the event's spare data, in thousandths
		static_cast<CNEO_Player *>(pOwner)->DoAnimationEvent(PLAYERANIMEVENT_RELOAD, static_cast<int>(flGestureStartCycle * 1000.0f));
	}

	// the wait to fire is never shorter than the animation that plays
	const float flEnd = gpGlobals->curtime + ((iSequence >= 0) ? ReloadSequenceDuration(iSequence) : SequenceDuration());
	if (pOwner)
	{
		pOwner->SetNextAttack(flEnd);
	}
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = flEnd;
	SetWeaponIdleTime(flEnd);
	m_flReloadStart = gpGlobals->curtime;
	m_bInReload = true;
}

bool CNEOBaseCombatWeapon::HasReloadSoundPieces() const
{
	const CNEOWeaponInfo &info = GetNEOWpnData();
	for (int t = 0; t < info.m_iNumReloadTables; ++t)
	{
		for (int i = 0; i < info.m_reloadTables[t].iNumRows; ++i)
		{
			if (info.m_reloadTables[t].rows[i].sounds[0].szName[0])
			{
				return true;
			}
		}
	}
	return false;
}

// Play a reload sound piece from the owner, as WeaponSound does: predicted for the owner, from the server for the rest
void CNEOBaseCombatWeapon::EmitReloadSoundPiece(const char *pszSound)
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return;
	}

	CSoundParameters params;
	if (!GetParametersForSound(pszSound, params, NULL))
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

// A dropped weapon (a throw, or its owner's death) cuts the reload short: its sound pieces stop
void CNEOBaseCombatWeapon::StopReloadOnDrop()
{
	if (HasReloadPhases())
	{
		StopReloadSoundPieces();
		if (m_bInReload)
		{
			EndReloadGesture();
		}
	}
}

// A switch cuts the reload sound pieces off
void CNEOBaseCombatWeapon::StopReloadSoundPieces()
{
	CBaseCombatCharacter *pOwner = GetOwner();
	if (!pOwner)
	{
		return;
	}

	const CNEOWeaponInfo &info = GetNEOWpnData();
	for (int t = 0; t < info.m_iNumReloadTables; ++t)
	{
		for (int i = 0; i < info.m_reloadTables[t].iNumRows; ++i)
		{
			for (int j = 0; j < RELOAD_SOUNDS_PER_ROW; ++j)
			{
				const char *pszName = info.m_reloadTables[t].rows[i].sounds[j].szName;
				if (pszName[0])
				{
					StopSound(pOwner->entindex(), pszName);
				}
			}
		}
	}
}

int CNEOBaseCombatWeapon::GetMaxRoundsInGun() const
{
	return GetMaxClip1() + ((HasReloadPhases() && GetNEOWpnData().m_bChamberedRound) ? 1 : 0);
}

void CNEOBaseCombatWeapon::CreditReloadPhase(ReloadPhase_t phase)
{
	switch (phase)
	{
	case RELOAD_PHASE_DROPPED:
		// the old magazine and its rounds are gone, a chambered round stays;
		// a revolver's cylinder (no chamber) keeps its live rounds and drops only the empty cases
		if (GetNEOWpnData().m_bChamberedRound)
		{
			m_iClip1 = (m_iClip1 > 0 && !IsReloadDry()) ? 1 : 0;
		}
		break;
	case RELOAD_PHASE_SEATED:
	{
		// a fresh magazine, or the rounds that fill a cylinder's empty slots
		const int iRoom = GetNEOWpnData().m_bChamberedRound ? GetMaxClip1() : GetMaxClip1() - m_iClip1;
		const int iLoad = Min(iRoom, m_iPrimaryAmmoCount.Get());
		m_iClip1 += iLoad;
		m_iPrimaryAmmoCount -= iLoad;
		break;
	}
	case RELOAD_PHASE_BOLTED:
		m_bChamberEmpty = false;
		break;
	default:
		break;
	}

	m_nReloadPhase = phase;
}

void CNEOBaseCombatWeapon::CheckReloadWithPhases()
{
	if (m_bInReload)
	{
		RunReloadPhases();
		if (m_flNextPrimaryAttack > gpGlobals->curtime)
		{
			return;
		}

		// the sequence has ended
		m_bInReload = false;
		EndReloadGesture();
		m_nReloadTable = -1;
		if (!IsReloadPending())
		{
			m_nReloadPhase = RELOAD_PHASE_NONE;
		}
		return;
	}

	// an interrupted reload resumes once the weapon is drawn again
	if (IsReloadPending())
	{
		Reload();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Reload has finished.
//-----------------------------------------------------------------------------
void CNEOBaseCombatWeapon::FinishReload(void)
{
	// A reload with phases loads at its seat credit
	if (HasReloadPhases())
	{
		return;
	}

	// The magazine was already loaded at the reload's ready time
	if (m_flReloadReadyTime == 0.0f)
	{
		return;
	}
	m_flReloadReadyTime = 0.0f;

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
		m_bTriggerReset = true;

		// An empty weapon draws with its slide held back where the model has that draw,
		// which carries no activity, so it is picked by name
		static constexpr const char *VM_SEQ_DRAW_EMPTY = "drawempty";
		const int iDrawEmpty = (UsesClipsForAmmo1() && m_iClip1 <= 0) ? LookupSequence(VM_SEQ_DRAW_EMPTY) : ACTIVITY_NOT_AVAILABLE;
		if (iDrawEmpty != ACTIVITY_NOT_AVAILABLE)
		{
			SetSequence(iDrawEmpty);
			SendViewModelAnim(iDrawEmpty);
		}

		// A reload interrupted after the magazine dropped resumes in the draw, where the model has that draw
		if (HasReloadPhases() && m_nReloadPhase == RELOAD_PHASE_DROPPED && IsReloadPending())
		{
			m_bReloadDry = IsReloadDry();
			int iSequence = -1;
			const int iTable = SelectReloadTable(true, &iSequence);
			if (iTable >= 0)
			{
				// the draw is part of the motion: the full gesture fits the draw-resume
				StartReloadSequence(iTable, iSequence, 0.0f);
			}
		}

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

			if (pOwner->m_nButtons & IN_ZOOM && IsAllowedToZoom(this))
			{
				// Should already be aiming, but doesn't hurt to check
				if (!pOwner->IsInAim())
				{
					pOwner->Weapon_SetZoom(true);
				}
				else
				{
					pOwner->SetFOV(pOwner, GetNEOWpnData().iAimFOV, 0.1);
				}
			}
			else
			{
				pOwner->Weapon_SetZoom(false);
			}
		}
	}

	return ret;
}

float CNEOBaseCombatWeapon::GetFireRate()
{
	return GetNEOWpnData().m_flCycleTime;
}

float CNEOBaseCombatWeapon::GetPenetration() const
{
	return GetNEOWpnData().m_flPenetration;
}

bool CNEOBaseCombatWeapon::Holster(CBaseCombatWeapon* pSwitchingTo)
{
	// The phases credited so far stay; a reload with nothing left to do is over
	if (HasReloadPhases())
	{
		if (m_bInReload)
		{
			EndReloadGesture();
		}
		if (!IsReloadPending())
		{
			m_nReloadPhase = RELOAD_PHASE_NONE;
		}
		m_nReloadTable = -1;
		StopReloadSoundPieces();
	}

	return BaseClass::Holster(pSwitchingTo);
}

void CNEOBaseCombatWeapon::CheckReload(void)
{
	if (HasReloadPhases())
	{
		CheckReloadWithPhases();
		return;
	}

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
	BaseClass::ItemPreFrame();
	UpdateInaccuracy();
}

void CNEOBaseCombatWeapon::ItemBusyFrame(void)
{
	BaseClass::ItemBusyFrame();
	UpdateInaccuracy();

	if (HasReloadPhases())
	{
		RunReloadPhases();
		return;
	}

	// The owner stays busy until the reload animation ends, but the magazine loads at the reload's ready time
	if (m_bInReload && m_flReloadReadyTime > 0.0f && gpGlobals->curtime >= m_flReloadReadyTime)
	{
		FinishReload();
	}
}

void CNEOBaseCombatWeapon::ItemHolsterFrame(void)
{
	BaseClass::ItemHolsterFrame();
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
		m_flNextPrimaryAttack = Max(gpGlobals->curtime + nextAttackDelay, m_flNextPrimaryAttack.Get());
		m_flNextSecondaryAttack = m_flNextPrimaryAttack;
	};

	// NEO JANK (Adam) Why do we have to bombard the zr68l viewmodel with SendWeaponAnim(ACT_VM_IDLE_LOWERED) during sprint to make it act normally? Breakpoint in SendWeaponAnim isn't triggered by anything else after this animation is sent while sprinting
	const bool loweredCheck = GetNeoWepBits() & NEO_WEP_ZR68_L ? true : !m_bLowered;
	if (loweredCheck && !m_bInReload &&
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
	else if (m_bLowered && gpGlobals->curtime > m_flNextPrimaryAttack)
	{
		SetWeaponIdleTime(gpGlobals->curtime + 0.2);
		m_flNextPrimaryAttack = Max(gpGlobals->curtime + 0.2f, m_flNextPrimaryAttack.Get());
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

	if (IsSemiAuto() && !(pOwner->m_nButtons & IN_ATTACK))
	{
		m_bTriggerReset = true;
	}

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
	if ((pOwner->m_nButtons & IN_RELOAD) && UsesClipsForAmmo1() && !m_bInReload)
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
#ifdef DBGFLAG_ASSERT
	// Special case for gracefully handling the bots requesting spread info for weird weapons
	static_assert(NEO_WEP_INVALID == 0); // required this value to be zero for the logic to work
	if (m_weaponHandling.weaponID == NEO_WEP_INVALID)
	{
		// Bits for all of the non-invalid guns which are allowed to use the invalid weaponhandling entry.
		// These are the guns for which concepts like "spread" make no sense.
		// Or more accurately, guns for which there is no entry in the global handlingTable.
		const auto allowedInvalidSpreadInfoGuns = (
			NEO_WEP_GHOST | NEO_WEP_KNIFE | NEO_WEP_EXPLOSIVE);
		Assert(GetNeoWepBits() & allowedInvalidSpreadInfoGuns);
	}
	else
	{
		Assert(!(GetNeoWepBits() & NEO_WEP_INVALID));
		Assert(m_weaponHandling.weaponID & GetNeoWepBits());
	}
#endif
	return m_weaponHandling.spreadInfo[0];
}

const Vector &CNEOBaseCombatWeapon::GetBulletSpread(void)
{
	auto pOwner = ToNEOPlayer(GetOwner());

	// We lerp from very accurate to inaccurate over time
	static Vector cone;
	const auto& weaponSpread = GetSpreadInfo();
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

extern ConVar sv_suppress_viewpunch;

void CNEOBaseCombatWeapon::AddViewKick()
{
	auto owner = ToNEOPlayer(GetOwner());

	if (!owner)
	{
		return;
	}

	// Reset any existing viewkick so it doesn't accumulate
	// TODO: Resetting viewpunch here doesn't just affect viewkick, but also aimpunch.
	// This if-check is a temporary workaround until we can separate the two.
	if (!sv_suppress_viewpunch.GetBool()) {
		owner->ViewPunchReset();
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
#ifdef GAME_DLL
	CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);
#endif
	SendWeaponAnim(ACT_VM_DRYFIRE);
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFastestDryRefireTime(); // SequenceDuration();
}

// An empty weapon idles on its empty sequence where the model has one, such as a pistol with its slide held back
bool CNEOBaseCombatWeapon::SendWeaponAnim(int iActivity)
{
	if (iActivity == ACT_VM_IDLE && UsesClipsForAmmo1() && m_iClip1 <= 0 && HasEmptyIdle())
	{
		iActivity = ACT_VM_IDLE_EMPTY;
	}

	return BaseClass::SendWeaponAnim(iActivity);
}

bool CNEOBaseCombatWeapon::HasEmptyIdle()
{
	return SelectWeightedSequence(ACT_VM_IDLE_EMPTY) != ACTIVITY_NOT_AVAILABLE;
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

	// a dry reload seated the magazine but has not chambered a round yet
	else if (HasReloadPhases() && m_bChamberEmpty && m_iClip1 > 0)
	{
		CheckReload();
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

	if (IsSemiAuto() && !m_bTriggerReset)
	{
		return;
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

	if (IsSemiAuto())
	{
		m_bTriggerReset = false;
	}

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
#ifdef GAME_DLL
		CSoundEnt::InsertSound(SOUND_COMBAT, GetAbsOrigin(), (GetNeoWepBits() & NEO_WEP_SUPPRESSED) ? SOUNDENT_VOLUME_NEO_SUPPRESSED : SOUNDENT_VOLUME_PISTOL, 0.2, GetOwner(), SOUNDENT_CHANNEL_WEAPON);
#endif
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

	if (HasReloadPhases())
	{
		// the last round leaves the chamber empty, and a shot shows the seated magazine feeds without its slap
		if (GetNEOWpnData().m_bChamberedRound && m_iClip1 <= 0)
		{
			m_bChamberEmpty = true;
		}
		if (m_nReloadPhase == RELOAD_PHASE_SEATED)
		{
			m_nReloadPhase = RELOAD_PHASE_NONE;
		}
	}

	// the last round's shot locks a pistol's slide back:
	// the pistols' dry fire sequence is a shot's kick with the slide held back,
	static constexpr NEO_WEP_BITS_UNDERLYING_TYPE NEO_WEP_SLIDE_LOCK = NEO_WEP_MILSO | NEO_WEP_TACHI;
	if (UsesClipsForAmmo1() && m_iClip1 <= 0 && (GetNeoWepBits() & NEO_WEP_SLIDE_LOCK))
	{
		SendWeaponAnim(ACT_VM_DRYFIRE);
	}
	else
	{
		SendWeaponAnim(GetPrimaryAttackActivity());
	}

	// and goes straight on to the empty idle when it ends
	const bool bEmptyIdleNext = UsesClipsForAmmo1() && m_iClip1 <= 0 && HasEmptyIdle();
	SetWeaponIdleTime(gpGlobals->curtime + (bEmptyIdleNext ? SequenceDuration() : 2.0));

	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iAmmoType = m_iPrimaryAmmoType;
	info.m_iTracerFreq = UsesTracers() ? 1 : 0;

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

	m_flAccuracyPenalty = Min(
		GetMaxAccuracyPenalty(),
		m_flAccuracyPenalty + GetAccuracyPenalty() * sv_neo_accuracy_penalty_scale.GetFloat()
	);

	//Add our view kick in
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
	QAngle angles;
	if (!GetAttachment(iAttachment, vAttachment, angles))
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
	FX_MuzzleEffect( vAttachment, angles, 0.5f, GetRefEHandle(), NULL, true );
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
		crosshair->SetCrosshair(GetWpnData().iconCrosshair, crosshair->m_crosshairInfo.wep[CROSSHAIR_WEP_DEFAULT].color);
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

	// No supernatural gunowners allowed here
	if (!pOwner->IsAlive())
	{
#if DEBUG
		static int deathTicks[MAX_PLAYERS_ARRAY_SAFE]{};
		if (!*deathTicks)
			for (int i = 0; i < ARRAYSIZE(deathTicks); ++i) deathTicks[i] = -1;
		const int ownerIdx = pOwner->entindex();
		Assert(pOwner->IsPlayer()); // player index guaranteed within array range
		const int deathTick = deathTicks[ownerIdx];
		if (deathTick != -1)
		{
			int tickDelta = gpGlobals->tickcount - deathTick;
			AssertMsg(tickDelta != 1, "Owner has been dead for two consecutive ticks!!");
		}
		deathTicks[ownerIdx] = gpGlobals->tickcount;
#endif
		return false;
	}

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

#ifdef GLOWS_ENABLE
extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
int CNEOBaseCombatWeapon::DrawModel(int flags)
{
	auto pTargetPlayer = C_NEO_Player::GetVisionTargetNEOPlayer();
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
	if (auto pPlayer = static_cast<C_NEO_Player*>(GetOwner()))
	{
		return pPlayer->IsDrawnTransparent() ? RENDER_GROUP_TRANSLUCENT_ENTITY : RENDER_GROUP_OPAQUE_ENTITY;
	}

	return BaseClass::GetRenderGroup();
}

bool CNEOBaseCombatWeapon::UsesPowerOfTwoFrameBufferTexture()
{
	if (auto pPlayer = static_cast<C_NEO_Player*>(GetOwner()))
	{
		return pPlayer->IsDrawnTransparent();
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

	if (!weaponstay.GetBool() || NEORules()->CanRespawnAnyTime())
	{ // regardless of the value of mp_weaponstay, disappear weapons in game modes with respawns enabled. Otherwise things can get too chaotic
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
	m_OnPlayerUse.FireOutput( pActivator, pCaller );

	if (m_pfnTouch)
	{
		if (CNEO_Player* pNeoPlayer = ToNEOPlayer(pActivator);
			pNeoPlayer && CanBePickedUpByClass(pNeoPlayer->GetClass()))
		{
			CBaseCombatWeapon* pActiveWeapon = pNeoPlayer->GetActiveWeapon();
			const int activeSlot = pActiveWeapon ? pActiveWeapon->GetSlot() : -1;
			pNeoPlayer->Weapon_DropSlot(GetSlot());

			(this->*m_pfnTouch)(pActivator);

			if (GetOwner() == pNeoPlayer && activeSlot == GetSlot())
			{
				pNeoPlayer->Weapon_Switch(this);
			}
		}
	}

	// Calling BaseClass::Use will pick the weapon up without waiting for the touch cooldown, don't see anything important there that we need to do that we aren't doing here
}
#endif

const char *CNEOBaseCombatWeapon::GetDeathIcon(const CNEOBaseCombatWeapon *pNeoWep,
		const EDeathIconType eType, bool isGrenade, bool isRemoteDetpack)
{
	if (eType == DEATHICONTYPE_IDX && pNeoWep)
	{
		isGrenade = pNeoWep->WeaponIndex() == NEO_WIDX_FRAG_GRENADE;
		isRemoteDetpack = pNeoWep->WeaponIndex() == NEO_WIDX_DETPACK;
	}

	if (isGrenade)
	{
		return "2";
	}
	else if (isRemoteDetpack)
	{
		return "A";
	}
	else if (pNeoWep)
	{
		return pNeoWep->GetNEOWpnData().szDeathIcon;
	}
	return "";
}
