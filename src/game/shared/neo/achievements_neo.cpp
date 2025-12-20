#include "cbase.h"
#ifdef CLIENT_DLL
#include "achievements_neo.h"

CAchievementMgr g_AchievementMgrNEO;	// global achievement mgr for NEO

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
#endif
