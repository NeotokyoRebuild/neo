#include "cbase.h"
#include "neo_hud_ghostbeacon.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
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
	m_flTexScale = 1.0f;
	m_flDistMeters = 0;

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

	m_hFont = scheme->GetFont("Default", true);

	m_hTex = surface()->CreateNewTextureID();
	Assert(m_hTex > 0);
	surface()->DrawSetTextureFile(m_hTex, "vgui/hud/ctg/g_beacon_enemy", 1, false);

	surface()->DrawGetTextureSize(m_hTex, m_beaconTexWidth, m_beaconTexHeight);

	SetVisible(false);
}

// NEO HACK (Rain): This is a sort of magic number to help with screenspace hud elements
// scaling in world space. There's most likely some nicer and less confusing way to do this.
// Check which files make reference to this, if you decide to tweak it.
extern ConVar neo_ghost_beacon_scale_baseline("neo_ghost_beacon_scale_baseline", "0.65", FCVAR_USERINFO,
	"Scale baseline for the HUD ghost beacons.", true, 0, true, 10);

ConVar neo_ghost_beacon_alpha("neo_ghost_beacon_alpha", "150", FCVAR_USERINFO,
	"Alpha channel transparency of HUD ghost beacons.", true, 0, true, 255);

static inline double GetColorPulse()
{
	const double startPulse = 0.5;
	static double colorPulse = startPulse;
	static double pulseStep = 0.001;
	colorPulse = clamp(colorPulse + pulseStep, startPulse, 1);
	if (colorPulse >= 1 || colorPulse <= startPulse)
	{
		pulseStep = -pulseStep;
	}

	return colorPulse;
}

void CNEOHud_GhostBeacon::Paint()
{
	BaseClass::Paint();

	// Since the distance format is a known length,
	// we hardcode to save the unicode length check each time.
	const size_t beaconTextLen = 4;
	char beaconText[beaconTextLen + 1];
	V_snprintf(beaconText, sizeof(beaconText), "%02d M", FastFloatToSmallInt(m_flDistMeters));

	wchar_t beaconTextUnicode[(sizeof(beaconText) + 1) * sizeof(wchar_t*)];
	g_pVGuiLocalize->ConvertANSIToUnicode(beaconText, beaconTextUnicode, sizeof(beaconTextUnicode));

	const Color textColor = Color(220, 180, 180, neo_ghost_beacon_alpha.GetInt());

	surface()->DrawSetTextColor(textColor);
	surface()->DrawSetTextFont(m_hFont);
	//surface()->DrawSetTextScale(1.0f, 1.0f);
	surface()->DrawSetTextPos(m_posX, m_posY);
	surface()->DrawPrintText(beaconTextUnicode, beaconTextLen);
	//surface()->SwapBuffers(g_pClientMode->GetViewport()->GetVPanel());

	const double colorPulse = GetColorPulse();

	const Color beaconColor = Color(
		colorPulse * 255,
		colorPulse * 20,
		colorPulse * 20,
		neo_ghost_beacon_alpha.GetInt());

	surface()->DrawSetColor(beaconColor);
	surface()->DrawSetTexture(m_hTex);

	// This is kind of awful, see the cvar comments for details.
	const float hackyScale = (neo_ghost_beacon_scale_baseline.GetFloat() - m_flTexScale);

	// Offset screen space starting positions by half of the texture x/y coords,
	// so it starts centered on target.
	const int posfix_X = m_posX - ((m_beaconTexWidth / 2) * hackyScale);
	const int posfix_Y = m_posY - ((m_beaconTexHeight / 2) * hackyScale);

	// End coordinates according to art size (and our distance scaling)
	surface()->DrawTexturedRect(
		posfix_X,
		posfix_Y,
		posfix_X + (m_beaconTexWidth * hackyScale),
		posfix_Y + (m_beaconTexHeight * hackyScale));
}
