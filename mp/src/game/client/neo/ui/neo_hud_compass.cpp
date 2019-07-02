#include "cbase.h"
#include "neo_hud_compass.h"

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

ConVar neo_cl_hud_compass_enabled("neo_cl_hud_compass_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD compass is enabled or not.", true, 0, true, 1);
// NEO TODO (Rain): descriptions for 2 cvars
ConVar neo_cl_hud_compass_pos_x("neo_cl_hud_compass_pos_x", "2", FCVAR_USERINFO,
	"", true, 1, false, 10);
ConVar neo_cl_hud_compass_pos_y("neo_cl_hud_compass_pos_y", "80", FCVAR_USERINFO,
	"", true, 1, false, 10);
ConVar neo_cl_hud_compass_needle("neo_cl_hud_compass_needle", "1", FCVAR_USERINFO,
	"Whether or not to show HUD compass needle.", true, 0, true, 1);

ConVar neo_cl_hud_debug_compass_enabled("neo_cl_hud_debug_compass_enabled", "0", FCVAR_USERINFO | FCVAR_CHEAT,
	"Whether the Debug HUD compass is enabled or not.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_debug_compass_pos_x("neo_cl_hud_debug_compass_pos_x", "0.45", FCVAR_USERINFO | FCVAR_CHEAT,
	"Horizontal position of the Debug compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_debug_compass_pos_y("neo_cl_hud_debug_compass_pos_y", "0.925", FCVAR_USERINFO | FCVAR_CHEAT,
	"Vertical position of the Debug compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar neo_cl_hud_debug_compass_color_r("neo_cl_hud_debug_compass_color_r", "190", FCVAR_USERINFO | FCVAR_CHEAT,
	"Red color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_compass_color_g("neo_cl_hud_debug_compass_color_g", "185", FCVAR_USERINFO | FCVAR_CHEAT,
	"Green color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_compass_color_b("neo_cl_hud_debug_compass_color_b", "205", FCVAR_USERINFO | FCVAR_CHEAT,
	"Blue value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_compass_color_a("neo_cl_hud_debug_compass_color_a", "255", FCVAR_USERINFO | FCVAR_CHEAT,
	"Alpha color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);

CNEOHud_Compass::CNEOHud_Compass(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	m_pOwner = NULL;

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

	SetVisible(neo_cl_hud_compass_enabled.GetBool());
}

void CNEOHud_Compass::SetOwner(C_NEO_Player *owner)
{
	Assert(owner);

	if (owner)
	{
		m_pOwner = owner;
	}
}

void CNEOHud_Compass::Paint()
{
	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	if (neo_cl_hud_debug_compass_enabled.GetBool())
	{
		DrawDebugCompass();
	}

	// NEO FIXME (Rain): hook cvar to re-enable ourselves
	if (!neo_cl_hud_compass_enabled.GetBool())
	{
		SetVisible(false);
		return;
	}

	BaseClass::Paint();

	DrawCompass();
}

static inline double GetColorPulse(double startPulse = 0.5, double pulseStep = 0.1,
	double minPulse = -20, double maxPulse = 20)
{
	static double colorPulse = startPulse;
	colorPulse += pulseStep;

	if (colorPulse >= maxPulse || colorPulse <= minPulse)
	{
		pulseStep = -pulseStep;
		colorPulse = 0;
	}

	return colorPulse;
}

inline void CNEOHud_Compass::DrawCompass(void)
{
	if (!m_pOwner)
	{
		return;
	}

	// Direction in -180 to 180
	float angle = -1 * m_pOwner->EyeAngles()[YAW];

	// Bring us back to safety
	if (angle > 180)
	{
		angle -= 360;
	}
	else if (angle < -180)
	{
		angle += 360;
	}

	// Char representation of the compass strip
	const char rose[] =
               "N                |                NE                |\
                E                |                SE                |\
                S                |                SW                |\
                W                |                NW                |\
                ";

	// One compass tick represents this many degrees of rotation
	const float unitAccuracy = 360.0f / sizeof(rose);

	// How many characters should be visible around each side of the needle position
	const int numCharsVisibleAroundNeedle = 24;

	// Get index offset for this angle's compass position
	int offset = RoundFloatToInt((angle / unitAccuracy)) - numCharsVisibleAroundNeedle;
	if (offset < 0)
	{
		offset += sizeof(rose);
	}

	// Both sides + center + terminator
	const size_t compassStrSize = numCharsVisibleAroundNeedle * 2 + 2;
	char compass[compassStrSize];
	int i;
	for (i = 0; i < sizeof(compass) - 1; i++)
	{
		// Get our index by circling around the compass strip.
		// We do modulo -1, because sizeof would land us on NULL
		// and terminate the string early.
		const int wrappedIndex = (offset + i) % (sizeof(rose) - 1);

		compass[i] = rose[wrappedIndex];
	}
	// Finally, make sure we have a null terminator
	compass[i] = '\0';

	wchar_t compassUnicode[compassStrSize * sizeof(wchar_t)];
	g_pVGuiLocalize->ConvertANSIToUnicode(compass, compassUnicode, compassStrSize * sizeof(wchar_t));

	const Color textColor = Color(
		neo_cl_hud_debug_compass_color_r.GetInt(),
		neo_cl_hud_debug_compass_color_g.GetInt(),
		neo_cl_hud_debug_compass_color_b.GetInt(),
		neo_cl_hud_debug_compass_color_a.GetInt());

	surface()->DrawSetTextFont(m_hFont);

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, compassUnicode, fontWidth, fontHeight);

	const int xpos = m_resX - (m_resX / neo_cl_hud_compass_pos_x.GetInt());
	const int ypos = m_resY - (m_resY / neo_cl_hud_compass_pos_y.GetInt());
	
	// Print compass objective arrow
	if (neo_cl_hud_compass_needle.GetBool())
	{
		double pulse = GetColorPulse() * 3;
		Color alert = Color(180 + pulse, 10 + pulse, 0 + pulse, 200);

		// Print a unicode arrow to signify compass needle
		wchar_t arrowUnicode[] = L"▼";
		surface()->DrawSetTextColor(alert);
		surface()->DrawSetTextPos(
			xpos - ((fontWidth / (numCharsVisibleAroundNeedle - 1)) / 2) + (pulse / 2),
			ypos - (fontHeight * 1.75f));
		surface()->DrawPrintText(arrowUnicode, Q_UnicodeLength(arrowUnicode));
	}

	surface()->DrawSetTextColor(textColor);
	surface()->DrawSetTextPos(xpos - (fontWidth / 2), ypos - (fontHeight / 2));
	surface()->DrawPrintText(compassUnicode, sizeof(compass));

	surface()->DrawSetColor(Color(20, 20, 20, 200));
	// Draw right half of the background fade...
	surface()->DrawFilledRectFade(
		xpos, ypos - (fontHeight / 2),
		xpos + (fontWidth / 2), ypos + (fontHeight / 2),
		neo_cl_hud_debug_compass_color_a.GetInt(),
		0,
		true);
	// ...And then the left side.
	surface()->DrawFilledRectFade(
		xpos - (fontWidth / 2), ypos - (fontHeight / 2),
		xpos - 1, ypos + (fontHeight / 2),
		0,
		neo_cl_hud_debug_compass_color_a.GetInt(),
		true);

	// Draw the compass "needle"
	if (neo_cl_hud_compass_needle.GetBool())
	{
		const Color jinColor = Color(70, 145, 80, 200);
		const Color nsfColor = Color(20, 70, 180, 200);
		const Color specColor = Color(240, 120, 40, 200);

		if (m_pOwner->GetTeamNumber() == TEAM_JINRAI)
		{
			surface()->DrawSetColor(jinColor);
		}
		else if (m_pOwner->GetTeamNumber() == TEAM_NSF)
		{
			surface()->DrawSetColor(nsfColor);
		}
		else
		{
			surface()->DrawSetColor(jinColor);
			//surface()->DrawSetColor(specColor);
		}

		surface()->DrawFilledRect(xpos - 1, ypos - (fontHeight / 2),
			xpos + 1, ypos + (fontHeight / 2));
	}
}

