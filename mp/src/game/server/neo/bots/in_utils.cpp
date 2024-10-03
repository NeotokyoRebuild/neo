//==== Woots 2016. http://creativecommons.org/licenses/by/2.5/mx/ ===========//

#include "cbase.h"
#include "in_utils.h"

#include "physobj.h"
#include "collisionutils.h"
#include "movevars_shared.h"

#include "ai_basenpc.h"
#include "ai_initutils.h"
#include "ai_hint.h"

#include "BasePropDoor.h"
#include "doors.h"
#include "func_breakablesurf.h"

#include "bots\bot_defs.h"
#include "nav_pathfind.h"
#include "util_shared.h"

#ifdef INSOURCE_DLL
#include "players_system.h"
#include "in_player.h"
#include "in_attribute_system.h"
#endif

#ifdef APOCALYPSE
    #include "ap_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef INSOURCE_DLL
extern ConVar sv_gameseed;
#else
ConVar sv_gameseed( "sv_gameseed", "", FCVAR_NOT_CONNECTED | FCVAR_SERVER );
#endif

#define COS_TABLE_SIZE 256
static float cosTable[ COS_TABLE_SIZE ];

//================================================================================
//================================================================================
static int SeedLineHash( int seedvalue, const char *sharedname )
{
    CRC32_t retval;

    CRC32_Init( &retval );

    CRC32_ProcessBuffer( &retval, (void *)&seedvalue, sizeof( int ) );
    CRC32_ProcessBuffer( &retval, (void *)sharedname, Q_strlen( sharedname ) );

    CRC32_Final( &retval );

    return (int)(retval);
}

//================================================================================
//================================================================================
float Utils::RandomFloat( const char *sharedname, float flMinVal, float flMaxVal )
{
    int seed = SeedLineHash( sv_gameseed.GetInt(), sharedname );
    RandomSeed( seed );
    return ::RandomFloat( flMinVal, flMaxVal );
}

//================================================================================
//================================================================================
int Utils::RandomInt( const char *sharedname, int iMinVal, int iMaxVal )
{
    int seed = SeedLineHash( sv_gameseed.GetInt(), sharedname );
    RandomSeed( seed );
    return ::RandomInt( iMinVal, iMaxVal );
}

//================================================================================
//================================================================================
Vector Utils::RandomVector( const char *sharedname, float minVal, float maxVal )
{
    int seed = SeedLineHash( sv_gameseed.GetInt(), sharedname );
    RandomSeed( seed );
    // HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
    // Get a random vector.
    Vector random;
    random.x = ::RandomFloat( minVal, maxVal );
    random.y = ::RandomFloat( minVal, maxVal );
    random.z = ::RandomFloat( minVal, maxVal );
    return random;
}

//================================================================================
//================================================================================
QAngle Utils::RandomAngle( const char *sharedname, float minVal, float maxVal )
{
    Assert( CBaseEntity::GetPredictionRandomSeed() != -1 );

    int seed = SeedLineHash( sv_gameseed.GetInt(), sharedname );
    RandomSeed( seed );

    // HACK:  Can't call RandomVector/Angle because it uses rand() not vstlib Random*() functions!
    // Get a random vector.
    Vector random;
    random.x = ::RandomFloat( minVal, maxVal );
    random.y = ::RandomFloat( minVal, maxVal );
    random.z = ::RandomFloat( minVal, maxVal );
    return QAngle( random.x, random.y, random.z );
}

//================================================================================
//================================================================================
void Utils::NormalizeAngle( float& fAngle )
{
    if (fAngle < 0.0f)
        fAngle += 360.0f;
    else if (fAngle >= 360.0f)
        fAngle -= 360.0f;
}

//================================================================================
//================================================================================
void Utils::DeNormalizeAngle( float& fAngle )
{
    if (fAngle < -180.0f)
        fAngle += 360.0f;
    else if (fAngle >= 180.0f)
        fAngle -= 360.0f;
}

//================================================================================
//================================================================================
void Utils::GetAngleDifference( QAngle const& angOrigin, QAngle const& angDestination, QAngle& angDiff )
{
    angDiff = angDestination - angOrigin;

    Utils::DeNormalizeAngle( angDiff.x );
    Utils::DeNormalizeAngle( angDiff.y );
}

