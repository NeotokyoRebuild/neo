#include "cbase.h"
#include "neo_hud_game_event.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

CNEOHud_GameEvent::CNEOHud_GameEvent(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme_Neo.res", "ClientScheme_Neo");
	SetScheme(neoscheme);

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	surface()->GetScreenSize(m_iResX, m_iResX);
	SetBounds(0, 0, m_iResX, m_iResX);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCR");

	SetMessage("");

	SetVisible(false);
}

void CNEOHud_GameEvent::SetMessage(const wchar_t *message, size_t size)
{
	V_memcpy(m_pszMessage, message, size);
}

void CNEOHud_GameEvent::SetMessage(const char* message)
{
	g_pVGuiLocalize->ConvertANSIToUnicode(message, m_pszMessage, NEO_MAX_HUD_GAME_EVENT_MSG_SIZE);
}

void CNEOHud_GameEvent::Paint()
{
	if (!IsHudReadyForPaintNow())
	{
		return;
	}

	BaseClass::Paint();

	int wide, tall;
	surface()->GetTextSize(m_hFont, m_pszMessage, wide, tall);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));
	surface()->DrawSetTextColor(255, 255, 0, 255);
	surface()->DrawSetTextPos(m_iResX / 2, m_iResY / 2);
	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawPrintText(m_pszMessage, NEO_MAX_HUD_GAME_EVENT_MSG_SIZE);
}
