#include "cbase.h"
#include "neo_hud_elements.h"

#include "GameEventListener.h"

#include "neo_hud_compass.h"

#include "vgui/ISurface.h"

#include "c_neo_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

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

	m_pCompass = NULL;
}

CNeoHudElements::~CNeoHudElements()
{
	
}

void CNeoHudElements::OnThink()
{
	BaseClass::OnThink();
}

void CNeoHudElements::Reset()
{
	m_fNextUpdateTime = 0;

	if (m_pCompass)
	{
		m_pCompass->DeletePanel();
		m_pCompass = NULL;
	}

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

	for (int i = 1; i <= gpGlobals->maxClients; i++)
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
	InitCompass();
}

void CNeoHudElements::InitCompass()
{
	m_pCompass = new CNEOHud_Compass("neo_compass", this);
}

CNEOHud_Compass *CNeoHudElements::GetCompass()
{
	return m_pCompass;
}