//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\squad_manager.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CSquadManager g_SquadManager;
CSquadManager *TheSquads = &g_SquadManager;

//================================================================================
// Comandos
//================================================================================

//================================================================================
//================================================================================
CSquadManager::CSquadManager() : CAutoGameSystemPerFrame("SquadManager")
{
}

//================================================================================
//================================================================================
void CSquadManager::LevelShutdownPostEntity() 
{
    // Eliminamos los escuadrones
    m_nSquads.RemoveAll();
}

//================================================================================
//================================================================================
void CSquadManager::FrameUpdatePostEntityThink() 
{
    // No hay escuadrones todavía
    if ( m_nSquads.Count() == 0 )
        return;

    // Cada escuadron debe verificar a sus miembros
    FOR_EACH_VEC( m_nSquads, it )
    {
        CSquad *pSquad = m_nSquads.Element(it);
        pSquad->Think();
    }
}

//================================================================================
//================================================================================
CSquad *CSquadManager::GetSquad( const char *name )
{
    // No hay escuadrones todavía
    if ( m_nSquads.Count() == 0 )
        return NULL;

    FOR_EACH_VEC( m_nSquads, it )
    {
        CSquad *pSquad = m_nSquads.Element(it);

        // No es el que buscamos
        if ( !pSquad->IsNamed(name) )
            continue;

        return pSquad;
    }

    return NULL;
}

//================================================================================
//================================================================================
CSquad *CSquadManager::GetOrCreateSquad( const char *name )
{
    // Verificamos si ya existe
    CSquad *pSquad = GetSquad( name );

    // yep
    if ( pSquad )
        return pSquad;

    // Creamos el escuadron
    pSquad = new CSquad();
    pSquad->SetName( name );

    return pSquad;
}

//================================================================================
//================================================================================
void CSquadManager::AddSquad( CSquad *pSquad )
{
    int index = m_nSquads.Find( pSquad );

    // Ya se encuentra en nuestra lista!
    if ( index > -1 )
        return;

    // Lo agregamos
    m_nSquads.AddToTail( pSquad );
}