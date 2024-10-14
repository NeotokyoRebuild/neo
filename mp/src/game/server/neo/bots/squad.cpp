//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
// Authors: 
// Iván Bravo Bravo (linkedin.com/in/ivanbravobravo), 2017

#include "cbase.h"
#include "bots\squad.h"

#include "bots\squad_manager.h"

#include "bots\bot.h"
#include "bots\bot_squad.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define FOR_EACH_MEMBER( code ) FOR_EACH_VEC( m_nMembers, it ) { \
    CPlayer *pMember = GetMember( it ); \
    if ( !pMember ) \
        continue; \
    code \
}

//================================================================================
//================================================================================

DECLARE_REPLICATED_COMMAND( sv_squad_replace_leader, "1", "" );

//================================================================================
// Constructor
//================================================================================
CSquad::CSquad()
{
    // Predeterminado
    SetName( "WithoutName" );
    SetMemberLimit( 0 );
    SetTacticalMode( 99 );
    SetStrategie( ENDURE_UNTIL_DEATH );
    SetSkill( 99 );
    SetFollowLeader( false );
    SetLeader( NULL );

    m_nController = NULL;

    // Nos agregamos a la lista
    TheSquads->AddSquad( this );
}

//================================================================================
// Pensamiento
//================================================================================
void CSquad::Think()
{
    if ( IsEmpty() )
        return;

    // Checamos a los miembros
    FOR_EACH_VEC( m_nMembers, it )
    {
        CPlayer *pMember = GetMember( it );

        if ( !pMember ) {
            RemoveMember( it );
            continue;
        }

        if ( !pMember->IsAlive() )
            RemoveMember( pMember );
    }
}

//================================================================================
// Devuelve si el escuadron se llama así
//================================================================================
bool CSquad::IsNamed( const char *name )
{
    const char *myName = GetName();
    return FStrEq( myName, name );
}

//================================================================================
// Establece el nombre del escuadron
//================================================================================
void CSquad::SetName( const char *name )
{
    m_nName = MAKE_STRING( name );

    if ( IsNamed( "player" ) ) {
        SetFollowLeader( true );
    }
}

//================================================================================
// Devuelve el líder del escuadron
//================================================================================
CPlayer *CSquad::GetLeader() 
{
    return m_nLeader;
}

//================================================================================
// Establece el líder del escuadron
//================================================================================
void CSquad::SetLeader( CPlayer *member )
{
    if ( member && !IsMember( member ) ) {
        Assert( !"SetLeader == Not Member" );
        return;
    }

    if ( m_nLeader == member )
        return;

    m_nLeader = member;

    if ( member ) {
        DevMsg( "%s es lider del escuadron %s \n", m_nLeader->GetPlayerName(), GetName() );

        FOR_EACH_MEMBER(
        {
            pMember->OnNewLeader( member );

            if ( pMember->GetBotController() && ShouldFollowLeader() ) {
                pMember->GetBotController()->GetFollow()->Start( member );
            }
        });

        if ( GetController() ) {
            variant_t value;
            value.SetEntity( member );
            GetController()->m_OnNewLeader.FireOutput( value, member, NULL );
        }
    }
}

//================================================================================
// Devuelve la cantidad de miembros capaces de atacar
//================================================================================
int CSquad::GetActiveCount() 
{
    int count = 0;

    FOR_EACH_MEMBER(
    {

        // Incapacitado
        //if ( pMember->IsDejected() )
            //continue;

        ++count;
    })    

    return count;
}

//================================================================================
// Devuelve la ID del miembro
//================================================================================
int CSquad::GetMemberIndex( CPlayer *member ) 
{
    FOR_EACH_MEMBER(
    {
        if ( pMember == member )
            return it;
    })    

    return -1;
}

//================================================================================
// Devuelve el miembro en la ID especificada
//================================================================================
CPlayer *CSquad::GetMember( int index ) 
{
    CBaseEntity *pEntity = m_nMembers.Element( index ).Get();

    if ( !pEntity )
        return NULL;

    return ToInPlayer( pEntity );
}

