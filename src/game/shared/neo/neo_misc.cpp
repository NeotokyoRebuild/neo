#include "neo_misc.h"

#ifdef CLIENT_DLL
#include "steamclientpublic.h"
#endif // CLIENT_DLL
#include "cbase.h"
#include "neo_gamerules.h"
#include "vgui/ISystem.h"
#include "tier3.h"
#include <filesystem.h>

extern ConVar sv_neo_comp_name;

#ifdef CLIENT_DLL
#define DEMOS_DIRECTORY_NAME "demos"
#else
#define DEMOS_DIRECTORY_NAME "serverdemos"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

[[nodiscard]] bool InRect(const vgui::IntRect &rect, const int x, const int y)
{
	return IN_BETWEEN_EQ(rect.x0, x, rect.x1) && IN_BETWEEN_EQ(rect.y0, y, rect.y1);
}

[[nodiscard]] int LoopAroundMinMax(const int iValue, const int iMin, const int iMax)
{
	if (iValue < iMin)
	{
		return iMax;
	}
	else if (iValue > iMax)
	{
		return iMin;
	}
	return iValue;
}

[[nodiscard]] int LoopAroundInArray(const int iValue, const int iSize)
{
	return LoopAroundMinMax(iValue, 0, iSize - 1);
}

bool StartAutoRecording()
{
#ifdef GAME_DLL
	CBasePlayer* sourceTV = nullptr;
	for (int i = 0; i < gpGlobals->maxClients; i++)
	{
		CBasePlayer* pPlayer = UTIL_PlayerByIndex(i);
		if (pPlayer && pPlayer->IsHLTV())
		{
			sourceTV = pPlayer;
			break;
		}
	}
	if (!sourceTV)
	{
		ConMsg("SourceTV client not found, server side demo not recording.\n");
		return false;
	}
#endif // GAME_DLL

#ifdef CLIENT_DLL
	// SteamID
	char steamSection[64];
	const CSteamID steamID = GetSteamIDForPlayerIndex(GetLocalPlayerIndex());
	if (steamID.IsValid())
	{
		auto accountID = steamID.GetAccountID();
		V_snprintf(steamSection, sizeof(steamSection), "%d", accountID);
	}
	else
	{
		V_strncpy(steamSection, "nosteamid", sizeof(steamSection));
	}
#endif // CLIENT_DLL

	// Map name
	char mapSection[256];
#ifdef CLIENT_DLL
	V_strncpy(mapSection, GameRules()->MapName(), sizeof(mapSection));
#else
	V_strncpy(mapSection, gpGlobals->mapname.ToCStr(), sizeof(mapSection));
#endif // CLIENT_DLL

	// Time and date
	char timeSection[16];
	int year, month, dayOfWeek, day, hour, minute, second;
	g_pVGuiSystem->GetCurrentTimeAndDate(&year, &month, &dayOfWeek, &day, &hour, &minute, &second);
	V_snprintf(timeSection, sizeof(timeSection), "%04d%02d%02d-%02d%02d", year, month, day, hour, minute);

	// Competition name
	char compSection[32];
	V_strncpy(compSection, sv_neo_comp_name.GetString(), sizeof(compSection));

	// Build the filename
	char replayName[MAX_PATH];

	if (compSection[0] != '\0')
	{
#ifdef CLIENT_DLL
		V_snprintf(replayName, sizeof(replayName), "%s_%s_%s_%s", compSection, timeSection, mapSection, steamSection);
#else
		V_snprintf(replayName, sizeof(replayName), "%s_%s_%s", compSection, timeSection, mapSection);
#endif // CLIENT_DLL
	}
	else
	{
#ifdef CLIENT_DLL
		V_snprintf(replayName, sizeof(replayName), "%s_%s_%s", timeSection, mapSection, steamSection);
#else
		V_snprintf(replayName, sizeof(replayName), "%s_%s", timeSection, mapSection);
#endif // CLIENT_DLL
	}

	if (!g_pFullFileSystem->IsDirectory(DEMOS_DIRECTORY_NAME))
	{
		g_pFullFileSystem->CreateDirHierarchy(DEMOS_DIRECTORY_NAME);
	}

#ifdef CLIENT_DLL
	engine->StartDemoRecording(replayName, DEMOS_DIRECTORY_NAME); // Start recording
	return engine->IsRecordingDemo();
#else
	engine->ServerCommand(UTIL_VarArgs("tv_stoprecord;tv_record \"%s/%s\";\n", DEMOS_DIRECTORY_NAME, replayName));
	return true; // hopefully we're recording
#endif // CLIENT_DLL
}