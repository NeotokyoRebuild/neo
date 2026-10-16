#ifndef NEO_WEAPON_SUPA7_H
#define NEO_WEAPON_SUPA7_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponSupa7 C_WeaponSupa7
#endif

class CWeaponSupa7 : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponSupa7, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponSupa7();

	NEO_WEP_BITS_UNDERLYING_TYPE WeaponIndex() const override { return NEO_WIDX_SUPA7; }
	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const OVERRIDE { return NEO_WEP_SUPA7 | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const OVERRIDE { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.7f; }

	virtual int GetMinBurst() OVERRIDE { return 1; }
	virtual int GetMaxBurst() OVERRIDE { return 3; }

	virtual const WeaponSpreadInfo_t& GetSpreadInfo(void) override;

	bool StartReload(void);
	bool StartReloadSlug(void);
	bool Reload(void);
	bool ReloadSlug(void);
	bool SlugLoaded(void) const;

	void FillClip(void);
	void FillClipSlug(void);
	void FinishReload(void);
	void ItemPostFrame(void);
	void PrimaryAttack(void);
	void SecondaryAttack(void);

	void Drop(const Vector& vecVelocity) OVERRIDE;

	void ClearDelayedInputs(void);

	bool CanBePickedUpByClass(int classId) OVERRIDE;

	// What sits in the chamber: a live shell or slug, the hull of a fired one, or nothing
	enum Supa7Chamber_t
	{
		SUPA7_CHAMBER_EMPTY = 0,
		SUPA7_CHAMBER_SHELL,
		SUPA7_CHAMBER_SLUG,
		SUPA7_CHAMBER_HULL,
	};
	int TubeCount() const { return m_nTubeCount; }
	bool IsTubeSlug(int iPosition) const { return (m_nTubeSlugs & (1 << iPosition)) != 0; }
	Supa7Chamber_t Chamber() const { return static_cast<Supa7Chamber_t>(m_nChamber.Get()); }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	// Purpose: Only update next attack time if it's further away in the future.
	void ProposeNextAttack(const float flNextAttackProposal)
	{
		if (m_flNextPrimaryAttack < flNextAttackProposal)
		{
			m_flNextPrimaryAttack = flNextAttackProposal;
		}
	}

	bool IsChamberLive() const { return m_nChamber == SUPA7_CHAMBER_SHELL || m_nChamber == SUPA7_CHAMBER_SLUG; }
	int TubeCapacity() const;
	void SyncTube();
	void TubePush(bool bSlug);
	void Rack();
	void UpdateClipFromTube();

	void SetShotgunShellVisible(bool is_visible)
	{
		const int bodygroup_shell = 1;
		const int shell_visible = 0, shell_invisible = 1;
		SetBodygroup(bodygroup_shell, is_visible ? shell_visible : shell_invisible);
	}

private:
	CNetworkVar(bool, m_bSlugDelayed); // Load slug into tube next
	CNetworkVar(bool, m_bSlugLoaded); // Slug currently loaded in chamber
	CNetworkVar(bool, m_bWeaponRaised); // Slug currently loaded in chamber
	CNetworkVar(bool, m_bShellInChamber); // Slug currently loaded in chamber
	CNetworkVar(float, m_flNextReload); // Time to load the next shell

	// The tube is a stack, the last item loaded is the first fed; m_iClip1 stays the live rounds in the gun
	CNetworkVar(int, m_nTubeCount); // items in the tube
	CNetworkVar(int, m_nTubeSlugs); // bit i: the item at tube position i (0 = loaded first) is a slug
	CNetworkVar(int, m_nChamber); // Supa7Chamber_t
	CNetworkVar(bool, m_bRackPending); // a slug was loaded to be fired next: the reload ends with a rack

private:
	CWeaponSupa7(const CWeaponSupa7 &other);
};

#endif // NEO_WEAPON_SUPA7_H
