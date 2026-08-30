#pragma once

#include "steam/steam_api.h"

struct NeoAvatar
{
	CSteamID m_SteamID;
	int m_iTextureID = 0;
	int m_iTextureDeadID = 0;
	int m_iTexForAvatarWH = 0;
	float m_flPrevLoadAttempt = 0.0f;

	void SetSteamID(const CSteamID &steamID);
	void Fetch(const int iAvatarWH);
	void Paint(const int x, const int y, const int widetall,
			const bool bDead = false) const;
};

