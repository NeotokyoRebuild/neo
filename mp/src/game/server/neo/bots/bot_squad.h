//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef BOT_SQUAD_H
#define BOT_SQUAD_H

#ifdef _WIN32
#pragma once
#endif

//================================================================================
// Spawn Flags
//================================================================================

//#define SF_SPAWN_IMMEDIATELY 1

//================================================================================
// Una entidad para configurar un escuadron
//================================================================================
class CBotSquad : public CBaseEntity
{
public:
    DECLARE_CLASS( CBotSquad, CBaseEntity );
    DECLARE_DATADESC();

    virtual int ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
    virtual void Spawn();

    // Inputs
    void InputDestroy( inputdata_t &inputdata );

protected:
    COutputEvent m_OnMemberDead;
    COutputEHANDLE m_OnReportEnemy;
    COutputEHANDLE m_OnReportPlayerEnemy;
    COutputEHANDLE m_OnReportHumanEnemy;
    COutputEvent m_OnReportDamage;
    COutputEHANDLE m_OnNewLeader;

    friend class CSquad;

protected:
    string_t m_nSquadName;
    int m_iSquadLimit;
    int m_iSquadTacticalMode;
    int m_iSquadStrategie;
    int m_iSquadSkill;
    bool m_bFollowLeader;

    // Outputs
    
};

#endif // BOT_SQUAD_H
