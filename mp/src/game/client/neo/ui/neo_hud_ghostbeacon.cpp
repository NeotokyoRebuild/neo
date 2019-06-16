#include "cbase.h"
#include "neo_hud_ghostbeacon.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

CNEOHud_GhostBeacon::CNEOHud_GhostBeacon(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	m_posX = 0;
	m_posY = 0;

	SetAutoDelete(true);

	SetScheme("ClientScheme.res");

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	// NEO HACK (Rain): this is kind of awkward, we should get the handle on ApplySchemeSettings
	vgui::IScheme *scheme = vgui::scheme()->GetIScheme(vgui::scheme()->GetDefaultScheme());
	Assert(scheme);

	font = scheme->GetFont("Default", true);

	SetVisible(false);
}

void CNEOHud_GhostBeacon::Paint()
{
	BaseClass::Paint();
	
	const wchar_t text[] = L"GHOST TARGET";
	const size_t len = Q_UnicodeLength(text);

	Assert(surface());

	surface()->DrawSetColor(255, 255, 0, 255);
	surface()->DrawSetTextColor(255, 0, 255, 255);
	surface()->DrawSetTextFont(font);
	//surface()->DrawSetTextScale(1.0f, 1.0f);
	surface()->DrawSetTextPos(m_posX, m_posY);
	surface()->DrawPrintText(text, len);
	//surface()->SwapBuffers(g_pClientMode->GetViewport()->GetVPanel());

	//DrawBox(m_posX, m_posY, 10, 10, Color(255, 0, 255), 0.75f);
}

// "vgui\\hud\\ctg\\g_beacon_enemy"