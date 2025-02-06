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

constexpr int FLAVOUR_TEXT_SIZE = 29;
struct FlavourTextEntry
{
	const wchar_t* text;
	const int length;
} FLAVOUR_TEXT[FLAVOUR_TEXT_SIZE] = {
	{ L".Entering critical section", 26},
	{ L".Sleeping", 9 },
	{ L".Defraging memory", 17 },
	{ L".Garbage collection", 19 },
	{ L".Reference count not zero", 25 },
	{ L".Reconfiguring threads for optimal speed", 40 },
	{ L".Code 0x0", 9 },
	{ L".Warning, this has been code depreciated", 40 },
	{ L".Loading runtime combat libraries", 33 },
	{ L".Analyzing signal cross talk", 28 },
	{ L".Linking to local systems", 25 },
	{ L".HHOS", 5 },
	{ L".Initializing MARTYR(TM) system intercepts", 42 },
	{ L".Checking ROE subsystem", 23 },
	{ L".Rebooting command module", 25 },
	{ L".Staging interrupts", 19 },
	{ L".Repairing netsplit", 19 },
	{ L".Boosting comm signal", 21 },
	{ L".Compensating for combat network latency", 40 },
	{ L".Testing neural weapon link sequencing", 38 },
	{ L".Configuring firewalls", 22 },
	{ L".Booting combat readiness systems", 33 },
	{ L".Updating firmware", 18 },
	{ L".Zero divide warning", 20 },
	{ L".ASSERT", 7 },
	{ L".Downloading update", 19 },
	{ L".Charging flux capacitor", 24 },
	{ L".Muxing input signals", 21 },
	{ L".Init sequence started", 22 },
};

std::random_device startup_sequence_rd{};
std::mt19937 startup_sequence_gen{ startup_sequence_rd() };
std::normal_distribution<float> startup_sequence_distribution(0.25, 0.25);

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
		m_flNextTimeChangeText = gpGlobals->curtime + 0.333 + max(0, startup_sequence_distribution(startup_sequence_gen));
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