//================================================================================
//================================================================================
bool Utils::IsBreakableSurf( CBaseEntity *pEntity )
{
    if ( !pEntity )
        return false;

    // No puede recibir daño
    if ( pEntity->m_takedamage != DAMAGE_YES )
        return false;

    // Es una superficie que se puede romper
    if ( (dynamic_cast<CBreakableSurface *>(pEntity)) )
        return true;

    // Es una pared que se puede romper
    if ( (dynamic_cast<CBreakable *>(pEntity)) )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool Utils::IsBreakable( CBaseEntity *pEntity )
{
    if ( !pEntity )
        return false;

    // No puede recibir daño
    if ( pEntity->m_takedamage != DAMAGE_YES )
        return false;

    // Es una entidad que se puede romper
    if ( (dynamic_cast<CBreakableProp *>(pEntity)) )
        return true;

    // Es una pared que se puede romper
    if ( (dynamic_cast<CBreakable *>(pEntity)) )
        return true;

    // Es una superficie que se puede romper
    if ( (dynamic_cast<CBreakableSurface *>(pEntity)) )
    {
        CBreakableSurface *surf = static_cast< CBreakableSurface * >( pEntity );

        // Ya esta roto
        if ( surf->m_bIsBroken )
            return false;
        
        return true;
    }

    // Es una puerta
    if ( (dynamic_cast<CBasePropDoor *>(pEntity)) )
        return true;

    return false;
}

//================================================================================
//================================================================================
bool Utils::IsDoor( CBaseEntity *pEntity )
{
    if ( !pEntity )
        return false;

    CBaseDoor *pDoor = dynamic_cast<CBaseDoor *>( pEntity );

    if ( pDoor )
        return true;

    CBasePropDoor *pPropDoor = dynamic_cast<CBasePropDoor *>( pEntity );

    if ( pPropDoor )
        return true;

    return false;
}

//================================================================================
//================================================================================
CBaseEntity *Utils::FindNearestPhysicsObject( const Vector &vOrigin, float fMaxDist, float fMinMass, float fMaxMass, CBaseEntity *pFrom )
{
    CBaseEntity *pFinalEntity    = NULL;
    CBaseEntity *pThrowEntity    = NULL;
    float flNearestDist            = 0;

    // Buscamos los objetos que podemos lanzar
    do
    {
        // Objetos con físicas
        pThrowEntity = gEntList.FindEntityByClassnameWithin( pThrowEntity, "prop_physics", vOrigin, fMaxDist );
    
        // Ya no existe
        if ( !pThrowEntity )
            continue;

        // La entidad que lo quiere no puede verlo
        if ( pFrom ) {
            if ( !pFrom->FVisible(pThrowEntity) )
                continue;
        }

        // No se ha podido acceder a la información de su fisica
        if ( !pThrowEntity->VPhysicsGetObject() )
            continue;

        // No se puede mover o en si.. lanzar
        if ( !pThrowEntity->VPhysicsGetObject()->IsMoveable() )
            continue;

        Vector v_center    = pThrowEntity->WorldSpaceCenter();
        float flDist    = UTIL_DistApprox2D( vOrigin, v_center );

        // Esta más lejos que el objeto anterior
        if ( flDist > flNearestDist && flNearestDist != 0 )
            continue;

        // Calcular la distancia al enemigo
        if ( pFrom && pFrom->IsNPC() ) {
            CAI_BaseNPC *pNPC = dynamic_cast<CAI_BaseNPC *>( pFrom );

            if ( pNPC && pNPC->GetEnemy() ) {
                Vector vecDirToEnemy    = pNPC->GetEnemy()->GetAbsOrigin() - pNPC->GetAbsOrigin();
                vecDirToEnemy.z            = 0;

                Vector vecDirToObject = pThrowEntity->WorldSpaceCenter() - vOrigin;
                VectorNormalize(vecDirToObject);
                vecDirToObject.z = 0;

                if ( DotProduct(vecDirToEnemy, vecDirToObject) < 0.8 )
                    continue;
            }
        }

        // Obtenemos su peso
        float pEntityMass = pThrowEntity->VPhysicsGetObject()->GetMass();

        // Muy liviano
        if ( pEntityMass < fMinMass && fMinMass > 0 )
            continue;
            
        // ¡Muy pesado!
        if ( pEntityMass > fMaxMass )
            continue;

        // No lanzar objetos que esten sobre mi cabeza
        if ( v_center.z > vOrigin.z )
            continue;

        if ( pFrom ) {
            Vector vecGruntKnees;
            pFrom->CollisionProp()->NormalizedToWorldSpace( Vector(0.5f, 0.5f, 0.25f), &vecGruntKnees );

            vcollide_t *pCollide = modelinfo->GetVCollide( pThrowEntity->GetModelIndex() );
        
            Vector objMins, objMaxs;
            physcollision->CollideGetAABB(&objMins, &objMaxs, pCollide->solids[0], pThrowEntity->GetAbsOrigin(), pThrowEntity->GetAbsAngles());

            if ( objMaxs.z < vecGruntKnees.z )
                continue;
        }

        // Este objeto es perfecto, guardamos su distancia por si encontramos otro más cerca
        flNearestDist    = flDist;
        pFinalEntity    = pThrowEntity;

    } while( pThrowEntity );

    // No pudimos encontrar ningún objeto
    if ( !pFinalEntity )
        return NULL;

    return pFinalEntity;
}

//================================================================================
//================================================================================
bool Utils::IsMoveableObject( CBaseEntity *pEntity )
{
    if ( !pEntity || pEntity->IsWorld() )
        return false;

    if ( pEntity->IsPlayer() || pEntity->IsNPC() )
        return true;

    if ( !pEntity->VPhysicsGetObject() )
        return false;

    if ( !pEntity->VPhysicsGetObject()->IsMoveable() )
        return false;        

    return true;
}

//================================================================================
//================================================================================
bool Utils::RunOutEntityLimit( int iTolerance )
{
    if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - iTolerance) )
        return false;

    return true;
}

