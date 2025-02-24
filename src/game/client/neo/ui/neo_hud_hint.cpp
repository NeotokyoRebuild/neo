#include "cbase.h"
#include "neo_hud_hint.h"
#include "neo_gamerules.h"

#include "iclientmode.h"
#include <vgui/ISurface.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

static ConVar cl_neo_showhints("cl_neo_showhints", "1", FCVAR_CLIENTDLL, "Toggle gameplay hints");

DECLARE_NAMED_HUDELEMENT(CNEOHud_Hint, neo_hint);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Hint, 0.1);

CNEOHud_Hint::CNEOHud_Hint(const char* pElementName, vgui::Panel* parent)
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

	for (int i = 0; i < 23; ++i)
	{
		m_pHintTexture[i] = surface()->CreateNewTextureID();
		Assert(m_pHintTexture[i] > 0);
		char textureFilePath[64];
		V_sprintf_safe(textureFilePath, "vgui/hud/hints/hint%d", i);
		surface()->DrawSetTextureFile(m_pHintTexture[i], textureFilePath, true, false);
	}

	surface()->DrawGetTextureSize(m_pHintTexture[0], m_iTextureWidth, m_iTextureHeight);

	SetVisible(true);

	SetHiddenBits(HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT);
}

void CNEOHud_Hint::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	int screenWidth, screenHeight;
	surface()->GetScreenSize(screenWidth, screenHeight);

	float xRatio = screenWidth / 640.f;
	float yRatio = screenHeight / 480.f;
	float finalRatio = xRatio / yRatio; // how much wider the screen is compared to how much taller it is

	SetBounds(xpos * finalRatio, ypos - ((wide * finalRatio) - wide), wide * finalRatio, tall * finalRatio);
}

void CNEOHud_Hint::DrawNeoHudElement()
{
	if (!cl_neo_showhints.GetBool())
	{
		return;
	}

	auto* localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!ShouldDraw() || (!localPlayer || localPlayer->IsObserver()))
	{
		return;
	}

	auto gameRules = static_cast<CNEORules*>(g_pGameRules);
	NeoRoundStatus roundStatus = gameRules->GetRoundStatus();
	if (roundStatus == NeoRoundStatus::PreRoundFreeze)
	{
		if (!m_bIsVisible)
		{
			m_bIsVisible = true; // Only do this once until next time we want to display a hint
			m_iSelectedTexture = RandomInt(0, 22);
		}

		surface()->DrawSetColor(255, 255, 255, 255);
		surface()->DrawSetTexture(m_pHintTexture[m_iSelectedTexture]);
		surface()->DrawTexturedRect(0, 0, GetWide(), GetTall());
	}
	else
	{
		m_bIsVisible = false;
	}
}

void CNEOHud_Hint::Paint()
{
	BaseClass::Paint();
	PaintNeoElement();
}

void CNEOHud_Hint::UpdateStateForNeoHudElementDraw()
{
}
