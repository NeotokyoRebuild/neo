#pragma once

#include "shareddefs.h"

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

void NeoUserIDsLocalKilledClear();

// Global instance of CNEOKillerInfos and local-player's kill record
inline CNEOKillerInfos g_neoKillerInfos;
inline int g_neoUserIDsLocalKilledSize;
inline int g_neoUserIDsLocalKilled[MAX_PLAYERS_ARRAY_SAFE];