//================================================================================
//================================================================================
IGameEvent *Utils::CreateLesson( const char *pLesson, CBaseEntity *pSubject )
{
    IGameEvent *pEvent = gameeventmanager->CreateEvent( pLesson, true );

    if ( pEvent ) {
        if ( pSubject )
            pEvent->SetInt( "subject", pSubject->entindex() );
    }

    return pEvent;
}

#ifdef INSOURCE_DLL
//================================================================================
// Aplica el modificador del atributo a todos los jugadores en el radio especificado
//================================================================================
bool Utils::AddAttributeModifier( const char *name, float radius, const Vector &vecPosition, int team )
{
	CTeamRecipientFilter filter( team );
	return AddAttributeModifier( name, radius, vecPosition, filter );
}

//================================================================================
// Aplica el modificador a todos los jugadores
//================================================================================
bool Utils::AddAttributeModifier( const char *name, int team ) 
{
	return AddAttributeModifier( name, 0.0f, vec3_origin, team );
}

//================================================================================
// Aplica el modificador del atributo a todos los jugadores en el radio especificado
//================================================================================
bool Utils::AddAttributeModifier( const char *name, float radius, const Vector &vecPosition, CRecipientFilter &filter ) 
{
	AttributeInfo info;

	// El modificador no existe
	if ( !TheAttributeSystem->GetModifierInfo(name, info) )
		return false;

	for ( int i = 1; i <= gpGlobals->maxClients; ++i ) 
	{
		CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(i) );

        if ( !pPlayer )
            continue;

		// Jugador muerto
        if ( !pPlayer->IsAlive() )
            continue;

		// Esta muy lejos
		if ( radius > 0 )
		{
			float distance = pPlayer->GetAbsOrigin().DistTo( vecPosition );

			if ( distance > radius )
				continue;
		}

		// Verificamos si esta en el filtro
		for ( int s = 0; s < filter.GetRecipientCount(); ++s )
		{
			CPlayer *pItem = ToInPlayer( UTIL_PlayerByIndex( filter.GetRecipientIndex(s) ) );
		
			// Aquí esta
			if ( pItem == pPlayer )
			{
				// Agregamos el modificador
				pPlayer->AddAttributeModifier(name);
			}
		}
	}

	return true;
}
#endif

//================================================================================
// Set [bones] with the Hitbox IDs of the entity.
// TODO: Rename? (I think technically there are no "bones" here.)
//================================================================================
bool Utils::GetEntityBones( CBaseEntity *pEntity, HitboxBones &bones )
{
    // Invalid
    bones.head = -1;
    bones.chest = -1;
    bones.leftLeg = -1;
    bones.rightLeg = -1;

#ifdef APOCALYPSE
    if ( pEntity->ClassMatches("npc_infected") ) {
        bones.head = 16;
        bones.chest = 10;
        bones.leftLeg = 4;
        bones.rightLeg = 7;
        return true;
    }

    if ( pEntity->IsPlayer() ) {
        bones.head = 12;
        bones.chest = 9;
        bones.leftLeg = 1;
        bones.rightLeg = 4;
        return true;
    }
#elif HL2MP
    enum
    {
        TEAM_COMBINE = 2,
        TEAM_REBELS,
    };

    if ( pEntity->IsPlayer() ) {
        if ( pEntity->GetTeamNumber() == TEAM_REBELS ) {
            bones.head = 0;
            bones.chest = 0;
            bones.leftLeg = 7;
            bones.rightLeg = 11;
            return true;
        }
        else if ( pEntity->GetTeamNumber() == TEAM_COMBINE ) {
            bones.head = 17;
            bones.chest = 17;
            bones.leftLeg = 8;
            bones.rightLeg = 12;
            return true;
        }
    }
#endif

    return false;
}

