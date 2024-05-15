#ifndef WEAPON_NEO_BASECOMBATWEAPON_SHARED_H
#define WEAPON_NEO_BASECOMBATWEAPON_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
#else
	#include "neo_player.h"
#endif

#include "neo_player_shared.h"

#include "weapon_hl2mpbasehlmpcombatweapon.h"

#ifdef CLIENT_DLL
	#define CNEOBaseCombatWeapon C_NEOBaseCombatWeapon
#endif

// Weapon bit flags
enum NeoWepBits : NEO_WEP_BITS_UNDERLYING_TYPE {
	NEO_WEP_INVALID =			0x0,

	NEO_WEP_AA13 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 0),
	NEO_WEP_DETPACK =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 1),
	NEO_WEP_GHOST =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 2),
	NEO_WEP_FRAG_GRENADE =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 3),
	NEO_WEP_JITTE =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 4),
	NEO_WEP_JITTE_S =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 5),
	NEO_WEP_KNIFE =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 6),
	NEO_WEP_KYLA =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 7),
	NEO_WEP_M41 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 8),
	NEO_WEP_M41_L =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 9),
	NEO_WEP_M41_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 10),
	NEO_WEP_MILSO =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 11),
	NEO_WEP_MPN =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 12),
	NEO_WEP_MPN_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 13),
	NEO_WEP_MX =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 14),
	NEO_WEP_MX_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 15),
	NEO_WEP_PROX_MINE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 16),
	NEO_WEP_PZ =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 17),
	NEO_WEP_SMAC =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 18),
	NEO_WEP_SMOKE_GRENADE =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 19),
	NEO_WEP_SRM =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 20),
	NEO_WEP_SRM_S =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 21),
	NEO_WEP_SRS =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 22),
	NEO_WEP_SUPA7 =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 23),
	NEO_WEP_TACHI =				(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 24),
	NEO_WEP_ZR68_C =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 25),
	NEO_WEP_ZR68_L =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 26),
	NEO_WEP_ZR68_S =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 27),
	NEO_WEP_SCOPEDWEAPON =		(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 28), // Scoped weapons should OR this in their flags.
	NEO_WEP_THROWABLE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 29), // Generic for grenades
	
	// NOTE!!! remember to update NEP_WEP_BITS_LAST_VALUE below, if editing this/these last values!
	NEO_WEP_EXPLOSIVE =			(static_cast<NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 30), // Generic for weapons that count as explosive kills on killfeed.
#ifdef INCLUDE_WEP_PBK
	NEO_WEP_PBK56S =			(static_cast <NEO_WEP_BITS_UNDERLYING_TYPE>(1) << 31),
#endif
	
#ifndef INCLUDE_WEP_PBK
	NEP_WEP_BITS_LAST_VALUE = NEO_WEP_EXPLOSIVE
#else
	NEP_WEP_BITS_LAST_VALUE = NEO_WEP_PBK56S
#endif
};
// All bits must fit in the data type. Simple sanity check to make sure we aren't overflowing.
COMPILE_TIME_ASSERT(NEP_WEP_BITS_LAST_VALUE > NEO_WEP_INVALID);
// Some other related type safety checks also rely on this equaling zero.
COMPILE_TIME_ASSERT(NEO_WEP_INVALID == 0);

// These are the .res file id numbers for
// NEO weapon loadout choices used by the
// client cvar "loadout <int>"
enum {
	NEO_WEP_LOADOUT_ID_MPN = 0,
	NEO_WEP_LOADOUT_ID_SRM,
	NEO_WEP_LOADOUT_ID_SRM_S,
	NEO_WEP_LOADOUT_ID_JITTE,
	NEO_WEP_LOADOUT_ID_JITTE_S,
	NEO_WEP_LOADOUT_ID_ZR68C,
	NEO_WEP_LOADOUT_ID_ZR68S,
	NEO_WEP_LOADOUT_ID_ZR68L,
	NEO_WEP_LOADOUT_ID_MX,
	NEO_WEP_LOADOUT_ID_PZ,
	NEO_WEP_LOADOUT_ID_SUPA7,
	NEO_WEP_LOADOUT_ID_M41,
	NEO_WEP_LOADOUT_ID_M41L,

	NEO_WEP_LOADOUT_ID_COUNT
};

const char *GetWeaponByLoadoutId(int id);

#if(1)
		// This does nothing; dummy value for network test. Remove when not needed anymore.
#define DEFINE_NEO_BASE_WEP_PREDICTION
		// This does nothing; dummy value for network test. Remove when not needed anymore.
