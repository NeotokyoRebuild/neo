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

#include "weapon_bits.h"

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

struct WeaponSpreadInfo_t {
	Vector minSpreadHip;
	Vector maxSpreadHip;
	Vector minSpreadAim;
	Vector maxSpreadAim;
};

struct WeaponKickInfo_t {
	float minX;
	float maxX;
	float minY;
	float maxY;
};

struct WeaponRecoilInfo_t {
	float hipFactor;
	float adsFactor;
	float minX;
	float maxX;
	float minY;
	float maxY;
};

struct WeaponHandlingInfo_t
{
	NeoWepBits weaponID;
	WeaponSpreadInfo_t spreadInfo[2];
	WeaponKickInfo_t kickInfo;
	WeaponRecoilInfo_t recoilInfo;
};

struct WeaponSeeds_t
{
	const char *punchX;
	const char *punchY;
	const char *recoilX;
	const char *recoilY;
};

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
	virtual void Precache() override;
	virtual void Spawn() override;
	virtual void Activate() override;

#ifdef CLIENT_DLL
	virtual void ClientThink() override;
#endif // CLIENT_DLL

	virtual void Equip(CBaseCombatCharacter* pOwner) override;
	virtual	void CheckReload(void) override;

	virtual bool Reload( void ) override;
	virtual void FinishReload(void) override;

	virtual bool CanBeSelected(void) override;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { Assert(false); return NEO_WEP_INVALID; } // Should never call this base class; implement in children.
	virtual int GetNeoWepXPCost(const int neoClass) const { Assert(false); return 0; } // Should never call this base class; implement in children.

	virtual float GetSpeedScale(void) const { Assert(false); return 1.0; } // Should never call this base class; implement in children.

	virtual void ItemPreFrame(void) override;
	virtual void ItemPostFrame(void) override;

	virtual void PrimaryAttack(void) override;
	virtual void SecondaryAttack(void) override;
#ifdef GAME_DLL
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;
#endif
	virtual void DryFire(void);

	virtual Activity GetPrimaryAttackActivity(void) override;
#ifdef CLIENT_DLL
	virtual void ProcessMuzzleFlashEvent(void) override;
	void DrawCrosshair() override;
#endif
	bool IsGhost(void) const { return (GetNeoWepBits() & NEO_WEP_GHOST) ? true : false; }

	bool IsExplosive(void) const { return (GetNeoWepBits() & NEO_WEP_EXPLOSIVE) ? true : false; }

	bool ShootingIsPrevented(void) const
	{
		auto owner = static_cast<CNEO_Player*>(GetOwner());
		if (!owner)
		{
			return true;
		}
		if (owner->GetNeoFlags() & NEO_FL_FREEZETIME || owner->GetFlags() & FL_FROZEN)
		{
			return true;
		}
		return false;
	}

	float GetLastAttackTime(void) const { return m_flLastAttackTime; }

	virtual void ProcessAnimationEvents();
	bool m_bWeaponIsLowered;

	int GetNumShotsFired(void) const { return m_nNumShotsFired; }

	// Whether this weapon should fire automatically when holding down the attack.
	virtual bool IsAutomatic(void) const
	{
		return ((GetNeoWepBits() & (NEO_WEP_AA13 | NEO_WEP_JITTE | NEO_WEP_JITTE_S |
			NEO_WEP_KNIFE | NEO_WEP_MPN | NEO_WEP_MPN_S | NEO_WEP_MX | NEO_WEP_MX_S |
			NEO_WEP_PZ | NEO_WEP_SMAC | NEO_WEP_SRM | NEO_WEP_SRM_S | NEO_WEP_ZR68_C | NEO_WEP_ZR68_S | NEO_WEP_BALC
#ifdef INCLUDE_WEP_PBK
			| NEO_WEP_PBK56S
#endif
			)) ? true : false);
	}

	// Whether this weapon should fire only once per each attack command, even if held down.
	bool IsSemiAuto(void) const { return !IsAutomatic(); }

	virtual const Vector& GetBulletSpread(void) override;
	virtual const WeaponSpreadInfo_t& GetSpreadInfo(void);
	virtual void AddViewKick(void) override;

	virtual bool CanBePickedUpByClass(int classId);
	virtual bool CanDrop(void);

	virtual void SetPickupTouch(void) override;

#ifdef CLIENT_DLL
	virtual bool Holster(CBaseCombatWeapon* pSwitchingTo);
	virtual void ItemHolsterFrame() override;
	virtual bool ShouldDraw(void) override;
	virtual void ThirdPersonSwitch(bool bThirdPerson) override;
	virtual int RestoreData(const char* context, int slot, int type) override;
	virtual int DrawModel(int flags) override;
	virtual RenderGroup_t GetRenderGroup() override;
	virtual bool UsesPowerOfTwoFrameBufferTexture() override;
#endif

	virtual bool Deploy(void);

	virtual float GetFireRate() override final;
	virtual bool GetRoundChambered() const { return 0; }
	virtual bool GetRoundBeingChambered() const { return 0; }
	float GetPenetration() const;
#ifdef CLIENT_DLL
	float m_flTemperature;
#endif // CLIENT_DLL

protected:
	WeaponHandlingInfo_t m_weaponHandling;
	WeaponSeeds_t m_weaponSeeds;

	virtual void UpdateInaccuracy(void);
	virtual float GetAccuracyPenalty() const { return 0.8; }
	virtual float GetMaxAccuracyPenalty() const { return 1.0; }
	virtual float GetAccuracyPenaltyDecay();
	virtual float GetFastestDryRefireTime() const { Assert(false); return 0; } // Should never call this base class; implement in children.

protected:
	CNetworkVar(float, m_flSoonestAttack);
	CNetworkVar(float, m_flLastAttackTime);
	CNetworkVar(float, m_flAccuracyPenalty);

	CNetworkVar(int, m_nNumShotsFired);
	CNetworkVar(bool, m_bRoundChambered);
	CNetworkVar(bool, m_bRoundBeingChambered);

private:
	CNEOBaseCombatWeapon(const CNEOBaseCombatWeapon &other);

};

#endif // WEAPON_NEO_BASECOMBATWEAPON_SHARED_H
