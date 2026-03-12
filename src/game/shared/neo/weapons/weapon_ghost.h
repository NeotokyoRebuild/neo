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

#ifdef CLIENT_DLL
	virtual void ClientThink() override final
	{
		BaseClass::ClientThink();
		UpdateNearestGhostBeaconDist();
		TryGhostPing();
	}
#endif

	// When the player "activates" the ghost, there is a slight time delay until they're allowed
	// to start seeing enemy beacons. This function returns whether that time has elapsed.
	inline bool IsBootupCompleted(float timeOffset=0) const
	{
		extern ConVar sv_neo_ctg_ghost_beacons_when_inactive;
		const float ghostActivationTime = sv_neo_ctg_ghost_beacons_when_inactive.GetBool()
			? m_flPickupTime : m_flDeployTime;
		const float deltaTime = gpGlobals->curtime - ghostActivationTime;
		return deltaTime + timeOffset >= sv_neo_ghost_delay_secs.GetFloat();
	}

	// If the enemy is within this ghost's range, returns true and passes its distance by reference.
	// If the enemy is not in range, the distance will not be written into.
	[[nodiscard]] bool BeaconRange(CBaseEntity* enemy, float& outDistance) const;

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
	virtual bool CanAim() final { return false; }

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const { return NEO_WEP_GHOST; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.85f; }
	static float GetGhostRangeInHammerUnits();
	void UpdateNearestGhostBeaconDist();

#ifdef GAME_DLL
	int UpdateTransmitState() override;
	int ShouldTransmit(const CCheckTransmitInfo *pInfo) override;
#endif

#ifdef CLIENT_DLL
	void StopGhostSound(void);
	void HandleGhostEquip(void);
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
	void PlayGhostSound(float volume = 1.0f);
#ifdef CLIENT_DLL
	void TryGhostPing();

	bool m_bHavePlayedGhostEquipSound;
	bool m_bHaveHolsteredTheGhost;
	float m_flLastGhostBeepTime;
#endif

	CNetworkVar(float, m_flDeployTime); // The timestamp when the ghost was last equipped as the active weapon.
	CNetworkVar(float, m_flNearestEnemyDist);
	CNetworkVar(float, m_flPickupTime); // The timestamp when the ghost was last picked up by a player into their inventory.

	CWeaponGhost(const CWeaponGhost &other);
};

#endif // NEO_WEAPON_GHOST_H
