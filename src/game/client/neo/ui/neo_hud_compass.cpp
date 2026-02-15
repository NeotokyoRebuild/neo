#include "cbase.h"
#include "neo_hud_compass.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "view.h"

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

using vgui::surface;

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
}

void CNEOHud_Compass::Paint()
{
	PaintNeoElement();

	SetFgColor(Color(0, 0, 0, 0));
	SetBgColor(Color(0, 0, 0, 0));

	BaseClass::Paint();
}

static C_NEO_Player *GetFirstPersonPlayer()
{
	C_NEO_Player *pFPPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (pFPPlayer->GetObserverMode() == OBS_MODE_IN_EYE)
	{
		auto *pTargetPlayer = dynamic_cast<C_NEO_Player *>(pFPPlayer->GetObserverTarget());
		if (pTargetPlayer && !pTargetPlayer->IsObserver())
		{
			pFPPlayer = pTargetPlayer;
		}
	}
	return pFPPlayer;
}

static float safeAngle(float angle) {
	while (angle < -180) {
		angle += 360;
	}
	while (angle >= 180) {
		angle -= 360;
	}
	return angle;
}

void CNEOHud_Compass::UpdateStateForNeoHudElementDraw()
{
	auto pFPPlayer = GetFirstPersonPlayer();
	Assert(pFPPlayer);

	// Point the objective arrow to the relevant objective, if it exists
	if (NEORules()->GhostExists() || NEORules()->GetJuggernautMarkerPos() != vec3_origin)
	{
		const Vector objPos = NEORules()->GetGameType() == NEO_GAME_TYPE_JGR ? NEORules()->GetJuggernautMarkerPos() : NEORules()->GetGhostPos();
		const Vector objVec = objPos - MainViewOrigin();
		const float objYaw = RAD2DEG(atan2f(objVec.y, objVec.x));
		float objAngle = safeAngle(- objYaw + MainViewAngles()[YAW]);
		m_objAngle = Clamp(objAngle, -(float)m_fov / 2, (float)m_fov / 2);
	}

	if (cl_neo_hud_rangefinder_enabled.GetBool())
	{
		if (pFPPlayer->IsInAim())
		{
			// Update Rangefinder
			trace_t tr;
			Vector vecForward;
			AngleVectors(MainViewAngles(), &vecForward);
			UTIL_TraceLine(MainViewOrigin(), MainViewOrigin() + (vecForward * MAX_TRACE_LENGTH),
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

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
	SetZPos(90);
	m_fov = Clamp(m_fov, 45, 360);
	m_fadeExp = Clamp(m_fadeExp, 0, 10);
}

void CNEOHud_Compass::DrawCompass() const
{
	static const wchar_t* ROSE[] = {
		L"S", L"|", L"SW", L"|", L"W", L"|", L"NW", L"|", L"N", L"|", L"NE", L"|", L"E", L"|", L"SE", L"|",
	};

	auto player = GetFirstPersonPlayer();
	Assert(player);

	surface()->DrawSetTextFont(m_hFont);

	DrawNeoHudRoundedBox(
		m_xPos, m_yPos,
		m_xPos + m_width, m_yPos + m_height,
		m_boxColor);

	const float angle = -MainViewAngles()[YAW];
	const int steps = ARRAYSIZE(ROSE);
	for (int i = 0; i < steps; i += m_separators ? 1 : 2) {
		const float stepAngle = (float)(i * 360) / steps;
		float drawAngle = safeAngle(stepAngle - angle + (float)m_fov / 2);
		if (drawAngle < 0) {
			drawAngle += 360;
		}
		if (drawAngle >= m_fov) {
			continue;
		}
		const float proportion = drawAngle / m_fov;
		const float alpha = !m_fadeExp ? 1 : 1 - pow(abs(2 * (proportion - 0.5)), m_fadeExp);
		surface()->DrawSetTextColor(m_textColor.r(), m_textColor.g(), m_textColor.b(), alpha * m_textColor.a());
		int labelWidth, labelHeight;
		surface()->GetTextSize(m_hFont, ROSE[i], labelWidth, labelHeight);
		const float padding = (float)labelHeight;
		surface()->DrawSetTextPos(m_xPos + padding + (m_width - padding * 2) * proportion - (float)labelWidth / 2, m_yPos + (float)m_height / 2 - (float)labelHeight / 2);
		surface()->DrawPrintText(ROSE[i], Q_UnicodeLength(ROSE[i]));	
	}

	// Print compass objective arrow
	if (m_objectiveVisible && !player->IsObjective())
	{
		// Point the objective arrow to the relevant objective, if it exists
		if (NEORules()->GhostExists() || NEORules()->GetJuggernautMarkerPos() != vec3_origin)
		{
			const float proportion = m_objAngle / m_fov + 0.5;
			
			// Print a unicode arrow to signify objective
			const wchar_t arrowUnicode[] = L"▼";

			const int ghosterTeam = NEORules()->GetGhosterTeam();
			const int ownTeam = player->GetTeam()->GetTeamNumber();

			const auto teamClr32 = player->GetTeam()->GetRenderColor();
			const Color teamColor = Color(teamClr32.r, teamClr32.g, teamClr32.b, teamClr32.a);

			const bool ghostIsBeingCarried = (ghosterTeam == TEAM_JINRAI || ghosterTeam == TEAM_NSF);
			const bool ghostIsCarriedByEnemyTeam = (ghosterTeam != ownTeam);

			surface()->DrawSetTextColor(ghostIsBeingCarried ? ghostIsCarriedByEnemyTeam ? COLOR_RED : teamColor : COLOR_WHITE);
			int labelWidth, labelHeight;
			surface()->GetTextSize(m_hFont, arrowUnicode, labelWidth, labelHeight);
			const float padding = (float)labelHeight;
			surface()->DrawSetTextPos(m_xPos + padding + (m_width - padding * 2) * proportion - (float)labelWidth / 2, m_yPos - labelHeight);
			surface()->DrawPrintText(arrowUnicode, Q_UnicodeLength(arrowUnicode));
		}
	}
}
