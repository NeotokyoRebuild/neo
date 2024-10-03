//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#ifndef SQUAD_H
#define SQUAD_H

#ifdef _WIN32
#pragma once
#endif

#include "bots\bot_defs.h"

#ifdef INSOURCE_DLL
#include "in_player.h"
#endif

class CBotSquad;
typedef CUtlVector<EHANDLE> MembersVector;

struct SquadOptions_t
{

};

//================================================================================
// Define un escuadron, el enlace para comunicarse entre miembros
//================================================================================
class CSquad
{
public:
    CSquad();

    virtual void Think();

    virtual const char *GetName() { return STRING(m_nName);  }
    virtual bool IsNamed( const char *name );
    virtual void SetName( const char *name );

    virtual CPlayer *GetLeader();
    virtual void SetLeader( CPlayer *pMember );

    virtual int GetCount() { return m_nMembers.Count(); }
    virtual bool IsEmpty() { return m_nMembers.Count() == 0; }
    virtual int GetActiveCount();

    virtual int GetMemberIndex( CPlayer *pMember );
    virtual CPlayer *GetMember( int index );
    virtual CPlayer *GetRandomMember();

    virtual bool IsMember( CPlayer *pMember );
    virtual void AddMember( CPlayer *pMember );
    virtual void PrepareBot( IBot *pBot );

    virtual void RemoveMember( CPlayer *pMember );
    virtual void RemoveMember( int index );

    virtual void SetMemberLimit( int limit ) { m_iMemberLimit = limit; }
    virtual int GetMemberLimit() { return m_iMemberLimit; }

    virtual void SetTacticalMode( int value ) { m_iTacticalMode = value; }
    virtual int GetTacticalMode() { return m_iTacticalMode; }

    virtual void SetStrategie( BotStrategie value ) { m_iStrategie = value; }
    virtual BotStrategie GetStrategie() { return m_iStrategie; }

    virtual void SetSkill( int skill ) { m_iSkill = skill; }
    virtual int GetSkill() { return m_iSkill; }

    virtual void SetFollowLeader( bool value ) { m_bFollowLeader = value; }
    virtual bool ShouldFollowLeader() { return m_bFollowLeader; }

    virtual bool IsSomeoneLooking( CBaseEntity *pTarget, CPlayer *pIgnore = NULL );
    virtual bool IsSomeoneLooking( const Vector &vecTarget, CPlayer *pIgnore = NULL );
    virtual bool IsSomeoneGoing( const Vector &vecDestination, CPlayer *pIgnore = NULL );

    virtual CBotSquad *GetController() { return m_nController; }
    virtual void SetController( CBotSquad *pEntity ) { m_nController = pEntity; }

public:
	virtual bool IsSquadEnemy( CBaseEntity *pEntity, CPlayer *pIgnore = NULL );

public:
    virtual void ReportTakeDamage( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void ReportDeath( CPlayer *pMember, const CTakeDamageInfo &info );
    virtual void ReportEnemy( CPlayer *pMember, CBaseEntity *pEnemy );

public:
    MembersVector m_nMembers;

protected:
    string_t m_nName;
    CPlayer *m_nLeader;
    CBotSquad *m_nController;

    // Opciones
    int m_iMemberLimit;
    int m_iTacticalMode;
    BotStrategie m_iStrategie;
    int m_iSkill;
    bool m_bFollowLeader;
    
};

#endif // SQUAD_H