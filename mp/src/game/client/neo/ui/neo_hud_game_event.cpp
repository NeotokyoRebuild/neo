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

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GameEvent, 0.1)

DECLARE_HUD_MESSAGE(CNEOHud_GameEvent, RoundResult);
CNEOHud_GameEvent::CNEOHud_GameEvent(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	HOOK_HUD_MESSAGE(CNEOHud_GameEvent, RoundResult);

	jinraiWin = surface()->CreateNewTextureID();
	nsfWin = surface()->CreateNewTextureID();
	tie = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile(jinraiWin, "vgui/hud/roundend_jinrai", true, false);
	surface()->DrawSetTextureFile(nsfWin, "vgui/hud/roundend_nsf", true, false);
	surface()->DrawSetTextureFile(tie, "vgui/hud/roundend_tie", true, false);
	Assert(jinraiWin > 0);
	Assert(nsfWin > 0);
	Assert(tie > 0);

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

	surface()->GetScreenSize(m_iResX, m_iResY);
	SetBounds(0, 0, m_iResX, m_iResY);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCR");

	gHUD.AddHudElement(this);

	SetMessage("", 1);

	SetVisible(false);
}

void CNEOHud_GameEvent::SetMessage(const wchar_t *message, size_t size)
{
	Assert(sizeof(m_pszMessage) >= size);
	V_memcpy(m_pszMessage, message, sizeof(m_pszMessage));
}

void CNEOHud_GameEvent::SetMessage(const char* message, size_t size)
{
	Assert(sizeof(m_pszMessage) >= size);
	g_pVGuiLocalize->ConvertANSIToUnicode(message, m_pszMessage, sizeof(m_pszMessage));
}

void CNEOHud_GameEvent::UpdateStateForNeoHudElementDraw()
{
}


void CNEOHud_GameEvent::MsgFunc_RoundResult(bf_read& msg)
{
	msg.ReadString(teamWon, 64);
	timeWon = msg.ReadFloat();
	msg.ReadString(message, 64 + MAX_PLACE_NAME_LENGTH);

	mbstowcs(messageWord, message, 64 + MAX_PLACE_NAME_LENGTH);
	surface()->GetTextSize(m_hFont, messageWord, messageWidth, messageHeight);
	messageLength = (int)(strchr(message, '\0') - strchr(message, message[0]));

	SetVisible(true);
	C_NEO_Player* player = C_NEO_Player::GetLocalNEOPlayer();
	player->m_bShowTestMessage = true;
}

void CNEOHud_GameEvent::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	float timeSinceWon = gpGlobals->curtime - timeWon;
	if (timeSinceWon < 2)
	{
		return;
	}
	if (timeSinceWon > 10)
	{
		return;
	}

	int alpha = 255;
	if (timeSinceWon < 4)
	{
		alpha *= ((timeSinceWon-2) / 2);
	}
	else if (timeSinceWon > 8)
	{
		alpha *= (1 - ((timeSinceWon-8) / 2));
	}

	if (Q_stristr(teamWon, "jinrai"))
	{
		surface()->DrawSetTexture(jinraiWin);
	}
	else if (Q_stristr(teamWon, "nsf"))
	{
		surface()->DrawSetTexture(nsfWin);
	}
	else
	{
		surface()->DrawSetTexture(tie);
	}
	surface()->DrawSetColor(Color(255, 255, 255, alpha));
	surface()->DrawTexturedRect((m_iResX/2) - 256, image_y_offset, (m_iResX / 2) + 256, image_y_offset + 256);

	surface()->DrawSetTextColor(255, 255, 255, alpha);
	surface()->DrawSetTextPos((m_iResX / 2) - (messageWidth /2), text_y_offset);
	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawPrintText(messageWord, messageLength); // the address of the null character minus the address of the first character
}

void CNEOHud_GameEvent::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}
