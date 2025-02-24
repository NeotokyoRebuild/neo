#ifndef NEO_WEAPON_KNIFE_H
#define NEO_WEAPON_KNIFE_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_hl2mpbasebasebludgeon.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponKnife C_WeaponKnife
#endif

class CWeaponKnife : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponKnife, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponKnife();

	virtual void PrimaryAttack() final;
	virtual void Drop(const Vector &vecVelocity) final { /* knives shouldn't drop */ }

	virtual bool CanBePickedUpByClass(int classId) final;
	virtual bool CanDrop() final { return false; }
	virtual bool CanPerformSecondaryAttack() const override final { return false; }

#ifdef CLIENT_DLL
	virtual bool ShouldDraw() final;
#else
	virtual bool IsViewable() final;
#endif

	virtual	void Spawn() final;
	virtual void ItemPostFrame() final;

	virtual Activity GetPrimaryAttackActivity() final { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity() final { return ACT_VM_HITCENTER2; }
	virtual float GetSpeedScale() const final { return 1.0; }
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits() const { return NEO_WEP_KNIFE; }

protected:
	void		ImpactEffect(trace_t &traceHit);
	void		Swing();
	Activity	ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins, const Vector& maxs, CBasePlayer* pOwner);
	bool		ImpactWater(const Vector &start, const Vector &end);
	void		Hit(trace_t& traceHit, Activity nHitActivity);
private:
	CWeaponKnife(const CWeaponKnife &other);
};

#endif // NEO_WEAPON_KNIFE_H
