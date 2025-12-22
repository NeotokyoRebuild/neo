#pragma once
#include "achievementmgr.h"
#include "neo_enums.h"

#ifdef CLIENT_DLL
extern CAchievementMgr g_AchievementMgrNEO;
#endif

class CAchievementNEOBasic : public CBaseAchievement
{
public:
	void Init()
	{
		SetFlags( ACH_SAVE_GLOBAL );
		SetGoal( 1 );
	}
};

// Basic achievement

#define DECLARE_BASIC_NEO_ACHIEVEMENT_( achievementID, achievementName, bHidden ) \
class CAchievementNEO_##achievementID : public CAchievementNEOBasic {};		\
DECLARE_ACHIEVEMENT_( CAchievementNEO_##achievementID, achievementID, achievementName, NULL, 5, bHidden )	\

#define DECLARE_BASIC_NEO_ACHIEVEMENT( achievementID, achievementName )	\
	DECLARE_BASIC_NEO_ACHIEVEMENT_( achievementID, achievementName, false )

#define DECLARE_BASIC_NEO_ACHIEVEMENT_HIDDEN( achievementID, achievementName )	\
	DECLARE_BASIC_NEO_ACHIEVEMENT_( achievementID, achievementName, true )

// Make your own class, listen to game events, be incremental or map specific

#define DECLARE_NEO_ACHIEVEMENT_( className, achievementID, achievementName, bHidden ) \
	DECLARE_ACHIEVEMENT_( className, achievementID, achievementName, NULL, 5, bHidden )

#define DECLARE_NEO_ACHIEVEMENT( className, achievementID, achievementName )	\
	DECLARE_NEO_ACHIEVEMENT_( className, achievementID, achievementName, false )

#define DECLARE_NEO_ACHIEVEMENT_HIDDEN( className, achievementID, achievementName )	\
	DECLARE_NEO_ACHIEVEMENT_( className, achievementID, achievementName, true )