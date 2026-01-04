#include "cbase.h"
#include "neo_player.h"
#include "neo_gamerules.h"
#include "bot/neo_bot.h"
#include "bot/neo_bot_path_compute.h"
#include "bot/behavior/neo_bot_jgr_juggernaut.h"

//---------------------------------------------------------------------------------------------
static CSound* SearchGunfireSounds(CNEOBot* me)
{
	CSound* pClosestSound = NULL;
	float flClosestDistSqr = FLT_MAX;
	const Vector& vecMyOrigin = me->GetAbsOrigin();

	int iSound = CSoundEnt::ActiveList();
	while (iSound != SOUNDLIST_EMPTY)
	{
		CSound* pSound = CSoundEnt::SoundPointerForIndex(iSound);
		if (!pSound)
			break;

		if ((pSound->SoundType() & (SOUND_COMBAT | SOUND_BULLET_IMPACT)) && pSound->ValidateOwner())
		{
			// Don't listen to sounds I was responsible for
			if (pSound->m_hOwner.Get() == me->GetEntity())
			{
				iSound = pSound->NextSound();
				continue;
			}

			// Search for the closest gunfire sounds
			float distSqr = (pSound->GetSoundOrigin() - vecMyOrigin).LengthSqr();
			if (distSqr < flClosestDistSqr)
			{
				flClosestDistSqr = distSqr;
				pClosestSound = pSound;
			}
		}

		iSound = pSound->NextSound();
	}

	return pClosestSound;
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrJuggernaut::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_jgrSpawns.RemoveAll();

	CBaseEntity* pSearch = NULL;
	while ((pSearch = gEntList.FindEntityByClassname(pSearch, "neo_juggernautspawnpoint")) != NULL)
	{
		m_jgrSpawns.AddToTail(pSearch);
	}

	m_stuckTimer.Invalidate();
	m_bStuckCrouchPhase = false;

	return Continue();
}

//---------------------------------------------------------------------------------------------
ActionResult< CNEOBot > CNEOBotJgrJuggernaut::Update( CNEOBot *me, float interval )
{
	ActionResult< CNEOBot > result = UpdateCommon( me, interval );
	if ( result.IsRequestingChange() || result.IsDone() )
		return result;

	// Listen for gunfire
	if ( !m_soundSearchTimer.HasStarted() || m_soundSearchTimer.IsElapsed() )
	{
		m_soundSearchTimer.Start( 0.25f );

		CSound* pBestSound = SearchGunfireSounds(me);
		if (pBestSound)
		{
			// Only change goal if recent gunfire is radically different than where I was going
			constexpr float flThresholdSqr = 200.0f * 200.0f;
			if (m_vGoalPos.DistToSqr(pBestSound->GetSoundOrigin()) > flThresholdSqr)
			{
				m_vGoalPos = pBestSound->GetSoundOrigin();
				// gunfire is not an entity target
				m_bGoingToTargetEntity = false;

				if (CNEOBotPathCompute(me, m_path, m_vGoalPos, FASTEST_ROUTE))
				{
					return Continue(); 
				}
			}
		}
	}

	return Continue();
}

//---------------------------------------------------------------------------------------------
void CNEOBotJgrJuggernaut::RecomputeSeekPath( CNEOBot *me )
{
	m_hTargetEntity = NULL;
	m_bGoingToTargetEntity = false;
	m_vGoalPos = vec3_origin;

	// Listen for gunfights
	CSound* pBestSound = SearchGunfireSounds(me);
	if (pBestSound)
	{
		m_vGoalPos = pBestSound->GetSoundOrigin();
		m_bGoingToTargetEntity = false;

		if (CNEOBotPathCompute(me, m_path, m_vGoalPos, FASTEST_ROUTE) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
		{
			return;
		}
	}

	// If unaware of gunfights, patrol spawn points
	if (m_jgrSpawns.Count() > 0)
	{
		CBaseEntity* pTargetSpawn = m_jgrSpawns[ RandomInt(0, m_jgrSpawns.Count() - 1) ];
		if (pTargetSpawn)
		{
			m_vGoalPos = pTargetSpawn->GetAbsOrigin();
			m_bGoingToTargetEntity = false;

			if (CNEOBotPathCompute(me, m_path, m_vGoalPos, DEFAULT_ROUTE) && m_path.IsValid() && m_path.GetResult() == Path::COMPLETE_PATH)
			{
				return;
			}
		}
	}
	
	// Fallback to base behavior
	CNEOBotSeekAndDestroy::RecomputeSeekPath( me );
}


//---------------------------------------------------------------------------------------------
// Adds a periodic crouch attempt on top of CNEOBotMainAction::OnStuck for height clearance
EventDesiredResult< CNEOBot > CNEOBotJgrJuggernaut::OnStuck( CNEOBot *me )
{
	UTIL_LogPrintf( "\"%s<%i><%s>\" stuck (position \"%3.2f %3.2f %3.2f\") (duration \"%3.2f\") ",
			me->GetNeoPlayerName(),
			me->GetUserID(),
			me->GetNetworkIDString(),
			me->GetAbsOrigin().x, me->GetAbsOrigin().y, me->GetAbsOrigin().z,
			me->GetLocomotionInterface()->GetStuckDuration() );

	const PathFollower *path = me->GetCurrentPath();
	if ( path && path->GetCurrentGoal() )
	{
		UTIL_LogPrintf( "   path_goal ( \"%3.2f %3.2f %3.2f\" )\n",
				path->GetCurrentGoal()->pos.x, 
				path->GetCurrentGoal()->pos.y, 
				path->GetCurrentGoal()->pos.z );
	}
	else
	{
		UTIL_LogPrintf( "   path_goal ( \"NULL\" )\n" );
	}

	if ( !m_stuckTimer.HasStarted() || m_stuckTimer.IsElapsed() )
	{
		m_stuckTimer.Start( 1.0f );
		m_bStuckCrouchPhase = !m_bStuckCrouchPhase;
	}

	// Try to crouch under doorways/gates
	if ( m_bStuckCrouchPhase )
	{
		me->PressCrouchButton();
	}
	// ... in addition to usual OnStuck behavior
	else
	{
		me->GetLocomotionInterface()->Jump();
	}

	if ( RandomInt( 0, 1 ) )
	{
		me->PressLeftButton();
	}
	else
	{
		me->PressRightButton();
	}

	return TryContinue();
}

