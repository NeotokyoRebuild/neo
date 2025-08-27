#include "cbase.h"
#ifdef GAME_DLL
#include "neo_player.h"
#else
#include "c_neo_player.h"
#endif

class CNEO_Juggernaut : public CBaseAnimating
{
public:
	DECLARE_CLASS(CNEO_Juggernaut, CBaseAnimating);
#ifdef GAME_DLL
	DECLARE_SERVERCLASS();
#else
	DECLARE_CLIENTCLASS();
#endif
	DECLARE_DATADESC();

	void	Precache(void);
	void	Spawn(void);
    void	Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

	virtual int	ObjectCaps(void) { return BaseClass::ObjectCaps() | FCAP_ONOFF_USE; }

	bool m_bPostDeath = false;

private:
	void	Think(void);
	void	HoldCancel(void);
	void	AnimThink(void);
#ifdef CLIENT_DLL
	int		DrawModel(int flags) override;
#endif

	CHandle<CNEO_Player>	m_hPlayer;
	float m_flWarpedPlaybackRate;

	CNetworkVar(float, m_flHoldStartTime);
	CNetworkVar(bool, m_bIsHolding);

#ifdef GAME_DLL
	hudtextparms_t	m_textParms;
	COutputEvent m_OnPlayerActivate;
#endif
};