//================================================================================
// Fill in [positions] with the entity's hitbox positions
// If we do not have the Hitbox IDs of the entity then it will return generic positions.
// Use with care: this is often heavy for the engine.
//================================================================================
bool Utils::GetHitboxPositions( CBaseEntity *pEntity, HitboxPositions &positions )
{
    positions.Reset();

    if ( pEntity == NULL )
        return false;

    // Generic Positions
    positions.head = pEntity->EyePosition();
    positions.chest = positions.leftLeg = positions.rightLeg = pEntity->WorldSpaceCenter();

    CBaseAnimating *pModel = pEntity->GetBaseAnimating();

    if ( pModel == NULL )
        return false;

    CStudioHdr *studioHdr = pModel->GetModelPtr();

    if ( studioHdr == NULL )
        return false;

    mstudiohitboxset_t *set = studioHdr->pHitboxSet( pModel->GetHitboxSet() );

    if ( set == NULL )
        return false;

    QAngle angles;
    mstudiobbox_t *box = NULL;
    HitboxBones bones;

    if ( !GetEntityBones( pEntity, bones ) )
        return false;

    if ( bones.head >= 0 ) {
        box = set->pHitbox( bones.head );
        pModel->GetBonePosition( box->bone, positions.head, angles );
    }

    if ( bones.chest >= 0 ) {
        box = set->pHitbox( bones.chest );
        pModel->GetBonePosition( box->bone, positions.chest, angles );
    }

    if ( bones.leftLeg >= 0 ) {
        box = set->pHitbox( bones.leftLeg );
        pModel->GetBonePosition( box->bone, positions.leftLeg, angles );
    }

    if ( bones.rightLeg >= 0 ) {
        box = set->pHitbox( bones.rightLeg );
        pModel->GetBonePosition( box->bone, positions.rightLeg, angles );
    }

    return true;
}

//================================================================================
// Sets [vecPosition] to the desired hitbox position of the entity'
// 
//================================================================================
bool Utils::GetHitboxPosition( CBaseEntity *pEntity, Vector &vecPosition, HitboxType type )
{
    vecPosition.Invalidate();

    HitboxPositions positions;
    GetHitboxPositions( pEntity, positions );

    if ( !positions.IsValid() )
        return false;

    switch ( type ) {
        case HITGROUP_HEAD:
            vecPosition = positions.head;
            break;

        case HITGROUP_CHEST:
        default:
            vecPosition = positions.chest;
            break;

        case HITGROUP_LEFTLEG:
            vecPosition = positions.leftLeg;
            break;

        case HITGROUP_RIGHTLEG:
            vecPosition = positions.rightLeg;
            break;
    }

    return true;
}

//================================================================================
//================================================================================
void Utils::InitBotTrig()
{
    for( int i=0; i<COS_TABLE_SIZE; ++i )
    {
        float angle = (float)(2.0f * M_PI * i / (float)(COS_TABLE_SIZE-1));
        cosTable[i] = (float)cos( angle ); 
    }
}

//================================================================================
//================================================================================
float Utils::BotCOS( float angle )
{
    angle = AngleNormalizePositive( angle );
    int i = (int)( angle * (COS_TABLE_SIZE-1) / 360.0f );
    return cosTable[i];
}

//================================================================================
//================================================================================
float Utils::BotSIN( float angle )
{
    angle = AngleNormalizePositive( angle - 90 );
    int i = (int)( angle * (COS_TABLE_SIZE-1) / 360.0f );
    return cosTable[i];
}

//================================================================================
//================================================================================
bool Utils::IsIntersecting2D( const Vector &startA, const Vector &endA, const Vector &startB, const Vector &endB, Vector *result )
{
    float denom = (endA.x - startA.x) * (endB.y - startB.y) - (endA.y - startA.y) * (endB.x - startB.x);
    if (denom == 0.0f)
    {
        // parallel
        return false;
    }

    float numS = (startA.y - startB.y) * (endB.x - startB.x) - (startA.x - startB.x) * (endB.y - startB.y);
    if (numS == 0.0f)
    {
        // coincident
        return true;
    }

    float numT = (startA.y - startB.y) * (endA.x - startA.x) - (startA.x - startB.x) * (endA.y - startA.y);

    float s = numS / denom;
    if (s < 0.0f || s > 1.0f)
    {
        // intersection is not within line segment of startA to endA
        return false;
    }

    float t = numT / denom;
    if (t < 0.0f || t > 1.0f)
    {
        // intersection is not within line segment of startB to endB
        return false;
    }

    // compute intesection point
    if (result)
        *result = startA + s * (endA - startA);

    return true;
}