//================================================================================
// Devuelve un miembro al azar
//================================================================================
CPlayer * CSquad::GetRandomMember()
{
    if ( IsEmpty() )
        return NULL;

    return GetMember( RandomInt(0, GetCount()-1) );
}

//================================================================================
// Devuelve si el escuadron ya es miembro del escuadron
//================================================================================
bool CSquad::IsMember( CPlayer *pMember ) 
{
    return ( GetMemberIndex(pMember) > -1 );
}

//================================================================================
// Agrega un jugador al escuadron
//================================================================================
void CSquad::AddMember( CPlayer *pMember )
{
    if ( IsMember( pMember ) )
        return;

    if ( GetMemberLimit() > 0 && GetCount() >= GetMemberLimit() )
        return;

    m_nMembers.AddToTail( pMember );

    if ( GetCount() == 1 )
        SetLeader( pMember );

    if ( pMember->GetBotController() ) {
        PrepareBot( pMember->GetBotController() );
    }
}

//================================================================================
// Prepara al Bot para ser parte del escuadron
//================================================================================
void CSquad::PrepareBot( IBot *pBot ) 
{
    // Modo táctico
    if ( GetTacticalMode() != 99 )
        pBot->SetTacticalMode( GetTacticalMode() );

    // Dificultad
    if ( GetSkill() != 99 )
        pBot->SetSkill( GetSkill() );
    
    // Debes seguir al líder
    if ( GetLeader() && ShouldFollowLeader() ) {
        pBot->GetFollow()->Start( GetLeader() );
    }
}

//================================================================================
//================================================================================
void CSquad::RemoveMember( CPlayer *member ) 
{
    // Obtenemos el index del miembro
    int index = GetMemberIndex( member );

    // No eres miembro!
    if ( index == -1 )
        return;

    // Eras el líder!
    if ( GetLeader() == member )
    {
        SetLeader( NULL );

        // Debemos reemplazarte
        if ( sv_squad_replace_leader.GetBool() )
        {
            // Buscamos el primer miembro activo y lo convertimos en líder
            // @TODO: Una forma más flexible
            FOR_EACH_MEMBER( {
                SetLeader( pMember );
                break;
            } );
        }
    }

    RemoveMember( index );
}

//================================================================================
//================================================================================
void CSquad::RemoveMember( int index ) 
{
    m_nMembers.Remove( index );
}

//================================================================================
// Devuelve si algún miembro del escuadron esta mirando al objetivo
//================================================================================
bool CSquad::IsSomeoneLooking( CBaseEntity * pTarget, CPlayer *pIgnore )
{
    FOR_EACH_VEC( m_nMembers, it )
    {
        CPlayer *pMember = GetMember( it );

        if ( !pMember )
            continue;

        if ( !pMember->IsAlive() )
            continue;

        if ( pMember == pIgnore )
            continue;

        if ( pMember->GetBotController() && pMember->GetBotController()->GetVision() && pMember->GetBotController()->GetVision()->GetAimTarget() ) {
            if ( pMember->GetBotController()->GetVision()->GetAimTarget() == pTarget )
                return true;

            continue;
        }

        CBulletsTraceFilter traceFilter( COLLISION_GROUP_NONE );
        traceFilter.SetPassEntity( pMember );

        Vector vecForward, vecOrigin( pMember->EyePosition() );
        pMember->GetVectors( &vecForward, NULL, NULL );

        trace_t tr;
        UTIL_TraceLine( vecOrigin, vecOrigin + vecForward * 3000.0f, MASK_SHOT, &traceFilter, &tr );

        if ( tr.m_pEnt == pTarget )
            return true;
    }

    return false;
}

