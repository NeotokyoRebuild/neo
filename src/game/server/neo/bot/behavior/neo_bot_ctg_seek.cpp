#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_ctg_seek.h"
#include "bot/behavior/neo_bot_ctg_lone_wolf.h"
#include "bot/behavior/neo_bot_ctg_escort.h"
#include "bot/behavior/neo_bot_ctg_enemy.h"
#include "bot/behavior/neo_bot_ctg_carrier.h"
#include "bot/behavior/neo_bot_ctg_capture.h"
#include "bot/neo_bot_path_compute.h"
#include "weapon_ghost.h"

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotCtgSeek::Update( CNEOBot *me, float interval )
{
	if (NEORules()->GetGameType() != NEO_GAME_TYPE_CTG)
	{
		return Done( "Game mode is no longer CTG" );
	}

	ActionResult< CNEOBot > result = UpdateCommon( me, interval );
	if ( result.IsRequestingChange() || result.IsDone() )
	{
		return result;
	}

	int team_members = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CNEO_Player *pPlayer = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
		if ( pPlayer && pPlayer->IsAlive() && pPlayer->GetTeamNumber() == me->GetTeamNumber() )
		{
			team_members++;
		}
	}

	if ( team_members == 1 )
	{
		return SuspendFor( new CNEOBotCtgLoneWolf, "I'm the last one on my team!" );
	}

	if (NEORules()->GhostExists())
	{
		int iGhosterPlayer = NEORules()->GetGhosterPlayer();
		if (iGhosterPlayer > 0 && iGhosterPlayer <= gpGlobals->maxClients)
		{
			CNEO_Player* pGhostCarrier = ToNEOPlayer(UTIL_PlayerByIndex(iGhosterPlayer));
			if (pGhostCarrier && pGhostCarrier != me)
			{
				if (pGhostCarrier->GetTeamNumber() == me->GetTeamNumber())
				{
					return SuspendFor(new CNEOBotCtgEscort, "Protecting the ghost carrier!");
				}
				else
				{
					return SuspendFor(new CNEOBotCtgEnemy, "Stopping the ghost carrier!");
				}
			}

			// If I have the ghost, switch to ghost behavior
			if (me->IsCarryingGhost())
			{
				return SuspendFor(new CNEOBotCtgCarrier, "I am the ghost carrier!");
			}
		}
	}

	// Ghost capture logic
	if (m_bGoingToTargetEntity && m_hTargetEntity)
	{
		const float useRangeSq = 100.0f * 100.0f;
		if ( me->GetAbsOrigin().DistToSqr( m_hTargetEntity->GetAbsOrigin() ) < useRangeSq ) 
		{
			if ( !m_hTargetEntity->IsPlayer() )
			{
				if ( me->IsLineOfSightClear( m_hTargetEntity, CBaseCombatCharacter::IGNORE_ACTORS ) )
				{
					const char *classname = m_hTargetEntity->GetClassname();
					if ( FStrEq( classname, "weapon_ghost" ) )
					{
						CBaseCombatWeapon* pWeapon = m_hTargetEntity->MyCombatWeaponPointer();
						if ( pWeapon && !pWeapon->GetOwner() )
						{
							CWeaponGhost *pGhost = assert_cast<CWeaponGhost*>( m_hTargetEntity.Get() );
							return SuspendFor( new CNEOBotCtgCapture( pGhost ), "Capturing Ghost" );
						}
					}
				}
				else
				{
					return Done("Capture target was not a ghost");
				}
			}
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotCtgSeek::RecomputeSeekPath( CNEOBot *me )
{
	if (NEORules()->GetGameType() != NEO_GAME_TYPE_CTG)
	{
		// Wait until next tick to exit behavior
		return;
	}

	m_hTargetEntity = nullptr;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = vec3_origin;

	if (NEORules()->GhostExists())
	{
		const int iGhosterPlayer = NEORules()->GetGhosterPlayer();

		if (iGhosterPlayer > 0)
		{
			// Get ready to transition into CTG specific role next tick
			m_path.Invalidate();
			return;
		}
		else
		{
			// Search for ghost on the ground
			CBaseEntity* pGhost = gEntList.FindEntityByClassname( nullptr, "weapon_ghost" );
			if ( pGhost )
			{
				m_vGoalPos = pGhost->WorldSpaceCenter();
				m_bGoingToTargetEntity = true;
				m_hTargetEntity = pGhost;

				if ( CNEOBotPathCompute( me, m_path, m_vGoalPos, DEFAULT_ROUTE ) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH )
				{
					return;
				}
			}
		}
	}

	// Fallback to base behavior (roaming spawn points)
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}