//================================================================================
//================================================================================
CPlayer *Utils::GetClosestPlayer( const Vector &vecPosition, float *distance, CPlayer *pIgnore, int team )
{
    CPlayer *pClosest = NULL;
    float closeDist = 999999999999.9f;

    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) 
	{
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(i) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( team && pPlayer->GetTeamNumber() != team )
            continue;

        Vector position = pPlayer->GetAbsOrigin();
        float dist = vecPosition.DistTo( position );

        if ( dist < closeDist ) {
            closeDist = dist;
            pClosest  = pPlayer;
        }
    }
    
    if ( distance )
        *distance = closeDist;

    return pClosest;
}

//================================================================================
// Devuelve si algún jugador del equipo especificado esta en el rango indicado
// cerca de la posición indicada
//================================================================================
bool Utils::IsSpotOccupied( const Vector &vecPosition, CPlayer *pIgnore, float closeRange, int avoidTeam )
{
    float distance;
    CPlayer *pPlayer = Utils::GetClosestPlayer( vecPosition, &distance, pIgnore, avoidTeam );

    if ( pPlayer && distance < closeRange )
        return true;

    return false;
}

CPlayer * Utils::GetClosestPlayerByClass( const Vector & vecPosition, Class_T classify, float *distance, CPlayer *pIgnore ) 
{
	CPlayer *pClosest = NULL;
    float closeDist = 999999999999.9f;

    for ( int i = 1; i <= gpGlobals->maxClients; ++i ) 
	{
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(i) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( pPlayer->Classify() != classify )
            continue;

        Vector position = pPlayer->GetAbsOrigin();
        float dist = vecPosition.DistTo( position );

        if ( dist < closeDist ) {
            closeDist = dist;
            pClosest  = pPlayer;
        }
    }
    
    if ( distance )
        *distance = closeDist;

    return pClosest;
}

bool Utils::IsSpotOccupiedByClass( const Vector &vecPosition, Class_T classify, CPlayer * pIgnore, float closeRange ) 
{
	float distance;
    CPlayer *pPlayer = Utils::GetClosestPlayerByClass( vecPosition, classify, &distance, pIgnore );

    if ( pPlayer && distance < closeRange )
        return true;

    return false;
}

