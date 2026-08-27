#include "neo_hud_place_name.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>
#include "c_neo_player.h"
#include "view.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_PlaceName, neo_place_name);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(PlaceName, 0.1)

static CNEOHud_PlaceName *g_PlaceName = nullptr;

ConVar cl_neo_hud_place_names_depth_test("cl_neo_hud_place_names_depth_test", "0", FCVAR_ARCHIVE, "Depth test in-world nearby place names", true, 0.0f, true, 1.0f,
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		if (!g_PlaceName)
			return;

		IMaterial* textMaterial = g_PlaceName->GetFont();
		if (!textMaterial)
			return;

		textMaterial->SetMaterialVarFlag( MATERIAL_VAR_IGNOREZ, !cl_neo_hud_place_names_depth_test.GetBool() );
});

const char* TEXT_MATERIAL = "editor/worldtext_9";
CNEOHud_PlaceName::CNEOHud_PlaceName(const char *pElementName, vgui::Panel *parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_PLACE_NAME;

	if (parent) {
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}
	
	SetVisible(true);
	
	g_PlaceName = this;
    m_szPlaceName[0] = L'\0';

	PrecacheMaterial( TEXT_MATERIAL );
	m_Font.Init(TEXT_MATERIAL, TEXTURE_GROUP_PRECACHED, true);

	const PointWorldText foo = {"Important Place", {100, 100, 400}, &m_Font};
	places.AddToTail(foo);
}

CNEOHud_PlaceName::~CNEOHud_PlaceName()
{
	if (g_PlaceName == this)
	{
		g_PlaceName = nullptr;
	}
}

void CNEOHud_PlaceName::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int wide = 0, tall = 0;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

enum
{
	TEXTALIGN_LEFT = 0,
	TEXTALIGN_CENTER,
	TEXTALIGN_RIGHT
};
void CNEOHud_PlaceName::UpdateStateForNeoHudElementDraw()
{
	C_NEO_Player* pTargetPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pTargetPlayer)
	{
		return;
	}

	if (pTargetPlayer->IsPlayerDead())
	{
		if (const int observerMode = pTargetPlayer->GetObserverMode();
			observerMode == OBS_MODE_IN_EYE || observerMode == OBS_MODE_CHASE)
		{
			if (C_BaseEntity* pObserverTarget = pTargetPlayer->GetObserverTarget();
				pObserverTarget && pObserverTarget->IsPlayer())
			{
				pTargetPlayer = static_cast<C_NEO_Player*>(pObserverTarget);
			}
		}
	}

	V_snwprintf(m_szPlaceName, MAX_PLACE_NAME_LENGTH, L"%hs", pTargetPlayer->GetLastKnownPlaceName());
	switch (textXAlignment)
	{
		case TEXTALIGN_LEFT:
		default:
			textXOffset = 0;
			break;
		case TEXTALIGN_CENTER:
		case TEXTALIGN_RIGHT:
			int textWidth = 0, textHeight = 0;
			vgui::surface()->GetTextSize(textFont, m_szPlaceName, textWidth, textHeight);
			textXOffset = textXAlignment == TEXTALIGN_CENTER ? (wide / 2) - (textWidth / 2) : wide - textWidth;
			break;
	}
}

ConVar cl_neo_hud_curent_place_name_draw("cl_neo_hud_curent_place_name_draw", "1", FCVAR_ARCHIVE, "Draw the current place name", true, 0.0f, true, 1.0f);

static bool shouldDrawPlaceNames = false;
static ConCommand startshowplacenames("+showplacenames", [](const CCommand& args)->void {shouldDrawPlaceNames = true; });
static ConCommand endshowplacenames("-showplacenames", [](const CCommand& args)->void {shouldDrawPlaceNames = false; });

static float placeNamesRadiusSquared = 0.0f;
ConVar cl_neo_hud_place_names_radius("cl_neo_hud_place_names_radius", "2048", FCVAR_ARCHIVE, "Radius from the camera within which to draw the names of all nearby places", true, 0.0f, false, 0.0f,
	[](IConVar* var, const char* pOldValue, float flOldValue)->void{
		placeNamesRadiusSquared = pow(cl_neo_hud_place_names_radius.GetFloat(), 2);
});

void CNEOHud_PlaceName::DrawNeoHudElement()
{
	if (!ShouldDraw())
		return;

	if (cl_neo_hud_curent_place_name_draw.GetBool())
	{
		vgui::surface()->DrawSetTextFont(textFont);
		vgui::surface()->DrawSetTextColor(textColor);
		vgui::surface()->DrawSetTextPos(textXOffset, 0);
		vgui::surface()->DrawPrintText(m_szPlaceName, V_wcslen(m_szPlaceName));
	}
}

void CNEOHud_PlaceName::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

static float placeNameAlpha = 0.f;
void CNEOHud_PlaceName::DrawPlaceNames()
{
	constexpr int ANIMATION_SPEED = 2;
	if (shouldDrawPlaceNames)
	{
		placeNameAlpha = min(1.0f, placeNameAlpha + (gpGlobals->frametime * ANIMATION_SPEED));
	}
	else
	{
		placeNameAlpha = max(0.0f, placeNameAlpha - (gpGlobals->frametime * ANIMATION_SPEED));
	}
	if (placeNameAlpha)
	{
		for (PointWorldText place : places)
		{
			float alpha = placeNameAlpha;
			const float distanceSquared = MainViewOrigin().DistToSqr(place.GetAbsOrigin());
			if (distanceSquared > placeNamesRadiusSquared)
			{
				alpha *= 1.f - min(1.f, ((distanceSquared - placeNamesRadiusSquared) / placeNamesRadiusSquared));
			}

			place.DrawModel(alpha);
		}
	}
}

CNEOHud_PlaceName* GetPlaceName()
{
	return g_PlaceName;
}
