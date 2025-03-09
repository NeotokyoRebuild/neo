#pragma once

#include <steam/isteammatchmaking.h>
#include <utlvector.h>

enum GameServerType
{
	GS_INTERNET = 0,
	GS_LAN,
	GS_FRIENDS,
	GS_FAVORITES,
	GS_HISTORY,
	GS_SPEC,

	GS__TOTAL,
};

enum GameServerInfoW
{
	GSIW_LOCKED = 0,
	GSIW_VAC,
	GSIW_NAME,
	GSIW_MAP,
	GSIW_PLAYERS,
	GSIW_PING,

	GSIW__TOTAL,
};

enum AnitCheatMode
{
	ANTICHEAT_ANY,
	ANTICHEAT_ON,
	ANTICHEAT_OFF,

	ANTICHEAT__TOTAL,
};

extern int g_iGSIX[GSIW__TOTAL];

struct GameServerSortContext
{
	GameServerInfoW col = GSIW_NAME;
	bool bDescending = false;
};

class CNeoServerList : public ISteamMatchmakingServerListResponse
{
public:
	GameServerType m_iType;
	CUtlVector<gameserveritem_t> m_servers;
	CUtlVector<gameserveritem_t> m_filteredServers;

	void UpdateFilteredList();
	void RequestList();
	void ServerResponded(HServerListRequest hRequest, int iServer) final;
	void ServerFailedToRespond(HServerListRequest hRequest, int iServer) final;
	void RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response) final;
	HServerListRequest m_hdlRequest = nullptr;
	bool m_bModified = false;
	bool m_bSearching = false;
	bool m_bReloadedAtLeastOnce = false;
	GameServerSortContext *m_pSortCtx = nullptr;
};

class CNeoServerPlayers : public ISteamMatchmakingPlayersResponse
{
public:
	~CNeoServerPlayers();
	void RequestList(uint32 unIP, uint16 usPort);
	void AddPlayerToList(const char *pchName, int nScore, float flTimePlayed) final;
	void PlayersFailedToRespond() final;
	void PlayersRefreshComplete() final;

	struct PlayerInfo
	{
		wchar_t wszName[33];
		int iScore;
		float flTimePlayed;
	};
	CUtlVector<PlayerInfo> m_players;
	HServerQuery m_hdlQuery = HSERVERQUERY_INVALID;
	bool m_bFetching = false;
};

struct ServerBrowserFilters
{
	bool bServerNotFull = false;
	bool bHasUsersPlaying = false;
	bool bIsNotPasswordProtected = false;
	int iAntiCheat = 0;
	int iMaxPing = 0;
};

void ServerBrowserDrawRow(const gameserveritem_t &server);
