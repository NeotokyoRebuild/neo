#ifndef NEO_PLAYER_H
#define NEO_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

class CNEO_Player;
class INEOPlayerAnimState;

#include "basemultiplayerplayer.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "utldict.h"
#include "hl2mp_player.h"
#include "in_buttons.h"

#include "neo_player_shared.h"

enum EPauseMenuSelect
{
	PAUSE_MENU_SELECT_SHORT = 1,
	PAUSE_MENU_SELECT_LONG = 2,
	PAUSE_MENU_SELECT_DISMISS = 3,
};

enum EMenuSelectType
{
	MENU_SELECT_TYPE_NONE = 0,
	MENU_SELECT_TYPE_PAUSE,
};

class CNEO_Player : public CHL2MP_Player
{
	friend class CNEORules;
public:
	DECLARE_CLASS(CNEO_Player, CHL2MP_Player);

	CNEO_Player();
	virtual ~CNEO_Player(void);

	static CNEO_Player *CreatePlayer(const char *className, edict_t *ed)
	{
		CNEO_Player::s_PlayerEdict = ed;
		return (CNEO_Player*)CreateEntityByName(className);
	}

	void SendTestMessage(const char *message);
	void SetTestMessageVisible(bool visible);

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	DECLARE_ENT_SCRIPTDESC();

	virtual void Precache(void) OVERRIDE;
	virtual void Spawn(void) OVERRIDE;
	virtual void PostThink(void) OVERRIDE;
	virtual void CalculateSpeed(void);
	virtual void PreThink(void) OVERRIDE;
	virtual void PlayerDeathThink(void) OVERRIDE;
	virtual bool HandleCommand_JoinTeam(int team) OVERRIDE;
	virtual bool ClientCommand(const CCommand &args) OVERRIDE;
	virtual void CreateViewModel(int viewmodelindex = 0) OVERRIDE;
	virtual bool BecomeRagdollOnClient(const Vector &force) OVERRIDE;
	virtual void Event_Killed(const CTakeDamageInfo &info) OVERRIDE;
	virtual float GetReceivedDamageScale(CBaseEntity* pAttacker) OVERRIDE;
	virtual bool WantsLagCompensationOnEntity(const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits) const OVERRIDE;
	virtual void FireBullets(const FireBulletsInfo_t &info) OVERRIDE;
	virtual void Weapon_Equip(CBaseCombatWeapon* pWeapon) OVERRIDE;
	virtual bool Weapon_Switch(CBaseCombatWeapon *pWeapon, int viewmodelindex = 0) OVERRIDE;
	virtual bool Weapon_CanSwitchTo(CBaseCombatWeapon *pWeapon) OVERRIDE;
	virtual bool BumpWeapon(CBaseCombatWeapon *pWeapon) OVERRIDE;
	bool Weapon_GetPosition(int slot, int position);
	virtual void ChangeTeam(int iTeam) OVERRIDE;
	virtual void PickupObject(CBaseEntity *pObject, bool bLimitMassAndSize) OVERRIDE;
	virtual void PlayStepSound(Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force) OVERRIDE;
	const char* GetOverrideStepSound(const char* pBaseStepSound) override;
	virtual void Weapon_Drop(CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL) OVERRIDE;
	void Weapon_DropOnDeath(CNEOBaseCombatWeapon *pWeapon, Vector damageForce);
	void Weapon_DropAllOnDeath(const CTakeDamageInfo &info);
	virtual void UpdateOnRemove(void) OVERRIDE;
	virtual void DeathSound(const CTakeDamageInfo &info) OVERRIDE;
	virtual CBaseEntity* EntSelectSpawnPoint(void) OVERRIDE;
	virtual void EquipSuit(bool bPlayEffects = true) OVERRIDE;
	virtual void RemoveSuit(void) OVERRIDE;
	virtual void GiveDefaultItems(void) OVERRIDE;
	virtual int	OnTakeDamage_Alive(const CTakeDamageInfo& info) OVERRIDE;

	virtual void InitVCollision(const Vector& vecAbsOrigin, const Vector& vecAbsVelocity) OVERRIDE;

	virtual void ModifyFireBulletsDamage(CTakeDamageInfo* dmgInfo) OVERRIDE;

	virtual const Vector GetPlayerMins(void) const OVERRIDE;
	virtual const Vector GetPlayerMaxs(void) const OVERRIDE;

	// -----------------------
	// For bots, calculate how obscuring "fog" variables like thermoptic camoflage affect the visibility of a target.
	// While the functions aren't actually about fog in the NT context, the abstraction of visibility percentage seemed to fit.
	// -----------------------
	virtual bool		IsHiddenByFog(CBaseEntity* target) const OVERRIDE;        ///< return true if given target cant be seen because of "fog"
	virtual float		GetFogObscuredRatio(CBaseEntity* target) const OVERRIDE;  ///< return 0-1 ratio where zero is not obscured, and 1 is completely obscured

	void AddNeoFlag(int flags)
	{
		m_NeoFlags.GetForModify() = (GetNeoFlags() | flags);
	}

