#include "cbase.h"
#include "neo_hud_elements.h"

#include "GameEventListener.h"

#include "neo_hud_ammo.h"
#include "neo_hud_compass.h"
#include "neo_hud_friendly_marker.h"
#include "neo_hud_game_event.h"
#include "neo_hud_health_thermoptic_aux.h"
#include "neo_hud_ghost_marker.h"
#include "neo_hud_round_state.h"

#include "vgui/ISurface.h"

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define UI_ELEMENT_NAME_AMMO "neo_ammo"
#define UI_ELEMENT_NAME_COMPASS "neo_compass"
#define UI_ELEMENT_NAME_HTA "neo_hta"
#define UI_ELEMENT_NAME_IFF "neo_iff"
#define UI_ELEMENT_GAME_EVENT "neo_game_event_indicator"
#define UI_ELEMENT_NAME_GHOST_MARKER "neo_ghost_marker"
#define UI_ELEMENT_NAME_GHOST_BEACONS "neo_ghost_beacons"
#define UI_ELEMENT_ROUND_STATE "neo_round_state"
#define UI_ELEMENT_TARGET_ID "target_id"

using namespace vgui;

CNeoHudElements::CNeoHudElements(IViewPort *pViewPort)
	: EditablePanel(NULL, PANEL_NEO_HUD)
{
	m_iPlayerIndexSymbol = KeyValuesSystem()->GetSymbolForString("playerIndex");

	m_pViewPort = pViewPort;

	SetProportional(true);
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(false);

	// Set the scheme before any child control is created
	SetScheme("ClientScheme");

	// We need to update IFF if these happen
	ListenForGameEvent("player_death");
	ListenForGameEvent("player_team");

	m_pAmmo = NULL;
	m_pCompass = NULL;
	m_pFriendlyMarker = NULL;
	m_pGameEvent = NULL;
	m_pHTA = NULL;
	m_pRoundState = NULL;
	m_pLastUpdater = NULL;
	m_pTargetID = NULL;
	m_pGhostBeacons = NULL;
}

CNeoHudElements::~CNeoHudElements()
{
	FreePanelChildren();
}

void CNeoHudElements::OnThink()
{
	BaseClass::OnThink();
}

void CNeoHudElements::FreePanelChildren()
{
	if (m_pAmmo)
	{
		m_pAmmo->DeletePanel();
		m_pAmmo = NULL;
	}

	if (m_pCompass)
	{
		m_pCompass->DeletePanel();
		m_pCompass = NULL;
	}

	if (m_pFriendlyMarker)
	{
		m_pFriendlyMarker->DeletePanel();
		m_pFriendlyMarker = NULL;
	}

	if (m_pGameEvent)
	{
		m_pGameEvent->DeletePanel();
		m_pGameEvent = NULL;
	}

	if (m_pHTA)
	{
		m_pHTA->DeletePanel();
		m_pHTA = NULL;
	}

	if (m_pRoundState)
	{
		m_pRoundState->DeletePanel();
		m_pRoundState = NULL;
	}

	if (m_pTargetID)
	{
		m_pTargetID->DeletePanel();
		m_pTargetID = NULL;
	}

	if(m_pGhostBeacons)
	{
		m_pGhostBeacons->DeletePanel();
		m_pGhostBeacons = NULL;
	}

	for (int i = 0; i < m_vecGhostMarkers.Count(); i++)
	{
		if (m_vecGhostMarkers[i])
		{
			m_vecGhostMarkers[i]->DeletePanel();
		}
		else
		{
			// We should be never carrying a null pointer
			Assert(false);
		}
	}
	m_vecGhostMarkers.RemoveAll();
}

void CNeoHudElements::Reset()
{
	m_fNextUpdateTime = 0;

	FreePanelChildren();

	InitHud();
}

void CNeoHudElements::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int sizex, sizey;
	surface()->GetScreenSize(sizex, sizey);
	SetBounds(0, 0, sizex, sizey);

	PostApplySchemeSettings(pScheme);
}

void CNeoHudElements::PostApplySchemeSettings(vgui::IScheme* pScheme)
{
	SetBgColor(Color(255, 255, 0, 0));
}

