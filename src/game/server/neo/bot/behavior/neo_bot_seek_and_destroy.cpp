#include "cbase.h"
#include "util_shared.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "team_control_point_master.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_attack.h"
#include "bot/behavior/neo_bot_seek_and_destroy.h"
#include "nav_mesh.h"
#include "neo_ghost_cap_point.h"
#include "neo_smokegrenade.h"

extern ConVar neo_bot_path_lookahead_range;
extern ConVar neo_bot_offense_must_push_time;
extern ConVar neo_bot_defense_must_defend_time;

ConVar neo_bot_debug_seek_and_destroy( "neo_bot_debug_seek_and_destroy", "0", FCVAR_CHEAT );
ConVar neo_bot_disable_seek_and_destroy( "neo_bot_disable_seek_and_destroy", "0", FCVAR_CHEAT );

ConVar sv_neo_bot_cmdr_stop_distance_sq("sv_neo_bot_cmdr_stop_distance_sq", "5000",
	FCVAR_NONE, "Minimum distance gap between following bots", true, 3000, true, 100000);
ConVar sv_neo_bot_cmdr_look_weights_friendly_repulsion("sv_neo_bot_cmdr_look_weights_friendly_repulsion", "20",
	FCVAR_NONE, "Weight for friendly bot repulsion force", true, 1, true, 9999);
ConVar sv_neo_bot_cmdr_look_weights_wall_repulsion("sv_neo_bot_cmdr_look_weights_wall_repulsion", "10",
	FCVAR_NONE, "Weight for wall repulsion force", true, 1, true, 9999);
ConVar sv_neo_bot_cmdr_look_weights_explosives_repulsion("sv_neo_bot_cmdr_look_weights_explosives_repulsion", "20",
	FCVAR_NONE, "Weight for explosive repulsion force", true, 1, true, 9999);