	void RemoveNeoFlag(int flags)
	{
		m_NeoFlags.GetForModify() = (GetNeoFlags() & ~flags);
	}

	int GetNeoFlags() const { return m_NeoFlags.Get(); }

	void GiveLoadoutWeapon(void);
	void SetPlayerTeamModel(void);
	void SetDeadModel(const CTakeDamageInfo& info);
	void SetPlayerCorpseModel(int type);
	virtual void PickDefaultSpawnTeam(void) OVERRIDE;

	virtual bool StartObserverMode(int mode) OVERRIDE;
	virtual void StopObserverMode(void) OVERRIDE;
	virtual bool ModeWantsSpectatorGUI(int iMode) override;

	virtual bool	CanHearAndReadChatFrom(CBasePlayer *pPlayer) OVERRIDE;

	bool IsCarryingGhost(void) const;

	void Weapon_AimToggle(CNEOBaseCombatWeapon *pWep, const NeoWeponAimToggleE toggleType);

	const char *InternalGetNeoPlayerName(const CNEO_Player *viewFrom = nullptr) const;
	// "neo_name" if available otherwise "name"
	// Set "viewFrom" if fetching the name in the view of another player
	const char *GetNeoPlayerName(const CNEO_Player *viewFrom = nullptr) const;
	// "neo_name" even if it's nothing
	const char *GetNeoPlayerNameDirect() const;
	void SetNeoPlayerName(const char *newNeoName);
	void SetClientWantNeoName(const bool b);
	const char *GetNeoClantag() const;

	void Lean(void);
	void SoftSuicide(void);
	void GiveAllItems(void);
	bool ProcessTeamSwitchRequest(int iTeam);

	void Weapon_SetZoom(const bool bZoomIn);

	void SuperJump(void);

	void RequestSetClass(int newClass);
	void RequestSetSkin(int newSkin);
	bool RequestSetLoadout(int loadoutNumber);
	void RequestSetStar(int newStar);

	int GetSkin() const { return m_iNeoSkin; }
	int GetClass() const { return m_iNeoClass; }
	int GetStar() const { return m_iNeoStar; }
	bool IsInAim() const { return m_bInAim; }
	int GetBotDetectableBleedingInjuryEvents() const { return m_iBotDetectableBleedingInjuryEvents; }

	bool IsAirborne() const { return (!(GetFlags() & FL_ONGROUND)); }

	bool GetInThermOpticCamo() const { return m_bInThermOpticCamo; }
	// bots can't see anything, so they need an additional timer for cloak disruption events
	bool GetBotPerceivedCloakState() const { return m_botThermOpticCamoDisruptedTimer.IsElapsed() && m_bInThermOpticCamo; }
	bool GetSpectatorTakeoverPlayerPending() const { return m_bSpectatorTakeoverPlayerPending; }

	virtual void StartAutoSprint(void) OVERRIDE;
	virtual void StartSprinting(void) OVERRIDE;
	virtual void StopSprinting(void) OVERRIDE;
	virtual void InitSprinting(void) OVERRIDE;
	virtual bool CanSprint(void) OVERRIDE;
	virtual void EnableSprint(bool bEnable) OVERRIDE;

	virtual void StartWalking(void) OVERRIDE;
	virtual void StopWalking(void) OVERRIDE;

	// Cloak Power Interface
	float CloakPower_Get(void) const ;
	void CloakPower_Update(void);
	bool CloakPower_Drain(float flPower);
	void CloakPower_Charge(float flPower);
	float CloakPower_Cap() const;

	bool CanBreatheUnderwater() const override { return false; }

	float GetNormSpeed_WithActiveWepEncumberment(void) const;
	float GetCrouchSpeed_WithActiveWepEncumberment(void) const;
	float GetSprintSpeed_WithActiveWepEncumberment(void) const;
	float GetNormSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetCrouchSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetSprintSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetNormSpeed(void) const;
	float GetCrouchSpeed(void) const;
	float GetSprintSpeed(void) const;

	void HandleSpeedChangesLegacy();
#if 0
	void HandleSpeedChanges( CMoveData *mv ) override;
#endif
	
	int ShouldTransmit( const CCheckTransmitInfo *pInfo) OVERRIDE;

	int GetAttackersScores(const int attackerIdx) const;
	int GetAttackerHits(const int attackerIdx) const;

	void SetNameDupePos(const int dupePos);
	int NameDupePos() const;

	AttackersTotals GetAttackersTotals() const;
	void StartShowDmgStats(const CTakeDamageInfo *info);

	void AddPoints(int score, bool bAllowNegativeScore);
	inline void SetDeathTime(const float deathTime) { m_flDeathTime.Set(deathTime); }

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED(m_EyeAngleOffset);

	void InputSetPlayerModel( inputdata_t & inputData );
	void InputRefillAmmo( inputdata_t & inputData );
	void CloakFlash(float time = 0.f);

	void BecomeJuggernaut();
	void SpawnJuggernautPostDeath();

private:
	bool m_bAllowGibbing;

