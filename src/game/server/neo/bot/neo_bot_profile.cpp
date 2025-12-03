#include "neo_bot_profile.h"

#include <filesystem.h>
#include <KeyValues.h>
#include <utlhashtable.h>
#include <convar.h>

#include "neo_misc.h"
#include "neo_bot.h"
#include "neo_weapon_loadout.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define LOG_DBG_LEVEL 0 // NEO NOTE (nullsystem): Must turn back to 0 when submitting PR
#ifndef DEBUG
static_assert(LOG_DBG_LEVEL == 0);
#endif

#define PROFILE_WARN_PREFIX "BOT profiles validation WARNING: "
#define MAIN_PROFILE_FNAME "ntre_bot_profiles.txt"

static inline CUtlVector<CNEOBotProfile> g_botProfiles;
static inline CUtlVector<bool> g_vbBotUsed;
static inline CThreadMutex g_vbBotUsedMutex;

ConVar neo_bot_prefix_name_with_difficulty("neo_bot_prefix_name_with_difficulty", "0", FCVAR_GAMEDLL, "Append the skill level of the bot to the bot's name", true, 0.0f, true, 1.0f);

inline const CNEOBotProfile FIXED_DEFAULT_PROFILE = {
	// GCC 10 (Steam RT 3.0) designated initalizer bug: Cannot place = string here, fixed in 11+
#ifdef WIN32
	.szName = BOT_DEFAULT_NAME,
#endif
	.flagDifficulty = BOT_DIFFICULTY_FLAG_ALL,
	.iDifficultyForced = -1,
	.flagClass = BOT_CLASS_FLAG_ALL,
	.flagsWepPrefs = {
		{
			// Recon
			NEO_WEP_SRM, 							// Private
			NEO_WEP_JITTE_S, 						// Corporal
			NEO_WEP_ZR68_C, 						// Sergeant
			NEO_WEP_SUPA7, 							// Lieutenant
		},
		{
			// Assault
			NEO_WEP_ZR68_S, 						// Private
			NEO_WEP_ZR68_S | NEO_WEP_M41_S, 		// Corporal
			NEO_WEP_MX, 							// Sergeant
			NEO_WEP_AA13 | NEO_WEP_SRS, 			// Lieutenant
		},
		{
			// Support
			NEO_WEP_ZR68_C | NEO_WEP_SUPA7, 		// Private
			NEO_WEP_MX, 							// Corporal
			NEO_WEP_MX | NEO_WEP_MX_S, 				// Sergeant
			NEO_WEP_MX | NEO_WEP_MX_S | NEO_WEP_PZ, // Lieutenant
		},
		{
			// VIP
			NEO_WEP_SMAC,							// Private
			NEO_WEP_SRM, 							// Corporal
			NEO_WEP_SRM | NEO_WEP_JITTE, 			// Sergeant
			NEO_WEP_ZR68_C | NEO_WEP_SUPA7, 		// Lieutenant
		},
	},
};

template <typename FLAG>
struct FlagCmp
{
	FLAG flag;
	const char *szValue;
};

static const FlagCmp<BotDifficultyFlag> BOT_DIFFICULTY_FLAG_CMP[] = {
	{ BOT_DIFFICULTY_FLAG_EASY, "easy" },
	{ BOT_DIFFICULTY_FLAG_NORMAL, "normal" },
	{ BOT_DIFFICULTY_FLAG_HARD, "hard" },
	{ BOT_DIFFICULTY_FLAG_EXPERT, "expert" },
};

static const FlagCmp<BotClassFlag> BOT_CLASS_FLAG_CMP[] = {
	{ BOT_CLASS_FLAG_RECON, "recon" },
	{ BOT_CLASS_FLAG_ASSAULT, "assault" },
	{ BOT_CLASS_FLAG_SUPPORT, "support" },
};

#define FLAGCMP_WEP_DEF(M_BITWNAME, M_NAME, M_DISPLAY_NAME, ...) { NEO_WEP_##M_BITWNAME, #M_NAME },
#define FLAGCMP_WEP_DEF_ALT(M_BITWNAME, M_NAME, M_DISPLAY_NAME, M_NAMEWEP, ...) { NEO_WEP_##M_BITWNAME, M_NAMEWEP },

