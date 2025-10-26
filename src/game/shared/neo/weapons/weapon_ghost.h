#ifndef NEO_WEAPON_GHOST_H
#define NEO_WEAPON_GHOST_H
#ifdef _WIN32
#pragma once
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponGhost C_WeaponGhost
#define CBaseCombatCharacter C_BaseCombatCharacter
#define CBasePlayer C_BasePlayer
#define CNEO_Player C_NEO_Player
#endif

class CNEO_Player;

class CWeaponGhost : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponGhost, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
#endif

	CWeaponGhost(void);
#ifdef GAME_DLL
	virtual ~CWeaponGhost();
#endif

	float GetDeployTime() const { return m_flDeployTime; }
	float GetPickupTime() const { return m_flPickupTime; }

	virtual void ItemPreFrame(void) OVERRIDE;
	virtual void PrimaryAttack(void) OVERRIDE { }
	virtual void SecondaryAttack(void) OVERRIDE { }

	virtual bool Deploy() override;
	virtual void Drop(const Vector &vecVelocity) override;
	virtual void ItemHolsterFrame(void);
	virtual void Equip(CBaseCombatCharacter *pNewOwner) override;
	virtual int	ObjectCaps(void) { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;};
	void HandleGhostUnequip(void);
	bool CanBePickedUpByClass(int classId) OVERRIDE;

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_GHOST; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.85f; }
	static float GetGhostRangeInHammerUnits();

#ifdef GAME_DLL
	int UpdateTransmitState() override;
	int ShouldTransmit(const CCheckTransmitInfo *pInfo) override;
#endif

	void PlayGhostSound(float volume = 1.0f);
#ifdef CLIENT_DLL
	void StopGhostSound(void);
	void HandleGhostEquip(void);
#endif

#ifdef GAME_DLL
	void TryGhostPing(CNEO_Player* ghoster, float closestEnemy);
#else
	void TryGhostPing(float closestEnemy);
#endif

	virtual void UpdateOnRemove() override
	{
		if (GetOwner())
		{
			auto neoPlayer = static_cast<CNEO_Player*>(GetPlayerOwner());
			neoPlayer->m_bCarryingGhost = false;
		}
#ifdef CLIENT_DLL
		StopGhostSound();
#endif //CLIENT_DLL
		BaseClass::UpdateOnRemove();
	};

private:

#ifdef CLIENT_DLL
	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;
#endif
	float m_flLastGhostBeepTime;

	CNetworkVar(float, m_flDeployTime);
	CNetworkVar(float, m_flPickupTime);

	CWeaponGhost(const CWeaponGhost &other);
};

#endif // NEO_WEAPON_GHOST_H
