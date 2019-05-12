#ifndef NEO_GAMERULES_H
#define NEO_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

#include "gamerules.h"
#include "teamplay_gamerules.h"
#include "gamevars_shared.h"
#include "hl2mp_gamerules.h"
#include "shareddefs.h"

#ifndef CLIENT_DLL
    #include "neo_player.h"
#endif

enum
{
	TEAM_JINRAI = LAST_SHARED_TEAM + 1,
	TEAM_NSF,
};

#define TEAM_STR_JINRAI "Jinrai"
#define TEAM_STR_NSF "NSF"
#define TEAM_STR_SPEC "Spectator"

#define NEO_GAME_NAME "Neotokyo: Revamp"

#ifdef CLIENT_DLL
	#define CNEORules C_NEORules
	#define CNEOGameRulesProxy C_NEOGameRulesProxy
#endif

class CNEOGameRulesProxy : public CHL2MPGameRulesProxy
{
public:
    DECLARE_CLASS( CNEOGameRulesProxy, CHL2MPGameRulesProxy );
    DECLARE_NETWORKCLASS();
};

class NEOViewVectors : public HL2MPViewVectors
{
public:
	NEOViewVectors( 
        // Same as HL2MP, passed to parent ctor
		Vector vView,
		Vector vHullMin,
		Vector vHullMax,
		Vector vDuckHullMin,
		Vector vDuckHullMax,
		Vector vDuckView,
		Vector vObsHullMin,
		Vector vObsHullMax,
		Vector vDeadViewHeight,
		Vector vCrouchTraceMin,
		Vector vCrouchTraceMax,
        
        // NEO specific
        Vector vViewLeanLeft,
        Vector vViewLeanRight,
        Vector vViewAngLeanLeft,
        Vector vViewAngLeanRight,
        Vector vHullLeanLeftMin,
        Vector vHullLeanLeftMax,
        Vector vHullLeanRightMin,
        Vector vHullLeanRightMax ) :
			HL2MPViewVectors( 
                vView,
                vHullMin,
                vHullMax,
                vDuckHullMin,
                vDuckHullMax,
                vDuckView,
                vObsHullMin,
                vObsHullMax,
                vDeadViewHeight,
                vCrouchTraceMin,
                vCrouchTraceMax )
	{
        m_vViewLeanLeft = vViewLeanLeft;
        m_vViewLeanRight = vViewLeanRight;

        m_vViewAngLeanLeft = vViewAngLeanLeft;
        m_vViewAngLeanRight = vViewAngLeanRight;

        m_vHullLeanLeftMin = vHullLeanLeftMin;
        m_vHullLeanLeftMax = vHullLeanLeftMax;
        m_vHullLeanRightMin = vHullLeanRightMin;
        m_vHullLeanRightMax = vHullLeanRightMax;
	}

    Vector m_vViewLeanLeft, m_vViewLeanRight;
    Vector m_vViewAngLeanLeft, m_vViewAngLeanRight;
    Vector m_vHullLeanLeftMin, m_vHullLeanLeftMax;
    Vector m_vHullLeanRightMin, m_vHullLeanRightMax;
};

class CNEORules : public CHL2MPRules
{
public:
    DECLARE_CLASS( CNEORules, CHL2MPRules );

// This makes datatables able to access our private vars.
#ifdef CLIENT_DLL
    DECLARE_CLIENTCLASS_NOBASE();
#else
    DECLARE_SERVERCLASS_NOBASE();
#endif

    CNEORules();
    virtual ~CNEORules();

    virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

    virtual void Think( void );
    virtual void CreateStandardEntities( void );
    virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
    virtual const char *GetGameDescription( void );
    virtual const CViewVectors* GetViewVectors() const;
    const NEOViewVectors* GetNEOViewVectors() const;

    float GetMapRemainingTime();
	void CleanUpMap();
	void CheckRestartGame();
	void RestartGame();
};

inline CNEORules *NEORules()
{
    return static_cast<CNEORules*>(g_pGameRules);
}

#endif // NEO_GAMERULES_H