// Start from the first weapon not empty as that's 1 << 0 in bitwise
static const FlagCmp<NEO_WEP_BITS_UNDERLYING_TYPE> WEP_BIT_FLAG_CMP[] = {
FOR_LIST_WEAPONS(FLAGCMP_WEP_DEF, FLAGCMP_WEP_DEF_ALT)
#ifdef INCLUDE_WEP_PBK
FLAGCMP_WEP_DEF(PBK56S, pbk56s, "PBK56")
#endif
};

template <typename FLAG, size_t MAX_LEN_IN_FLAGS_CMP>
FLAG FlagsFromStr(
		const FLAG flagDefault,
		const FlagCmp<FLAG> (&flagsCmp)[MAX_LEN_IN_FLAGS_CMP],
		const char *szValue,
		BotTemplateApplied *appliedFlags,
		const BotTemplateApplied_ hasAppliedFlag)
{
	FLAG flags = 0;

	if (szValue && szValue[0] != '\0')
	{
		// Include the null ending in loop
		int iStartIdx = 0;
		const int iSzValueSize = V_strlen(szValue);
		for (int iChP = 0; iChP <= iSzValueSize; ++iChP)
		{
			const int iTextSize = iChP - iStartIdx;
			if ((szValue[iChP] == ' ' || szValue[iChP] == '\0') && (iTextSize > 0))
			{
				bool bApplied = false;
				for (const auto &flagCmp : flagsCmp)
				{
					// Have to do length check also as we're trying to match
					// within a non-mutated substring rather than just simply a string match
					if ((V_strlen(flagCmp.szValue) == iTextSize) &&
							V_strnicmp(szValue + iStartIdx, flagCmp.szValue, iTextSize) == 0)
					{
						flags |= flagCmp.flag;
						bApplied = true;
#if LOG_DBG_LEVEL >= 2
						Msg("    FlagsFromStr reading: %.*s (%d)\n", iTextSize, szValue + iStartIdx, flagCmp.flag);
#endif
						break;
					}
				}

				if (!bApplied)
				{
					Warning(PROFILE_WARN_PREFIX "'%.*s' is not a valid flag set.\n", iTextSize, szValue + iStartIdx);
				}

				iStartIdx = iChP + 1;
			}
		}
	}

#if LOG_DBG_LEVEL >= 2
	Msg("  FlagsFromStr result: %d\n", flags);
#endif

	if (flags == 0)
	{
		*appliedFlags = *appliedFlags & ~hasAppliedFlag;
		return flagDefault;
	}

	*appliedFlags = *appliedFlags | hasAppliedFlag;
	return flags;
}

