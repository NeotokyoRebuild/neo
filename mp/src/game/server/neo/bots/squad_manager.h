//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#ifndef SQUAD_MANAGER_H
#define SQUAD_MANAGER_H

#ifdef _WIN32
#pragma once
#endif

#include "squad.h"

//================================================================================
// Administrador de escuadrones
//================================================================================
class CSquadManager : public CAutoGameSystemPerFrame
{
public:
    CSquadManager();

public:
    virtual void LevelShutdownPostEntity();
    virtual void FrameUpdatePostEntityThink();

public:
    virtual CSquad *GetSquad( const char *name );
    virtual CSquad *GetOrCreateSquad( const char *name );
    virtual void AddSquad( CSquad *pSquad );

protected:
    CUtlVector<CSquad *> m_nSquads;
};

extern CSquadManager *TheSquads;

#endif // SQUAD_MANAGER_H