//================================================================================
// Devuelve si algún jugador esta en la línea de fuego (FOV) de un punto de salida
// a un punto de destino
//================================================================================
bool Utils::IsCrossingLineOfFire( const Vector &vecStart, const Vector &vecFinish, CPlayer *pIgnore, int ignoreTeam  )
{
    for ( int i = 1; i <= gpGlobals->maxClients; ++i )
    {
        CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(i) );

        if ( !pPlayer )
            continue;

        if ( !pPlayer->IsAlive() )
            continue;

        if ( pPlayer == pIgnore )
            continue;

        if ( ignoreTeam && pPlayer->GetTeamNumber() == ignoreTeam )
            continue;

        // compute player's unit aiming vector 
        Vector viewForward;
        AngleVectors( pPlayer->EyeAngles() + pPlayer->GetPunchAngle(), &viewForward );

        const float longRange = 5000.0f;
        Vector playerOrigin = pPlayer->GetAbsOrigin();
        Vector playerTarget = playerOrigin + longRange * viewForward;

        Vector result( 0, 0, 0 );

        if ( IsIntersecting2D(vecStart, vecFinish, playerOrigin, playerTarget, &result) )
        {
            // simple check to see if intersection lies in the Z range of the path
            float loZ, hiZ;

            if ( vecStart.z < vecFinish.z )
            {
                loZ = vecStart.z;
                hiZ = vecFinish.z;
            }
            else
            {
                loZ = vecFinish.z;
                hiZ = vecStart.z;
            }

            if ( result.z >= loZ && result.z <= hiZ + HumanHeight )
                return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve si la posición es válida usando los filtros de [criteria]
//================================================================================
bool Utils::IsValidSpot( const Vector & vecSpot, const Vector & vecOrigin, const CSpotCriteria & criteria, CPlayer * pOwner )
{
    // Esta lejos del limite
    if ( criteria.m_flMaxRange > 0 && vecOrigin.DistTo( vecSpot ) > criteria.m_flMaxRange ) {
        return false;
    }

    // Este escondite esta en la línea de fuego
    if ( criteria.m_bOutOfLineOfFire && IsCrossingLineOfFire( vecOrigin, vecSpot, pOwner ) ) {
        return false;
    }

    // Tenemos que evitar escondites cerca del equipo enemigo
    if ( criteria.m_iAvoidTeam && IsSpotOccupied( vecSpot, pOwner, criteria.m_flMinDistanceFromEnemy, criteria.m_iAvoidTeam ) ) {
        return false;
    }

#ifdef INSOURCE_DLL
    CUsersRecipientFilter filter( criteria.m_iAvoidTeam, false );

    // Fuera de la visibilidad del equipo enemigo
    if ( criteria.m_iAvoidTeam && criteria.m_bOutOfVisibility && ThePlayersSystem->IsAbleToSee( vecSpot, filter, CBaseCombatCharacter::DISREGARD_FOV ) ) {
        return false;
    }

    // No es visible
    if ( criteria.m_bOnlyVisible && pOwner && !pOwner->IsAbleToSee( vecSpot, CBaseCombatCharacter::DISREGARD_FOV ) ) {
        return false;
    }
#else
    if ( criteria.m_iAvoidTeam && criteria.m_bOutOfVisibility ) {
        for ( int it = 0; it <= gpGlobals->maxClients; ++it ) {
            CPlayer *pPlayer = ToInPlayer( UTIL_PlayerByIndex(it) );

            if ( !pPlayer )
                continue;

            if ( !pPlayer->IsAlive() )
                continue;

            if ( pPlayer->GetTeamNumber() != criteria.m_iAvoidTeam )
                return false;

            if ( pPlayer->FVisible( vecSpot ) && pPlayer->IsInFieldOfView( vecSpot ) )
                return false;
        }
    }

    if ( criteria.m_bOnlyVisible && pOwner ) {
        if ( !pOwner->FVisible( vecSpot ) || !pOwner->IsInFieldOfView( vecSpot ) )
            return false;
    }
#endif

    return true;
}

//================================================================================
// Devuelve una posición donde ocultarse en un rango máximo
//================================================================================
// Los Bots lo usan como primera opción para ocultarse de los enemigos.
//================================================================================
bool Utils::FindNavCoverSpot( Vector *vecResult, const Vector &vecOrigin, const CSpotCriteria &criteria, CPlayer *pPlayer, SpotVector *list )
{
    if ( vecResult )
        vecResult->Invalidate();

    CNavArea *pStartArea = TheNavMesh->GetNearestNavArea( vecOrigin );

    if ( !pStartArea )
        return false;

    int hidingType = (criteria.m_bIsSniper) ? HidingSpot::IDEAL_SNIPER_SPOT : HidingSpot::IN_COVER;

    while ( true ) {
        CollectHidingSpotsFunctor collector( pPlayer, vecOrigin, criteria.m_flMaxRange, hidingType );
        SearchSurroundingAreas( pStartArea, vecOrigin, collector, criteria.m_flMaxRange );

        // Filtros
        for ( int i = 0; i < collector.m_count; ++i ) {
            if ( !IsValidSpot( *collector.m_hidingSpot[i], vecOrigin, criteria, pPlayer ) ) {
                collector.RemoveSpot( i );
                --i;
                continue;
            }

            if ( list ) {
                Vector vec = *collector.m_hidingSpot[i];
                list->AddToTail( vec );
            }
        }

        if ( !vecResult )
            return false;

        // No se ha encontrado ningún lugar 
        if ( collector.m_count == 0 ) {
            // Intentemos con un lugar para Sniper
            if ( hidingType == HidingSpot::IN_COVER ) {
                hidingType = HidingSpot::IDEAL_SNIPER_SPOT;

                if ( criteria.m_bIsSniper )
                    break;
            }

            // Intentemos con un lugar menos adecuado
            if ( hidingType == HidingSpot::IDEAL_SNIPER_SPOT )
                hidingType = HidingSpot::GOOD_SNIPER_SPOT;

            // No hay ningún lugar cerca
            if ( hidingType == HidingSpot::GOOD_SNIPER_SPOT ) {
                hidingType = HidingSpot::IN_COVER;

                if ( !criteria.m_bIsSniper )
                    break;
            }
        }
        else {
            if ( criteria.m_bUseNearest ) {
                float closest = 9999999999999999999.0f;

                for ( int it = 0; it < collector.m_count; ++it ) {
                    Vector vecDummy = *collector.m_hidingSpot[it];
                    float distance = vecOrigin.DistTo( vecDummy );

                    if ( distance < closest ) {
                        closest = distance;
                        *vecResult = vecDummy;
                    }
                }

                return true;
            }
            else {
                int random = (criteria.m_bUseRandom) ? ::RandomInt( 0, collector.m_count - 1 ) : collector.GetRandomHidingSpot();
                *vecResult = *collector.m_hidingSpot[random];
                return true;
            }
        }
    }

    return false;
}

//================================================================================
// Devuelve una posición donde ocultarse dentro del area indicado
//================================================================================
// Los Bots lo usan como última opción para ocultarse de los enemigos.
//================================================================================
bool Utils::FindNavCoverSpotInArea( Vector *vecResult, const Vector &vecOrigin, CNavArea *pArea, const CSpotCriteria &criteria, CPlayer *pPlayer, SpotVector *list )
{
    if ( vecResult )
        vecResult->Invalidate();

    if ( !pArea )
        return false;

    CUtlVector<Vector> collector;

    for ( int e = 0; e <= 15; ++e ) {
        Vector position = pArea->GetRandomPoint();
        position.z += HumanCrouchHeight;

        if ( !position.IsValid() )
            continue;

        if ( collector.Find( position ) > -1 )
            continue;

        if ( !IsValidSpot( position, vecOrigin, criteria, pPlayer ) ) {
            continue;
        }

        collector.AddToTail( position );

        // Lo agregamos a la lista
        if ( list )
            list->AddToTail( position );
    }

    if ( !vecResult )
        return false;

    if ( collector.Count() > 0 ) {
        if ( criteria.m_bUseNearest ) {
            float closest = 9999999999999999999.0f;

            for ( int it = 0; it < collector.Count(); ++it ) {
                Vector vecDummy = collector.Element( it );
                float distance = vecOrigin.DistTo( vecDummy );

                if ( distance < closest ) {
                    closest = distance;
                    *vecResult = vecDummy;
                }
            }

            return true;
        }
        else {
            int random = (criteria.m_bUseRandom) ? ::RandomInt( 0, collector.Count() - 1 ) : 0;
            *vecResult = collector.Element( random );

            return true;
        }
    }

    return false;
}

//================================================================================
// Devuelve una posición donde ocultarse usando los ai_hint (CAI_Hint)
//================================================================================
// Los Bots lo usan como segunda opción para ocultarse de los enemigos.
//================================================================================
CAI_Hint *Utils::FindHintSpot( const Vector &vecOrigin, const CHintCriteria &hintCriteria, const CSpotCriteria &criteria, CPlayer *pPlayer, SpotVector *list )
{
    CUtlVector<CAI_Hint *> collector;
    CAI_HintManager::FindAllHints( vecOrigin, hintCriteria, &collector );

    if ( collector.Count() == 0 )
        return NULL;

    FOR_EACH_VEC( collector, it )
    {
        CAI_Hint *pHint = collector[it];

        if ( !pHint )
            continue;

        Vector position = pHint->GetAbsOrigin();

        if ( !IsValidSpot( position, vecOrigin, criteria, pPlayer ) ) {
            collector.Remove( it );
            --it;
            continue;
        }

        if ( list ) {
            Vector vec = position;
            list->AddToTail( vec );
        }
    }

    if ( collector.Count() > 0 ) {
        if ( criteria.m_bUseNearest ) {
            float closest = 9999999999999999999.0f;
            CAI_Hint *pClosest = NULL;

            for ( int it = 0; it < collector.Count(); ++it ) {
                CAI_Hint *pDummy = collector[it];
                Vector vecDummy = pDummy->GetAbsOrigin();
                float distance = vecOrigin.DistTo( vecDummy );

                if ( distance < closest ) {
                    closest = distance;
                    pClosest = pDummy;
                }
            }

            return pClosest;
        }
        else {
            int random = (criteria.m_bUseRandom) ? ::RandomInt( 0, collector.Count() - 1 ) : 0;
            return collector[random];
        }
    }

    return NULL;
}

//================================================================================
// Devuelve un lugar interesante para el jugador
//================================================================================
bool Utils::FindIntestingPosition( Vector *vecResult, CPlayer *pPlayer, const CSpotCriteria &criteria )
{
    Vector vecOrigin = pPlayer->GetAbsOrigin();

    if ( criteria.m_vecOrigin.IsValid() )
        vecOrigin = criteria.m_vecOrigin;

    // Hints
    if ( ::RandomInt( 0, 10 ) < 6 ) {
        CHintCriteria hintCriteria;
        hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING );
        hintCriteria.AddHintType( HINT_WORLD_WINDOW );

        if ( criteria.m_iTacticalMode == TACTICAL_MODE_STEALTH )
            hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING_STEALTH );

        if ( criteria.m_iTacticalMode == TACTICAL_MODE_ASSAULT ) {
#ifdef INSOURCE_DLL
            hintCriteria.AddHintType( HINT_TACTICAL_VISUALLY_INTERESTING );
            hintCriteria.AddHintType( HINT_TACTICAL_ASSAULT_APPROACH );
#endif
            hintCriteria.AddHintType( HINT_TACTICAL_PINCH );
        }

        //if ( criteria.m_bUseNearest )
            //hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );

        if ( criteria.m_flMaxRange > 0 )
            hintCriteria.AddIncludePosition( vecOrigin, criteria.m_flMaxRange );

        CAI_Hint *pHint = FindHintSpot( vecOrigin, hintCriteria, criteria, pPlayer );

        if ( pHint ) {
            if ( vecResult ) {
                pHint->GetPosition( pPlayer, vecResult );
            }

            return true;
        }
    }

    // NavMesh
    if ( FindNavCoverSpot( vecResult, vecOrigin, criteria, pPlayer ) ) {
        return true;
    }

    return false;
}

