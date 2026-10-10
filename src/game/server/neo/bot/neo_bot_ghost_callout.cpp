#include "cbase.h"
#include "bot/neo_bot_ghost_callout.h"
#include "bot/neo_bot.h"
#include "neo_gamerules.h"
#include "neo_player.h"
#include "weapon_ghost.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_bot_ghost_callout_interval( "sv_neo_bot_ghost_callout_interval", "1.5", FCVAR_CHEAT,
	"How often bots learn the enemy at the center of their ghost carrier's aim.", true, 0.1f, false, 0 );

namespace NEOBotGhostCallout
{
	// A carrier calls out an enemy within this angle of the center of their view
	static constexpr float CALLOUT_MAX_ANGLE_DEG = 30.0f;

	static float s_flNextCalloutTime = 0.0f;

	// The carrier whose ghost has shown beacons during this carry, so they know where enemies are
	static int s_iBootedCarrier = 0;

	// Track whether the current carry has booted the ghost, by the beacon HUD's rule
	static bool IsCarryBooted( CNEO_Player *pCarrier )
	{
		if ( s_iBootedCarrier != pCarrier->entindex() )
		{
			s_iBootedCarrier = pCarrier->GetBeaconingGhost() ? pCarrier->entindex() : 0;
		}

		return s_iBootedCarrier == pCarrier->entindex();
	}

	// The enemy within the ghost's reveal range that is closest to the center of the carrier's view.
	// No line of sight test: the ghost reveals enemies through walls.
	static CNEO_Player *FindAimedEnemy( CNEO_Player *pCarrier, const CWeaponGhost *pGhost )
	{
		Vector vecForward;
		pCarrier->EyeVectors( &vecForward );
		const Vector vecEye = pCarrier->EyePosition();

		CNEO_Player *pBest = nullptr;
		float flBestDot = cosf( DEG2RAD( CALLOUT_MAX_ANGLE_DEG ) );
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CNEO_Player *pEnemy = ToNEOPlayer( UTIL_PlayerByIndex( i ) );
			if ( !pEnemy || pEnemy->GetTeamNumber() == pCarrier->GetTeamNumber() )
			{
				continue;
			}

			float flDistIgnored;
			if ( !pGhost->BeaconRange( pEnemy, flDistIgnored ) )
			{
				continue;
			}

			Vector vecToEnemy = pEnemy->WorldSpaceCenter() - vecEye;
			vecToEnemy.NormalizeInPlace();

			const float flDot = DotProduct( vecToEnemy, vecForward );
			if ( flDot > flBestDot )
			{
				flBestDot = flDot;
				pBest = pEnemy;
			}
		}

		return pBest;
	}

	void Update()
	{
		if ( !NEORules()->IsTeamplay() )
		{
			return;
		}

		CNEO_Player *pCarrier = ToNEOPlayer( UTIL_PlayerByIndex( NEORules()->GetGhosterPlayer() ) );
		if ( !pCarrier || !pCarrier->IsAlive() )
		{
			s_iBootedCarrier = 0;
			return;
		}

		// Once the ghost has booted during this carry, the carrier calls out with any weapon out,
		// as a human shouts the position of the enemy they are fighting
		if ( !IsCarryBooted( pCarrier ) || gpGlobals->curtime < s_flNextCalloutTime )
		{
			return;
		}

		const CWeaponGhost *pGhost = assert_cast<const CWeaponGhost *>( GetNeoWepWithBits( pCarrier, NEO_WEP_GHOST ) );
		if ( !pGhost )
		{
			return;
		}

		s_flNextCalloutTime = gpGlobals->curtime + sv_neo_bot_ghost_callout_interval.GetFloat();

		CNEO_Player *pTarget = FindAimedEnemy( pCarrier, pGhost );
		if ( !pTarget )
		{
			return;
		}

		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CNEOBot *pMate = ToNEOBot( UTIL_PlayerByIndex( i ) );
			if ( !pMate || !pMate->IsAlive() || pMate == pCarrier || pMate->GetTeamNumber() != pCarrier->GetTeamNumber() )
			{
				continue;
			}

			// a position snapshot, as a callout gives
			pMate->GetVisionInterface()->UpdateKnownEntityPosition( pTarget );
		}
	}

	void Reset()
	{
		s_flNextCalloutTime = 0.0f;
		s_iBootedCarrier = 0;
	}
}
