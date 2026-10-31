#include "neo_hud_walking_indicator.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>

#include "c_neo_player.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_WalkingIndicator, neo_walking_indicator);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(WalkingIndicator, 0.1)

CNEOHud_WalkingIndicator::CNEOHud_WalkingIndicator(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_WALKING_INDICATOR;

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	SetVisible(true);
}

const wchar_t notWalkingText[] = L"NOT ";
const wchar_t walkingText[] = L"WALKING";
int notWalkingTextWidth = 0;
int textHeight = 0;
void CNEOHud_WalkingIndicator::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int w,h;
	vgui::surface()->GetTextSize(font, notWalkingText, notWalkingTextWidth, h);
	int walkingTextWidth;
	vgui::surface()->GetTextSize(font, walkingText, walkingTextWidth, h);
	textHeight = vgui::surface()->GetFontTall(font);

	vgui::ipanel()->GetSize( GetVParent(), w, h );

	SetBounds(xpos, ypos, w - xpos, textHeight * 2);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

ConVar cl_neo_hud_rangefinder_enabled("cl_neo_hud_rangefinder_enabled", "1", FCVAR_ARCHIVE,
									  "Show the rangefinder", true, 0.0f, true, 1.0f);
wchar_t m_wszRangeFinder[11];
void CNEOHud_WalkingIndicator::UpdateStateForNeoHudElementDraw()
{
	C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalPlayer)
		return;

	C_NEO_Player* pTargetPlayer = pLocalPlayer->IsObserver() && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE) ? static_cast<C_NEO_Player *>(pLocalPlayer->GetObserverTarget())
																																										  : pLocalPlayer;
	if (!pTargetPlayer || !pTargetPlayer->IsAlive())
		return;

	if (cl_neo_hud_rangefinder_enabled.GetBool())
	{
		if (pTargetPlayer->IsInAim())
		{
			// Update Rangefinder
			trace_t tr;
			Vector vecForward;
			AngleVectors(MainViewAngles(), &vecForward);
			UTIL_TraceLine(MainViewOrigin(), MainViewOrigin() + (vecForward * MAX_TRACE_LENGTH),
						   MASK_SHOT, pTargetPlayer, COLLISION_GROUP_NONE, &tr);
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

void CNEOHud_WalkingIndicator::DrawRangeFinder(C_NEO_Player * pPlayer, int &offset)
{
	if (!cl_neo_hud_rangefinder_enabled.GetBool())
		return;

	if (pPlayer->IsInAim())
	{
		vgui::surface()->DrawSetTextColor(color);
		vgui::surface()->DrawSetTextFont(font);
		vgui::surface()->DrawSetTextPos(notWalkingTextWidth, offset);
		vgui::surface()->DrawPrintText(m_wszRangeFinder, ARRAYSIZE(m_wszRangeFinder) - 1);
	}
	offset += textHeight;
}

ConVar cl_neo_hud_walkindicator_enabled("cl_neo_hud_walkindicator_enabled", "1", FCVAR_ARCHIVE,
									  "Show the walking indicator", true, 0.0f, true, 1.0f);
void CNEOHud_WalkingIndicator::DrawWalkingIndicator(C_NEO_Player * pPlayer, int &offset)
{
	if (!cl_neo_hud_walkindicator_enabled.GetBool())
		return;

	static float animWalkKeyPressed = 0.f;
	const bool isWalking = pPlayer->IsWalking();
	static float animIsMakingFootsteps = 0.f;
	const bool isMakingFootsteps = pPlayer->IsMakingFootstepSounds() || !(pPlayer->GetFlags() & FL_ONGROUND);
	if (isWalking)
	{
		if (animWalkKeyPressed == 0.f)
		{
			if (isMakingFootsteps)
			{
				animIsMakingFootsteps = 1.f;
			}
			else
			{
				animIsMakingFootsteps = 0.f;
			}
		}
		animWalkKeyPressed += gpGlobals->frametime * animationSpeed;
	}
	else
	{
		animWalkKeyPressed -= gpGlobals->frametime * animationSpeed;
	}
	animWalkKeyPressed = Clamp(animWalkKeyPressed, 0.f, 1.f);

	if (animWalkKeyPressed == 0.f)
	{
		offset += textHeight;
		return;
	}

	if (isMakingFootsteps)
	{
		animIsMakingFootsteps += gpGlobals->frametime * animationSpeed;
	}
	else
	{
		animIsMakingFootsteps -= gpGlobals->frametime * animationSpeed;
	}
	animIsMakingFootsteps = Clamp(animIsMakingFootsteps, 0.f, 1.f);

	const float animWalkKeyPressedSmooth = animWalkKeyPressed * animWalkKeyPressed * (3.0f - 2.0f * animWalkKeyPressed);
	const float animIsMakingFootstepsSmoothStep = animIsMakingFootsteps * animIsMakingFootsteps * (3.0f - 2.0f * animIsMakingFootsteps);
	const float inverseAnimIsMakingFootstepsSmooth = 1.f - animIsMakingFootstepsSmoothStep;

	const Color finalColor = Color((color2.r() * animIsMakingFootstepsSmoothStep) + (color.r() * inverseAnimIsMakingFootstepsSmooth),
								   (color2.g() * animIsMakingFootstepsSmoothStep) + (color.g() * inverseAnimIsMakingFootstepsSmooth),
								   (color2.b() * animIsMakingFootstepsSmoothStep) + (color.b() * inverseAnimIsMakingFootstepsSmooth),
								   ((color2.a() * animIsMakingFootstepsSmoothStep) + (color.a() * inverseAnimIsMakingFootstepsSmooth)) * animWalkKeyPressedSmooth);
	vgui::surface()->DrawSetTextFont(font);
	if (animIsMakingFootsteps > 0.f)
	{
		vgui::surface()->DrawSetTextColor(finalColor.r(), finalColor.g(), finalColor.b(), finalColor.a() * animIsMakingFootstepsSmoothStep);
		vgui::surface()->DrawSetTextPos(0, offset);
		vgui::surface()->DrawPrintText(notWalkingText, ARRAYSIZE(notWalkingText) - 1);
	}
	vgui::surface()->DrawSetTextColor(finalColor);
	vgui::surface()->DrawSetTextPos(notWalkingTextWidth, offset);
	vgui::surface()->DrawPrintText(walkingText, ARRAYSIZE(walkingText) - 1);
	
	offset += textHeight;
}

void CNEOHud_WalkingIndicator::DrawNeoHudElement()
{
	if (!ShouldDraw() || !visible)
		return;

	C_NEO_Player* pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pLocalPlayer)
		return;

	C_NEO_Player* pTargetPlayer = pLocalPlayer->IsObserver() && (pLocalPlayer->GetObserverMode() == OBS_MODE_IN_EYE || pLocalPlayer->GetObserverMode() == OBS_MODE_CHASE) ? static_cast<C_NEO_Player *>(pLocalPlayer->GetObserverTarget())
																																										  : pLocalPlayer;
	if (!pTargetPlayer || !pTargetPlayer->IsAlive())
		return;

	int offset = 0;
	DrawRangeFinder(pTargetPlayer, offset);
	DrawWalkingIndicator(pTargetPlayer, offset);
}

void CNEOHud_WalkingIndicator::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}