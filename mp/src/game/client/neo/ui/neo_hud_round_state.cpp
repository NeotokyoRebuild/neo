#include "cbase.h"
#include "neo_hud_round_state.h"

#include "iclientmode.h"
#include "ienginevgui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>

#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar neo_cl_hud_roundstatus_pos_x("neo_cl_hud_roundstatus_pos_x", "0", FCVAR_USERINFO,
	"Offset in pixels.");
ConVar neo_cl_hud_roundstatus_pos_y("neo_cl_hud_roundstatus_pos_y", "0", FCVAR_USERINFO,
	"Offset in pixels.");

CNEOHud_RoundState::CNEOHud_RoundState(const char *pElementName, vgui::Panel *parent)
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

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(neoscheme);
	Assert(scheme);

	m_hFont = scheme->GetFont("NHudOCRSmall");
}

void CNEOHud_RoundState::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));
}

void CNEOHud_RoundState::Paint()
{
	BaseClass::Paint();

	DrawRoundStatus();
}

void CNEOHud_RoundState::DrawRoundStatus()
{
#if(0)
	// Only update once per second
	static float lastTime = gpGlobals->curtime;
	float deltaTime = gpGlobals->curtime - lastTime;
	lastTime = gpGlobals->curtime;
	if (deltaTime < 1)
	{
		return;
	}
#endif

	float roundTimeLeft = NEORules()->GetRoundRemainingTime();

	// Exactly zero means there's no time limit, so we don't need to draw anything.
	if (roundTimeLeft == 0)
	{
		return;
	}
	// Less than 0 means round is over, but cap the timer to zero for nicer display.
	else if (roundTimeLeft < 0)
	{
		roundTimeLeft = 0;
	}

	const int secsTotal = RoundFloatToInt(roundTimeLeft);
	const int secsRemainder = secsTotal % 60;
	const int minutes = (secsTotal - secsRemainder) / 60;

	const int maxLen = 6;
	char statusANSI[maxLen];
	V_sprintf_safe(statusANSI, "%02d:%02d", minutes, secsRemainder);

	wchar_t statusUnicode[maxLen * sizeof(wchar_t)];
	g_pVGuiLocalize->ConvertANSIToUnicode(statusANSI, statusUnicode, sizeof(statusUnicode));

	surface()->DrawSetTextFont(m_hFont);

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, statusUnicode, fontWidth, fontHeight);

	const int xpos = (m_resX / 2) + neo_cl_hud_roundstatus_pos_x.GetInt();
	const int ypos = (m_resY * 0.01) + neo_cl_hud_roundstatus_pos_y.GetInt();

	surface()->DrawSetTextColor(Color(255, 255, 255, 255));
	surface()->DrawSetTextPos(xpos - (fontWidth / 2), ypos - (fontHeight / 2));
	surface()->DrawPrintText(statusUnicode, maxLen);
}

