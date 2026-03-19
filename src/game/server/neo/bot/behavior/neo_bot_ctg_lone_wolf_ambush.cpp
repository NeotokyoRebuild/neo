#include "cbase.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf_ambush.h"
#include "bot/neo_bot_path_compute.h"
#include "neo_detpack.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "neo_player.h"
#include "soundent.h"
#include "weapon_detpack.h"

//---------------------------------------------------------------------------------------------
bool CNEOBotCtgLoneWolfAmbush::CanBotHearPosition( CNEOBot *me, const Vector &pos )
{
	CPASFilter filter( pos );
	for ( int i = 0; i < filter.GetRecipientCount(); ++i )
	{
		// Is bot in recipient list?
		if ( filter.GetRecipientIndex( i ) == me->entindex() )
		{
			return true;
		}
	}
	return false;
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgLoneWolfAmbush::EquipDetpackIfOwned( CNEOBot *me )
{
	if ( m_hDetpackWeapon && me->GetActiveWeapon() != m_hDetpackWeapon )
	{
		me->Weapon_Switch( m_hDetpackWeapon );
	}
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfAmbush::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_bInvestigatingGunfire = false;
	m_vecAmbushHidingSpot = CNEO_Player::VECTOR_INVALID_WAYPOINT;
	m_hDetpackWeapon = assert_cast<CWeaponDetpack*>( me->Weapon_OwnsThisType( "weapon_remotedet" ) );

	m_repathTimer.Invalidate();
	m_ambushExpirationTimer.Start( RandomFloat( 30.0f, 90.0f ) );

	if ( !m_hDetpackWeapon )
	{
		// Don't need to plant detpack
		m_path.Invalidate();
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotCtgLoneWolfAmbush::Update( CNEOBot *me, float interval )
{
	if ( !GetGhost() )
	{
		return Done( "Ghost not found" );
	}
	
	if ( m_ambushExpirationTimer.HasStarted() && m_ambushExpirationTimer.IsElapsed() )
	{
		return Done( "Ran out of patience waiting at ambush" );
	}

	// For now, assume the bot is forgetful of exact detpack deploy location and uses ghost beacon as reference
	// We could trace line of sight to the detpack, but most of the time bot is hiding around a corner
	// Players could move ghost by shooting it, but ideally the bot would investigate the gunshot sources
	Vector myVecDetpackPosChoice = GetGhostPosition( me ); // ghost can be moved
	float myDistToDetpackPosChoiceSq = me->GetAbsOrigin().DistToSqr( myVecDetpackPosChoice );

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat( true );
	if ( threat && threat->GetEntity() )
	{
		if ( m_hDetpackWeapon && (me->GetActiveWeapon() == m_hDetpackWeapon)
			&& (m_hDetpackWeapon->m_bThisDetpackHasBeenThrown)
			&& (myDistToDetpackPosChoiceSq > Square(NEO_DETPACK_DAMAGE_RADIUS)) )
		{
			// Detonate detpack in case enemy's friend is grabbing the ghost
			// or simply as a distraction
			me->PressFireButton();
		}
		return ChangeTo( new CNEOBotAttack(), "Engaging enemy from ambush" );
	}

	// Consider setting up detpack ambush
	bool bIsDetpackDeployed = m_hDetpackWeapon && m_hDetpackWeapon->m_bThisDetpackHasBeenThrown && !m_hDetpackWeapon->m_bRemoteHasBeenTriggered;
	if ( m_hDetpackWeapon && !bIsDetpackDeployed )
	{
		// Close enough to ghost to place it?
		float flStartArmingDistSq = GetDetpackDeployDistanceSq( me );

		if ( myDistToDetpackPosChoiceSq < flStartArmingDistSq )
		{
			if ( m_hDetpackWeapon && me->GetActiveWeapon() != m_hDetpackWeapon )
			{
				EquipDetpackIfOwned( me );
			}
			else
			{
				// Start arming
				me->PressFireButton();
				// Reset patience since just set trap
				m_ambushExpirationTimer.Start( RandomFloat( 30.0f, 60.0f ) );

				// Reduce movement jitter
				if (myDistToDetpackPosChoiceSq < Square(64.0f))
				{
					m_path.Invalidate();
				}
			}
		}
		
		// Move closer to ghost
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, GetGhostPosition( me ), FASTEST_ROUTE );
			m_path.Update( me );
			m_repathTimer.Start( 0.5f );

			// Reset patience timer since actively travelling
			m_ambushExpirationTimer.Start( RandomFloat( 30.0f, 60.0f ) );
		}
		else
		{
			m_path.Update( me );
		}
		return Continue();
	}
	else if ( me->IsCarryingGhost() )
	{
		// Drop ghost if accidentally picked it up and have set up ambush/detpack
		if (me->GetActiveWeapon() != GetGhost() )
		{
			me->Weapon_Switch( GetGhost() );
		}
		else
		{
			me->PressDropButton();
		}
	}

	// Investigate gunshots
	if ( m_bInvestigatingGunfire )
	{
		if ( me->GetAbsOrigin().DistToSqr( m_vecAmbushHidingSpot ) < Square( 100.0f ) )
		{
			// Didn't find source of gunshots, reconsidering ambush
			m_bInvestigatingGunfire = false;
			m_vecAmbushHidingSpot = CNEO_Player::VECTOR_INVALID_WAYPOINT;
		}
		else
		{
			me->EquipBestWeaponForThreat(threat);
		}
	}

	if ( m_vecAmbushHidingSpot == CNEO_Player::VECTOR_INVALID_WAYPOINT )
	{
		m_vecAmbushHidingSpot = GetNearestCapturePoint( me, true );

		if ( m_vecAmbushHidingSpot == CNEO_Player::VECTOR_INVALID_WAYPOINT )
		{
			// Rare edge case: No cap points defined on this map?
			m_vecAmbushHidingSpot = me->GetAbsOrigin();
		}
	}

	
	if ( !m_bInvestigatingGunfire && myDistToDetpackPosChoiceSq > Square( NEO_DETPACK_DAMAGE_RADIUS * 2.0f ) &&
		 !me->GetVisionInterface()->IsLineOfSightClear( myVecDetpackPosChoice ) )
	{
		// At a good enough hiding spot
		if ( m_vecAmbushHidingSpot != me->GetAbsOrigin() )
		{
			m_vecAmbushHidingSpot = me->GetAbsOrigin();
			m_path.Invalidate();
		}
	}
	else if ( me->GetAbsOrigin().DistToSqr(m_vecAmbushHidingSpot) > Square(64.0f) )
	{
		// Moving to ambush spot
		if ( !m_repathTimer.HasStarted() || m_repathTimer.IsElapsed() )
		{
			CNEOBotPathCompute( me, m_path, m_vecAmbushHidingSpot, SAFEST_ROUTE );
			m_path.Update( me );
			m_ambushExpirationTimer.Start( RandomFloat( 30.0f, 60.0f ) );
			m_repathTimer.Start( 0.5f );
		}
		else
		{
			m_path.Update( me );
		}
	}
	else
	{
		// Waiting in place
		EquipDetpackIfOwned( me );
		UpdateLookAround( me );
	}

	// Detpack trigger condition checks
	bool bShouldDetonate = false;
	CBaseCombatCharacter *pGhostOwner = GetGhost() ? GetGhost()->GetOwner() : nullptr;

	if (!bIsDetpackDeployed)
	{
		return Continue(); // assumes detpack trigger conditions are last
	}
	else if ( myDistToDetpackPosChoiceSq < Square( NEO_DETPACK_DAMAGE_RADIUS ) )
	{
		return Continue(); // either move away from detpack or engage threat next tick
	}
	else if ( pGhostOwner && !me->InSameTeam( pGhostOwner ) )
	{
		// Ghost picked up by enemy
		bShouldDetonate = true;
	}
	else if ( gpGlobals->curtime - me->GetLastDamageTime() < 1.0f )
	{
		// Panic! Took damage and don't necessarily know where enemy is coming from
		bShouldDetonate = true;
	}
	else
	{
		// Listen for activity around the ghost/detpack
		CSound *pSound = nullptr;
		for ( int iSound = CSoundEnt::ActiveList(); iSound != SOUNDLIST_EMPTY; iSound = pSound->NextSound() )
		{
			pSound = CSoundEnt::SoundPointerForIndex( iSound );
			if ( !pSound )
			{
				break;
			}


			if ( ( pSound->SoundType() & ( SOUND_COMBAT | SOUND_PLAYER ) ) == 0 )
			{
				continue;
			}

			CBaseEntity *pOwner = pSound->m_hOwner.Get();

			// Ignore non-player sounds and sounds we were responsible for
			if ( !pOwner || !pOwner->IsPlayer() || pOwner == me )
			{
				continue;
			}

			// Only care about sounds from the enemy
			if ( me->InSameTeam( pOwner ) )
			{
				continue;
			}

			bool bCanHearSound = CanBotHearPosition( me, pSound->GetSoundOrigin() );

			Vector vecShooterOrigin = pOwner->GetAbsOrigin();
			bool bCanHearShooter = CanBotHearPosition( me, vecShooterOrigin );

			if ( !bCanHearSound && !bCanHearShooter )
			{
				continue;
			}

			bool bTriggerDetpack = false;
			
			if ( bCanHearSound )
			{
				float flThreshold;
				switch ( me->GetDifficulty() )
				{
				case CNEOBot::EASY:
					flThreshold = 1.05f;
					break;
				case CNEOBot::NORMAL:
					flThreshold = 0.95f;
					break;
				case CNEOBot::HARD:
					flThreshold = 0.85f;
					break;
				case CNEOBot::EXPERT:
					flThreshold = 0.75f;
					break;
				default:
					flThreshold = 0.95f;
					break;
				}

				// Check if the sound happened within the detpack radius around the ghost
				float distSqr = pSound->GetSoundOrigin().DistToSqr( GetGhostPosition( me ) );
				if ( distSqr <= Square( NEO_DETPACK_DAMAGE_RADIUS * flThreshold ) )
				{
					bTriggerDetpack = true;
				}
			}

			if ( bTriggerDetpack )
			{
				bShouldDetonate = true;
				break;
			}
			else
			{
				if ( m_vecAmbushHidingSpot != vecShooterOrigin )
				{
					m_bInvestigatingGunfire = true;
					m_vecAmbushHidingSpot = vecShooterOrigin;
					m_path.Invalidate();
				}
			}
		}
	}

	if ( bShouldDetonate )
	{
		m_bInvestigatingGunfire = false;
		if ( me->GetActiveWeapon() == m_hDetpackWeapon )
		{
			me->PressFireButton();
		}
		else
		{
			EquipDetpackIfOwned( me );
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotCtgLoneWolfAmbush::ShouldRetreat( const INextBot *me ) const
{
	// Avoid tactical monitor interrupting detpack detonation
	return ANSWER_NO;
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgLoneWolfAmbush::OnSuspend( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnSuspend: Cancel out of ambush, need to reevaluate situation later" );
	// OnEnd will get called after Done
}

//---------------------------------------------------------------------------------------------
ActionResult<CNEOBot> CNEOBotCtgLoneWolfAmbush::OnResume( CNEOBot *me, Action<CNEOBot> *interruptingAction )
{
	return Done( "OnResume: Cancel out of ambush, need to reevaluate situation later" );
	// OnEnd will get called after Done
}
