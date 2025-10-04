#include "neo_root_serverbrowser.h"

#include <tier1/fmtstr.h>
#include <steam/steam_api.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <filesystem.h>

#include "neo_ui.h"
#include "neo_root.h"
#include "neo_misc.h"

#include <algorithm>

extern NeoUI::Context g_uiCtx;
extern IFileSystem *g_pFullFileSystem;

inline CUtlVector<ServerBlacklistInfo> g_blacklistedServers;
static inline bool g_bThisBlacklistUsed = false;

// SDK and NT;RE blacklist format is similar enough that
// NT;RE blacklist can also import SDK blacklist, but
// it's not backward compatible due to uint64 on date,
// GetUint64 will still read int type

void ServerBlacklistRead(const char *szPath)
{
	// Regardless if the file there or not, using this func already marks
	// this blacklist list being used
	g_bThisBlacklistUsed = true;

	KeyValues *kv = new KeyValues("serverblacklist");
	if (!kv->LoadFromFile(g_pFullFileSystem, szPath))
	{
		kv->deleteThis();
		return;
	}

	for (KeyValues *pSubKey = kv->GetFirstTrueSubKey(); pSubKey; pSubKey = pSubKey->GetNextTrueSubKey())
	{
		if (V_strcmp(pSubKey->GetName(), "server") == 0 ||
				V_strcmp(pSubKey->GetName(), "Server") == 0)
		{
			ServerBlacklistInfo sbInfo = {};

			V_wcscpy_safe(sbInfo.wszName, pSubKey->GetWString("name"));
			sbInfo.timeVal = pSubKey->GetUint64("date");
			sbInfo.netAdr.SetFromString(pSubKey->GetString("addr", "0.0.0.0:0"));
			const int iType = pSubKey->GetInt("type", SBLIST_TYPE_NETADR);
			sbInfo.eType = (iType >= 0 && iType < SBLIST_TYPE__TOTAL) ? static_cast<EServerBlacklistType>(iType) : SBLIST_TYPE_NETADR;
			ServerBlacklistCacheWsz(&sbInfo);

			g_blacklistedServers.AddToTail(sbInfo);
		}
	}

	kv->deleteThis();
}

void ServerBlacklistWrite(const char *szPath)
{
	if (!g_bThisBlacklistUsed)
	{
		return;
	}

	KeyValues *kv = new KeyValues("serverblacklist");

	for (const ServerBlacklistInfo &blacklist : g_blacklistedServers)
	{
		const CUtlNetAdrRender netAdrRender(blacklist.netAdr);

		KeyValues *pSubKey = new KeyValues("server");
		pSubKey->SetWString("name", blacklist.wszName);
		pSubKey->SetUint64("date", blacklist.timeVal);
		pSubKey->SetString("addr", netAdrRender.String());
		pSubKey->SetInt("type", blacklist.eType);
		kv->AddSubKey(pSubKey);
	}

	kv->SaveToFile(g_pFullFileSystem, szPath);
	kv->deleteThis();
}

void ServerBlacklistCacheWsz(ServerBlacklistInfo *sbInfo)
{
	char szDate[32] = {};
	char szTime[16] = {};
	BGetLocalFormattedDateAndTime(sbInfo->timeVal, szDate, sizeof(szDate), szTime, sizeof(szTime));
	wchar_t wszDate[32] = {};
	wchar_t wszTime[16] = {};
	g_pVGuiLocalize->ConvertANSIToUnicode(szDate, wszDate, sizeof(wszDate));
	g_pVGuiLocalize->ConvertANSIToUnicode(szTime, wszTime, sizeof(wszTime));
	V_swprintf_safe(sbInfo->wszDateTimeAdded, L"%ls %ls", wszDate, wszTime);
}

