#include "cbase.h"
#include "bot/neo_bot_memory_sound_combat.h"
#include "neo_gamerules.h"
#include "soundent.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_neo_bot_memory_sound_combat_time_window( "sv_neo_bot_memory_sound_combat_time_window", "3", FCVAR_CHEAT,
	"How long a combat sound source stays worth investigating after it goes quiet.", true, 0, false, 0 );
ConVar sv_neo_bot_memory_sound_combat_cluster_radius( "sv_neo_bot_memory_sound_combat_cluster_radius", "500.0", FCVAR_CHEAT,
	"Radius threshold to cluster combat sound sources as the same fight.", true, 0, false, 0 );

namespace NEOMemorySoundCombat
{

	struct Source
	{
		Vector pos = vec3_origin;
		float  lastHeard = -FLT_MAX;	// sound expiry: bumped by every new sound, so it tracks live combat
		int    team = 0;				// for ignoring shots from same team
		int    volume = 0;				// audible radius of the last sound heard from it
	};

	// Indexed by player entindex
	static Source s_sources[ MAX_PLAYERS + 1 ];

	void Update()
	{
		CSound *pSound = nullptr;
		for ( int i = CSoundEnt::ActiveList(); i != SOUNDLIST_EMPTY; i = pSound->NextSound() )
		{
			pSound = CSoundEnt::SoundPointerForIndex( i );
			if ( !pSound )
			{
				break;
			}

			if ( !( pSound->SoundType() & SOUND_COMBAT ) || !pSound->DoesSoundExpire() )
			{
				continue;
			}

			CBaseEntity *pOwner = pSound->m_hOwner.Get();
			if ( !pOwner || !pOwner->IsPlayer() )
			{
				continue;
			}

			const int iEnt = pOwner->entindex();
			if ( iEnt < 1 || iEnt > MAX_PLAYERS )
			{
				continue;
			}

			// All NEO weapons share one SOUNDENT_CHANNEL_WEAPON slot per player,
			// so this entry is that player's latest combat sound
			Source &s = s_sources[ iEnt ];
			s.pos = pSound->GetSoundOrigin();
			s.lastHeard = pSound->SoundExpirationTime();
			s.team = pOwner->GetTeamNumber();
			s.volume = pSound->Volume();
		}
	}

	void Reset()
	{
		for ( Source &s : s_sources )
		{
			s = Source();
		}
	}

	bool FindNearestFight( const Vector &vEar, int iMyTeam, int iMyEnt, Vector &vFight )
	{
		const float flNow = gpGlobals->curtime;
		const bool bTeamplay = NEORules() && NEORules()->IsTeamplay();
		const float flMemoryWindow = sv_neo_bot_memory_sound_combat_time_window.GetFloat();
		const float flClusterRadius = sv_neo_bot_memory_sound_combat_cluster_radius.GetFloat();

		// Gather the sources this listener would have heard, with a linear recency weight:
		// full while the sound is still live, fading to nothing after it stops.
		Vector pos[ MAX_PLAYERS + 1 ];
		float weight[ MAX_PLAYERS + 1 ];
		int nHeard = 0;
		int iNearest = -1;
		float flNearestSqr = FLT_MAX;

		for ( int i = 1; i <= MAX_PLAYERS; ++i )
		{
			const Source &s = s_sources[ i ];
			const float flAge = flNow - s.lastHeard;
			if ( flAge >= flMemoryWindow || i == iMyEnt )
			{
				continue;
			}
			if ( bTeamplay && s.team == iMyTeam )
			{
				continue;
			}
			const float flDistSqr = vEar.DistToSqr( s.pos );
			if ( flDistSqr > (float)s.volume * (float)s.volume )
			{
				continue;
			}

			// The nearest fight is the most urgent threat and the quickest to reach
			if ( flDistSqr < flNearestSqr )
			{
				flNearestSqr = flDistSqr;
				iNearest = nHeard;
			}

			pos[ nHeard ] = s.pos;
			weight[ nHeard ] = ( flAge > 0.0f ) ? ( 1.0f - flAge / flMemoryWindow ) : 1.0f;
			++nHeard;
		}

		if ( iNearest < 0 )
		{
			return false;
		}

		// Aim for the recency-weighted centroid of the sources fighting around the nearest one
		const float flClusterSqr = flClusterRadius * flClusterRadius;
		Vector vSum( 0, 0, 0 );
		float flWsum = 0.0f;
		for ( int j = 0; j < nHeard; ++j )
		{
			if ( pos[ iNearest ].DistToSqr( pos[ j ] ) <= flClusterSqr )
			{
				vSum += pos[ j ] * weight[ j ];
				flWsum += weight[ j ];
			}
		}

		vFight = vSum / flWsum;
		return true;
	}
}