	// tracks time since last cloak disruption event for bots who can't actually see
	CountdownTimer m_botThermOpticCamoDisruptedTimer;

private:
	float GetActiveWeaponSpeedScale() const;

private:
	void CheckThermOpticButtons();
	void CheckVisionButtons();
	void CheckLeanButtons();
	void PlayCloakSound(bool removeLocalPlayer = true);
	void SetCloakState(bool state);

	bool IsAllowedToSuperJump(void);

public:
	CNetworkVar(int, m_iNeoClass);
	CNetworkVar(int, m_iNeoSkin);
	CNetworkVar(int, m_iNeoStar);

	CNetworkVar(int, m_iXP);

	CNetworkVar(int, m_iLoadoutWepChoice);
	CNetworkVar(int, m_iNextSpawnClassChoice);

	CNetworkVar(bool, m_bShowTestMessage);
	CNetworkString(m_pszTestMessage, 32 * 2 + 1);

	CNetworkVar(bool, m_bInThermOpticCamo);
	CNetworkVar(bool, m_bLastTickInThermOpticCamo);
	CNetworkVar(bool, m_bInVision);
	CNetworkVar(bool, m_bHasBeenAirborneForTooLongToSuperJump);
	CNetworkVar(bool, m_bInAim);
	CNetworkVar(int, m_bInLean);
	CNetworkVar(bool, m_bCarryingGhost);
	CNetworkVar(bool, m_bIneligibleForLoadoutPick);
	CNetworkHandle(CBaseEntity, m_hDroppedJuggernautItem);

	CNetworkVar(float, m_flCamoAuxLastTime);
	CNetworkVar(int, m_nVisionLastTick);
	CNetworkVar(float, m_flJumpLastTime);
	CNetworkVar(float, m_flNextPingTime);

	CNetworkArray(int, m_rfAttackersScores, MAX_PLAYERS);
	CNetworkArray(float, m_rfAttackersAccumlator, MAX_PLAYERS);
	CNetworkArray(int, m_rfAttackersHits, MAX_PLAYERS);

	CNetworkVar(unsigned char, m_NeoFlags);
	CNetworkString(m_szNeoName, MAX_PLAYER_NAME_LENGTH);
	CNetworkString(m_szNeoClantag, NEO_MAX_CLANTAG_LENGTH);
	CNetworkString(m_szNeoCrosshair, NEO_XHAIR_SEQMAX);
	CNetworkVar(int, m_szNameDupePos);

	// NEO NOTE (nullsystem): As dumb as client sets -> server -> client it may sound,
	// cl_onlysteamnick directly doesn't even work properly for client set convars anyway
	CNetworkVar(bool, m_bClientWantNeoName);

	bool m_bIsPendingSpawnForThisRound;
	bool m_bSpawnedThisRound = false;
	bool m_bKilledInflicted = false; // Server-side var only
	int m_iTeamDamageInflicted = 0;
	int m_iTeamKillsInflicted = 0;
	bool m_bIsPendingTKKick = false; // To not spam the kickid ConCommand
	EMenuSelectType m_eMenuSelectType = MENU_SELECT_TYPE_NONE;
	bool m_bClientStreamermode = false;

	// Bot-only usage
	float m_flRanOutSprintTime = 0.0f;

private:
	bool m_bFirstDeathTick;
	bool m_bCorpseSet;
	bool m_bPreviouslyReloading;
	bool m_szNeoNameHasSet;

	float m_flLastAirborneJumpOkTime;
	float m_flLastSuperJumpTime;

	// Non-network version of m_szNeoName with dupe checker index
	mutable char m_szNeoNameWDupeIdx[MAX_PLAYER_NAME_LENGTH + 10];
	mutable bool m_szNeoNameWDupeIdxNeedUpdate;

	// blood decals are client-side, so track injury event count for bots
	int m_iBotDetectableBleedingInjuryEvents = 0;
	bool m_bSpectatorTakeoverPlayerPending{false};

private:
	CNEO_Player(const CNEO_Player&);

	// Spectator takeover player related functionality
	bool IsAFK(); // not const because GetTimeSinceLastUserCommand is non-const
	void SpectatorTryReplacePlayer(CNEO_Player* pNeoPlayerToReplace);
	void SpectatorTakeoverPlayerPreThink();
	void RestorePlayerFromSpectatorTakeover();
	void RestorePlayerFromSpectatorTakeover(const CTakeDamageInfo &pInfo);
	void SpectatorTakeoverPlayerInitiate(CNEO_Player* pPlayer);
	void SpectatorTakeoverPlayerRevert(CNEO_Player* pPlayer);

	CHandle<CNEO_Player> m_hSpectatorTakeoverPlayerTarget{nullptr};
	CHandle<CNEO_Player> m_hSpectatorTakeoverPlayerImpersonatingMe{nullptr};

};

inline CNEO_Player *ToNEOPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
	{
		return nullptr;
	}
	return assert_cast<CNEO_Player*>(pEntity);
}

#endif // NEO_PLAYER_H
