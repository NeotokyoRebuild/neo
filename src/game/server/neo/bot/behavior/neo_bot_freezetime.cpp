#include "cbase.h"
#include "neo_player.h"
#include "bot/neo_bot.h"
#include "bot/behavior/neo_bot_freezetime.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// sv_neo_bot_freezetime_planning_start_time_buffer should be longer than sv_neo_bot_freezetime_planning_end_time_buffer
// ideally with enough ticks left to plan next round paths
// used to calibrate throttle timer for checking if a bot is in freezetime in tactical monitor
ConVar sv_neo_bot_freezetime_planning_start_time_buffer( "sv_neo_bot_freezetime_planning_start_time_buffer", "2", FCVAR_NONE,
	"Seconds of freezetime that must be available before bots will start planning.", true, 0.0f, false, 0 );

// sv_neo_bot_freezetime_planning_end_time_buffer should leave enough ticks for path planning before the round starts
ConVar sv_neo_bot_freezetime_planning_end_time_buffer( "sv_neo_bot_freezetime_planning_end_time_buffer", "1", FCVAR_NONE,
	"Seconds of freezetime left for bots to stop planning prior to round start.", true, 0.0f, false, 0 );

ActionResult< CNEOBot > CNEOBotFreezeTime::OnStart( CNEOBot *me, Action< CNEOBot > *priorAction )
{
	m_bBuddyPairingDecisionMade = false;
	return Continue();
}

ActionResult< CNEOBot > CNEOBotFreezeTime::Update( CNEOBot* me, float interval )
{
	if ( !me || !me->IsAlive() )
	{
		return Done("Bot is not in valid state for freezetime planning");
	}

	const float freezeTimeLeft = NEORules()->GetRemainingPreRoundFreezeTime( true );
	if ( freezeTimeLeft < sv_neo_bot_freezetime_planning_end_time_buffer.GetFloat() )
	{
		return Done("Done planning, waiting for freeze time to end");
	}

	if ( CNEO_Player* buddy = me->m_hCommandingPlayer.Get() )
	{
		// Check if buddy was reassigned to another partner
		if ( buddy->m_hCommandingPlayer.Get() != nullptr )
		{
			// Retry looking for another buddy
			m_bBuddyPairingDecisionMade = false;
			me->m_hCommandingPlayer = nullptr;
		}
	}

	if ( !m_bBuddyPairingDecisionMade )
	{
		const bool foundBuddy = TryFindBuddy( me );
		if ( foundBuddy )
		{
			return ChangeTo( new CNEOBotCommandFollow, "Freeze-time buddy found" );
		}
	}

	return Continue();
}

bool CNEOBotFreezeTime::TryFindBuddy( CNEOBot *me )
{
	if ( m_bBuddyPairingDecisionMade )
	{
		return true;
	}

	// Check if I already have a commander
	// This could happen if a human selects a bot as a follower
	if ( me->m_hCommandingPlayer.Get() != nullptr )
	{
		return false;
	}

	m_bBuddyPairingDecisionMade = true;

	// We shouldn't be pairing up in free for all matches
	if ( !NEORules()->IsTeamplay() )
	{
		return false;
	}

	CUtlVector<CNEO_Player *> friendlyPlayers;
	CollectPlayers( &friendlyPlayers, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

	CNEO_Player *bestBuddy = nullptr;
	float bestDistanceSq = FLT_MAX;

	for ( int i = 0; i < friendlyPlayers.Count(); ++i )
	{
		CNEO_Player *friendly = friendlyPlayers[i];
		if ( !friendly || !WantsToPairWithBuddyCandidate( me, friendly ) )
		{
			continue;
		}

		const float distSq = ( friendly->GetAbsOrigin() - me->GetAbsOrigin() ).LengthSqr();
		if ( distSq < bestDistanceSq )
		{
			bestDistanceSq = distSq;
			bestBuddy = friendly;
		}
	}

	if ( !bestBuddy )
	{
		return false;
	}

	me->m_hCommandingPlayer = bestBuddy;
	me->m_hLeadingPlayer = bestBuddy;
	return true;
}

bool CNEOBotFreezeTime::WantsToPairWithBuddyCandidate( const CNEO_Player *me, const CNEO_Player *buddy )
{
	if ( !me || !buddy || me == buddy )
	{
		return false;
	}

	// CollectPlayers should only return teammates
	Assert ( buddy->GetTeamNumber() == me->GetTeamNumber() );

	// It might confuse human players if bots decide to follow them unprompted
	if ( !buddy->IsBot() )
	{
		return false;
	}

	// This bot is already paired with a leader
	if ( buddy->m_hCommandingPlayer.Get() != nullptr )
	{
		return false;
	}

	// Bots in different squad stars should not link with each other
	// Humans can rearrange bot squad star membership to influence valid pairings
	if ( buddy->GetStar() == me->GetStar())
	{
		return false;
	}

	const int myClass = me->GetClass();
	const int buddyClass = buddy->GetClass();

	if ( myClass == NEO_CLASS_RECON )
	{
		// Recons only pair with other recons
		return buddyClass == NEO_CLASS_RECON;
	}
	if ( myClass == NEO_CLASS_ASSAULT )
	{
		// Assaults avoid pairing with Supports and VIPS
		return buddyClass == NEO_CLASS_RECON || buddyClass == NEO_CLASS_ASSAULT;
	}
	if ( myClass == NEO_CLASS_SUPPORT )
	{
		// Supports avoid following VIPs
		return buddyClass != NEO_CLASS_VIP;
	}

	// VIPs can pair with anyone
	// JGR does not exist as a player in freezetime
	return true;
}