static void SetProfileTempBotCommon(CNEOBotProfile *pProfile, KeyValues *kv)
{
	static const constexpr char *SZ_CLASSES[NEO_CLASS__LOADOUTABLE_COUNT] = {
		"recon", "assault", "support", "vip"
	};
	static const constexpr char *SZ_RANKS[NEO_RANK__TOTAL] = {
		"private", "corporal", "sergeant", "lieutenant"
	};

	if (KeyValues *wepKv = kv->FindKey("weapons"))
	{
		static_assert(BOT_TEMPLATE_APPLIED_WEP_PREF_RECON_PRIVATE ==
				(1 << 0));
		static_assert(BOT_TEMPLATE_APPLIED_WEP_PREF_VIP_LIEUTENANT ==
				(1 << ((NEO_CLASS_LOADOUTABLE_COUNT * NEO_RANK_TOTAL) - 1)));

		int iBitWise = 0;

		for (int idxClass = 0; idxClass < NEO_CLASS__LOADOUTABLE_COUNT; ++idxClass)
		{
			KeyValues *wepClassKv = wepKv->FindKey(SZ_CLASSES[idxClass]);
			if (!wepClassKv)
			{
				iBitWise += NEO_RANK__TOTAL;
				continue;
			}

			for (int idxRank = 0; idxRank < NEO_RANK__TOTAL; ++idxRank)
			{
				const auto flagsWepPrefCur = FlagsFromStr(pProfile->flagsWepPrefs[idxClass][idxRank],
						WEP_BIT_FLAG_CMP, wepClassKv->GetString(SZ_RANKS[idxRank]),
						&pProfile->flagTemplateApplied, static_cast<BotTemplateApplied_>(1 << iBitWise));
				pProfile->flagsWepPrefs[idxClass][idxRank] = flagsWepPrefCur;

				// NEO NOTE (nullsystem): Validate against the loadout for the given class and rank
				{
					NEO_WEP_BITS_UNDERLYING_TYPE flagsValids = 0;
					for (int idxLoadout = 0; idxLoadout < MAX_WEAPON_LOADOUTS; ++idxLoadout)
					{
						const int iWepPrice = CNEOWeaponLoadout::s_LoadoutWeapons[idxClass][idxLoadout].m_iWeaponPrice;
						const int iWepMinRank = (GetRank(iWepPrice) - 1);
						if (iWepPrice == XP_ANY || iWepMinRank <= idxRank)
						{
							flagsValids |= CNEOWeaponLoadout::s_LoadoutWeapons[idxClass][idxLoadout].info.m_iWepBit;
						}
					}
					const NEO_WEP_BITS_UNDERLYING_TYPE flagsInvalids = (flagsWepPrefCur & ~flagsValids);
					if (flagsInvalids != 0)
					{
						char szInvalids[512] = {};
						for (int idxWep = 1; idxWep < NEO_WIDX__TOTAL; ++idxWep)
						{
							if (flagsInvalids & ((decltype(flagsInvalids))1 << (idxWep - 1)))
							{
								V_strcat_safe(szInvalids, " ");
								V_strcat_safe(szInvalids, WEP_BIT_FLAG_CMP[idxWep - 1].szValue);
							}
						}
						Warning(PROFILE_WARN_PREFIX "Invalid weapons for loadout of class %s rank %s:%s\n", SZ_CLASSES[idxClass], SZ_RANKS[idxRank], szInvalids);
					}
				}

				++iBitWise;
			}
		}
	}

	pProfile->flagDifficulty = FlagsFromStr(pProfile->flagDifficulty,
			BOT_DIFFICULTY_FLAG_CMP, kv->GetString("difficulty"),
			&pProfile->flagTemplateApplied, BOT_TEMPLATE_APPLIED_DIFFICULTY);

	const char *szDifficultySelect = kv->GetString("difficulty_select");
	if (szDifficultySelect && szDifficultySelect[0] != '\0')
	{
		// NEO NOTE (nullsystem): Probably good to base on this when we
		// need arg-based setting beyond difficulty_select. Probably more
		// like array where first args are index and points to functions
		// with an expected amount of args.
		enum EIdxArg
		{
			IDXARG_MATCH = 0,
			IDXARG_FORCE,
			IDXARG__TOTAL,
		};
		static constexpr const char *FIRST_ARGS[IDXARG__TOTAL] = {"match", "force"};
		static constexpr const int EARLY_BREAK_ARGPOS = 2; // Don't need to check more than 2 args
		int iSetIdxArgs = -1;
		int idxChosenDiff = -1;

		int iStartIdx = 0;
		int iArgPos = 0;
		const int iSzDiffSelectSize = V_strlen(szDifficultySelect);
		for (int iChP = 0; iChP <= iSzDiffSelectSize; ++iChP)
		{
			const int iTextSize = iChP - iStartIdx;
			if ((szDifficultySelect[iChP] == ' ' || szDifficultySelect[iChP] == '\0') && (iTextSize > 0))
			{
				if (iArgPos == 0)
				{
					// Check first argument
					for (int idxArg = 0; idxArg < IDXARG__TOTAL; ++idxArg)
					{
						if ((V_strlen(FIRST_ARGS[idxArg]) == iTextSize) &&
								(V_strnicmp(szDifficultySelect + iStartIdx, FIRST_ARGS[idxArg], iTextSize) == 0))
						{
							iSetIdxArgs = idxArg;
							break;
						}
					}
				}
				else
				{
					// There's no other first args that deals with other arguments apart from "force"
					if (iSetIdxArgs == IDXARG_FORCE && iArgPos == 1)
					{
						for (int idxDiff = 0; idxDiff < CNEOBot::NUM_DIFFICULTY_LEVELS; ++idxDiff)
						{
							if ((V_strlen(BOT_DIFFICULTY_FLAG_CMP[idxDiff].szValue) == iTextSize) &&
									V_strnicmp(szDifficultySelect + iStartIdx, BOT_DIFFICULTY_FLAG_CMP[idxDiff].szValue, iTextSize) == 0)
							{
								idxChosenDiff = idxDiff;
								break;
							}
						}
						if (idxChosenDiff < 0)
						{
							Warning(PROFILE_WARN_PREFIX "Cannot parse %.*s for difficulty_select\n",
									iTextSize, szDifficultySelect + iStartIdx);
						}
#if LOG_DBG_LEVEL >= 1
						else
						{
							Msg("BOTS: Set forced difficulty setting %.*s\n",
									iTextSize, szDifficultySelect + iStartIdx);
						}
#endif
					}
				}
				iStartIdx = iChP + 1;
				++iArgPos;

				if (iArgPos >= EARLY_BREAK_ARGPOS)
				{
					break;
				}
			}
		}

		switch (iSetIdxArgs)
		{
		case IDXARG_MATCH:
			pProfile->iDifficultyForced = -1;
			pProfile->flagTemplateApplied |= BOT_TEMPLATE_APPLIED_DIFFICULTY_SELECT;
			break;
		case IDXARG_FORCE:
			if (IN_BETWEEN_AR(0, idxChosenDiff, CNEOBot::NUM_DIFFICULTY_LEVELS))
			{
				pProfile->iDifficultyForced = idxChosenDiff;
				pProfile->flagTemplateApplied |= BOT_TEMPLATE_APPLIED_DIFFICULTY_SELECT;
			}
			break;
		default:
			Warning(PROFILE_WARN_PREFIX "Cannot parse \"%s\" for difficulty_select\n", szDifficultySelect);
			break;
		}
	}

	pProfile->flagClass = FlagsFromStr(pProfile->flagClass,
			BOT_CLASS_FLAG_CMP, kv->GetString("class"),
			&pProfile->flagTemplateApplied, BOT_TEMPLATE_APPLIED_CLASS);
}

