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
	void ItemBusyFrame(void) OVERRIDE;
	void PrimaryAttack(void);
	void SecondaryAttack(void);

	void Drop(const Vector& vecVelocity) OVERRIDE;

	void ClearDelayedInputs(void);

	bool CanBePickedUpByClass(int classId) OVERRIDE;
	bool Holster(CBaseCombatWeapon *pSwitchingTo) OVERRIDE;

	// Clip1 stays the live rounds in the gun: the tube plus the chambered round
	bool HasChamberSlot() const OVERRIDE;
	bool IsRoundChambered() const OVERRIDE;
	int RoundsInMagazine() const OVERRIDE;
	int MagazineCapacity() const OVERRIDE;

	// What sits in the chamber: a live shell or slug, the hull of a fired one, or nothing
	enum Supa7Chamber_t
	{
		SUPA7_CHAMBER_EMPTY = 0,
		SUPA7_CHAMBER_SHELL,
		SUPA7_CHAMBER_SLUG,
		SUPA7_CHAMBER_HULL,
	};

	// The round in the hand of the insert that is playing
	enum Supa7Insert_t
	{
		SUPA7_INSERT_NONE = 0,
		SUPA7_INSERT_SHELL,
		SUPA7_INSERT_SLUG,
	};

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

	bool IsTubeEnabled() const;
	bool IsTubeFull() const;

	// Tube phases (sv_neo_reload_experimental); each hook does nothing with the flag off
	bool TubePhasesOnStart();
	bool TubePhasesOnStartSlug();
	void TubePhasesOnLoadInput();
	void TubePhasesOnFreezeTime();
	bool TubePhasesOnFinish();
	bool TubePhasesCanFire();
	void TubePhasesOnShot();
	void TubePhasesOnDrop();
	void TubePhasesOnFrame();
	bool TubePhasesOnNextInsert();
	bool TubePhasesNeedRack() const;
	void TubePhasesOnReloadPress();
	void TubePhasesOnHolster();

	bool IsChamberLive() const { return m_nChamber == SUPA7_CHAMBER_SHELL || m_nChamber == SUPA7_CHAMBER_SLUG; }
	bool IsTubeSynced() const { return m_nTubeCount + (IsChamberLive() ? 1 : 0) == m_iClip1; }
	int TubeCapacity() const { return GetMaxClip1() - SUPA7_CHAMBER_ROUNDS; }
	void SyncTube();
	void TubePush(Supa7Insert_t round);
	bool IsSlugLoadActive() const { return IsTubeEnabled() && m_bSlugLoad; }
	bool IsInFreezeTime() const;
	void ConvertLoad(Supa7Insert_t round);
	void FeedChamber();
	void UpdateClipFromTube();
	void StartInsert(Supa7Insert_t round);
	void RunInsert();
	void CancelInsert();
	void StartRack();
	void RunRack();
	bool IsRackPending() const { return m_flRackFeedTime > 0.0f; }
	bool UsesPumpAfterInsert() const;
	bool IsLastShotWithoutPump() const;
	void StartPumpAfterInsert();
	float ViewModelSequenceDuration() const;
	int LookupViewModelSequence(const char *pszSequence) const;
	void EmitShotSoundPiece(const char *pszSound);
	bool IsSlugInHand() const;
	void UpdateSaddleRounds();
	void ShowSaddleRounds(int iShown);

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

	// sv_neo_reload_experimental: the tube is a stack, the last round loaded is the first fed
	static constexpr int SUPA7_CHAMBER_ROUNDS = 1;
	static constexpr int SUPA7_SADDLE_ROUNDS = 4;
	CNetworkVar(int, m_nTubeCount); // rounds in the tube
	CNetworkVar(int, m_nTubeSlugs); // bit i: the round at tube position i (0 = loaded first) is a slug
	CNetworkVar(int, m_nChamber); // Supa7Chamber_t
	CNetworkVar(int, m_nInsert); // Supa7Insert_t
	CNetworkVar(float, m_flInsertCreditTime); // when the inserted round enters the tube, 0 none
	CNetworkVar(float, m_flRackFeedTime); // when the rack's pump is home with the next round, 0 none
	CNetworkVar(bool, m_bSlugLoad); // the reload loads slugs, each round typed when it is credited
#ifdef GAME_DLL
	bool m_bFreezePreferenceApplied = false; // the owner's saved freeze-time load type, once per freeze time
#endif

private:
	CWeaponSupa7(const CWeaponSupa7 &other);
};

#endif // NEO_WEAPON_SUPA7_H
