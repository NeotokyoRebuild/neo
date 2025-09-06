#pragma once

#include <const.h>
#include <utlvector.h>

#include "weapon_bits.h"
#include "neo_enums.h"

enum BotDifficultyFlag_
{
	BOT_DIFFICULTY_FLAG_NONE = 0, // Basically equivalent to all difficulty
	BOT_DIFFICULTY_FLAG_EASY = 1 << 0,
	BOT_DIFFICULTY_FLAG_NORMAL = 1 << 1,
	BOT_DIFFICULTY_FLAG_HARD = 1 << 2,
	BOT_DIFFICULTY_FLAG_EXPERT = 1 << 3,

	BOT_DIFFICULTY_FLAG_ALL = BOT_DIFFICULTY_FLAG_EASY | BOT_DIFFICULTY_FLAG_NORMAL | BOT_DIFFICULTY_FLAG_HARD | BOT_DIFFICULTY_FLAG_EXPERT,
};
typedef int BotDifficultyFlag;

enum BotClassFlag_
{
	BOT_CLASS_FLAG_NONE = 0, // Equivalent to being available for all class
	BOT_CLASS_FLAG_RECON = 1 << NEO_CLASS_RECON,
	BOT_CLASS_FLAG_ASSAULT = 1 << NEO_CLASS_ASSAULT,
	BOT_CLASS_FLAG_SUPPORT = 1 << NEO_CLASS_SUPPORT,
	// But not VIP as that's auto-selected in VIP mode

	BOT_CLASS_FLAG_ALL = BOT_CLASS_FLAG_RECON | BOT_CLASS_FLAG_ASSAULT | BOT_CLASS_FLAG_SUPPORT,
};
typedef int BotClassFlag;

enum BotTemplateApplied_
{
	BOT_TEMPLATE_APPLIED_NONE 						= 0,
	BOT_TEMPLATE_APPLIED_WEP_PREF_RECON_PRIVATE 	= 1 << 0,
	BOT_TEMPLATE_APPLIED_WEP_PREF_RECON_CORPORAL	= 1 << 1,
	BOT_TEMPLATE_APPLIED_WEP_PREF_RECON_SERGEANT	= 1 << 2,
	BOT_TEMPLATE_APPLIED_WEP_PREF_RECON_LIEUTENANT	= 1 << 3,
	BOT_TEMPLATE_APPLIED_WEP_PREF_ASSAULT_PRIVATE	= 1 << 4,
	BOT_TEMPLATE_APPLIED_WEP_PREF_ASSAULT_CORPORAL	= 1 << 5,
	BOT_TEMPLATE_APPLIED_WEP_PREF_ASSAULT_SERGEANT	= 1 << 6,
	BOT_TEMPLATE_APPLIED_WEP_PREF_ASSAULT_LIEUTENANT= 1 << 7,
	BOT_TEMPLATE_APPLIED_WEP_PREF_SUPPORT_PRIVATE	= 1 << 8,
	BOT_TEMPLATE_APPLIED_WEP_PREF_SUPPORT_CORPORAL	= 1 << 9,
	BOT_TEMPLATE_APPLIED_WEP_PREF_SUPPORT_SERGEANT	= 1 << 10,
	BOT_TEMPLATE_APPLIED_WEP_PREF_SUPPORT_LIEUTENANT= 1 << 11,
	BOT_TEMPLATE_APPLIED_WEP_PREF_VIP_PRIVATE		= 1 << 12,
	BOT_TEMPLATE_APPLIED_WEP_PREF_VIP_CORPORAL		= 1 << 13,
	BOT_TEMPLATE_APPLIED_WEP_PREF_VIP_SERGEANT		= 1 << 14,
	BOT_TEMPLATE_APPLIED_WEP_PREF_VIP_LIEUTENANT	= 1 << 15,
	BOT_TEMPLATE_APPLIED_DIFFICULTY 				= 1 << 16,
	BOT_TEMPLATE_APPLIED_DIFFICULTY_SELECT 			= 1 << 17,
	BOT_TEMPLATE_APPLIED_CLASS 						= 1 << 18,
};
typedef int BotTemplateApplied;

// define rather than constexpr due to how it's being used
#define BOT_DEFAULT_NAME "NT;RE bot"

struct CNEOBotProfile
{
	char szName[MAX_PLAYER_NAME_LENGTH] = BOT_DEFAULT_NAME;
	BotDifficultyFlag flagDifficulty;
	int iDifficultyForced;
	BotClassFlag flagClass;
	NEO_WEP_BITS_UNDERLYING_TYPE flagsWepPrefs[NEO_CLASS__LOADOUTABLE_COUNT][NEO_RANK__TOTAL];
	BotTemplateApplied flagTemplateApplied;
};

extern const CNEOBotProfile FIXED_DEFAULT_PROFILE;

// 0/not set will be treated as ignored, typically equivalent to ALL flags
struct CNEOBotProfileFilter
{
	BotDifficultyFlag flagTargetDifficulty;
	BotClassFlag flagTargetClass;
};

void NEOBotProfileLoad();
bool NEOBotProfilePassFilter(const CNEOBotProfile &profile, const CNEOBotProfileFilter &filter);

struct CNEOBotProfileReturn
{
	const CNEOBotProfile &profile;
	int index;
};
CNEOBotProfileReturn NEOBotProfileNextPick(const CNEOBotProfileFilter &filter);
void NEOBotProfileResetPicks();
void NEOBotProfileFreePickByIdx(const int idx);

// Create full bot name for szBotName, returns actual bot skill (either iRequestedSkill or force skill)
int NEOBotProfileCreateNameRetSkill(char (&szBotName)[MAX_PLAYER_NAME_LENGTH],
		const CNEOBotProfile &profile,
		const int iRequestedSkill,
		const char *pszOverrideName = nullptr);

