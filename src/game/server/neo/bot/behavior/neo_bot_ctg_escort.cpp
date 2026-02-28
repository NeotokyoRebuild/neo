#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_escort.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "weapons/weapon_ghost.h"

//---------------------------------------------------------------------------------------------
CNEOBotCtgEscort::CNEOBotCtgEscort( void ) : 
	m_role( ROLE_SCREEN ),
	m_bHasGoal( false )
{
	m_vecGoalPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_repathTimer.Invalidate();
	m_lostSightOfCarrierTimer.Invalidate();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEscort::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_chasePath.Invalidate();
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	m_lostSightOfCarrierTimer.Invalidate();

	m_role = ROLE_SCREEN;
	m_vecGoalPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_bHasGoal = false;

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgEscort::Update( CNEOBot *me, float interval )
{
	if ( !NEORules()->GhostExists() )
	{
		return Done( "Ghost does not exist" );
	}

	if ( NEORules()->GetGhosterPlayer() <= 0 )
	{
		return Done( "Ghost is not held by a player" );
	}

	CNEO_Player* pGhostCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
	if ( !pGhostCarrier || pGhostCarrier->GetTeamNumber() != me->GetTeamNumber() || pGhostCarrier == me )
	{
		return Done( "Ghost carrier is not a teammate anymore" );
	}

	// Check if we can assist the Ghost Carrier (if they are a bot)
	CNEOBot *pBotGhostCarrier = ToNEOBot( pGhostCarrier );
	if ( pBotGhostCarrier )
	{
		const CKnownEntity *carrierThreat = pBotGhostCarrier->GetVisionInterface()->GetPrimaryKnownThreat();
		if ( carrierThreat )
		{
			// Check if the threat has a clear shot at our friend
			if ( me->IsLineOfFireClear( carrierThreat->GetLastKnownPosition(), pGhostCarrier, CNEOBot::LINE_OF_FIRE_FLAGS_DEFAULT ) )
			{
				return SuspendFor( new CNEOBotAttack, "Assisting Ghost Carrier with their threat" );
			}
		}
	}
	
	if ( m_repathTimer.IsElapsed() )
	{
		UpdateGoalPosition( me, pGhostCarrier );

		if ( m_bHasGoal )
		{
			m_role = UpdateRoleAssignment( me, pGhostCarrier, m_vecGoalPos );
		}
		else
		{
			m_role = ROLE_SCREEN;
		}
	}

	bool bCanSeeCarrier = me->GetVisionInterface()->IsLineOfSightClear( pGhostCarrier->WorldSpaceCenter() );
	if ( bCanSeeCarrier )
	{
		m_lostSightOfCarrierTimer.Invalidate();

		if ( !m_bHasGoal )
		{
			// Asymmetric defense: No goal cap zone, so defend the carrier.
			// Look away from the carrier to cover their blind spots
			Vector vecFromCarrier = me->GetAbsOrigin() - pGhostCarrier->GetAbsOrigin();
			vecFromCarrier.z = 0.0f; // Bias towards horizontal scanning
			// CNEOBotTacticalMonitor::AvoidBumpingFriends handles <32hu distance
			if ( VectorNormalize( vecFromCarrier ) > 32.0f )
			{
				// Look at a point far away in the opposite direction of the carrier
				Vector vecLookTarget = me->EyePosition() + ( vecFromCarrier * 500.0f );
				me->GetBodyInterface()->AimHeadTowards( vecLookTarget, IBody::IMPORTANT, 0.2f, nullptr, "Escort: Scanning away from carrier" );
			}
		}
	}
	else
	{
		// If we can't see them, check if we are in grace period
		if ( !m_lostSightOfCarrierTimer.HasStarted() )
		{
			// JUST lost sight
			m_lostSightOfCarrierTimer.Start( 3.0f );
			// Treat as visible for now
			bCanSeeCarrier = true;
		}
		else if ( !m_lostSightOfCarrierTimer.IsElapsed() )
		{
			// Still in grace period
			bCanSeeCarrier = true;
		}
		else
		{
			// Timer elapsed, truly lost sight
			bCanSeeCarrier = false;
		}
	}

	// Check again based on grace period checks above
	if ( !bCanSeeCarrier )
	{
		m_path.Invalidate();
		CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, SAFEST_ROUTE );
	}
	else
	{
		if ( m_role == ROLE_BODYGUARD )
		{
			// Dont crowd the carrier
			if ( me->GetAbsOrigin().DistToSqr( pGhostCarrier->GetAbsOrigin() ) < ( 100.0f * 100.0f ) )
			{
				m_path.Invalidate();
				m_chasePath.Invalidate();
				return Continue();
			}

			CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, SAFEST_ROUTE );
			return Continue();
		}

		if ( !m_bHasGoal )
		{
			// Asymmetric defense: No goal cap zone, so defend the carrier.
			if ( pBotGhostCarrier )
			{
				const CKnownEntity *carrierThreat = pBotGhostCarrier->GetVisionInterface()->GetPrimaryKnownThreat();
				if ( carrierThreat && carrierThreat->GetEntity() && carrierThreat->GetEntity()->IsAlive() )
				{
					me->GetVisionInterface()->AddKnownEntity( carrierThreat->GetEntity() );
					return SuspendFor( new CNEOBotAttack, "Attacking enemy during asymmetric defense" );
				}
			}

			// No active threats to carrier
			float flDistToCarrierSq = me->GetAbsOrigin().DistToSqr( pGhostCarrier->GetAbsOrigin() );
			constexpr float regroupDistanceSq = 300.0f * 300.0f;
			if ( flDistToCarrierSq > regroupDistanceSq )
			{
				// Regroup
				CNEOBotPathUpdateChase( me, m_chasePath, pGhostCarrier, SAFEST_ROUTE );
			}
			else
			{
				// Hold position
				m_chasePath.Invalidate();
				m_path.Invalidate();
			}
			return Continue();
		}

		m_chasePath.Invalidate();

		Vector& vecMoveTarget = m_vecGoalPos;


		if ( m_role == ROLE_SCREEN && pBotGhostCarrier )
		{
			CWeaponGhost *pGhost = dynamic_cast<CWeaponGhost*>( pBotGhostCarrier->Weapon_GetSlot( 0 ) );
			// Only screen the threat if the ghost is actually booted up and ready to reveal enemies
			// Otherwise just stick to the goal path
			if ( pGhost && pGhost->IsBootupCompleted() )
			{
				const CKnownEntity *carrierThreat = pBotGhostCarrier->GetVisionInterface()->GetPrimaryKnownThreat();
				if ( carrierThreat && carrierThreat->GetEntity() && carrierThreat->GetEntity()->IsAlive() )
				{
					vecMoveTarget = carrierThreat->GetLastKnownPosition();

				}
			}
		}

		// Move to Target (Goal or Threat)
		// Throttling repath to avoid excessive computations
		if ( m_repathTimer.IsElapsed() )
		{
			m_chasePath.Invalidate();

			CNEOBotPathCompute( me, m_path, vecMoveTarget, SAFEST_ROUTE );
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
		}
		m_path.Update( me );
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgEscort::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	m_chasePath.Invalidate();
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	return Continue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEscort::OnStuck( CNEOBot *me )
{
	m_chasePath.Invalidate();
	m_path.Invalidate();
	m_repathTimer.Invalidate();
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEscort::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotCtgEscort::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	m_repathTimer.Invalidate(); 
	return TryContinue();
}

//---------------------------------------------------------------------------------------------
CNEOBotCtgEscort::EscortRole CNEOBotCtgEscort::UpdateRoleAssignment( CNEOBot *me, CNEO_Player *pGhostCarrier, const Vector &vecGoalPos )
{
	// Roles:
	// 1. Scout/Vanguard: Closest to Goal.
	// 2. Bodyguard: Closest to Carrier.
	// 3. Wall/Screen: Everyone else.

	CNEO_Player* pScout = nullptr;
	CNEO_Player* pBodyguard = nullptr;
	const int iMyTeam = me->GetTeamNumber();

	// Collect candidates and process roles in one pass
	float flBestDistToGoal = FLT_MAX;
	float flBestDistToCarrier = FLT_MAX;
	float flSecondBestDistToCarrier = FLT_MAX;
	CNEO_Player* pBestToCarrier = nullptr;
	CNEO_Player* pSecondBestToCarrier = nullptr;

	const Vector& vecCarrierOrigin = pGhostCarrier->GetAbsOrigin();

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer || !pPlayer->IsAlive() || pPlayer->GetTeamNumber() != iMyTeam || pPlayer == pGhostCarrier )
		{
			continue;
		}

		const Vector& vecPlayerOrigin = pPlayer->GetAbsOrigin();

		// Check for Scout (Best dist to goal)
		float goalDist = vecPlayerOrigin.DistToSqr( vecGoalPos );
		if ( goalDist < flBestDistToGoal )
		{
			flBestDistToGoal = goalDist;
			pScout = pPlayer;
		}

		// Check for Bodyguard candidates (Dist to carrier)
		float carrierDist = vecPlayerOrigin.DistToSqr( vecCarrierOrigin );
		if ( carrierDist < flBestDistToCarrier )
		{
			flSecondBestDistToCarrier = flBestDistToCarrier;
			pSecondBestToCarrier = pBestToCarrier;

			flBestDistToCarrier = carrierDist;
			pBestToCarrier = pPlayer;
		}
		else if ( carrierDist < flSecondBestDistToCarrier )
		{
			flSecondBestDistToCarrier = carrierDist;
			pSecondBestToCarrier = pPlayer;
		}
	}

	// If the best bodyguard candidate is the scout, take the second best.
	if ( pBestToCarrier && pBestToCarrier == pScout )
	{
		pBodyguard = pSecondBestToCarrier;
	}
	else
	{
		pBodyguard = pBestToCarrier;
	}

	if ( me == pScout )
	{
		return ROLE_SCOUT;
	}
	else if ( me == pBodyguard )
	{
		return ROLE_BODYGUARD;
	}
	else
	{
		return ROLE_SCREEN;
	}
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgEscort::UpdateGoalPosition( CNEOBot *me, CNEO_Player *pGhostCarrier )
{
	m_vecGoalPos = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_bHasGoal = false;
	
	float flNearestCapDistSq = FLT_MAX;
	const int iMyTeam = me->GetTeamNumber();

	for( int i=0; i<NEORules()->m_pGhostCaps.Count(); ++i )
	{
		CNEOGhostCapturePoint *pCapPoint = dynamic_cast<CNEOGhostCapturePoint*>( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
		if ( !pCapPoint ) continue;
		if ( pCapPoint->owningTeamAlternate() == iMyTeam )
		{
			float d = pGhostCarrier->GetAbsOrigin().DistToSqr( pCapPoint->GetAbsOrigin() );
			if ( d < flNearestCapDistSq )
			{
				flNearestCapDistSq = d;
				m_vecGoalPos = pCapPoint->GetAbsOrigin();
				m_bHasGoal = true;
			}
		}
	}
}
