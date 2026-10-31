#include "cbase.h"
#ifdef CLIENT_DLL
#include "achievements_neo.h"
#include "c_playerresource.h"
#include "neo_gamerules.h"

CAchievementMgr g_AchievementMgrNEO( CAchievementMgr::SteamCloudPersist_On );	// global achievement mgr for NEO

/*
	Tutorial Achievements
*/

class CAchievementNEO_TutorialComplete : public CBaseAchievement
{
	void Init()
	{
		static const char *szComponents[] =
		{
			"NEO_TUTORIAL_RECON", "NEO_TUTORIAL_ASSAULT", "NEO_TUTORIAL_SUPPORT",
		};
		m_pszComponentNames = szComponents;
		m_iNumComponents = ARRAYSIZE( szComponents );
		SetComponentPrefix( "NEO_TUTORIAL" );
		SetFlags( ACH_SAVE_GLOBAL | ACH_HAS_COMPONENTS | ACH_LISTEN_COMPONENT_EVENTS );
		SetGoal( m_iNumComponents );
		SetMapNameFilter( "ntre_class_tut" );
		SetStoreProgressInSteam( true );
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_TutorialComplete, ACHIEVEMENT_NEO_TUTORIAL_COMPLETE, "NEO_TUTORIAL_COMPLETE" );

class CAchievementNEO_Trial50Seconds : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "ntre_shooting_tut" );
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_Trial50Seconds, ACHIEVEMENT_NEO_TRIAL_50_SECONDS, "NEO_TRIAL_50_SECONDS" );

class CAchievementNEO_Trial40Seconds : public CBaseAchievement
{
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
		SetMapNameFilter( "ntre_shooting_tut" );
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_Trial40Seconds, ACHIEVEMENT_NEO_TRIAL_40_SECONDS, "NEO_TRIAL_40_SECONDS" );

/*
	Manufacturer Achievements
*/

static constexpr int NEO_MANUFACTURER_KILLS_GOAL = 100;

class CAchievementNEO_ManufacturerKills : public CBaseAchievement
{
protected:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( NEO_MANUFACTURER_KILLS_GOAL );
		SetStoreProgressInSteam( true );
	}

	void ListenForEvents()
	{
		ListenForGameEvent( "player_death" );
	}

	void FireGameEvent_Internal( IGameEvent *event )
	{
		if ( event->GetBool( "suicide" ) ) return;

		C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
		if ( !pLocalPlayer ) return;

		const int iAttacker = engine->GetPlayerForUserID( event->GetInt( "attacker" ) );
		const int iVictim = engine->GetPlayerForUserID( event->GetInt( "userid" ) );
		if ( iAttacker != pLocalPlayer->entindex() || iVictim == iAttacker ) return;

		if ( NEORules()->IsTeamplay() && ( !g_PR || g_PR->GetTeam( iVictim ) == pLocalPlayer->GetTeamNumber() ) ) return;

		const char *pszWeapon = event->GetString( "weapon" );
		for ( int i = 0; i < m_iNumWeapons; i++ )
		{
			if ( !V_stricmp( pszWeapon, m_ppszWeapons[i] ) )
			{
				IncrementCount();
				return;
			}
		}
	}

	const char **m_ppszWeapons = nullptr;
	int m_iNumWeapons = 0;
};

class CAchievementNEO_ManufacturerKills_BurakuNigai : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"m41", "m41l", "m41s",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_BurakuNigai, ACHIEVEMENT_NEO_MANU_KILLS_BURAKU_NIGAI, "NEO_MANU_KILLS_BURAKU_NIGAI" );

class CAchievementNEO_ManufacturerKills_Bursebo : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"aa13", "balc",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_Bursebo, ACHIEVEMENT_NEO_MANU_KILLS_BURSEBO, "NEO_MANU_KILLS_BURSEBO" );

class CAchievementNEO_ManufacturerKills_HRV : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"zr68c", "zr68s", "zr68l",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_HRV, ACHIEVEMENT_NEO_MANU_KILLS_HRV, "NEO_MANU_KILLS_HRV" );

class CAchievementNEO_ManufacturerKills_JHIndustries : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"mx", "mx_silenced", "tachi",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_JHIndustries, ACHIEVEMENT_NEO_MANU_KILLS_JH_INDUSTRIES, "NEO_MANU_KILLS_JH_INDUSTRIES" );

class CAchievementNEO_ManufacturerKills_Murata : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"supa7",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_Murata, ACHIEVEMENT_NEO_MANU_KILLS_MURATA, "NEO_MANU_KILLS_MURATA" );

class CAchievementNEO_ManufacturerKills_Sanjuro : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"mpn_unsilenced", "mpn", "srm", "srm_s", "jitte", "jittescoped", "pz", "kyla",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_Sanjuro, ACHIEVEMENT_NEO_MANU_KILLS_SANJURO, "NEO_MANU_KILLS_SANJURO" );

class CAchievementNEO_ManufacturerKills_SRS : public CAchievementNEO_ManufacturerKills
{
	void Init()
	{
		static const char *pszWeapons[] =
		{
			"srs",
		};
		m_ppszWeapons = pszWeapons;
		m_iNumWeapons = ARRAYSIZE( pszWeapons );
		CAchievementNEO_ManufacturerKills::Init();
	}
};
DECLARE_NEO_ACHIEVEMENT( CAchievementNEO_ManufacturerKills_SRS, ACHIEVEMENT_NEO_MANU_KILLS_SRS, "NEO_MANU_KILLS_SRS" );
#endif