void NEOBotProfileLoad()
{
	g_botProfiles.Purge();
	g_vbBotUsed.Purge();

	CNEOBotProfile defaultProfile;
	V_memcpy(&defaultProfile, &FIXED_DEFAULT_PROFILE, sizeof(CNEOBotProfile));

	KeyValues *kv = new KeyValues("ntre_bot_profiles");
	if (!kv->LoadFromFile(g_pFullFileSystem, "scripts/" MAIN_PROFILE_FNAME))
	{
		Msg("BOTS: ERROR: Did not load scripts/" MAIN_PROFILE_FNAME "\n");
		kv->deleteThis();
		return;
	}

	if (KeyValues *defKv = kv->FindKey("default"))
	{
		SetProfileTempBotCommon(&defaultProfile, defKv);
#if LOG_DBG_LEVEL >= 1
		Msg("BOTS: Added default\n");
#endif
	}

	CUtlHashtable<CUtlString, CNEOBotProfile> templateProfiles;
	if (KeyValues *tmplAllKv = kv->FindKey("templates"))
	{
		for (KeyValues *tmplKv = tmplAllKv->GetFirstTrueSubKey(); tmplKv; tmplKv = tmplKv->GetNextTrueSubKey())
		{
			CNEOBotProfile newTemplateProfile;
			V_memcpy(&newTemplateProfile, &defaultProfile, sizeof(CNEOBotProfile));

			const char *pszTemplateName = tmplKv->GetName();
			SetProfileTempBotCommon(&newTemplateProfile, tmplKv);

			char szKey[256] = {};
			V_strcpy_safe(szKey, pszTemplateName);

			templateProfiles.Insert(szKey, newTemplateProfile);
#if LOG_DBG_LEVEL >= 1
			Msg("BOTS: Added template %s\n", pszTemplateName);
#endif
		}
	}

	if (KeyValues *botsKv = kv->FindKey("bots"))
	{
		for (KeyValues *botKv = botsKv->GetFirstTrueSubKey(); botKv; botKv = botKv->GetNextTrueSubKey())
		{
			const char *pszName = botKv->GetName();
			if (!pszName || pszName[0] == '\0')
			{
				continue;
			}

			CNEOBotProfile newBotProfile;
			V_memcpy(&newBotProfile, &defaultProfile, sizeof(CNEOBotProfile));
			V_strcpy_safe(newBotProfile.szName, pszName);
#if LOG_DBG_LEVEL >= 2
			Msg("BOTS: Starting to add bot %s\n", newBotProfile.szName);
#endif
			if (const char *pszTemplates = botKv->GetString("templates");
					pszTemplates && pszTemplates[0] != '\0' && (templateProfiles.Count() > 0))
			{
				// Include the null ending in loop
				int iStartIdx = 0;
				const int iSzValueSize = V_strlen(pszTemplates);
				for (int iChP = 0; iChP <= iSzValueSize; ++iChP)
				{
					const int iPszTemplateSize = iChP - iStartIdx + 1;
					if ((pszTemplates[iChP] == ' ' || pszTemplates[iChP] == '\0') && (iPszTemplateSize > 0))
					{
						char szKey[256] = {};
						V_strncpy(szKey, pszTemplates + iStartIdx, Min(iPszTemplateSize, 256));
#if LOG_DBG_LEVEL >= 2
						Msg("  BOTS: Checking template %s\n", szKey);
#endif
						const CNEOBotProfile *templateProfile = templateProfiles.GetPtr(szKey);
						if (templateProfile)
						{
							int iBitWise = 0;
							for (int idxClass = 0; idxClass < NEO_CLASS__LOADOUTABLE_COUNT; ++idxClass)
							{
								for (int idxRank = 0; idxRank < NEO_RANK__TOTAL; ++idxRank)
								{
									if (templateProfile->flagTemplateApplied & (1 << iBitWise))
									{
#if LOG_DBG_LEVEL >= 2
										Msg("  BOTS: Template (%s): Applied wep pref %d,%d = %d\n", szKey, idxClass, idxRank, templateProfile->flagsWepPrefs[idxClass][idxRank]);
#endif
										newBotProfile.flagsWepPrefs[idxClass][idxRank] = templateProfile->flagsWepPrefs[idxClass][idxRank];
									}
									++iBitWise;
								}
							}
							if (templateProfile->flagTemplateApplied & BOT_TEMPLATE_APPLIED_DIFFICULTY)
							{
#if LOG_DBG_LEVEL >= 2
								Msg("  BOTS: Template (%s): Applied difficulty %d\n", szKey, templateProfile->flagDifficulty);
#endif
								newBotProfile.flagDifficulty = templateProfile->flagDifficulty;
							}
							if (templateProfile->flagTemplateApplied & BOT_TEMPLATE_APPLIED_DIFFICULTY_SELECT)
							{
#if LOG_DBG_LEVEL >= 2
								Msg("  BOTS: Template (%s): Applied difficulty_select %d\n", szKey, templateProfile->iDifficultyForced);
#endif
								newBotProfile.iDifficultyForced = templateProfile->iDifficultyForced;
							}
							if (templateProfile->flagTemplateApplied & BOT_TEMPLATE_APPLIED_CLASS)
							{
#if LOG_DBG_LEVEL >= 2
								Msg("  BOTS: Template (%s): Applied classes %d\n", szKey, templateProfile->flagClass);
#endif
								newBotProfile.flagClass = templateProfile->flagClass;
							}
						}
						iStartIdx = iChP + 1;
					}
				}
			}
			SetProfileTempBotCommon(&newBotProfile, botKv);

			g_botProfiles.AddToTail(newBotProfile);

#if LOG_DBG_LEVEL >= 1
			Msg("BOTS: Added bot %s\n", newBotProfile.szName);
#endif
		}
	}

	NEOBotProfileResetPicks();

	kv->deleteThis();
}

