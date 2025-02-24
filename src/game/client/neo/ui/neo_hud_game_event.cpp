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

DECLARE_HUDELEMENT(CNEOHud_GameEvent);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(GameEvent, 0.1)

DECLARE_HUD_MESSAGE(CNEOHud_GameEvent, RoundResult);
CNEOHud_GameEvent::CNEOHud_GameEvent(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
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

	SetMessage("", 1);

	SetVisible(false);
}

void CNEOHud_GameEvent::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("scripts/HudLayout.res");

	m_hFont = pScheme->GetFont("NHudOCRNoAdditive");

	surface()->GetScreenSize(m_iResX, m_iResY);
	SetBounds(0, 0, m_iResX, m_iResY);
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

	surface()->DrawSetTextFont(m_hFont);

	const int textPosX = (m_iResX / 2) - (messageWidth /2);
	surface()->DrawSetTextColor(Color(0, 0, 0, alpha));
	surface()->DrawSetTextPos(textPosX + 2, text_y_offset + 2);
	surface()->DrawPrintText(messageWord, messageLength); // the address of the null character minus the address of the first character

	surface()->DrawSetTextColor(Color(255, 255, 255, alpha));
	surface()->DrawSetTextPos(textPosX, text_y_offset);
	surface()->DrawPrintText(messageWord, messageLength); // the address of the null character minus the address of the first character

	if (NEORules()->IsTeamplay())
	{
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

		const vgui::IntRect texRect{
			.x0 = (m_iResX/2) - 256,
			.y0 = image_y_offset,
			.x1 = (m_iResX / 2) + 256,
			.y1 = image_y_offset + 256,
		};
		surface()->DrawSetColor(Color(0, 0, 0, alpha));
		surface()->DrawTexturedRect(texRect.x0 + 2, texRect.y0 + 2, texRect.x1 + 2, texRect.y1 + 2);

		surface()->DrawSetColor(Color(255, 255, 255, alpha));
		surface()->DrawTexturedRect(texRect.x0, texRect.y0, texRect.x1, texRect.y1);
	}
}

void CNEOHud_GameEvent::Paint()
{
	PaintNeoElement();
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	BaseClass::Paint();
}
