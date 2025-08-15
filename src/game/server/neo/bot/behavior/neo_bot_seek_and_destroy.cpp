#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "team_control_point_master.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "nav_mesh.h"
#include "neo_ghost_cap_point.h"

extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_offense_must_push_time;
extern ConVar neo_bot_defense_must_defend_time;

ConVar neo_bot_debug_seek_and_destroy( "neo_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT );
ConVar neo_bot_disable_seek_and_destroy( "neo_bot_disable_seek_and_destroy", "0", FCVAR_CHEAT );


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
	if ( m_giveUpTimer.HasStarted() && m_giveUpTimer.IsElapsed() )
	{
		return Done( "Behavior duration elapsed" );
	}

	if ( neo_bot_disable_seek_and_destroy.GetBool() )
	{
		return Done( "Disabled." );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat )
	{
		const float engageRange = 1000.0f;
		if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), engageRange ) )
		{
			return SuspendFor( new CNEOBotAttack, "Going after an enemy" );
		}
	}
	else
	{
		// Out of combat
		me->DisableCloak();

		// Reload when safe
		CNEOBaseCombatWeapon* myWeapon = static_cast<CNEOBaseCombatWeapon*>(me->GetActiveWeapon());
		if (myWeapon && myWeapon->GetPrimaryAmmoCount() > 0)
		{
			bool shouldReload = false;
			// SUPA7 reload doesn't discard ammo
			if ((myWeapon->GetNeoWepBits() & NEO_WEP_SUPA7) && (myWeapon->Clip1() < myWeapon->GetMaxClip1()))
			{
				shouldReload = true;
			}
			else
			{
				int maxClip = myWeapon->GetMaxClip1();
				bool isBarrage = me->IsBarrageAndReloadWeapon(myWeapon);

				int baseThreshold = isBarrage ? (maxClip / 3) : (maxClip / 2);

				float aggressionFactor = 1.0f - me->HealthFraction();

				float dynamicThreshold = baseThreshold + aggressionFactor * (maxClip - baseThreshold);

				if (myWeapon->Clip1() < static_cast<int>(dynamicThreshold))
				{
					shouldReload = true;
				}
			}

			if (shouldReload)
			{
				me->ReleaseFireButton();
				me->PressReloadButton();
				me->PressCrouchButton(0.3f);
			}

		}
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
		m_repathTimer.Start( 45.0f );

		RecomputeSeekPath( me );
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
				CNEOBotPathCost cost( me, SAFEST_ROUTE );
				m_hTargetEntity = pClosestWeapon;
				m_bGoingToTargetEntity = true;
				m_vGoalPos = pClosestWeapon->WorldSpaceCenter();
				if ( m_path.Compute( me, m_vGoalPos, cost, 0.0f, true, true ) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
					return;
			}
		}
	}
