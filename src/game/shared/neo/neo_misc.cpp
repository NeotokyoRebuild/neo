#include "neo_misc.h"

#ifdef CLIENT_DLL
#include "steamclientpublic.h"
#include "vgui/ISystem.h"
#include "tier3.h"

extern ConVar sv_neo_comp_name;
#endif

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

#ifdef CLIENT_DLL
void StartAutoClientRecording()
{
	// SteamID
	char steamSection[64];
	const CSteamID steamID = GetSteamIDForPlayerIndex(GetLocalPlayerIndex());
	if (steamID.IsValid())
	{
		auto accountID = steamID.GetAccountID();
		V_snprintf(steamSection, sizeof(steamSection), "%d", accountID);
	}

	// Map name
	char mapSection[256];
	V_strcpy(mapSection, GameRules()->MapName());

	// Time and date
	char timeSection[16];
	int year, month, dayOfWeek, day, hour, minute, second;
	g_pVGuiSystem->GetCurrentTimeAndDate(&year, &month, &dayOfWeek, &day, &hour, &minute, &second);
	V_snprintf(timeSection, sizeof(timeSection), "%04d%02d%02d-%02d%02d", year, month, day, hour, minute);

	// Competition name
	char compSection[32];
	V_strcpy(compSection, sv_neo_comp_name.GetString());

	// Build the filename
	char replayName[sizeof(steamSection) + sizeof(mapSection) + sizeof(timeSection) + sizeof(compSection) + 2];

	if (compSection[0] != '\0')
	{
		V_snprintf(replayName, sizeof(replayName), "%s_%s_%s_%s", compSection, timeSection, mapSection, steamSection);
	}
	else
	{
		V_snprintf(replayName, sizeof(replayName), "%s_%s_%s", timeSection, mapSection, steamSection);
	}

	engine->StopDemoRecording(); // Stop any previous recording

	engine->StartDemoRecording(replayName); // Start recording
}
#endif