bool NEOBotProfilePassFilter(const CNEOBotProfile &profile, const CNEOBotProfileFilter &filter)
{
	if (filter.flagTargetDifficulty > 0)
	{
		const bool bPassDifficulty = profile.flagDifficulty & filter.flagTargetDifficulty;
		if (!bPassDifficulty)
		{
			return false;
		}
	}

	if (filter.flagTargetClass > 0)
	{
		const bool bPassClass = profile.flagClass & filter.flagTargetClass;
		if (!bPassClass)
		{
			return false;
		}
	}

	return true;
}

CNEOBotProfileReturn NEOBotProfileNextPick(const CNEOBotProfileFilter &filter)
{
	if (g_botProfiles.Count() == 0)
	{
		return CNEOBotProfileReturn{
			.profile = FIXED_DEFAULT_PROFILE,
			.index = -1,
		};
	}

	// Must not return early before g_vbBotUsedMutex.Unlock()
	g_vbBotUsedMutex.Lock();

	bool bShouldReset = true;
	// It's unlikely for g_vbBotUsed and g_botProfiles to differ in size
	// but just in-case
	if (g_vbBotUsed.Count() == g_botProfiles.Count())
	{
		for (int idxBot = 0; idxBot < g_botProfiles.Count(); ++idxBot)
		{
			if (!g_vbBotUsed[idxBot] && NEOBotProfilePassFilter(g_botProfiles[idxBot], filter))
			{
				bShouldReset = false;
				break;
			}
		}
	}

	if (bShouldReset)
	{
		// Not using NEOBotProfileResetPicks as we're already locked
		g_vbBotUsed.SetCount(g_botProfiles.Count());
		g_vbBotUsed.FillWithValue(false);
	}

	int iVisited = 0;
	int iNum = RandomInt(0, g_botProfiles.Count() - 1);
	while ((g_vbBotUsed[iNum] || !NEOBotProfilePassFilter(g_botProfiles[iNum], filter)) &&
			iVisited < g_botProfiles.Count())
	{
		iNum = LoopAroundInArray(iNum + 1, g_botProfiles.Count());
		++iVisited;
	}

	const CNEOBotProfile *retProfile = &FIXED_DEFAULT_PROFILE;
	if (iVisited < g_botProfiles.Count())
	{
		g_vbBotUsed[iNum] = true;
		retProfile = &g_botProfiles[iNum];
	}
	g_vbBotUsedMutex.Unlock();
	return CNEOBotProfileReturn{
		.profile = *retProfile,
		.index = iNum,
	};
}