void CNeoHudElements::ShowPanel(bool bShow)
{
	if (BaseClass::IsVisible() == bShow)
	{
		return;
	}

	if (bShow)
	{
		Reset();
		Update();
		SetVisible(true);
		MoveToFront();
	}
	else
	{
		BaseClass::SetVisible(false);
	}
}

void CNeoHudElements::FireGameEvent(IGameEvent* event)
{
}

bool CNeoHudElements::NeedsUpdate()
{
	return (m_fNextUpdateTime < gpGlobals->curtime);
}

void CNeoHudElements::Update()
{
	FillIFFs();

	m_fNextUpdateTime = gpGlobals->curtime + 1.0f;
}

void CNeoHudElements::UpdatePlayerIFF(int playerIndex, KeyValues* kv)
{
	
}

void CNeoHudElements::FillIFFs()
{
	auto localPlayer = C_BasePlayer::GetLocalPlayer();

	if (!localPlayer)
	{
		return;
	}

	int localTeam = localPlayer->GetTeamNumber();

	for (int i = 1; i <= gpGlobals->maxClients; ++i)
	{
		auto otherPlayer = UTIL_PlayerByIndex(i);

		if (!otherPlayer || otherPlayer == localPlayer)
		{
			continue;
		}

		if (otherPlayer->GetTeamNumber() != localTeam)
		{
			continue;
		}

		KeyValues *kv = NULL;
		UpdatePlayerIFF(i, kv);
	}
}

int CNeoHudElements::FindIFFItemIDForPlayerIndex(int playerIndex)
{
	return 0;
}

void CNeoHudElements::InitHud()
{
	InitAmmo();
	InitCompass();
	InitFriendlyMarker();
	InitGameEventIndicator();
	InitGhostMarkers();
	InitHTA();
	InitRoundState();
	InitTargetID();
	InitGhostBeacons();
}

void CNeoHudElements::InitGhostBeacons()
{
	Assert(!m_pGhostBeacons);
	m_pGhostBeacons = new CNEOHud_GhostBeacons(UI_ELEMENT_NAME_GHOST_BEACONS, this);
}

void CNeoHudElements::InitAmmo()
{
	Assert(!m_pAmmo);
	m_pAmmo = new CNEOHud_Ammo(UI_ELEMENT_NAME_AMMO, this);
}

void CNeoHudElements::InitCompass()
{
	Assert(!m_pCompass);
	m_pCompass = new CNEOHud_Compass(UI_ELEMENT_NAME_COMPASS, this);
}

void CNeoHudElements::InitFriendlyMarker()
{
	Assert(!m_pFriendlyMarker);
	m_pFriendlyMarker = new CNEOHud_FriendlyMarker(UI_ELEMENT_NAME_IFF, this);
}

void CNeoHudElements::InitGameEventIndicator()
{
	Assert(!m_pGameEvent);
	m_pGameEvent = new CNEOHud_GameEvent(UI_ELEMENT_GAME_EVENT, this);
}

void CNeoHudElements::InitGhostMarkers()
{
	Assert(m_vecGhostMarkers.Count() == 0);
	const int numGhosts = 1;
	for (int i = 0; i < numGhosts; i++)
	{
		auto marker = new CNEOHud_GhostMarker(UI_ELEMENT_NAME_GHOST_MARKER, this);
		m_vecGhostMarkers.AddToTail(marker);
	}
}

void CNeoHudElements::InitHTA()
{
	Assert(!m_pHTA);
	m_pHTA = new CNEOHud_HTA(UI_ELEMENT_NAME_HTA, this);
}

void CNeoHudElements::InitRoundState()
{
	Assert(!m_pRoundState);
	m_pRoundState = new CNEOHud_RoundState(UI_ELEMENT_ROUND_STATE, this);
}

void CNeoHudElements::InitTargetID()
{
	Assert(!m_pTargetID);
	m_pTargetID = new CTargetID(UI_ELEMENT_TARGET_ID);
}

CNEOHud_GhostMarker* CNeoHudElements::GetGhostMarker()
{
	if (m_vecGhostMarkers.Count() < 1)
	{
		Assert(false);
		return NULL;
	}

	// Just return first, for now
	CNEOHud_GhostMarker* ptr = m_vecGhostMarkers[m_vecGhostMarkers.Count() - 1];
	Assert(ptr);
	return ptr;
}