bool ServerBlacklisted(const gameserveritem_t &server)
{
	const netadr_t netAdrCmp(server.m_NetAdr.GetIP(), server.m_NetAdr.GetConnectionPort());

	for (const ServerBlacklistInfo &blacklist : g_blacklistedServers)
	{
		switch (blacklist.eType)
		{
		case SBLIST_TYPE_NETADR:
		{
			if (blacklist.netAdr.ip[3] == 0)
			{
				if (blacklist.netAdr.CompareClassCAdr(netAdrCmp))
				{
					return true;
				}
			}
			else
			{
				if (blacklist.netAdr.CompareAdr(netAdrCmp, blacklist.netAdr.GetPort() == 0))
				{
					return true;
				}
			}
		} break;
		case SBLIST_TYPE_SUBNAME:
		{
			char szBlacklistName[128] = {};
			g_pVGuiLocalize->ConvertUnicodeToANSI(blacklist.wszName, szBlacklistName, sizeof(szBlacklistName));
			if (V_strstr(server.GetName(), szBlacklistName))
			{
				return true;
			}
		} break;
		}
	}
	return false;
}

void ServerBlacklistUpdateSortedList(const GameServerSortContext &sortCtx)
{
	if (g_blacklistedServers.IsEmpty())
	{
		g_pNeoRoot->m_iSelectedServer = -1;
		return;
	}

	ServerBlacklistInfo curSbi;
	if (IN_BETWEEN_AR(0, g_pNeoRoot->m_iSelectedServer, g_blacklistedServers.Count()))
	{
		V_memcpy(&curSbi, &g_blacklistedServers[g_pNeoRoot->m_iSelectedServer], sizeof(ServerBlacklistInfo));
	}

	std::sort(g_blacklistedServers.begin(), g_blacklistedServers.end(),
			[&sortCtx](const ServerBlacklistInfo &blLeft, const ServerBlacklistInfo &blRight) -> bool {
		// Always set wszLeft/wszRight to name as fallback
		const wchar_t *wszLeft = blLeft.wszName;
		const wchar_t *wszRight = blRight.wszName;
		int64 iLeft, iRight;
		switch (sortCtx.col)
		{
		case SBLIST_COL_TYPE:
			iLeft = static_cast<int64>(blLeft.eType);
			iRight = static_cast<int64>(blRight.eType);
			break;
		case SBLIST_COL_DATETIME:
			iLeft = blLeft.timeVal;
			iRight = blRight.timeVal;
			break;
		default:
			break;
		}

		switch (sortCtx.col)
		{
		case SBLIST_COL_TYPE:
		case SBLIST_COL_DATETIME:
			if (iLeft != iRight) return (sortCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
			break;
		default:
			break;
		}

		return (sortCtx.bDescending) ? (V_wcscmp(wszRight, wszLeft) > 0) : (V_wcscmp(wszLeft, wszRight) > 0);
	});

	if (IN_BETWEEN_AR(0, g_pNeoRoot->m_iSelectedServer, g_blacklistedServers.Count()))
	{
		g_pNeoRoot->m_iSelectedServer = -1;
		for (int i = 0; i < g_blacklistedServers.Count(); ++i)
		{
			if (V_memcmp(&curSbi, &g_blacklistedServers[i], sizeof(ServerBlacklistInfo)) == 0)
			{
				g_pNeoRoot->m_iSelectedServer = i;
				break;
			}
		}
	}
}

void CNeoServerList::UpdateFilteredList()
{
	if (m_iType == GS_BLACKLIST)
	{
		Assert(m_pSortCtx);
		ServerBlacklistUpdateSortedList(*m_pSortCtx);
		return;
	}

	gameserveritem_t curGsi;
	if (IN_BETWEEN_AR(0, g_pNeoRoot->m_iSelectedServer, m_filteredServers.Count()))
	{
		V_memcpy(&curGsi, &m_filteredServers[g_pNeoRoot->m_iSelectedServer], sizeof(gameserveritem_t));
	}

	m_filteredServers = m_servers;
	if (m_filteredServers.IsEmpty())
	{
		g_pNeoRoot->m_iSelectedServer = -1;
		return;
	}

	std::sort(m_filteredServers.begin(), m_filteredServers.end(),
			  [this](const gameserveritem_t &gsiLeft, const gameserveritem_t &gsiRight) -> bool {
		// Always set szLeft/szRight to name as fallback
		const char *szLeft = gsiLeft.GetName();
		const char *szRight = gsiRight.GetName();
		int iLeft, iRight;
		bool bLeft, bRight;
		switch (m_pSortCtx->col)
		{
		case GSIW_LOCKED:
			bLeft = gsiLeft.m_bPassword;
			bRight = gsiRight.m_bPassword;
			break;
		case GSIW_VAC:
			bLeft = gsiLeft.m_bSecure;
			bRight = gsiRight.m_bSecure;
			break;
		case GSIW_MAP:
		{
			const char *szMapLeft = gsiLeft.m_szMap;
			const char *szMapRight = gsiRight.m_szMap;
			if (V_strcmp(szMapLeft, szMapRight) != 0)
			{
				szLeft = szMapLeft;
				szRight = szMapRight;
			}
			break;
		}
		case GSIW_PLAYERS:
			iLeft = gsiLeft.m_nPlayers - gsiLeft.m_nBotPlayers;
			iRight = gsiRight.m_nPlayers - gsiRight.m_nBotPlayers;
			if (iLeft == iRight)
			{
				iLeft = gsiLeft.m_nMaxPlayers;
				iRight = gsiRight.m_nMaxPlayers;
			}
			break;
		case GSIW_PING:
			iLeft = gsiLeft.m_nPing;
			iRight = gsiRight.m_nPing;
			break;
		case GSIW_NAME:
		default:
			// no-op, already assigned (default)
			break;
		}

		switch (m_pSortCtx->col)
		{
		case GSIW_LOCKED:
		case GSIW_VAC:
			if (bLeft != bRight) return (m_pSortCtx->bDescending) ? bLeft < bRight : bLeft > bRight;
			break;
		case GSIW_PLAYERS:
		case GSIW_PING:
			if (iLeft != iRight) return (m_pSortCtx->bDescending) ? iLeft < iRight : iLeft > iRight;
			break;
		default:
			break;
		}

		return (m_pSortCtx->bDescending) ? (V_strcmp(szRight, szLeft) > 0) : (V_strcmp(szLeft, szRight) > 0);
	});

	if (IN_BETWEEN_AR(0, g_pNeoRoot->m_iSelectedServer, m_filteredServers.Count()))
	{
		g_pNeoRoot->m_iSelectedServer = -1;
		for (int i = 0; i < m_filteredServers.Count(); ++i)
		{
			if (V_memcmp(&curGsi, &m_filteredServers[i], sizeof(gameserveritem_t)) == 0)
			{
				g_pNeoRoot->m_iSelectedServer = i;
				break;
			}
		}
	}
}

void CNeoServerList::RequestList()
{
	if (m_iType == GS_BLACKLIST)
	{
		return;
	}

	static MatchMakingKeyValuePair_t mmFilters[] = {
		{"gamedir", "neo"},
	};
	MatchMakingKeyValuePair_t *pMMFilters = mmFilters;

	static constexpr HServerListRequest (ISteamMatchmakingServers::*P_FN_REQ[GS__TOTAL])(
				AppId_t, MatchMakingKeyValuePair_t **, uint32, ISteamMatchmakingServerListResponse *) = {
		&ISteamMatchmakingServers::RequestInternetServerList,
		nullptr,
		&ISteamMatchmakingServers::RequestFriendsServerList,
		&ISteamMatchmakingServers::RequestFavoritesServerList,
		&ISteamMatchmakingServers::RequestHistoryServerList,
		&ISteamMatchmakingServers::RequestSpectatorServerList,
		nullptr,
	};

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	m_hdlRequest = (m_iType == GS_LAN) ?
				steamMM->RequestLANServerList(engine->GetAppID(), this) :
				(steamMM->*P_FN_REQ[m_iType])(engine->GetAppID(), &pMMFilters, ARRAYSIZE(mmFilters), this);
	m_bSearching = true;
}

// Server has responded ok with updated data
void CNeoServerList::ServerResponded(HServerListRequest hRequest, int iServer)
{
	if (m_iType == GS_BLACKLIST || hRequest != m_hdlRequest) return;

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	gameserveritem_t *pServerDetails = steamMM->GetServerDetails(hRequest, iServer);
	if (pServerDetails)
	{
		m_servers.AddToTail(*pServerDetails);
		m_bModified = true;
	}
}

// Server has failed to respond
void CNeoServerList::ServerFailedToRespond(HServerListRequest hRequest, [[maybe_unused]] int iServer)
{
	if (m_iType == GS_BLACKLIST || hRequest != m_hdlRequest) return;
}

// A list refresh you had initiated is now 100% completed
void CNeoServerList::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
	if (m_iType == GS_BLACKLIST || hRequest != m_hdlRequest) return;

	m_bSearching = false;
	if (response == eNoServersListedOnMasterServer && m_servers.IsEmpty())
	{
		m_bModified = true;
	}
}

