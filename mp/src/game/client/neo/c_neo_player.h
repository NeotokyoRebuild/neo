#ifndef NEO_PLAYER_H
#define NEO_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "in_buttons.h"

class C_NEO_Player;
#include "c_hl2mp_player.h"

#include "neo_player_shared.h"

class C_NEOPredictedViewModel;

class CNeoHudElements;

class C_NEO_Player : public C_HL2MP_Player
{
public:
	DECLARE_CLASS(C_NEO_Player, C_HL2MP_Player);

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_NEO_Player();
	virtual ~C_NEO_Player();

	static C_NEO_Player *GetLocalNEOPlayer() { return static_cast<C_NEO_Player*>(C_BasePlayer::GetLocalPlayer()); }

	virtual int DrawModel( int flags );
	virtual void AddEntity( void );

	// Should this object cast shadows?
	virtual ShadowType_t		ShadowCastType( void );

	virtual C_BaseAnimating *BecomeRagdollOnClient();
	virtual const QAngle& GetRenderAngles();
	virtual bool ShouldDraw( void );
	//virtual bool ShouldInterpolate() { return true; }
	virtual void OnDataChanged( DataUpdateType_t type );
	virtual float GetFOV( void );
	virtual CStudioHdr *OnNewModel( void );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void ItemPreFrame( void );
	virtual void ItemPostFrame( void );
	virtual float GetMinFOV()	const;
	virtual Vector GetAutoaimVector( float flDelta );
	virtual void NotifyShouldTransmit( ShouldTransmitState_t state );
	virtual void CreateLightEffects( void );
	virtual bool ShouldReceiveProjectedTextures( int flags );
	virtual void PostDataUpdate( DataUpdateType_t updateType );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void DoImpactEffect( trace_t &tr, int nDamageType );
	IRagdoll* GetRepresentativeRagdoll() const;
	virtual void CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov );
	virtual const QAngle& EyeAngles( void );

	virtual void ClientThink( void );
	virtual void PreThink( void );
	virtual void PostThink( void );
	virtual void Spawn( void );

	virtual void StartSprinting(void);
	virtual void StopSprinting(void);
	virtual bool CanSprint(void);

	virtual void StartWalking(void);
	virtual void StopWalking(void);

	void Lean(void);

	virtual const Vector GetPlayerMaxs(void) const;

	// Implementing in header in hopes of compiler picking up the inlined base method
	virtual float GetModelScale() const
	{
		switch (GetClass())
		{
		case NEO_CLASS_RECON:
			return C_BaseAnimating::GetModelScale() * NEO_RECON_MODEL_SCALE;
		case NEO_CLASS_SUPPORT:
			return C_BaseAnimating::GetModelScale() * NEO_SUPPORT_MODEL_SCALE;
		default:
			return C_BaseAnimating::GetModelScale() * NEO_ASSAULT_MODEL_SCALE;
		}
	}

	float GetNormSpeed_WithActiveWepEncumberment(void) const;
	float GetCrouchSpeed_WithActiveWepEncumberment(void) const;
	float GetWalkSpeed_WithActiveWepEncumberment(void) const;
	float GetSprintSpeed_WithActiveWepEncumberment(void) const;
	float GetNormSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetCrouchSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetWalkSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetSprintSpeed_WithWepEncumberment(CNEOBaseCombatWeapon* pNeoWep) const;
	float GetNormSpeed(void) const;
	float GetCrouchSpeed(void) const;
	float GetWalkSpeed(void) const;
	float GetSprintSpeed(void) const;

private:
	float GetActiveWeaponSpeedScale() const;
	float GetBackwardsMovementPenaltyScale() const { return ((m_nButtons & IN_BACK) ? NEO_SLOW_MODIFIER : 1.0); }

public:
	bool ShouldDrawHL2StyleQuickHud( void );

	int GetClass() const { return m_iNeoClass; }

	bool IsCarryingGhost(void);

	virtual void SetLocalViewAngles( const QAngle &viewAngles ) OVERRIDE
	{
		BaseClass::SetLocalViewAngles(viewAngles);
	}
	virtual void SetViewAngles( const QAngle& ang ) OVERRIDE
	{
		BaseClass::SetViewAngles(ang);
	}

	void SuperJump(void);

	void DrawCompass(void);

	void Weapon_AimToggle(C_BaseCombatWeapon *pWep);
	void Weapon_SetZoom(const bool bZoomIn);

	void Weapon_Drop(C_BaseCombatWeapon *pWeapon);

	C_NEOPredictedViewModel *GetNEOViewModel() { return static_cast<C_NEOPredictedViewModel*>(GetViewModel()); }

	inline void ZeroFriendlyPlayerLocArray(void);

	bool IsCloaked() const { return m_bInThermOpticCamo; }
	bool IsAirborne() const { return (!(GetFlags() & FL_ONGROUND)); }
	bool IsInVision() const { return m_bInVision; }
	bool IsInAim() const { return m_bInAim; }

private:
	void CheckThermOpticButtons();
	void CheckVisionButtons();
	void PlayCloakSound();

	bool IsAllowedToSuperJump(void);

public:
	CNetworkVar(bool, m_bShowTestMessage);
	CNetworkString(m_pszTestMessage, 32 * 2 + 1);
	//wchar_t m_pszTestMessage;

	CNetworkVar(int, m_iXP);
	CNetworkVar(int, m_iCapTeam);
	CNetworkVar(int, m_iLoadoutWepChoice);
	CNetworkVar(int, m_iNextSpawnClassChoice);

	CNetworkArray(Vector, m_rvFriendlyPlayerPositions, MAX_PLAYERS);

	bool m_bShowClassMenu, m_bShowTeamMenu;
	CNetworkVar(bool, m_bHasBeenAirborneForTooLongToSuperJump);

	CNetworkVar(bool, m_bGhostExists);

	CNetworkVar(float, m_flCamoAuxLastTime);

	CNetworkVector(m_vecGhostMarkerPos);

	CNetworkVar(int, m_iGhosterTeam);

	CNetworkVar(bool, m_bInThermOpticCamo);
	CNetworkVar(bool, m_bLastTickInThermOpticCamo);
	CNetworkVar(bool, m_bInVision);
	CNetworkVar(bool, m_bInAim);

	CNetworkVar(int, m_iNeoClass);
	CNetworkVar(int, m_iNeoSkin);

protected:
	bool m_bIsClassMenuOpen, m_bIsTeamMenuOpen;

private:
	bool m_bFirstDeathTick;
	bool m_bPreviouslyReloading;
	bool m_bPreviouslyPreparingToHideMsg;

	CNeoHudElements *m_pNeoPanel;

	float m_flLastAirborneJumpOkTime;
	float m_flLastSuperJumpTime;

private:
	C_NEO_Player(const C_NEO_Player &);
};

inline C_NEO_Player *ToNEOPlayer(C_BaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
	{
		return NULL;
	}
#if _DEBUG
	Assert(dynamic_cast<C_NEO_Player*>(pEntity));
#endif
	return static_cast<C_NEO_Player*>(pEntity);
}

extern ConVar cl_drawhud_quickinfo;
extern ConVar loadout_choice;

extern ConCommand teammenu;

#endif // NEO_PLAYER_H
