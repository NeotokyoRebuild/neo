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

	virtual void PrimaryAttack(void) override;
	virtual void SecondaryAttack(void) override;
	virtual void Drop(const Vector &vecVelocity) override { /* knives shouldn't drop */ }

	virtual bool CanBePickedUpByClass(int classId) override;
	virtual bool CanDrop(void) override { return false; }

#ifdef CLIENT_DLL
	virtual bool ShouldDraw() override;
#else
	virtual bool IsViewable() override;
#endif

	virtual	void Spawn(void) override;
	virtual void ItemPreFrame(void) override;
	virtual void ItemBusyFrame(void) override;
	virtual void ItemPostFrame(void) override;

	virtual Activity GetPrimaryAttackActivity(void) override { return ACT_VM_HITCENTER; }
	virtual Activity GetSecondaryAttackActivity(void) override { return ACT_VM_HITCENTER2; }
	float GetRange(void) const { return 51.0f; }
	float GetDamageForActivity(Activity hitActivity) { return 25.0f; }
	virtual float GetFireRate(void) const { return 0.534f; }
	virtual float GetSpeedScale(void) const { return 1.0; }
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_KNIFE; }

protected:
	void		ImpactEffect(trace_t &traceHit);
	void		Swing(int bIsSecondary);
	Activity	ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins, const Vector& maxs, CBasePlayer* pOwner);
	bool		ImpactWater(const Vector &start, const Vector &end);
	void		Hit(trace_t& traceHit, Activity nHitActivity);
private:
	CWeaponKnife(const CWeaponKnife &other);
};

#endif // NEO_WEAPON_KNIFE_H