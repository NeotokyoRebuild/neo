#pragma once
#include "achievementmgr.h"

extern CAchievementMgr g_AchievementMgrNEO;

// Achievements
enum
{
	ACHIEVEMENT_NEO_TUTORIAL_COMPLETE = 0,
	ACHIEVEMENT_NEO_TRIAL_50_SECONDS,
	ACHIEVEMENT_NEO_TRIAL_40_SECONDS,
};

class CAchievementNEOBasic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};

#define DECLARE_NEO_ACHIEVEMENT_( achievementID, achievementName, gameDirFilter, iPointValue, bHidden ) \
class CAchievementNEO_##achievementID : public CAchievementNEOBasic {};		\
DECLARE_ACHIEVEMENT_( CAchievementNEO_##achievementID, achievementID, achievementName, gameDirFilter, iPointValue, bHidden )	\

#define DECLARE_NEO_ACHIEVEMENT( achievementID, achievementName, iPointValue )	\
	DECLARE_NEO_ACHIEVEMENT_( achievementID, achievementName, NULL, iPointValue, false )

#define DECLARE_NEO_ACHIEVEMENT_HIDDEN( achievementID, achievementName, iPointValue )	\
	DECLARE_NEO_ACHIEVEMENT_( achievementID, achievementName, NULL, iPointValue, true )