void NEOBotProfileResetPicks()
{
	g_vbBotUsedMutex.Lock();
	g_vbBotUsed.SetCount(g_botProfiles.Count());
	g_vbBotUsed.FillWithValue(false);
	g_vbBotUsedMutex.Unlock();
}

void NEOBotProfileFreePickByIdx(const int idx)
{
	if (IN_BETWEEN_AR(0, idx, g_vbBotUsed.Count()))
	{
		g_vbBotUsedMutex.Lock();
		g_vbBotUsed[idx] = false;
		g_vbBotUsedMutex.Unlock();
	}
}

int NEOBotProfileCreateNameRetSkill(char (&szBotName)[MAX_PLAYER_NAME_LENGTH],
		const CNEOBotProfile &profile,
		const int iRequestedSkill,
		const char *pszOverrideName)
{
	int iCurBotSkill = iRequestedSkill;
	if (IN_BETWEEN_EQ(CNEOBot::EASY, profile.iDifficultyForced, CNEOBot::EXPERT))
	{
		iCurBotSkill = profile.iDifficultyForced;
	}

	const char *pszDifficultyStr = "";
	if (neo_bot_prefix_name_with_difficulty.GetBool())
	{
		if (IN_BETWEEN_EQ(CNEOBot::EASY, iCurBotSkill, CNEOBot::EXPERT))
		{
			static constexpr const char *DIFF_NAME_STRS[] = { "Easy ", "Normal ", "Hard ", "Expert " };
			pszDifficultyStr = DIFF_NAME_STRS[iCurBotSkill];
		}
		else
		{
			pszDifficultyStr = "Undefined ";
		}
	}

	V_sprintf_safe(szBotName, "%sBOT %s", pszDifficultyStr, (pszOverrideName) ? pszOverrideName : profile.szName);
	return iCurBotSkill;
}

