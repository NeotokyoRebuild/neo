#include "neo_root_serverbrowser.h"

#include <steam/steam_api.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>

#include "neo_ui.h"

extern NeoUI::Context g_uiCtx;
inline int g_iGSIX[GSIW__TOTAL] = {};

void CNeoServerList::UpdateFilteredList()
{
	// TODO: g_pNeoRoot->m_iSelectedServer Select kept with sorting
	m_filteredServers = m_servers;
	if (m_filteredServers.IsEmpty())
	{
		return;
	}

	// Can't use lamda capture for this, so pass through context
	V_qsort_s(m_filteredServers.Base(), m_filteredServers.Size(), sizeof(gameserveritem_t),
			  [](void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
		const GameServerSortContext gsCtx = *(static_cast<GameServerSortContext *>(vpCtx));
		auto *pGsiLeft = static_cast<const gameserveritem_t *>(vpLeft);
		auto *pGsiRight = static_cast<const gameserveritem_t *>(vpRight);

		// Always set szLeft/szRight to name as fallback
		const char *szLeft = pGsiLeft->GetName();
		const char *szRight = pGsiRight->GetName();
		int iLeft, iRight;
		bool bLeft, bRight;
		switch (gsCtx.col)
		{
		case GSIW_LOCKED:
			bLeft = pGsiLeft->m_bPassword;
			bRight = pGsiRight->m_bPassword;
			break;
		case GSIW_VAC:
			bLeft = pGsiLeft->m_bSecure;
			bRight = pGsiRight->m_bSecure;
			break;
		case GSIW_MAP:
		{
			const char *szMapLeft = pGsiLeft->m_szMap;
			const char *szMapRight = pGsiRight->m_szMap;
			if (V_strcmp(szMapLeft, szMapRight) != 0)
			{
				szLeft = szMapLeft;
				szRight = szMapRight;
			}
			break;
		}
		case GSIW_PLAYERS:
			iLeft = pGsiLeft->m_nPlayers;
			iRight = pGsiRight->m_nPlayers;
			if (iLeft == iRight)
			{
				iLeft = pGsiLeft->m_nMaxPlayers;
				iRight = pGsiRight->m_nMaxPlayers;
			}
			break;
		case GSIW_PING:
			iLeft = pGsiLeft->m_nPing;
			iRight = pGsiRight->m_nPing;
			break;
		case GSIW_NAME:
		default:
			// no-op, already assigned (default)
			break;
		}

		switch (gsCtx.col)
		{
		case GSIW_LOCKED:
		case GSIW_VAC:
			if (bLeft != bRight) return (gsCtx.bDescending) ? bLeft < bRight : bLeft > bRight;
			break;
		case GSIW_PLAYERS:
		case GSIW_PING:
			if (iLeft != iRight) return (gsCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
			break;
		default:
			break;
		}

		return (gsCtx.bDescending) ? V_strcmp(szRight, szLeft) : V_strcmp(szLeft, szRight);
	}, m_pSortCtx);
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
}

void CNeoServerPlayers::PlayersFailedToRespond()
{
	// no-op
}

void CNeoServerPlayers::PlayersRefreshComplete()
{
	m_hdlQuery = HSERVERQUERY_INVALID;
}

void ServerBrowserDrawRow(const gameserveritem_t &server)
{
	int xPos = 0;
	const int fontStartYPos = g_uiCtx.fonts[g_uiCtx.eFont].iYOffset;

	if (server.m_bPassword)
	{
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(L"P", 1);
	}
	xPos += g_iGSIX[GSIW_LOCKED];

	if (server.m_bSecure)
	{
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(L"S", 1);
	}
	xPos += g_iGSIX[GSIW_VAC];

	{
		wchar_t wszServerName[k_cbMaxGameServerName];
		const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.GetName(), wszServerName, sizeof(wszServerName));
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(wszServerName, iSize);
	}
	xPos += g_iGSIX[GSIW_NAME];

	{
		// In lower resolution, it may overlap from name, so paint a background here
		NeoUI::GCtxDrawFilledRectXtoX(xPos, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

		wchar_t wszMapName[k_cbMaxGameServerMapName];
		const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(server.m_szMap, wszMapName, sizeof(wszMapName));
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(wszMapName, iSize);
	}
	xPos += g_iGSIX[GSIW_MAP];

	{
		// In lower resolution, it may overlap from name, so paint a background here
		NeoUI::GCtxDrawFilledRectXtoX(xPos, -g_uiCtx.iRowTall, g_uiCtx.dPanel.wide, 0);

		wchar_t wszPlayers[10];
		const int iSize = V_swprintf_safe(wszPlayers, L"%d/%d", server.m_nPlayers, server.m_nMaxPlayers);
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(wszPlayers, iSize);
	}
	xPos += g_iGSIX[GSIW_PLAYERS];

	{
		wchar_t wszPing[10];
		const int iSize = V_swprintf_safe(wszPing, L"%d", server.m_nPing);
		NeoUI::GCtxDrawSetTextPos(xPos + g_uiCtx.iMarginX, g_uiCtx.iLayoutY - g_uiCtx.iRowTall + fontStartYPos);
		vgui::surface()->DrawPrintText(wszPing, iSize);
	}
}
