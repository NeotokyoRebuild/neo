#include "cbase.h"
#include "neo_hud_startup_sequence.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include "c_neo_player.h"
#include "neo_gamerules.h"
#include <random>

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
	surface()->DrawSetTextFont(m_hFont);

	SetBounds(xpos, ypos, wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_StartupSequence::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	const int FLAVOUR_TEXT_SIZE = 29;
	const wchar_t *FLAVOUR_TEXT[FLAVOUR_TEXT_SIZE] = {
		L".Entering critical section",
		L".Sleeping",
		L".Defraging memory",
		L".Garbage collection",
		L".Reference count not zero",
		L".Reconfiguring threads for optimal speed",
		L".Code 0x0",
		L".Warning, this has been code depreciated",
		L".Loading runtime combat libraries",
		L".Analyzing signal cross talk",
		L".Linking to local systems",
		L".HHOS",
		L".Initializing MARTYR(TM) system intercepts",
		L".Checking ROE subsystem",
		L".Rebooting command module",
		L".Staging interrupts",
		L".Repairing netsplit",
		L".Boosting comm signal",
		L".Compensating for combat network latency",
		L".Testing neural weapon link sequencing",
		L".Configuring firewalls",
		L".Booting combat readiness systems",
		L".Updating firmware",
		L".Zero divide warning",
		L".ASSERT",
		L".Downloading update",
		L".Charging flux capacitor",
		L".Muxing input signals",
		L".Init sequence started"
	};

	if (NEORules()->GetRoundStatus() != NeoRoundStatus::PreRoundFreeze)
	{
		m_flNextTimeChangeText = gpGlobals->curtime + 1.f;
		m_iSelectedText = FLAVOUR_TEXT_SIZE - 1;
		return;
	}

	if (gpGlobals->curtime >= m_flNextTimeChangeText)
	{
		std::random_device rd{};
		std::mt19937 gen{ rd() };
		std::normal_distribution<float> distribution(0.25, 0.25);
		m_flNextTimeChangeText = gpGlobals->curtime + 0.333 + max(0, distribution(gen));
		m_iSelectedText = random->RandomInt(0, FLAVOUR_TEXT_SIZE - 2);
	}

	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextColor(COLOR_FADED_WHITE);
	surface()->DrawSetTextPos(0,0);
	surface()->DrawPrintText(FLAVOUR_TEXT[m_iSelectedText], wcslen(FLAVOUR_TEXT[m_iSelectedText]));
}

void CNEOHud_StartupSequence::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}