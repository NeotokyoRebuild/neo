#pragma once

#include "cbase.h"
#ifdef GAME_DLL
#include "neo_player.h"
#else
#include "c_neo_player.h"
#endif

#ifdef CLIENT_DLL
#define CNEO_Juggernaut C_NEO_Juggernaut
#endif

class CNEO_Juggernaut : public CBaseAnimating
{
public:
	DECLARE_CLASS(CNEO_Juggernaut, CBaseAnimating);
	
	static float GetUseDuration();
	static float GetUseDistanceSquared();
#ifdef GAME_DLL
	virtual ~CNEO_Juggernaut();
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
#endif
	DECLARE_DATADESC();

#ifdef GAME_DLL
	void	Precache(void);
	void	Spawn(void);
    void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	virtual int	ObjectCaps(void) { return BaseClass::ObjectCaps() | FCAP_ONOFF_USE; }
	virtual int UpdateTransmitState() override;

	const bool IsBeingActivatedByLosingTeam();

	bool	m_bPostDeath = false;
#endif

	virtual unsigned int PhysicsSolidMaskForEntity() const final override { return MASK_PLAYERSOLID; }
	virtual void UpdateOnRemove() override;

	CNetworkVar(bool, m_bLocked);

private:
#ifdef GAME_DLL
	void	Think(void);
	void	SetSoftCollision(bool soft);
	void	MakePushThink();
	void	DisableSoftCollisionsThink();
	void	HoldCancel(void);
	void	AnimThink(void);
	void	InputLock(inputdata_t& inputData) { m_bLocked = true; }
	void	InputUnlock(inputdata_t& inputData) { m_bLocked = false; }
#else
	int		DrawModel(int flags) override;
#endif

#ifdef GAME_DLL
	CHandle<CNEO_Player> m_hHoldingPlayer;
	EHANDLE m_hPush;
	float m_flWarpedPlaybackRate;
	float m_flHoldStartTime = 0.0f;
	bool m_bIsHolding = false;
	bool m_bActivationRemoval = false;

	hudtextparms_t	m_textParms;
	COutputEvent m_OnPlayerActivate;
#endif
};