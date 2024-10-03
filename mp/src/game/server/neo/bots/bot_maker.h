//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_MAKER_H
#define BOT_MAKER_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot_defs.h"

//================================================================================
// Spawn Flags
//================================================================================

#define SF_SPAWN_IMMEDIATELY 1
#define SF_HIDE_FROM_PLAYERS 2
#define SF_KICK_ON_DEAD 4
#define SF_ONLY_ONE_ACTIVE_BOT 8
#define SF_USE_SPAWNER_POSITION 16
#define SF_PEACEFUL 32
#define SF_DONT_DROP_WEAPONS 64

//================================================================================
// Una entidad para crear un Bot
//================================================================================
class CBotSpawn : public CBaseEntity
{
public:
    DECLARE_CLASS( CBotSpawn, CBaseEntity );
    DECLARE_DATADESC();

    virtual int  ObjectCaps() { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
    virtual CPlayer *GetPlayer() { return m_pPlayer.Get(); }

    virtual void Spawn();
    virtual void DeathNotice( CBaseEntity *pVictim );

    virtual bool CanMake( bool bRespawn = false );
    virtual void SpawnBot();
    virtual void SetUpBot();

    // Inputs
    void InputSpawn( inputdata_t &inputdata );
	void InputRespawn( inputdata_t &inputdata );
    void InputEnable( inputdata_t &inputdata );
    void InputDisable( inputdata_t &inputdata );
    void InputToggle( inputdata_t &inputdata );

    void InputSetSkill( inputdata_t &inputdata );
    void InputSetTacticalMode( inputdata_t &inputdata );
    void InputBlockLook( inputdata_t &inputdata );
    void InputSetSquad( inputdata_t &inputdata );
    void InputDisableMovement( inputdata_t &inputdata );
    void InputEnableMovement( inputdata_t &inputdata );
    void InputStartPeaceful( inputdata_t &inputdata );
    void InputStopPeaceful( inputdata_t &inputdata );

    void InputDriveTo( inputdata_t &inputdata );

protected:
    string_t m_nBotTargetname;
    string_t m_nBotPlayername;
    string_t m_nBotSquadname;
    string_t m_nAdditionalEquipment;
	string_t m_nFollowEntity;

    int m_iBotTeam;
    int m_iBotClass;

    int m_iBotSkill;
    int m_iBotTacticalMode;
	int m_iBlockLookAround;
    int m_iPerformance;

    bool m_bIsLeader;
    bool m_bDisabledMovement;
    bool m_bDisabled;

	CHandle<CPlayer> m_pPlayer;

    // Outputs
    COutputEHANDLE m_OnSpawn;
    COutputEvent m_OnBotDead;
};

#endif // BOT_MAKER_H