//================================================================================
// Devuelve si algún miembro del escuadron esta mirando el lugar objetivo
//================================================================================
bool CSquad::IsSomeoneLooking( const Vector & vecTarget, CPlayer *pIgnore )
{
    FOR_EACH_VEC( m_nMembers, it )
    {
        CPlayer *pMember = GetMember( it );

        if ( !pMember )
            continue;

        if ( !pMember->IsAlive() )
            continue;

        if ( pMember == pIgnore )
            continue;

        if ( pMember->GetBotController() && pMember->GetBotController()->GetVision() && pMember->GetBotController()->GetVision()->HasAimGoal() ) {
            if ( pMember->GetBotController()->GetVision()->GetAimGoal() == vecTarget )
                return true;

            continue;
        }

        const float aimTolerance = 3.0f;

        Vector vecLook = vecTarget - pMember->EyePosition();
        QAngle angleLook;
        VectorAngles( vecLook, angleLook );

        float lookYaw = angleLook.y;
        float lookPitch = angleLook.x;
        QAngle viewAngle = pMember->pl.v_angle;

        float angleDiffYaw = AngleNormalize( lookYaw - viewAngle.y );
        float angleDiffPitch = AngleNormalize( lookPitch - viewAngle.x ); 

        if ( (angleDiffYaw < aimTolerance && angleDiffYaw > -aimTolerance) && (angleDiffPitch < aimTolerance && angleDiffPitch > -aimTolerance) ) {
            return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve si algún miembro del escuadron se dirige a la ubicación
//================================================================================
bool CSquad::IsSomeoneGoing( const Vector & vecDestination, CPlayer *pIgnore )
{
    FOR_EACH_VEC( m_nMembers, it )
    {
        CPlayer *pMember = GetMember( it );

        if ( !pMember )
            continue;

        if ( !pMember->IsAlive() )
            continue;

        if ( pMember == pIgnore )
            continue;

        // @TODO: Implementación para humanos
        if ( !pMember->IsBot() )
            continue;

        if ( !pMember->GetBotController()->GetLocomotion() )
            continue;

        if ( pMember->GetBotController()->GetLocomotion()->GetDestination() == vecDestination )
            return true;
    }

    return false;
}

//================================================================================
// Devuelve si la entidad especificada es enemigo de algún miembro del escuadron
// @TODO: Quizá se deba devolver el miembro que lo tiene como enemigo?
//================================================================================
bool CSquad::IsSquadEnemy( CBaseEntity *pEntity, CPlayer *pIgnore ) 
{
	FOR_EACH_MEMBER(
    {
		if ( pMember == pIgnore )
			continue;

        // Así es
        if ( pMember->GetEnemy() == pEntity )
            return true;
    });

	return false;
}

//================================================================================
//================================================================================
void CSquad::ReportTakeDamage( CPlayer *member, const CTakeDamageInfo &info ) 
{
    FOR_EACH_MEMBER(
    {
        // No lo queremos comunicar a nosotros mismos...
        if ( member == pMember )
            continue;

        pMember->OnMemberTakeDamage( member, info );
    });

    if ( GetController() )
        GetController()->m_OnReportDamage.FireOutput( member, NULL );
}

//================================================================================
//================================================================================
void CSquad::ReportDeath( CPlayer *member, const CTakeDamageInfo &info ) 
{
    FOR_EACH_MEMBER(
    {
        // No lo queremos comunicar a nosotros mismos...
        if ( member == pMember )
            continue;

        pMember->OnMemberDeath( member, info );
    });

    // Lo quitamos
    RemoveMember( member );

    if ( GetController() )
        GetController()->m_OnMemberDead.FireOutput( NULL, NULL );
}

//================================================================================
//================================================================================
void CSquad::ReportEnemy( CPlayer *member, CBaseEntity *pEnemy )
{
    FOR_EACH_MEMBER(
    {
        // No lo queremos comunicar a nosotros mismos...
        if ( member == pMember )
            continue;

        pMember->OnMemberReportEnemy( member, pEnemy );
    } );

    if ( GetController() ) {
        variant_t value;
        value.SetEntity( pEnemy );

        GetController()->m_OnReportEnemy.FireOutput( value, member, NULL );

        if ( pEnemy->IsPlayer() ) {
            CPlayer *pPlayer = (CPlayer *)pEnemy;
            Assert( pPlayer );

            GetController()->m_OnReportPlayerEnemy.FireOutput( value, member, NULL );

            if ( !pPlayer->IsBot() ) {
                GetController()->m_OnReportHumanEnemy.FireOutput( value, member, NULL );
            }
        }
    }
}
