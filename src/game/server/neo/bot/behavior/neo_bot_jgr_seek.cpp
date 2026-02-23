#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_jgr_seek.h"
#include "bot/behavior/neo_bot_jgr_escort.h"
#include "bot/behavior/neo_bot_jgr_enemy.h"
#include "bot/behavior/neo_bot_jgr_capture.h"
#include "bot/behavior/neo_bot_jgr_juggernaut.h"
#include "neo_juggernaut.h"


//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrSeek::Update( CNEOBot *me, float interval )
{
	m_bInvestigateGunfire = false; // Focus on capturing JGR

	if (NEORules()->GetGameType() != NEO_GAME_TYPE_JGR)
	{
		return Done( "Game mode is no longer JGR" );
	}

	if ( me->GetClass() != NEO_CLASS_JUGGERNAUT )
	{
		int iJuggernautPlayer = NEORules()->GetJuggernautPlayer();
		if (iJuggernautPlayer > 0)
		{
			CNEO_Player* pJuggernaut = ToNEOPlayer(UTIL_PlayerByIndex(iJuggernautPlayer));
			Assert( pJuggernaut != me );
			if (pJuggernaut && pJuggernaut != me && pJuggernaut->IsAlive())
			{
				if (pJuggernaut->GetTeamNumber() == me->GetTeamNumber())
				{
					return SuspendFor(new CNEOBotJgrEscort, "Escorting the Juggernaut");
				}
				else
				{
					return SuspendFor(new CNEOBotJgrEnemy, "Attacking the Juggernaut");
				}
			}
		}
	}

	ActionResult< CNEOBot > result = UpdateCommon( me, interval );
	if ( result.IsRequestingChange() || result.IsDone() )
		return result;

	if ( me->GetClass() == NEO_CLASS_JUGGERNAUT )
	{
		return SuspendFor( new CNEOBotJgrJuggernaut, "I am the Juggernaut now" );
	}

	// Juggernaut objective capture logic
	if (m_bGoingToTargetEntity && m_hTargetEntity)
	{
		const float useRangeSq = CNEO_Juggernaut::GetUseDistanceSquared() * 0.8f;
		if ( me->GetAbsOrigin().DistToSqr( m_hTargetEntity->GetAbsOrigin() ) < useRangeSq ) 
		{
			if ( !m_hTargetEntity->IsPlayer() )
			{
				if ( me->IsLineOfSightClear( m_hTargetEntity, CBaseCombatCharacter::IGNORE_ACTORS ) )
				{
					const char *classname = m_hTargetEntity->GetClassname();

					if ( FStrEq( classname, "neo_juggernaut" ) )
					{
						return SuspendFor( new CNEOBotJgrCapture( static_cast<CNEO_Juggernaut*>(m_hTargetEntity.Get()) ), "Capturing Juggernaut" );
					}
				}
			}
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrSeek::RecomputeSeekPath( CNEOBot *me )
{
	if (NEORules()->GetGameType() != NEO_GAME_TYPE_JGR)
	{
		CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
		return;
	}

	m_hTargetEntity = NULL;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = vec3_origin;

	if (NEORules()->JuggernautItemExists())
	{
		const int iJuggernautPlayer = NEORules()->GetJuggernautPlayer();

		// If it's on the ground (no owner)
		if (iJuggernautPlayer <= 0)
		{
			CBaseEntity* pJuggernaut = gEntList.FindEntityByClassname(NULL, "neo_juggernaut");
			if (pJuggernaut)
			{
				m_vGoalPos = pJuggernaut->WorldSpaceCenter();
				m_bGoingToTargetEntity = true;
				m_hTargetEntity = pJuggernaut;

				if (CNEOBotPathCompute(me, m_path, m_vGoalPos, FASTEST_ROUTE) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
				{
					return;
				}
			}
		}

	}

	// Fallback to base behavior (roaming spawn points)
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}