ConVar sv_neo_grenade_check_radius("sv_neo_grenade_check_radius", "1500",
	FCVAR_NONE, "Radius for bots to check for grenades", true, 1, true, 9999);

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
		const auto *neoThreat = ToNEOPlayer(threat->GetEntity());
		// This will just go to the ghoster RecomputeSeekPath logics instead of
		// only going after it
		const bool bDontSuspendForGhoster = (neoThreat && neoThreat->IsCarryingGhost());
		if (!bDontSuspendForGhoster)
		{
			const float engageRange = 1000.0f;
			if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), engageRange ) )
			{
				return SuspendFor( new CNEOBotAttack, "Going after an enemy" );
			}
		}
	}
	else
	{
		// Out of combat
		if ( me->m_hLeadingPlayer.Get() )
		{
			// Leading player's position tends to change frequently
			RecomputeSeekPath( me );
			// In contrast, a commander's ping waypoint tends to be relatively less mobile

			CNEO_Player *pPlayerToMirror = me->m_hLeadingPlayer.Get();
			if (pPlayerToMirror)
			{
				if (pPlayerToMirror->GetInThermOpticCamo())
				{
					me->EnableCloak(4.0f);
				}
				else
				{
					me->DisableCloak();
				}

				if (pPlayerToMirror->IsDucking())
				{
					me->PressCrouchButton(0.5f);
				}
				else
				{
					me->ReleaseCrouchButton();
				}
			}
		}
		else
		{
			me->DisableCloak();
			me->ReleaseCrouchButton();
		}

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

extern ConVar sv_neo_bot_cmdr_stop_distance_sq;
//---------------------------------------------------------------------------------------------
void CNEOBotSeekAndDestroy::RecomputeSeekPath( CNEOBot *me )
{
	if ( m_bOverrideApproach )
	{
		return;
	}

	if (FollowCommandChain(me))
	{
		// A successfully commanded bot ignores the usual seek and destroy logic
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

		const int iMyTeam = me->GetTeamNumber();
		const int iGhosterTeam = NEORules()->GetGhosterTeam();

		bool bGoToGoalPos = true;
		bool bGetCloserToGhoster = false;
		bool bQuickToGoalPos = false;

		if (iGhosterPlayer > 0)
		{
			const int iTargetCapTeam = (iGhosterTeam == iMyTeam) ? iMyTeam : iGhosterTeam;

			// If there's a player playing ghost, turn toward cap zones that's
			// closest to the ghoster player
			Vector vrTargetCapPos;
			int iMinCapGhostLength = INT_MAX;

			// Enemy team is carrying the ghost - try to defend the cap zone
			// You or friendly team is carrying the ghost - go towards the cap point

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
				// Friendly - get closer and assists, enemy - get closer and attack
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
				const int iDistSqrConsidered = (iGhosterTeam == iMyTeam) ? 50000 : 5000;
				if (m_vGoalPos.DistToSqr(me->GetAbsOrigin()) < iDistSqrConsidered)
				{
					// Don't stop targeting entity even when near enough
					return;
				}
			}
			else
			{
				constexpr int DISTANCE_CONSIDERED_ARRIVED_SQUARED = 100000;
				if (m_vGoalPos.DistToSqr(me->GetAbsOrigin()) < DISTANCE_CONSIDERED_ARRIVED_SQUARED)
				{
					FanOutAndCover(me, m_vGoalPos, true, DISTANCE_CONSIDERED_ARRIVED_SQUARED);
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

// ---------------------------------------------------------------------------------------------
// Bot commander utilities
//
// ---------------------------------------------------------------------------------------------
// Process commander ping waypoint commands for bots
// true - order has been processed, don't perform other actions in seek_and_destroy
// false - no order, continue with other seek_and_destroy logic
bool CNEOBotSeekAndDestroy::FollowCommandChain(CNEOBot* me)
{
	Assert(me);
	CNEO_Player* pCommander = me->m_hCommandingPlayer.Get();

	if (!pCommander)
	{
		return false;
	}

	float follow_stop_distance_sq = sv_neo_bot_cmdr_stop_distance_sq.GetFloat();
	if (pCommander->m_flBotDynamicFollowDistanceSq > follow_stop_distance_sq)
	{
		follow_stop_distance_sq = pCommander->m_flBotDynamicFollowDistanceSq;
	}

	if (pCommander->IsAlive())
	{
		// Follow commander if they are close enough to collect you (and ping cooldown elapsed)
		if (!me->m_hLeadingPlayer && pCommander->m_tBotPlayerPingCooldown.IsElapsed()
			&& me->GetAbsOrigin().DistToSqr(pCommander->GetAbsOrigin()) < sv_neo_bot_cmdr_stop_distance_sq.GetFloat())
		{
			// Use sv_neo_bot_cmdr_stop_distance_sq for consistent bot collection range
			// follow_stop_distance_sq would be confusing if player doesn't know about distance tuning
			me->m_hLeadingPlayer = pCommander;
			m_vGoalPos = vec3_origin;
			pCommander->m_vLastPingByStar.GetForModify(me->GetStar()) = vec3_origin;
		}
		// Go to commander's ping
		else if (pCommander->m_vLastPingByStar.Get(me->GetStar()) != vec3_origin)
		{
			// Check if there's been an update for this star's ping waypoint
			if (pCommander->m_vLastPingByStar.Get(me->GetStar()) != me->m_vLastPingByStar.Get(me->GetStar()))
			{
				me->m_hLeadingPlayer = nullptr; // Stop following and start travelling to ping
				m_vGoalPos = pCommander->m_vLastPingByStar.Get(me->GetStar());
				me->m_vLastPingByStar.GetForModify(me->GetStar()) = pCommander->m_vLastPingByStar.Get(me->GetStar());

				// Force a repath to allow for fine tuned positioning
				CNEOBotPathCost cost(me, SAFEST_ROUTE);
				if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
				{
					return true;
				}
			}

			if (FanOutAndCover(me, m_vGoalPos))
			{
				// FanOutAndCover true: arrived at destination and settled, so don't recompute path
				return true;
			}

			CNEOBotPathCost cost(me, SAFEST_ROUTE);
			if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				return true;
			}
			else
			{
				// Something went wrong with pathing
				me->m_hLeadingPlayer = nullptr;
				me->m_hCommandingPlayer = nullptr;
				return false;
			}
		}
	}
	else
	{
		// Commander died
		me->m_hLeadingPlayer = nullptr;
		me->m_hCommandingPlayer = nullptr;
		return false;
	}

	// Didn't have order from commander, so follow in snake formation
	if (me->m_hLeadingPlayer.Get())
	{
		CNEO_Player* pLeader = me->m_hLeadingPlayer.Get();
		if (pLeader && pLeader->IsAlive())
		{
			if (me->GetAbsOrigin().DistToSqr(pLeader->GetAbsOrigin()) < follow_stop_distance_sq)
			{
				// Anti-collision: follow neighbor in snake chain
				for (int idx = 1; idx <= gpGlobals->maxClients; ++idx)
				{
					CNEO_Player* pOther = static_cast<CNEO_Player*>(UTIL_PlayerByIndex(idx));
					if (!pOther || !pOther->IsBot() || pOther == me
						|| (pOther->m_hLeadingPlayer.Get() != me->m_hLeadingPlayer.Get()))
					{
						// Not another bot in the same command structure
						continue;
					}

					if (me->GetAbsOrigin().DistToSqr(pOther->GetAbsOrigin()) < follow_stop_distance_sq / 2)
					{
						// Follow person I bumped into and link up to snake chain of followers
						me->m_hLeadingPlayer = pOther;
						break;
					}
				}

				m_hTargetEntity = NULL;
				m_bGoingToTargetEntity = false;
				m_path.Invalidate();
				Vector tempLeaderOrigin = pLeader->GetAbsOrigin();  // don't want to override m_vGoalPos
				FanOutAndCover(me, tempLeaderOrigin, false, follow_stop_distance_sq);
				return true;
			}

			// Set the bot's goal to the leader's position.
			m_vGoalPos = pLeader->GetAbsOrigin();
			CNEOBotPathCost cost(me, SAFEST_ROUTE);
			if (m_path.Compute(me, m_vGoalPos, cost, 0.0f, true, true) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				// Prioritize following leader.
				return true;
			}
			else
			{
				// Something went wrong with pathing
				me->m_hLeadingPlayer = nullptr;
				me->m_hCommandingPlayer = nullptr;
				return false;
			}
		}
		else
		{
			// Commander is no longer valid or alive, stop following.
			me->m_hLeadingPlayer = nullptr;
			me->m_hCommandingPlayer = nullptr;
		}
	}
	else if (pCommander)
	{
		me->m_hLeadingPlayer = pCommander;
	}
	return false;
}
//
// ---------------------------------------------------------------------------------------------
// Process commander ping waypoint commands for bots
// The movementTarget is mutable to accomodate spreading out at goal zone 
bool CNEOBotSeekAndDestroy::FanOutAndCover(CNEOBot* me, Vector& movementTarget, bool bMoveToSeparate /*= false*/, float flArrivalZoneSizeSq /*= -1.0f*/)
{
	if (flArrivalZoneSizeSq == -1.0f)
	{
		flArrivalZoneSizeSq = sv_neo_bot_cmdr_stop_distance_sq.GetFloat();
	}
	Vector vBotRepulsion = vec3_origin;
	Vector vWallRepulsion = vec3_origin;
	Vector vExplosiveRepulsion = vec3_origin;
	Vector vFinalForce = vec3_origin; // Initialize to vec3_origin
	bool bTooClose = false;
	bool bPrevAvoidingExplosive = m_bAvoidingExplosive;

	auto processExplosive = [&](CBaseEntity* pExplosive, bool bIsSmokeGrenade = false)
	{
		Assert(pExplosive != nullptr);

		// Bots should stop being afraid of grenades that turn out to be smoke
		if (bIsSmokeGrenade)
		{
			// Assume on release builds that we didn't input the wrong entity query
			Assert(FStrEq(pExplosive->GetClassname(), "neo_grenade_smoke"));
			CNEOGrenadeSmoke* pSmokeGrenade = dynamic_cast<CNEOGrenadeSmoke*>(pExplosive);
			if (pSmokeGrenade && pSmokeGrenade->HasSettled())
			{
				// Already detonated, confirmed not HE
				return false;
			}
		}

		Vector vToExplosive = pExplosive->GetAbsOrigin() - me->GetAbsOrigin();
		vToExplosive.z = 0;
		float flDistSqr = vToExplosive.LengthSqr();

		// Check if the explosive is dangerous and visible to the bot
		trace_t tr;
		UTIL_TraceLine(pExplosive->GetAbsOrigin(), me->GetBodyInterface()->GetEyePosition(), MASK_PLAYERSOLID, pExplosive, COLLISION_GROUP_NONE, &tr);

		if (tr.fraction == 1.0f || tr.m_pEnt == me) // Explosive is visible to the bot
		{
			if (flDistSqr > 0.001f)
			{
				// Strong repulsion away from the explosive
				float flRepulsion = 1.0f - (sqrt(flDistSqr) / 10000.0f);
				vExplosiveRepulsion -= vToExplosive.Normalized() * flRepulsion;
				return true; // found dangerous explosive
			}
		}
		return false; // not a relevant entity
	};

	auto findAndProcessExplosives = [&](const char* classname, bool bIsSmokeGrenade = false)
	{
		bool bFoundExplosive = false;
		const float flGrenadeCheckRadius = sv_neo_grenade_check_radius.GetFloat();
		
		CBaseEntity *pSearch = nullptr;
		CBaseEntity *pNext = nullptr;
		
		// Limit entity search within a reasonable sphere of danger
		CUtlVector<CBaseEntity*> pEntitiesInSphere;

		while ((pNext = gEntList.FindEntityInSphere(pSearch, me->GetAbsOrigin(), flGrenadeCheckRadius)) != NULL)
		{
			pSearch = pNext; // Continue search from the last found entity
			pEntitiesInSphere.AddToTail(pSearch);
		}

		for (int i = 0; i < pEntitiesInSphere.Count(); ++i)
		{
			CBaseEntity* pEntity = pEntitiesInSphere[i];
			if (FStrEq(pEntity->GetClassname(), classname))
			{
				if (processExplosive(pEntity, bIsSmokeGrenade))
				{
					bFoundExplosive = true;
				}
			}
		}
		m_bAvoidingExplosive = bFoundExplosive;
	};

	if (m_bAvoidingExplosive || (gpGlobals->curtime > m_flNextFanOutLookCalcTime))
	{
		findAndProcessExplosives("neo_grenade_frag");
		if (me->GetDifficulty() <= CNEOBot::DifficultyType::NORMAL)
		{
			// Easier bots mistake smoke grenades as dangerous
			findAndProcessExplosives("neo_grenade_smoke", true);
		}

		// Combined loop for player forces and proximity checks
		if (!m_bAvoidingExplosive) // Don't worry about crashing into other players if panicking
		{
			for (int idx = 1; idx <= gpGlobals->maxClients; ++idx)
			{
				CBasePlayer* pPlayer = UTIL_PlayerByIndex(idx);
				if (!pPlayer || pPlayer == me || !pPlayer->IsAlive())
					continue;

				if (pPlayer->GetTeamNumber() == me->GetTeamNumber())
				{
					// Friendly player
					CNEO_Player* pOther = ToNEOPlayer(pPlayer);
					if (pOther)
					{
						// Determine if we are too close to any friendly bot
						if (me->GetAbsOrigin().DistToSqr(pOther->GetAbsOrigin()) < sv_neo_bot_cmdr_stop_distance_sq.GetFloat() / 2)
						{
							bTooClose = true;
						}

						// Check for line of sight from other player to me for repulsion
						trace_t tr;
						UTIL_TraceLine(pOther->EyePosition(), me->EyePosition(), MASK_PLAYERSOLID, pOther, COLLISION_GROUP_NONE, &tr);

						if (tr.DidHit()) // if we can see 'me'
						{
							float flRepulsion = 1.0f - (tr.fraction);
							vBotRepulsion -= tr.plane.normal * flRepulsion;
						}
					}
				}
			}
		}

		// Next look time
		if (bTooClose)
		{
			m_flNextFanOutLookCalcTime = gpGlobals->curtime + 0.15; // 150 ms
		}
		else
		{
			int waitSec = RandomFloat(0.2f, 3.0f);
			m_flNextFanOutLookCalcTime = gpGlobals->curtime + waitSec;
		}


		// Wall Repulsion
		trace_t tr;
		const int numWhiskers = 12;
		for (int i = 0; i < numWhiskers; ++i)
		{
			QAngle ang = me->GetLocalAngles();
			ang.y += (360.0f / numWhiskers) * i;
			Vector vWhiskerDir;
			AngleVectors(ang, &vWhiskerDir);
			float whiskerDist = 1000.0f;
			UTIL_TraceLine(me->GetBodyInterface()->GetEyePosition(), me->GetBodyInterface()->GetEyePosition() + vWhiskerDir * whiskerDist, MASK_SHOT, me, COLLISION_GROUP_PROJECTILE, &tr);
			if (tr.DidHit())
			{
				float flRepulsion = 1.0f - (tr.fraction);
				vWallRepulsion += tr.plane.normal * flRepulsion;
			}
		}

		// Combine forces and look at final direction
		float friendlyRepulsionWeight = sv_neo_bot_cmdr_look_weights_friendly_repulsion.GetFloat();
		float wallRepulsionWeight = sv_neo_bot_cmdr_look_weights_wall_repulsion.GetFloat();
		float explosiveRepulsionWeight = sv_neo_bot_cmdr_look_weights_explosives_repulsion.GetFloat();
		vFinalForce = (vBotRepulsion * friendlyRepulsionWeight) + (vWallRepulsion * wallRepulsionWeight) + (vExplosiveRepulsion * explosiveRepulsionWeight); // Add explosive repulsion
		vFinalForce.NormalizeInPlace();
		vFinalForce.z = 0; // avoid tilting awkwardly up or down
		me->GetBodyInterface()->AimHeadTowards(me->GetBodyInterface()->GetEyePosition() + vFinalForce * 500.0f);
	}

	// Fudge factor to reduce teammates running into each other trying to reach the same point
	if (me->GetAbsOrigin().DistToSqr(movementTarget) < flArrivalZoneSizeSq)
	{
		if (bMoveToSeparate || m_bAvoidingExplosive)
		{
			if (bTooClose || m_bAvoidingExplosive)
			{
				me->PressForwardButton();
			}

			if (m_bAvoidingExplosive)
			{
				me->PressRunButton();
			}
		}

		movementTarget = me->GetAbsOrigin();
		m_path.Invalidate();
		return true; // Is already at destination
	}

	// State change transition from avoiding to not avoiding explosive
	if (!m_bAvoidingExplosive && bPrevAvoidingExplosive)
	{
		CNEO_Player* pCommander = me->m_hCommandingPlayer.Get();
		if (pCommander)
		{
			Vector vPingWaypoint = pCommander->m_vLastPingByStar.Get(me->GetStar());
			if (vPingWaypoint != vec3_origin)
			{
				movementTarget = vPingWaypoint;
			}
			else
			{
				movementTarget = pCommander->GetAbsOrigin();
			}
			// Path will be recomputed by the calling context.
			return false; // in case another scenario added after
			// expecting return to be optimized out for release builds
		}
	}

	return false; // still moving to destination
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