//================================================================================
// Devuelve una posición donde el jugador puede ocultarse
//================================================================================
bool Utils::FindCoverPosition( Vector *vecResult, CPlayer *pPlayer, const CSpotCriteria &criteria )
{
	Vector vecOrigin = pPlayer->GetAbsOrigin();

    if ( criteria.m_vecOrigin.IsValid() ) {
        vecOrigin = criteria.m_vecOrigin;
    }

    // NavMesh
    if ( FindNavCoverSpot(vecResult, vecOrigin, criteria, pPlayer) )
        return true;

    // Hints
    CHintCriteria hintCriteria;
#ifdef INSOURCE_DLL
    hintCriteria.AddHintType( HINT_TACTICAL_COVER );
#else
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_MED );
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_LOW );
#endif

    CAI_Hint *pHint = FindHintSpot( vecOrigin, hintCriteria, criteria, pPlayer );

    if ( pHint ) {
        if ( vecResult ) {
            pHint->GetPosition( pPlayer, vecResult );
        }

        return true;
    }

    // Random Area Spot
    if ( FindNavCoverSpotInArea(vecResult, vecOrigin, pPlayer->GetLastKnownArea(), criteria, pPlayer) )
        return true;

    return false;
}

//================================================================================
//================================================================================
void Utils::FillIntestingPositions( SpotVector * list, CPlayer * pPlayer, const CSpotCriteria & criteria )
{
    Vector vecOrigin = pPlayer->GetAbsOrigin();

    if ( criteria.m_vecOrigin.IsValid() )
        vecOrigin = criteria.m_vecOrigin;

    // Hints
    CHintCriteria hintCriteria;
    hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING );
    hintCriteria.AddHintType( HINT_WORLD_WINDOW );

    if ( criteria.m_iTacticalMode == TACTICAL_MODE_STEALTH )
        hintCriteria.AddHintType( HINT_WORLD_VISUALLY_INTERESTING_STEALTH );

    if ( criteria.m_iTacticalMode == TACTICAL_MODE_ASSAULT ) {
#ifdef INSOURCE_DLL
        hintCriteria.AddHintType( HINT_TACTICAL_VISUALLY_INTERESTING );
#endif
        //hintCriteria.AddHintType( HINT_TACTICAL_ASSAULT_APPROACH );
        hintCriteria.AddHintType( HINT_TACTICAL_PINCH );
    }

    if ( criteria.m_flMaxRange > 0 )
        hintCriteria.AddIncludePosition( vecOrigin, criteria.m_flMaxRange );

    FindHintSpot( vecOrigin, hintCriteria, criteria, pPlayer, list );

    // NavMesh
    //FindNavCoverSpot( NULL, vecOrigin, criteria, pPlayer, list );
}

