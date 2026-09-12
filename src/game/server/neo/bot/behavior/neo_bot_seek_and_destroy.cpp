#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "neo_ghost_cap_point.h"
#include "team_control_point_master.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "bot/behavior/neo_bot_ctg_seek.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_jgr_seek.h"
#include "bot/neo_bot_memory_sound_combat.h"
#include "bot/neo_bot_path_compute.h"
#include "nav_mesh.h"

extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_offense_must_push_time;
extern ConVar neo_bot_defense_must_defend_time;

ConVar neo_bot_debug_seek_and_destroy( "neo_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT );
ConVar neo_bot_disable_seek_and_destroy( "neo_bot_disable_seek_and_destroy", "0", FCVAR_CHEAT );

ConVar sv_neo_bot_seek_and_destroy_combat_sound_commit_time( "sv_neo_bot_seek_and_destroy_combat_sound_commit_time", "3.0", FCVAR_CHEAT,
	"Having picked a combat sound, travel to it for this long instead of re-picking.", true, 0, false, 0 );
ConVar sv_neo_bot_seek_and_destroy_combat_sound_detour_ratio( "sv_neo_bot_seek_and_destroy_combat_sound_detour_ratio", "1.5", FCVAR_CHEAT,
	"When racing for an objective, divert only if going via the combat sound costs at most this multiple of direct distance.", true, 0, false, 0 );
ConVar sv_neo_bot_seek_and_destroy_combat_sound_arrive_range( "sv_neo_bot_seek_and_destroy_combat_sound_arrive_range", "200.0", FCVAR_CHEAT,
	"Combat sound close enough to count as investigated.", true, 0, false, 0 );


//---------------------------------------------------------------------------------------------
// Is the bot inside the potentially-audible set of a sound at vSoundPos?
static bool BotInSoundPAS( CNEOBot *me, const Vector &vSoundPos )
{
	CPASFilter filter( vSoundPos );
	for ( int i = 0; i < filter.GetRecipientCount(); ++i )
	{
		if ( filter.GetRecipientIndex( i ) == me->entindex() )
		{
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// Returns true if m_path now leads to a combat sound the bot heard
bool CNEOBotSeekAndDestroy::TryPathToCombatSound( CNEOBot *me )
{
	if ( !m_bListenForCombatSounds )
	{
		return false;
	}

	const float flArriveRange = sv_neo_bot_seek_and_destroy_combat_sound_arrive_range.GetFloat();
	const float flArriveSqr = flArriveRange * flArriveRange;
	const Vector vGoalBefore = m_vGoalPos;

	// Stick with the sound already picked until arrival or the commit time runs out
	const bool bCommitted = m_combatSoundCommitTimer.HasStarted() && !m_combatSoundCommitTimer.IsElapsed()
		&& me->GetAbsOrigin().DistToSqr( m_vCombatSoundSpot ) > flArriveSqr;

	if ( bCommitted && m_path.IsValid() && m_vGoalPos == m_vCombatSoundSpot )
	{
		return false;
	}

	if ( !bCommitted )
	{
		if ( m_soundSearchTimer.HasStarted() && !m_soundSearchTimer.IsElapsed() )
		{
			return false;
		}
		m_soundSearchTimer.Start( 0.25f );

		Vector vFight;
		if ( !NEOMemorySoundCombat::FindNearestFight( me->GetAbsOrigin(), me->GetTeamNumber(), me->entindex(), vFight )
			|| !BotInSoundPAS( me, vFight ) )
		{
			return false;
		}

		if ( vGoalBefore != vec3_origin )
		{
			// Already heading there
			if ( vGoalBefore.DistToSqr( vFight ) <= flArriveSqr )
			{
				return false;
			}

			// An objective runner only takes the detour if the sound is roughly on the way
			if ( IsSeekGoalAnObjective() )
			{
				const float flDirect = me->GetAbsOrigin().DistTo( vGoalBefore );
				const float flVia = me->GetAbsOrigin().DistTo( vFight ) + vFight.DistTo( vGoalBefore );
				if ( flVia > flDirect * sv_neo_bot_seek_and_destroy_combat_sound_detour_ratio.GetFloat() )
				{
					return false;
				}
			}
		}

		m_vCombatSoundSpot = vFight;
		m_combatSoundCommitTimer.Start( sv_neo_bot_seek_and_destroy_combat_sound_commit_time.GetFloat() );
	}

	if ( CNEOBotPathCompute( me, m_path, m_vCombatSoundSpot, DEFAULT_ROUTE )
			&& m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
	{
		m_vGoalPos = m_vCombatSoundSpot;
		m_bGoingToTargetEntity = false;
		return true;
	}

	// NEO Jank: the combat sound is unreachable, so give up on it and stop listening for a few seconds
	m_combatSoundCommitTimer.Invalidate();
	m_soundSearchTimer.Start( 3.0f );

	if ( vGoalBefore != vec3_origin )
	{
		m_vGoalPos = vGoalBefore;
		CNEOBotPathCompute( me, m_path, m_vGoalPos, DEFAULT_ROUTE );
	}

	return false;
}


//---------------------------------------------------------------------------------------------
CNEOBotSeekAndDestroy::CNEOBotSeekAndDestroy( float duration )
{
	if ( duration > 0.0f )
	{
		m_giveUpTimer.Start( duration );
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekAndDestroy::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	RecomputeSeekPath( me );

	// restart the timer if we have one
	if ( m_giveUpTimer.HasStarted() )
	{
		m_giveUpTimer.Reset();
	}

	if ( neo_bot_disable_seek_and_destroy.GetBool() )
	{
		return Done( "Disabled." );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot >	CNEOBotSeekAndDestroy::Update( CNEOBot *me, float interval )
{
	// Check for Game Type Specific behaviors and suspend for them
	if ( NEORules()->GetRemainingPreRoundFreezeTime( true ) > 0.0f )
	{
		if (NEORules()->GetGameType() == NEO_GAME_TYPE_CTG)
		{
			// Only switch to CTG behavior if there are available capture zones this round
			bool bHasAvailableCapZone = false;
			const int iMyTeam = me->GetTeamNumber();

			for( int i=0; i<NEORules()->m_pGhostCaps.Count(); ++i )
			{
				CNEOGhostCapturePoint *pCapPoint = dynamic_cast<CNEOGhostCapturePoint*>( UTIL_EntityByIndex( NEORules()->m_pGhostCaps[i] ) );
				if ( !pCapPoint || !pCapPoint->GetActive() )
				{
					continue;
				}

				const int iCapTeam = pCapPoint->owningTeamAlternate();
				if ( iCapTeam == iMyTeam || iCapTeam == TEAM_ANY )
				{
					bHasAvailableCapZone = true;
					break;
				}
			}

			if ( bHasAvailableCapZone )
			{
				return SuspendFor( new CNEOBotCtgSeek, "Switching to Ghost-related Seek and Destroy" );
			}

		}
		else if (NEORules()->GetGameType() == NEO_GAME_TYPE_JGR)
		{
			return SuspendFor( new CNEOBotJgrSeek, "Switching to Juggernaut-related Seek and Destroy" );
		}

		return Continue();
	}

	if (NEORules()->GetGameType() == NEO_GAME_TYPE_CTG)
	{
		// Check if enemy has the ghost
		if (NEORules()->GhostExists())
		{
			int iGhosterPlayer = NEORules()->GetGhosterPlayer();
			if (iGhosterPlayer > 0 && iGhosterPlayer <= gpGlobals->maxClients)
			{
				CNEO_Player* pGhostCarrier = ToNEOPlayer(UTIL_PlayerByIndex(iGhosterPlayer));
				if (pGhostCarrier && pGhostCarrier != me && pGhostCarrier->GetTeamNumber() != me->GetTeamNumber())
				{
					return SuspendFor(new CNEOBotCtgEnemy, "Stopping the ghost carrier!");
				}
			}
		}
	}

	ActionResult< CNEOBot > result = UpdateCommon( me, interval );
	if ( result.IsRequestingChange() || result.IsDone() )
	{
		return result;
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotSeekAndDestroy::UpdateCommon( CNEOBot *me, float interval )
{
	if ( m_giveUpTimer.HasStarted() && m_giveUpTimer.IsElapsed() )
	{
		return Done( "Behavior duration elapsed" );
	}

	if ( neo_bot_disable_seek_and_destroy.GetBool() )
	{
		return Done( "Disabled." );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat(true);

	if ( threat )
	{
		const auto *neoThreat = ToNEOPlayer(threat->GetEntity());
		// This will just go to the ghoster RecomputeSeekPath logics instead of
		// only going after it
		const bool bDontSuspendForGhoster = (neoThreat && neoThreat->IsCarryingGhost());
		if (!bDontSuspendForGhoster)
		{
			const Vector& threatLastKnownPos = threat->GetLastKnownPosition();
			// fall back to nearest teammate for backup if I am the closest contact to enemy
			if ( NEORules()->IsTeamplay() )
			{
				bool bAnyTeammatesCloserToEnemy = false;
				CNEO_Player* pNearestTeammate = nullptr;
				float distToNearestTeammateSqr = FLT_MAX;
				const Vector& myPos = me->GetAbsOrigin();
				const float distMeToThreatSqr = myPos.DistToSqr(threatLastKnownPos);

				for (int i = 1; i <= gpGlobals->maxClients; i++)
				{
					CBasePlayer *pPlayer = UTIL_PlayerByIndex(i);
					if (!pPlayer)
					{
						continue;
					}
					
					CNEO_Player *pNeoCandidate = ToNEOPlayer(pPlayer);
					if (!pNeoCandidate)
					{
						continue;
					}

					if (pNeoCandidate->InSameTeam(me) && (me != pNeoCandidate))
					{
						const Vector& candidatePos = pNeoCandidate->GetAbsOrigin();
						if (threatLastKnownPos.DistToSqr(candidatePos) < distMeToThreatSqr)
						{
							bAnyTeammatesCloserToEnemy = true;
							break;
						}	

						float distCandidateSqr = myPos.DistToSqr(candidatePos);
						if (distCandidateSqr < distToNearestTeammateSqr)
						{
							distToNearestTeammateSqr = distCandidateSqr;
							pNearestTeammate = pNeoCandidate;
						}
					}
				}

				if (!bAnyTeammatesCloserToEnemy && pNearestTeammate)
				{
					return SuspendFor( new CNEOBotAttack( pNearestTeammate->GetAbsOrigin() ), "Kiting enemy toward teammate for backup" );
				}
			}
			
			return SuspendFor( new CNEOBotAttack, "Going after an enemy" );
		}
	}
	else
	{
		// Out of combat
		me->DisableCloak();

		// Reload when safe
		me->ReloadIfLowClip();
	}

	// move towards our seek goal
	m_path.Update( me );

	m_bTimerElapsed = m_repathTimer.HasStarted() && m_repathTimer.IsElapsed();

	if ( m_bGoingToTargetEntity )
	{
		bool bEntityVisible = false;
		if ( m_hTargetEntity )
		{
			bEntityVisible = true;

			CBaseEntity* ent = m_hTargetEntity.Get();
			if (ent)
			{
				CBaseCombatWeapon* pWeapon = ent->MyCombatWeaponPointer();
				if (pWeapon)
				{
					if (pWeapon->IsEffectActive(EF_NODRAW))
						bEntityVisible = false;
					
					if (pWeapon->GetOwner() != NULL)
						bEntityVisible = false;
					
					// I don't want it anymore.
					if (me->Weapon_OwnsThisType(pWeapon->GetClassname()))
						bEntityVisible = false;
				}
			}
		}

		// If I can see the goal, and the entity isn't visible, then 
		if ( me->IsLineOfSightClear( m_vGoalPos ) && !bEntityVisible )
		{
			// Keep looking for a couple seconds.
			if ( !m_itemStolenTimer.HasStarted() )
				m_itemStolenTimer.Start( 2.0f );

			if ( m_itemStolenTimer.HasStarted() && m_itemStolenTimer.IsElapsed() )
			{
				m_itemStolenTimer.Reset();
				m_path.Invalidate();
			}
		}
	}

	if ( !m_path.IsValid() )
	{
		if ( !m_repathFailTimer.HasStarted() || m_repathFailTimer.IsElapsed() )
		{
			m_repathTimer.Start( 45.0f );

			RecomputeSeekPath( me );

			if ( m_path.IsValid() )
			{
				m_repathFailTimer.Invalidate();
			}
			else
			{
				m_repathFailTimer.Start( 1.0f );
			}
		}
	}
	// Listen for combat sounds on the way to wherever we were going
	else if ( TryPathToCombatSound( me ) )
	{
		m_repathTimer.Start( 45.0f );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotSeekAndDestroy::OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction )
{
	RecomputeSeekPath( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnStuck( CNEOBot *me )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnMoveToSuccess( CNEOBot *me, const Path *path )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason )
{
	RecomputeSeekPath( me );

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CNEOBotSeekAndDestroy::ShouldRetreat( const INextBot *meBot ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CNEOBotSeekAndDestroy::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}

class CNextSpawnFilter : public IEntityFindFilter
{
public:
	CNextSpawnFilter( EHANDLE hPlayer, float flRange )
		: m_hPlayer{ hPlayer }
		, m_flRange{ flRange }
	{
		
	}

	bool ShouldFindEntity( CBaseEntity *pEntity )
	{
		// If we find no truly valid marks, we'll just use the first.
		if ( !m_hEntityFound.Get() )
		{
			m_hEntityFound = pEntity;
		}

		if ( pEntity->GetAbsOrigin().DistToSqr( m_hPlayer->GetAbsOrigin() ) < ( m_flRange * m_flRange ) )
		{
			return false;
		}

		m_hEntityFound = pEntity;
		return true;
	}

	CBaseEntity *GetFilterResult( void )
	{
		return m_hEntityFound;
	}

private:
	EHANDLE		m_hPlayer;

	float		m_flRange;

	// To maintain backwards compatability, store off the first mark
	// we find. If we find no truly valid marks, we'll just use the first.
	EHANDLE		m_hEntityFound;
};

class CNotOwnedWeaponFilter : public IEntityFindFilter
{
public:
	CNotOwnedWeaponFilter( CBasePlayer *pPlayer )
		: m_hPlayer{ pPlayer }
	{
		
	}

	bool ShouldFindEntity( CBaseEntity *pEntity )
	{
		// If we find no truly valid marks, we'll just use the first.
		if ( !m_hEntityFound.Get() )
		{
			m_hEntityFound = pEntity;
		}

		CBaseCombatWeapon *pWeapon = dynamic_cast< CBaseCombatWeapon *>( pEntity );
		if ( !pWeapon )
			return false;

		if ( pWeapon->GetOwner() )
			return false;

		// ignore non-existent ammo to ensure we collect nearby existing ammo
		if ( pWeapon->IsEffectActive( EF_NODRAW ) )
			return false;

		if ( m_hPlayer->Weapon_OwnsThisType( pEntity->GetClassname() ) )
			return false;
		
		m_hEntityFound = pEntity;
		return true;
	}

	CBaseEntity *GetFilterResult( void )
	{
		return m_hEntityFound;
	}

private:
	CHandle<CBasePlayer>	m_hPlayer;

	// To maintain backwards compatability, store off the first mark
	// we find. If we find no truly valid marks, we'll just use the first.
	EHANDLE		m_hEntityFound;
};


//---------------------------------------------------------------------------------------------
void CNEOBotSeekAndDestroy::RecomputeSeekPath( CNEOBot *me )
{
	if ( m_bOverrideApproach )
	{
		return;
	}

	m_hTargetEntity = NULL;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = vec3_origin;

	if ( !TheNavAreas.Size() )
	{
		m_path.Invalidate();
		return;
	}

#if 0 // NEO TODO (Adam) search for the ghost separately, also can't pick up weapons on contact so bots just jump around weapons thinking they're stuck indefinitely
	// Don't try to find weapons if the timer elapsed. Probably went bad?
	if ( !m_bTimerElapsed )
	{
		CUtlVector<CBaseEntity*> pWeapons;

		CNotOwnedWeaponFilter weaponFilter( me );
		CBaseEntity* pSearch = NULL;
		while ( ( pSearch = gEntList.FindEntityByClassname( pSearch, "weapon_*", &weaponFilter ) ) != NULL )
		{
			if ( pSearch )
				pWeapons.AddToTail( pSearch );
		}

		pWeapons.SortPredicate(
			[&]( CBaseEntity* a, CBaseEntity* b )
			{
				float flDistA = me->GetAbsOrigin().DistToSqr( a->GetAbsOrigin() );
				float flDistB = me->GetAbsOrigin().DistToSqr( b->GetAbsOrigin() );

				return flDistA < flDistB;
			}
		);

		// Try and find weapons we don't have above all else on the map.
		for ( int i = 0; i < pWeapons.Size(); i++ )
		{
			CBaseEntity* pClosestWeapon = pWeapons[i];
			if ( pClosestWeapon )
			{
				m_hTargetEntity = pClosestWeapon;
				m_bGoingToTargetEntity = true;
				m_vGoalPos = pClosestWeapon->WorldSpaceCenter();
				if ( CNEOBotPathCompute( me, m_path, m_vGoalPos, DEFAULT_ROUTE ) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
					return;
			}
		}
	}
#endif
	
	// Listen for combat sounds
	if ( TryPathToCombatSound( me ) )
	{
		return;
	}

	// Fallback and roam random spawn points if we have all weapons.
	{
		CNextSpawnFilter spawnFilter( me, 128.0f );

		CUtlVector<CBaseEntity*> pSpawns;

		CBaseEntity* pSearch = NULL;
		while ( ( pSearch = gEntList.FindEntityByClassname( pSearch, "info_player_*", &spawnFilter ) ) != NULL )
		{
			if ( pSearch && Q_strcmp(pSearch->GetEntityName().ToCStr(), "info_player_start"))
				pSpawns.AddToTail( pSearch );
		}

		// Don't wander between spawns if there aren't that many.
		if ( pSpawns.Size() >= 3 )
		{
			for ( int i = 0; i < 10; i++ )
			{
				m_hTargetEntity = pSpawns[RandomInt( 0, pSpawns.Size() - 1 )];
				m_bGoingToTargetEntity = true;
				m_vGoalPos = m_hTargetEntity->WorldSpaceCenter();
				if ( CNEOBotPathCompute( me, m_path, m_vGoalPos, DEFAULT_ROUTE ) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
					return;
			}
		}
	}

	for ( int i = 0; i < 10; i++ )
	{
		// No spawns we can get to? Just wander... somewhere!

		Vector vWanderPoint = TheNavAreas[RandomInt( 0, TheNavAreas.Size() - 1 )]->GetCenter();
		m_vGoalPos = vWanderPoint;
		if ( CNEOBotPathCompute( me, m_path, vWanderPoint, DEFAULT_ROUTE ) )
			return;
	}

	m_path.Invalidate();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnTerritoryContested( CNEOBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Defending the point" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnTerritoryCaptured( CNEOBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Giving up due to point capture" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnTerritoryLost( CNEOBot *me, int territoryID )
{
	return TryDone( RESULT_IMPORTANT, "Giving up due to point lost" );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CNEOBot > CNEOBotSeekAndDestroy::OnCommandApproach( CNEOBot* me, const Vector& pos, float range )
{
	m_bOverrideApproach = true;
	m_vOverrideApproach = pos;

	CNEOBotPathCompute( me, m_path, m_vOverrideApproach, DEFAULT_ROUTE );

	return TryContinue();
}
