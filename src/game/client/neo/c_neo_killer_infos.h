#pragma once

static constexpr const int WEP_NAME_MAXSTRLEN = 32;

struct CNEOKillerInfos
{
	bool bHasDmgInfos = false;
	wchar_t wszKillerName[MAX_PLAYER_NAME_LENGTH + 1];
	int iEntIndex;
	int iClass;
	int iHP;
	float flDistance;
	wchar_t wszKilledWith[WEP_NAME_MAXSTRLEN];
};

// Global instance of CNEOKillerInfos
inline CNEOKillerInfos g_neoKillerInfos;
