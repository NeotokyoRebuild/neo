#pragma once

#include <tier1/netadr.h>
#include <steam/isteammatchmaking.h>
#include <utlvector.h>

enum EServerBlacklistType
{
	SBLIST_TYPE_NETADR = 0,
	SBLIST_TYPE_SUBNAME,

	SBLIST_TYPE__TOTAL,
};

struct ServerBlacklistInfo
{
	// Data section
	wchar_t wszName[256];
	time_t timeVal;
	netadr_s netAdr;
	EServerBlacklistType eType;

	// Visual cache only
	wchar_t wszDateTimeAdded[48];
};
extern CUtlVector<ServerBlacklistInfo> g_blacklistedServers;

enum ServerBlacklistCols
{
	SBLIST_COL_NAME = 0,
	SBLIST_COL_TYPE,
	SBLIST_COL_DATETIME,

	SBLIST_COL__TOTAL,
};

// NEO NOTE (nullsystem): NT;RE blacklist should be separate as it have more feature set
// and SDK's blacklist read/write would collide if it's the same filename
static constexpr const char SERVER_BLACKLIST_DEFFILE[] = "cfg/ntre_server_blacklist.txt";

struct GameServerSortContext;

void ServerBlacklistRead(const char *szPath);
void ServerBlacklistWrite(const char *szPath);
void ServerBlacklistCacheWsz(ServerBlacklistInfo *sbInfo);
bool ServerBlacklisted(const gameserveritem_t &server);
void ServerBlacklistUpdateSortedList(const GameServerSortContext& sortCtx);

enum GameServerType
{
	GS_INTERNET = 0,
	GS_LAN,
	GS_FRIENDS,
	GS_FAVORITES,
	GS_HISTORY,
	GS_SPEC,
	GS_BLACKLIST,

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

enum AntiCheatMode
{
	ANTICHEAT_ANY,
	ANTICHEAT_ON,
	ANTICHEAT_OFF,

	ANTICHEAT__TOTAL,
};

enum GameServerPlayerSort
{
	GSPS_SCORE = 0,
	GSPS_NAME,
	GSPS_TIME,

	GSPS__TOTAL,
};

struct GameServerSortContext
{
	// NEO NOTE (Rain): this type is not GameServerInfoW anymore, because it may be treated as ServerBlacklistCols
	// in the column sorting logic, and that would emit an error for the steamrt4 Clang 19 compiler for linux-debug build:
	// error: comparison of different enumeration types in switch statement ('GameServerInfoW' and 'ServerBlacklistCols')
	// Just always make sure col has a meaningful value for the context in which you evaluate it.
	int col = GSIW_NAME;
	bool bDescending = false;
};

struct GameServerPlayerSortContext
{
	GameServerPlayerSort col = GSPS_SCORE;
	bool bDescending = true;
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
	// Pointer because all GameServerType to unify under one sorting
	GameServerSortContext *m_pSortCtx = nullptr;
};

class CNeoServerPlayers : public ISteamMatchmakingPlayersResponse
{
public:
	~CNeoServerPlayers();
	void UpdateSortedList();
	void RequestList(uint32 unIP, uint16 usPort);
	void AddPlayerToList(const char *pchName, int nScore, float flTimePlayed) final;
	void PlayersFailedToRespond() final;
	void PlayersRefreshComplete() final;

	struct PlayerInfo
	{
		wchar_t wszName[MAX_PLAYER_NAME_LENGTH + 1];
		int iScore;
		float flTimePlayed;
	};
	CUtlVector<PlayerInfo> m_players;
	CUtlVector<PlayerInfo> m_sortedPlayers;
	HServerQuery m_hdlQuery = HSERVERQUERY_INVALID;
	bool m_bFetching = false;
	GameServerPlayerSortContext m_sortCtx;
};

struct ServerBrowserFilters
{
	bool bServerNotFull = false;
	bool bHasUsersPlaying = false;
	bool bIsNotPasswordProtected = false;
	int iAntiCheat = 0;
	int iMaxPing = 0;
};