void CNEOHud_Compass::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
}

inline void CNEOHud_Compass::DrawDebugCompass(void)
{
	// Direction in -180 to 180
	float angle = -1 * m_pOwner->EyeAngles()[YAW];

	// Bring us back to safety
	if (angle > 180)
	{
		angle -= 360;
	}
	else if (angle < -180)
	{
		angle += 360;
	}

	// Char representation of the compass strip
	const char rose[] = "N -- ne -- E -- se -- S -- sw -- W -- nw -- ";

	// One compass tick represents this many degrees of rotation
	const float unitAccuracy = 360.0f / sizeof(rose);

	// How many characters should be visible around each side of the needle position
	const int numCharsVisibleAroundNeedle = 6;

	// Get index offset for this angle's compass position
	int offset = RoundFloatToInt((angle / unitAccuracy)) - numCharsVisibleAroundNeedle;
	if (offset < 0)
	{
		offset += sizeof(rose);
	}

	// Both sides + center + terminator
	char compass[numCharsVisibleAroundNeedle * 2 + 2];
	int i;
	for (i = 0; i < sizeof(compass) - 1; i++)
	{
		// Get our index by circling around the compass strip.
		// We do modulo -1, because sizeof would land us on NULL
		// and terminate the string early.
		const int wrappedIndex = (offset + i) % (sizeof(rose) - 1);

		compass[i] = rose[wrappedIndex];
	}
	// Finally, make sure we have a null terminator
	compass[i] = '\0';

	// Draw the compass for this frame
	debugoverlay->AddScreenTextOverlay(
		neo_cl_hud_debug_compass_pos_x.GetFloat(),
		neo_cl_hud_debug_compass_pos_y.GetFloat(),
		gpGlobals->frametime,
		neo_cl_hud_debug_compass_color_r.GetInt(),
		neo_cl_hud_debug_compass_color_g.GetInt(),
		neo_cl_hud_debug_compass_color_b.GetInt(),
		neo_cl_hud_debug_compass_color_a.GetInt(),
		compass);
}