#define DEFINE_NEO_BASE_WEP_NETWORK_TABLE
#else
#ifdef CLIENT_DLL
#define DEFINE_NEO_BASE_WEP_PREDICTION DEFINE_PRED_FIELD(m_flSoonestAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),\
DEFINE_PRED_FIELD(m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),\
DEFINE_PRED_FIELD(m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE),\
DEFINE_PRED_FIELD(m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
#endif
#ifdef CLIENT_DLL
#define DEFINE_NEO_BASE_WEP_NETWORK_TABLE RecvPropTime(RECVINFO(m_flSoonestAttack)),\
RecvPropTime(RECVINFO(m_flLastAttackTime)),\
RecvPropFloat(RECVINFO(m_flAccuracyPenalty)),\
RecvPropInt(RECVINFO(m_nNumShotsFired)),
#else
#define DEFINE_NEO_BASE_WEP_NETWORK_TABLE SendPropTime(SENDINFO(m_flSoonestAttack)),\
SendPropTime(SENDINFO(m_flLastAttackTime)),\
SendPropFloat(SENDINFO(m_flAccuracyPenalty)),\
SendPropInt(SENDINFO(m_nNumShotsFired)),
#endif
#endif

class CNEOBaseCombatWeapon : public CBaseHL2MPCombatWeapon
{
	DECLARE_CLASS(CNEOBaseCombatWeapon, CBaseHL2MPCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_DATADESC();
#endif

	CNEOBaseCombatWeapon();

	virtual void Spawn();
	virtual	void CheckReload(void);

	virtual bool Reload( void );
	virtual void FinishReload(void) OVERRIDE;

	virtual bool CanBeSelected(void);

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { Assert(false); return NEO_WEP_INVALID; } // Should never call this base class; implement in children.
	virtual int GetNeoWepXPCost(const int neoClass) const { Assert(false); return 0; } // Should never call this base class; implement in children.

	virtual float GetSpeedScale(void) const { Assert(false); return 1.0; } // Should never call this base class; implement in children.

	virtual void ItemPreFrame(void);

	virtual void PrimaryAttack(void);

	virtual float GetInnateInaccuracy(void) const { return 0.0f; } // NEO TODO (Rain): make this abstract & implement some amount of inaccuracy (spread) for weapons?

	bool IsGhost(void) const { return (GetNeoWepBits() & NEO_WEP_GHOST) ? true : false; }

	// We do this check to avoid a player unintentionally aiming in due to holding down their aim key while an automatic wep switch occurs.
	bool IsReadyToAimIn(void) const { return m_bReadyToAimIn; }

	bool IsExplosive(void) const { return (GetNeoWepBits() & NEO_WEP_EXPLOSIVE) ? true : false; }

	bool ShootingIsPrevented(void) const
	{
		auto owner = static_cast<CNEO_Player*>(GetOwner());
		if (!owner)
		{
			return true;
		}
		if (owner->GetNeoFlags() & NEO_FL_FREEZETIME)
		{
			return true;
		}
		return false;
	}

	float GetLastAttackTime(void) const { return m_flLastAttackTime; }

	int GetNumShotsFired(void) const { return m_nNumShotsFired; }

	// Whether this weapon should fire automatically when holding down the attack.
	virtual bool IsAutomatic(void) const
	{
		return ((GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_JITTE | NEO_WEP_JITTE_S |
			NEO_WEP_KNIFE | NEO_WEP_MPN | NEO_WEP_MPN_S | NEO_WEP_MX | NEO_WEP_MX_S |
			NEO_WEP_PZ | NEO_WEP_SMAC | NEO_WEP_SRM | NEO_WEP_SRM_S | NEO_WEP_ZR68_C | NEO_WEP_ZR68_S
#ifdef INCLUDE_WEP_PBK
			| NEO_WEP_PBK56S
#endif
			)) ? true : false);
	}

	// Whether this weapon should fire only once per each attack command, even if held down.
	bool IsSemiAuto(void) const { return !IsAutomatic(); }

	virtual Vector GetMinCone() const { static Vector cone = VECTOR_CONE_1DEGREES; return cone; }
	virtual Vector GetMaxCone() const { static Vector cone = VECTOR_CONE_10DEGREES; return cone; }

	virtual const Vector& GetBulletSpread(void) OVERRIDE;

#ifdef CLIENT_DLL
	virtual bool Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual void ItemHolsterFrame() OVERRIDE;
#endif

	virtual bool Deploy(void);

	// NEO HACK/FIXME (Rain):
	// We override with empty implementation to avoid getting removed by
	// some game logic somewhere. There's probably some flag we could set
	// somewhere to achieve the same without having to do this.
	virtual void SUB_Remove(void) { }

	virtual float GetFireRate(void) OVERRIDE { Assert(false); return BaseClass::GetFireRate(); } // Should never call this base class; override in children.

protected:
	virtual float GetAccuracyPenalty() const { Assert(false); return 0; } // Should never call this base class; implement in children.
	virtual float GetMaxAccuracyPenalty() const { Assert(false); return 0; } // Should never call this base class; implement in children.
	virtual float GetFastestDryRefireTime() const { Assert(false); return 0; } // Should never call this base class; implement in children.

protected:
	CNetworkVar(float, m_flSoonestAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);

	CNetworkVar(int, m_nNumShotsFired);

private:
	bool m_bReadyToAimIn;

private:
	CNEOBaseCombatWeapon(const CNEOBaseCombatWeapon &other);
};

#endif // WEAPON_NEO_BASECOMBATWEAPON_SHARED_H