//================================================================================
//================================================================================
void Utils::FillCoverPositions( SpotVector * list, CPlayer * pPlayer, const CSpotCriteria & criteria )
{
    Vector vecOrigin = pPlayer->GetAbsOrigin();

    if ( criteria.m_vecOrigin.IsValid() )
        vecOrigin = criteria.m_vecOrigin;

    // NavMesh
    FindNavCoverSpot( NULL, vecOrigin, criteria, pPlayer, list );

    // Hints
    CHintCriteria hintCriteria;
#ifdef INSOURCE_DLL
    hintCriteria.AddHintType( HINT_TACTICAL_COVER );
#else
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_MED );
    hintCriteria.AddHintType( HINT_TACTICAL_COVER_LOW );
#endif

    FindHintSpot( vecOrigin, hintCriteria, criteria, pPlayer, list );

    // Random Area Spot
    FindNavCoverSpotInArea( NULL, vecOrigin, pPlayer->GetLastKnownArea(), criteria, pPlayer, list );
}

#ifdef INSOURCE_DLL
//================================================================================
//================================================================================
void Utils::AlienFX_SetColor( CPlayer *pPlayer, unsigned int lights, unsigned int color, float duration )
{
    if ( !pPlayer || !pPlayer->IsNetClient() )
        return;

    if ( pPlayer->ShouldThrottleUserMessage("AlienFX") )
        return;

    CSingleUserRecipientFilter user( pPlayer );
    user.MakeReliable();

    UserMessageBegin( user, "AlienFX" );        
        WRITE_SHORT( ALIENFX_SETCOLOR );
        WRITE_LONG( lights );
        WRITE_LONG( color );
        WRITE_FLOAT( duration );
    MessageEnd();
}
#endif