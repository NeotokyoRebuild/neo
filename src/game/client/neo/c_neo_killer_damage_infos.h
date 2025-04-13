#pragma once

struct CNEOKillerInfo
{
	int iEntIndex;
	int iClass;
	int iHP;
	float flDistance;
};

struct CNEOKillerDamageInfos
{
	bool bHasDmgInfos = false;
	CNEOKillerInfo killerInfo;
	wchar_t wszKillerName[MAX_PLAYER_NAME_LENGTH + 1];
	size_t iKillerNameSize = 0;
};

// Global instance of CNEOKillerDamageInfos
inline CNEOKillerDamageInfos g_neoKDmgInfos;
