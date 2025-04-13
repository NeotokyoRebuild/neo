#pragma once

static constexpr const int WEP_NAME_MAXSTRLEN = 32;

struct CNEOKillerInfo
{
	int iEntIndex;
	int iClass;
	int iHP;
	float flDistance;
	wchar_t wszKilledWith[WEP_NAME_MAXSTRLEN];
};

struct CNEOKillerDamageInfos
{
	bool bHasDmgInfos = false;
	CNEOKillerInfo killerInfo;
	wchar_t wszKillerName[MAX_PLAYER_NAME_LENGTH + 1];
};

// Global instance of CNEOKillerDamageInfos
inline CNEOKillerDamageInfos g_neoKDmgInfos;
