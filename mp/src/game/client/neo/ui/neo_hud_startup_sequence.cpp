#include "cbase.h"
#include "neo_hud_startup_sequence.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include "c_neo_player.h"
#include "neo_gamerules.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

DECLARE_NAMED_HUDELEMENT(CNEOHud_StartupSequence, neo_ghost_startup_sequence);
NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(StartupSequence, 0.1)

void CNEOHud_StartupSequence::UpdateStateForNeoHudElementDraw()
{
}

CNEOHud_StartupSequence::CNEOHud_StartupSequence(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);

	if (parent) {
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	SetVisible(true);
}

void CNEOHud_StartupSequence::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("BOOT");
	int fontTall = surface()->GetFontTall(m_hFont);
	surface()->DrawSetTextFont(m_hFont);

	SetBounds(xpos, ypos - fontTall, wide, fontTall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

constexpr int FLAVOUR_TEXT_SIZE = 29;
#define WSZ(str) str, sizeof(str) /sizeof(wchar_t) - 1
struct FlavourTextEntry
{
	const wchar_t* text;
	const int length;
} FLAVOUR_TEXT[FLAVOUR_TEXT_SIZE] = {
	{ WSZ(L".Entering critical section") },
	{ WSZ(L".Sleeping") },
	{ WSZ(L".Defraging memory") },
	{ WSZ(L".Garbage collection") },
	{ WSZ(L".Reference count not zero") },
	{ WSZ(L".Reconfiguring threads for optimal speed") },
	{ WSZ(L".Code 0x0") },
	{ WSZ(L".Warning, this has been code depreciated") },
	{ WSZ(L".Loading runtime combat libraries") },
	{ WSZ(L".Analyzing signal cross talk") },
	{ WSZ(L".Linking to local systems") },
	{ WSZ(L".HHOS") },
	{ WSZ(L".Initializing MARTYR(TM) system intercepts") },
	{ WSZ(L".Checking ROE subsystem") },
	{ WSZ(L".Rebooting command module") },
	{ WSZ(L".Staging interrupts") },
	{ WSZ(L".Repairing netsplit") },
	{ WSZ(L".Boosting comm signal") },
	{ WSZ(L".Compensating for combat network latency") },
	{ WSZ(L".Testing neural weapon link sequencing") },
	{ WSZ(L".Configuring firewalls") },
	{ WSZ(L".Booting combat readiness systems") },
	{ WSZ(L".Updating firmware") },
	{ WSZ(L".Zero divide warning") },
	{ WSZ(L".ASSERT") },
	{ WSZ(L".Downloading update") },
	{ WSZ(L".Charging flux capacitor") },
	{ WSZ(L".Muxing input signals") },
	{ WSZ(L".Init sequence started") },
};

void CNEOHud_StartupSequence::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	if (NEORules()->GetRoundStatus() != NeoRoundStatus::PreRoundFreeze)
	{
		m_flNextTimeChangeText = gpGlobals->curtime + 1.f;
		m_iSelectedText = FLAVOUR_TEXT_SIZE - 1;
		return;
	}

	if (gpGlobals->curtime >= m_flNextTimeChangeText)
	{
		m_flNextTimeChangeText = gpGlobals->curtime + 0.333 + MAX(0, randomgaussian->RandomFloat(0.25, 0.25));
		m_iSelectedText = random->RandomInt(0, FLAVOUR_TEXT_SIZE - 2);
	}

	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextColor(COLOR_FADED_WHITE);
	surface()->DrawSetTextPos(0,0);
	surface()->DrawPrintText(FLAVOUR_TEXT[m_iSelectedText].text, FLAVOUR_TEXT[m_iSelectedText].length);
}

void CNEOHud_StartupSequence::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}