#endif

	if (NEORules()->GhostExists())
	{
		const Vector vGhostPos = NEORules()->GetGhostPos();
		const int iGhosterPlayer = NEORules()->GetGhosterPlayer();

		bool bGoToGoalPos = true;
		bool bGetCloserToGhoster = false;
		bool bQuickToGoalPos = false;

		if (iGhosterPlayer > 0)
		{
			const int iMyTeam = me->GetTeamNumber();
			const int iGhosterTeam = NEORules()->GetGhosterTeam();

			// If there's a player playing ghost, turn toward cap zones that's
			// closest to the ghoster player
			Vector vrTargetCapPos;
			int iMinCapGhostLength = INT_MAX;

			// Enemy team is carrying the ghost - try to defend the cap zone
			// You or friendly team is carrying the ghost - go towards the cap point
			const int iTargetCapTeam = (iGhosterTeam == iMyTeam) ? iMyTeam : iGhosterTeam;

			for (int i = 0; i < NEORules()->m_pGhostCaps.Count(); i++)
			{
				auto pGhostCap = dynamic_cast<CNEOGhostCapturePoint *>(
						UTIL_EntityByIndex(NEORules()->m_pGhostCaps[i]));
				if (!pGhostCap)
				{
					continue;
				}

				const Vector vCapPos = pGhostCap->GetAbsOrigin();
				const Vector vGhostCapDist = vGhostPos - vCapPos;
				const int iGhostCapLength = static_cast<int>(vGhostCapDist.Length());
				const int iCapTeam = pGhostCap->owningTeamAlternate();

				if (iCapTeam == iTargetCapTeam && iGhostCapLength < iMinCapGhostLength)
				{
					vrTargetCapPos = vCapPos;
					iMinCapGhostLength = iGhostCapLength;
				}
			}

			if (!me->IsCarryingGhost())
			{
				// If a ghoster player carrying and nearby, get close to them
				const float flGhosterMeters = METERS_PER_INCH * me->GetAbsOrigin().DistTo(vGhostPos);
				const float flMinCapMeters = METERS_PER_INCH * iMinCapGhostLength;
				static const constexpr float FL_NEARBY_FOLLOW_METERS = 26.0f;
				static const constexpr float FL_NEARBY_CAPZONE_METERS = 18.0f;
				const bool bGhosterNearby = flGhosterMeters < FL_NEARBY_FOLLOW_METERS;
				const bool bCapzoneNearby = flMinCapMeters < FL_NEARBY_CAPZONE_METERS;
				// But a nearby capzone overrides a nearby ghoster
				bGetCloserToGhoster = !bCapzoneNearby && bGhosterNearby && flMinCapMeters > flGhosterMeters;
			}

			if (bGetCloserToGhoster)
			{
				m_vGoalPos = vGhostPos;
				bQuickToGoalPos = true;
			}
			else
			{
				// iMinCapGhostLength == INT_MAX should never happen, just disable going to target
				Assert(iMinCapGhostLength < INT_MAX);
				bGoToGoalPos = (iMinCapGhostLength < INT_MAX);

				m_vGoalPos = vrTargetCapPos;
				bQuickToGoalPos = (iGhosterTeam != iMyTeam);
			}
		}
		else
		{
			// If the ghost exists, go to the ghost
			m_vGoalPos = vGhostPos;
			// NEO TODO (nullsystem): More sophisticated on handling non-ghost playing scenario,
			// although it kind of already prefer hunting down players when they're in view, but
			// just going towards ghost isn't something that always happens in general.
		}

		if (bGoToGoalPos)
		{
			if (bGetCloserToGhoster)
			{
				constexpr int DISTANCE_CONSIDERED_ASSISTING_SQUARED = 50000;
				if (m_vGoalPos.DistToSqr(me->GetAbsOrigin()) < DISTANCE_CONSIDERED_ASSISTING_SQUARED)
				{
					// Don't stop targeting entity even when near enough
					return;
				}
			}
			else
			{
				constexpr int DISTANCE_CONSIDERED_ARRIVED_SQUARED = 10000;
				if (m_vGoalPos.DistToSqr(me->GetAbsOrigin()) < DISTANCE_CONSIDERED_ARRIVED_SQUARED)
				{
					constexpr float RECHECK_TIME = 30.f;
					m_repathTimer.Start(RECHECK_TIME);
					m_bGoingToTargetEntity = false;
					return;
				}
			}
			m_bGoingToTargetEntity = true;
			CNEOBotPathCost cost(me, bQuickToGoalPos ? FASTEST_ROUTE : SAFEST_ROUTE);
			if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				return;
			}
		}
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
				CNEOBotPathCost cost( me, SAFEST_ROUTE );
				m_hTargetEntity = pSpawns[RandomInt( 0, pSpawns.Size() - 1 )];
				m_bGoingToTargetEntity = true;
				m_vGoalPos = m_hTargetEntity->WorldSpaceCenter();
				if ( m_path.Compute( me, m_vGoalPos, cost, 0.0f, true, true ) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
					return;
			}
		}
	}

	for ( int i = 0; i < 10; i++ )
	{
		// No spawns we can get to? Just wander... somewhere!

		CNEOBotPathCost cost( me, SAFEST_ROUTE );
		Vector vWanderPoint = TheNavAreas[RandomInt( 0, TheNavAreas.Size() - 1 )]->GetCenter();
		m_vGoalPos = vWanderPoint;
		if ( m_path.Compute( me, vWanderPoint, cost ) )
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

	CNEOBotPathCost cost( me, SAFEST_ROUTE );
	m_path.Compute( me, m_vOverrideApproach, cost );

	return TryContinue();
}