CNeoServerPlayers::~CNeoServerPlayers()
{
	if (m_hdlQuery != HSERVERQUERY_INVALID)
	{
		steamapicontext->SteamMatchmakingServers()->CancelServerQuery(m_hdlQuery);
	}
}

void CNeoServerPlayers::UpdateSortedList()
{
	m_sortedPlayers = m_players;
	if (m_sortedPlayers.IsEmpty())
	{
		return;
	}

	std::sort(m_sortedPlayers.begin(), m_sortedPlayers.end(),
			  [this](const PlayerInfo &gspsLeft, const PlayerInfo &gspsRight) -> bool {
		switch (m_sortCtx.col)
		{
		case GSPS_SCORE:
		{
			const int iLeft = gspsLeft.iScore;
			const int iRight = gspsRight.iScore;
			if (iLeft != iRight) return (m_sortCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
		} break;
		case GSPS_TIME:
		{
			const float flLeft = gspsLeft.flTimePlayed;
			const float flRight = gspsRight.flTimePlayed;
			if (flLeft != flRight) return (m_sortCtx.bDescending) ? flLeft < flRight : flLeft > flRight;
		} break;
		default:
			// no-op, already assigned (default)
			break;
		}

		// Always set wszLeft/wszRight to name as fallback
		const wchar_t *wszLeft = gspsLeft.wszName;
		const wchar_t *wszRight = gspsRight.wszName;
		return (m_sortCtx.bDescending) ? (V_wcscmp(wszRight, wszLeft) > 0) : (V_wcscmp(wszLeft, wszRight) > 0);
	});
}

void CNeoServerPlayers::RequestList(uint32 unIP, uint16 usPort)
{
	auto *steamMM = steamapicontext->SteamMatchmakingServers();
	if (m_hdlQuery != HSERVERQUERY_INVALID)
	{
		steamMM->CancelServerQuery(m_hdlQuery);
	}
	m_hdlQuery = steamMM->PlayerDetails(unIP, usPort, this);
	m_players.RemoveAll();
}

void CNeoServerPlayers::AddPlayerToList(const char *pchName, int nScore, float flTimePlayed)
{
	PlayerInfo playerInfo{
		.iScore = nScore,
		.flTimePlayed = flTimePlayed,
	};
	g_pVGuiLocalize->ConvertANSIToUnicode(pchName, playerInfo.wszName, sizeof(playerInfo.wszName));
	m_players.AddToTail(playerInfo);
	UpdateSortedList();
}

void CNeoServerPlayers::PlayersFailedToRespond()
{
	// no-op
}

void CNeoServerPlayers::PlayersRefreshComplete()
{
	m_hdlQuery = HSERVERQUERY_INVALID;
}
