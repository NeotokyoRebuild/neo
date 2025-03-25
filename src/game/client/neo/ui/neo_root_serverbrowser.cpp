#include "neo_root_serverbrowser.h"

#include <steam/steam_api.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>

#include "neo_ui.h"

#include <algorithm>

extern NeoUI::Context g_uiCtx;

void CNeoServerList::UpdateFilteredList()
{
	// TODO: g_pNeoRoot->m_iSelectedServer Select kept with sorting
	m_filteredServers = m_servers;
	if (m_filteredServers.IsEmpty())
	{
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
			iLeft = gsiLeft.m_nPlayers;
			iRight = gsiRight.m_nPlayers;
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
}

void CNeoServerList::RequestList()
{
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
	if (hRequest != m_hdlRequest) return;

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
	if (hRequest != m_hdlRequest) return;
}

// A list refresh you had initiated is now 100% completed
void CNeoServerList::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
	if (hRequest != m_hdlRequest) return;

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
