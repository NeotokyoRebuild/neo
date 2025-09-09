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

#include "c_team.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define UNICODE_NEO_COMPASS_SIZE_BYTES (UNICODE_NEO_COMPASS_STR_LENGTH * sizeof(wchar_t))

using vgui::surface;

ConVar cl_neo_hud_debug_compass_enabled("cl_neo_hud_debug_compass_enabled", "0", FCVAR_USERINFO | FCVAR_CHEAT,
	"Whether the Debug HUD compass is enabled or not.", true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_debug_compass_pos_x("cl_neo_hud_debug_compass_pos_x", "0.45", FCVAR_USERINFO | FCVAR_CHEAT,
	"Horizontal position of the Debug compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_debug_compass_pos_y("cl_neo_hud_debug_compass_pos_y", "0.925", FCVAR_USERINFO | FCVAR_CHEAT,
	"Vertical position of the Debug compass, in range 0 to 1.", true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_debug_compass_color_r("cl_neo_hud_debug_compass_color_r", "190", FCVAR_USERINFO | FCVAR_CHEAT,
	"Red color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_compass_color_g("cl_neo_hud_debug_compass_color_g", "185", FCVAR_USERINFO | FCVAR_CHEAT,
	"Green color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_compass_color_b("cl_neo_hud_debug_compass_color_b", "205", FCVAR_USERINFO | FCVAR_CHEAT,
	"Blue value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_compass_color_a("cl_neo_hud_debug_compass_color_a", "255", FCVAR_USERINFO | FCVAR_CHEAT,
	"Alpha color value of the Debug compass, in range 0 - 255.", true, 0.0f, true, 255.0f);

ConVar cl_neo_hud_rangefinder_enabled("cl_neo_hud_rangefinder_enabled", "1", FCVAR_ARCHIVE,
									  "Whether the rangefinder HUD is enabled or not.", true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_rangefinder_pos_frac_x("cl_neo_hud_rangefinder_pos_frac_x", "0.55", FCVAR_ARCHIVE,
										 "In fractional to the total screen width, the x-axis position of the rangefinder.",
										 true, 0.0f, true, 1.0f);
ConVar cl_neo_hud_rangefinder_pos_frac_y("cl_neo_hud_rangefinder_pos_frac_y", "0.55", FCVAR_ARCHIVE,
										 "In fractional to the total screen height, the y-axis position of the rangefinder.",
										 true, 0.0f, true, 1.0f);

DECLARE_NAMED_HUDELEMENT(CNEOHud_Compass, NHudCompass);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Compass, 0.00695)

CNEOHud_Compass::CNEOHud_Compass(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_COMPASS;

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

	m_hFont = 0;

	SetVisible(true);

	COMPILE_TIME_ASSERT(sizeof(m_wszCompassUnicode) == UNICODE_NEO_COMPASS_SIZE_BYTES);
	Assert(g_pVGuiLocalize);
	g_pVGuiLocalize->ConvertANSIToUnicode("\0", m_wszCompassUnicode, UNICODE_NEO_COMPASS_SIZE_BYTES);
}

void CNEOHud_Compass::Paint()
{
	PaintNeoElement();

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	BaseClass::Paint();
}

void CNEOHud_Compass::GetCompassUnicodeString(const float angle, wchar_t* outUnicodeStr) const
{
	// Char representation of the compass strip
	static constexpr char ROSE[] =
		"N                |                NE                |\
                E                |                SE                |\
                S                |                SW                |\
                W                |                NW                |\
                ";

	// One compass tick represents this many degrees of rotation
	const float unitAccuracy = 360.0f / sizeof(ROSE);

	// Get index offset for this angle's compass position
	int offset = RoundFloatToInt(angle / unitAccuracy) - UNICODE_NEO_COMPASS_VIS_AROUND;
	if (offset < 0)
	{
		offset += sizeof(ROSE);
	}

	// Both sides + center + terminator
	char compass[UNICODE_NEO_COMPASS_STR_LENGTH];
	int i;
	for (i = 0; i < UNICODE_NEO_COMPASS_STR_LENGTH - 1; i++)
	{
		// Get our index by circling around the compass strip.
		// We do modulo -1, because sizeof would land us on NULL
		// and terminate the string early.
		const int wrappedIndex = (offset + i) % (sizeof(ROSE) - 1);

		compass[i] = ROSE[wrappedIndex];
	}
	// Finally, make sure we have a null terminator
	compass[i] = '\0';

	g_pVGuiLocalize->ConvertANSIToUnicode(compass, outUnicodeStr, UNICODE_NEO_COMPASS_SIZE_BYTES);
}

static C_NEO_Player *GetFirstPersonPlayer()
{
	auto pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	C_NEO_Player *pFPPlayer = pLocalPlayer;
	if (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
	{
		auto *pTargetPlayer = dynamic_cast<C_NEO_Player *>(pLocalPlayer->GetObserverTarget());
		if (pTargetPlayer && !pTargetPlayer->IsObserver())
		{
			pFPPlayer = pTargetPlayer;
		}
	}
	return pFPPlayer;
}

void CNEOHud_Compass::UpdateStateForNeoHudElementDraw()
{
	auto pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	Assert(pLocalPlayer);

	static auto safeAngle = [](const float angle) -> float {
		if (angle > 180.0f)
		{
			return angle - 360.0f;
		}
		else if (angle < -180.0f)
		{
			return angle + 360.0f;
		}
		return angle;
	};

	// Direction in -180 to 180
	float angle = pLocalPlayer->EyeAngles()[YAW] * -1;
	angle = safeAngle(angle);
	angle += 180.0f;	// NEO NOTE (nullsystem): Adjust it again to match OG:NT's compass angle
	angle = safeAngle(angle);
	GetCompassUnicodeString(angle, m_wszCompassUnicode);

	if (cl_neo_hud_rangefinder_enabled.GetBool())
	{
		C_NEO_Player *pFPPlayer = GetFirstPersonPlayer();
		if (pFPPlayer->IsInAim())
		{
			// Update Rangefinder
			trace_t tr;
			Vector vecForward;
			AngleVectors(pFPPlayer->EyeAngles(), &vecForward);
			UTIL_TraceLine(pFPPlayer->EyePosition(), pFPPlayer->EyePosition() + (vecForward * MAX_TRACE_LENGTH),
						   MASK_SHOT, pFPPlayer, COLLISION_GROUP_NONE, &tr);
			const float flDist = METERS_PER_INCH * tr.startpos.DistTo(tr.endpos);
			if (flDist >= 999.0f || tr.surface.flags & (SURF_SKY | SURF_SKY2D))
			{
				V_swprintf_safe(m_wszRangeFinder, L"RANGE ---m");
			}
			else
			{
				V_swprintf_safe(m_wszRangeFinder, L"RANGE %3.0fm", flDist);
			}
		}
	}
}

void CNEOHud_Compass::DrawNeoHudElement(void)
{
	if (!ShouldDraw())
	{
		return;
	}

	if (m_showCompass)
	{
		DrawCompass();
	}

	if (cl_neo_hud_debug_compass_enabled.GetBool())
	{
		DrawDebugCompass();
	}

	if (cl_neo_hud_rangefinder_enabled.GetBool() && GetFirstPersonPlayer()->IsInAim())
	{
		surface()->DrawSetTextColor(COLOR_NEO_WHITE);
		surface()->DrawSetTextFont(m_hFont);
		surface()->DrawSetTextPos((m_resX * cl_neo_hud_rangefinder_pos_frac_x.GetFloat()),
								  (m_resY * cl_neo_hud_rangefinder_pos_frac_y.GetFloat()));
		surface()->DrawPrintText(m_wszRangeFinder, ARRAYSIZE(m_wszRangeFinder) - 1);
	}
}

void CNEOHud_Compass::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("scripts/HudLayout.res");

	m_hFont = pScheme->GetFont("NHudOCRSmall");
	m_savedXBoxWidth = 0;

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
	SetZPos(90);
}

void CNEOHud_Compass::DrawCompass() const
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	surface()->DrawSetTextFont(m_hFont);

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, m_wszCompassUnicode, fontWidth, fontHeight);

	if (m_savedXBoxWidth == 0)
	{
		// Using the fontHeight for padding works out
		m_savedXBoxWidth = fontWidth + fontHeight;
	}
	// These are the compass box dimension, just based on the font's dimensions
	const int xBoxWidth = m_savedXBoxWidth;
	const int yBoxHeight = fontHeight;

	const int resXHalf = m_resX / 2;
	const int xBoxWidthHalf = xBoxWidth / 2;
	const int margin = m_yFromBottomPos;

	DrawNeoHudRoundedBox(
		resXHalf - xBoxWidthHalf, m_resY - yBoxHeight - margin,
		resXHalf + xBoxWidthHalf, m_resY - margin,
		m_boxColor);

	// Draw the compass "needle"
	if (m_needleVisible)
	{
		static const Color COMPASS_NEEDLE_COLOR_GREEN{25, 255, 25, 150};
		static const Color COMPASS_NEEDLE_COLOR_BLUE{25, 25, 255, 150};
		static const Color COMPASS_NEEDLE_COLOR_WHITE{255, 255, 255, 150};
		Color needleColor = COMPASS_NEEDLE_COLOR_WHITE;
		if (m_needleColored)
		{
			const int playerTeam = player->GetTeamNumber();
			switch (playerTeam)
			{
			break; case TEAM_JINRAI: needleColor = COMPASS_NEEDLE_COLOR_GREEN;
			break; case TEAM_NSF: needleColor = COMPASS_NEEDLE_COLOR_BLUE;
			break; default: break;
			}
		}
		surface()->DrawSetColor(needleColor);
		surface()->DrawFilledRect(resXHalf - 1, m_resY - yBoxHeight - margin, resXHalf + 1, m_resY - margin);
	}

	surface()->DrawSetTextColor(COLOR_WHITE);
	surface()->DrawSetTextPos(resXHalf - fontWidth / 2, m_resY - fontHeight - margin);
	surface()->DrawPrintText(m_wszCompassUnicode, UNICODE_NEO_COMPASS_STR_LENGTH);

	// Print compass objective arrow
	if (m_objectiveVisible && !player->IsCarryingGhost())
	{
		// Point the objective arrow to the ghost, if it exists
		if (NEORules()->GhostExists() || NEORules()->JuggernautItemExists())
		{
			int ghostMarkerX, ghostMarkerY;
			bool ghostIsInView = false;
			if (NEORules()->GetGameType() != NEO_GAME_TYPE_JGR)
			{
				ghostIsInView = GetVectorInScreenSpace(NEORules()->GetGhostPos(), ghostMarkerX, ghostMarkerY);
			}
			else
			{
				ghostIsInView = GetVectorInScreenSpace(NEORules()->GetJuggernautMarkerPos(), ghostMarkerX, ghostMarkerY);
			}
			if (ghostIsInView) {
				// Print a unicode arrow to signify compass needle
				const wchar_t arrowUnicode[] = L"▼";

				ghostMarkerX = clamp(ghostMarkerX, resXHalf - xBoxWidthHalf, resXHalf + xBoxWidthHalf);

				const int ghosterTeam = NEORules()->GetGhosterTeam();
				const int ownTeam = player->GetTeam()->GetTeamNumber();

				const auto teamClr32 = player->GetTeam()->GetRenderColor();
				const Color teamColor = Color(teamClr32.r, teamClr32.g, teamClr32.b, teamClr32.a);

				const bool ghostIsBeingCarried = (ghosterTeam == TEAM_JINRAI || ghosterTeam == TEAM_NSF);
				const bool ghostIsCarriedByEnemyTeam = (ghosterTeam != ownTeam);

				surface()->DrawSetTextColor(ghostIsBeingCarried ? ghostIsCarriedByEnemyTeam ? COLOR_RED : teamColor : COLOR_WHITE);
				surface()->DrawSetTextPos(ghostMarkerX, m_resY - fontHeight - margin * 2.25f);
				surface()->DrawPrintText(arrowUnicode, Q_UnicodeLength(arrowUnicode));
			}
		}
	}

	static const Color FADE_END_COLOR(116, 116, 116, 255);
	DrawNeoHudRoundedBoxFaded(
		resXHalf - xBoxWidthHalf, m_resY - yBoxHeight - margin,
		resXHalf, m_resY - margin,
		FADE_END_COLOR, 255, 0, true,
		true, false, true, false);
	DrawNeoHudRoundedBoxFaded(
		resXHalf, m_resY - yBoxHeight - margin,
		resXHalf + xBoxWidthHalf, m_resY - margin,
		FADE_END_COLOR, 0, 255, true,
		false, true, false, true);
}

void CNEOHud_Compass::DrawDebugCompass() const
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	// Direction in -180 to 180
	float angle = -(player->EyeAngles()[YAW]);

	// Clamp in 180 turn range.
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
		cl_neo_hud_debug_compass_pos_x.GetFloat(),
		cl_neo_hud_debug_compass_pos_y.GetFloat(),
		gpGlobals->frametime,
		cl_neo_hud_debug_compass_color_r.GetInt(),
		cl_neo_hud_debug_compass_color_g.GetInt(),
		cl_neo_hud_debug_compass_color_b.GetInt(),
		cl_neo_hud_debug_compass_color_a.GetInt(),
		compass);
}
