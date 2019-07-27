#ifndef NEO_PLAYER_H
#define NEO_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

class C_NEO_Player;
#include "c_hl2mp_player.h"

#include "neo_player_shared.h"

class C_NEOPredictedViewModel;
class CNEOHud_Compass;
class CNEOHud_GameEvent;
class CNEOHud_GhostMarker;
class CNEOHud_FriendlyMarker;

class C_NEO_Player : public C_HL2MP_Player
{
public:
	DECLARE_CLASS(C_NEO_Player, C_HL2MP_Player);

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();
	DECLARE_INTERPOLATION();

	C_NEO_Player();
	virtual ~C_NEO_Player();

	static C_NEO_Player *GetLocalNEOPlayer();

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

	bool ShouldDrawHL2StyleQuickHud( void );

	virtual void SetLocalViewAngles( const QAngle &viewAngles )
	{
		BaseClass::SetLocalViewAngles(viewAngles);
	}
	virtual void SetViewAngles( const QAngle& ang )
	{
		BaseClass::SetViewAngles(ang);
	}

	inline void DrawCompass(void);

	void Weapon_AimToggle(C_BaseCombatWeapon *pWep);
	inline void Weapon_SetZoom(bool bZoomIn);

	void Weapon_Drop(C_BaseCombatWeapon *pWeapon);

	C_NEOPredictedViewModel *GetNEOViewModel();

	inline void ZeroFriendlyPlayerLocArray(void);

	bool IsCloaked() const { return m_bInThermOpticCamo; }

private:
	inline void CheckThermOpticButtons();

public:
	bool m_bShowTestMessage;
	char m_pszTestMessage[32 * 2 + 1];
	//wchar_t m_pszTestMessage;

	int m_iCapTeam;

	Vector m_rvFriendlyPlayerPositions[MAX_PLAYERS];

	bool m_bShowClassMenu, m_bShowTeamMenu;

protected:
	Vector m_vecGhostMarkerPos;

	int m_iGhosterTeam;

	bool m_bGhostExists;
	bool m_bIsClassMenuOpen, m_bIsTeamMenuOpen;
	bool m_bInThermOpticCamo, m_bUnhandledTocChange;

	int m_iNeoClass;
	int m_iNeoSkin;

private:
	CNEOHud_Compass *m_pCompass;

	CNEOHud_GameEvent *m_pHudEvent_Test;

	CNEOHud_GhostMarker *m_pGhostMarker;

	CNEOHud_FriendlyMarker *m_pFriendlyMarker;

private:
	C_NEO_Player(const C_NEO_Player &);
};

inline C_NEO_Player *ToNEOPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
	{
		return NULL;
	}

	return dynamic_cast<C_NEO_Player*>(pEntity);
}

extern ConVar cl_autoreload_when_empty;
extern ConVar cl_drawhud_quickinfo;
extern ConCommand teammenu;

#endif // NEO_PLAYER_H