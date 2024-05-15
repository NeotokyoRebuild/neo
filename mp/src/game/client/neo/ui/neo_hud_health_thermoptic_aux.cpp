#include "cbase.h"
#include "neo_hud_health_thermoptic_aux.h"

#include "c_neo_player.h"

#include "iclientmode.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>
#include <vgui_controls/ImagePanel.h>
#include <vgui/IScheme.h>

#include <engine/ivdebugoverlay.h>
#include "ienginevgui.h"

#include "neo_hud_elements.h"
#include "inttostr.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(HTA, 0.00695);

using vgui::surface;

ConVar neo_cl_hud_hta_enabled("neo_cl_hud_hta_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD Health/ThermOptic/AUX module is enabled or not.", true, 0, true, 1);

CNEOHud_HTA::CNEOHud_HTA(const char* pElementName, vgui::Panel* parent)
	: CHudElement(pElementName), Panel(parent, pElementName)
{
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
	vgui::IScheme* scheme = vgui::scheme()->GetIScheme(neoscheme);
	if (!scheme) {
		Assert(scheme);
		Error("CNEOHud_Ammo: Failed to load neoscheme\n");
	}

	m_hFont = scheme->GetFont("NHudOCRSmall");

	InvalidateLayout();

	SetVisible(neo_cl_hud_hta_enabled.GetBool());

	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION);
}

void CNEOHud_HTA::Paint()
{
	PaintNeoElement();

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	BaseClass::Paint();
}

void CNEOHud_HTA::UpdateStateForNeoHudElementDraw()
{
	Assert(C_NEO_Player::GetLocalNEOPlayer());
}

void CNEOHud_HTA::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
}

void CNEOHud_HTA::DrawHTA() const
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	const Color textColor = COLOR_WHITE;

	char value_Integrity[4]{ '\0' };
	char value_ThermOptic[4]{ '\0' };
	char value_Aux[4]{ '\0' };

	wchar_t unicodeValue_Integrity[4]{ L'\0' };
	wchar_t unicodeValue_ThermOptic[4]{ L'\0' };
	wchar_t unicodeValue_Aux[4]{ L'\0' };

	const int health = player->GetHealth();
	const int thermopticValue = static_cast<int>(roundf(player->m_HL2Local.m_cloakPower));
	const float thermopticPercent = player->CloakPower_CurrentVisualPercentage();
	const int aux = player->m_HL2Local.m_flSuitPower;
	const bool playerIsNotSupport = (player->GetClass() != NEO_CLASS_SUPPORT);

	inttostr(value_Integrity, 10, health);
	if (playerIsNotSupport)
	{
		inttostr(value_ThermOptic, 10, thermopticValue);
		inttostr(value_Aux, 10, aux);
	}

	const int valLen_Integrity = V_strlen(value_Integrity);
	const int valLen_ThermOptic = V_strlen(value_ThermOptic);
	const int valLen_Aux = V_strlen(value_Aux);

	g_pVGuiLocalize->ConvertANSIToUnicode(value_Integrity, unicodeValue_Integrity, sizeof(unicodeValue_Integrity));
	if (playerIsNotSupport)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(value_ThermOptic, unicodeValue_ThermOptic, sizeof(unicodeValue_ThermOptic));
		g_pVGuiLocalize->ConvertANSIToUnicode(value_Aux, unicodeValue_Aux, sizeof(unicodeValue_Aux));
	}

	DrawNeoHudRoundedBox(xpos, ypos, xpos + wide, ypos + tall);

	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextColor(textColor);
	surface()->DrawSetTextPos(healthtext_xpos + xpos, healthtext_ypos + ypos);
	surface()->DrawPrintText(L"INTEGRITY", 9);
	if (playerIsNotSupport)
	{
		surface()->DrawSetTextPos(camotext_xpos + xpos, camotext_ypos + ypos);
		surface()->DrawPrintText(L"THERM-OPTIC", 11);
		surface()->DrawSetTextPos(sprinttext_xpos + xpos, sprinttext_ypos + ypos);
		surface()->DrawPrintText(L"AUX", 3);
	}

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hFont, unicodeValue_Integrity, fontWidth, fontHeight);
	surface()->DrawSetTextPos(healthnum_xpos + xpos - fontWidth, healthnum_ypos + ypos);
	surface()->DrawPrintText(unicodeValue_Integrity, valLen_Integrity);
	if (playerIsNotSupport)
	{
		surface()->GetTextSize(m_hFont, unicodeValue_ThermOptic, fontWidth, fontHeight);
		surface()->DrawSetTextPos(camonum_xpos + xpos - fontWidth, camonum_ypos + ypos);
		surface()->DrawPrintText(unicodeValue_ThermOptic, valLen_ThermOptic);
		surface()->GetTextSize(m_hFont, unicodeValue_Aux, fontWidth, fontHeight);
		surface()->DrawSetTextPos(sprintnum_xpos + xpos - fontWidth, sprintnum_ypos + ypos);
		surface()->DrawPrintText(unicodeValue_Aux, valLen_Aux);
	}

	surface()->DrawSetColor(COLOR_WHITE);

	// Integrity progress bar
	surface()->DrawFilledRect(
		healthbar_xpos + xpos,
		healthbar_ypos + ypos,
		healthbar_xpos + xpos + (healthbar_w * (health / 100.0)),
		healthbar_ypos + ypos + healthbar_h);

	surface()->DrawOutlinedRect(
		healthbar_xpos + xpos,
		healthbar_ypos + ypos,
		healthbar_xpos + xpos + healthbar_w,
		healthbar_ypos + ypos + healthbar_h);

	if (playerIsNotSupport)
	{
		// ThermOptic progress bar
		surface()->DrawFilledRect(
			camobar_xpos + xpos,
			camobar_ypos + ypos,
			camobar_xpos + xpos + (camobar_w * (thermopticPercent / 100.0)),
			camobar_ypos + ypos + camobar_h);

		surface()->DrawOutlinedRect(
			camobar_xpos + xpos,
			camobar_ypos + ypos,
			camobar_xpos + xpos + camobar_w,
			camobar_ypos + ypos + camobar_h);

		// AUX progress bar
		surface()->DrawFilledRect(
			sprintbar_xpos + xpos,
			sprintbar_ypos + ypos,
			sprintbar_xpos + xpos + (sprintbar_w * (aux / 100.0)),
			sprintbar_ypos + ypos + sprintbar_h);


		surface()->DrawOutlinedRect(
			sprintbar_xpos + xpos,
			sprintbar_ypos + ypos,
			sprintbar_xpos + xpos + sprintbar_w,
			sprintbar_ypos + ypos + sprintbar_h);
	}
}

void CNEOHud_HTA::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	if (neo_cl_hud_hta_enabled.GetBool())
	{
		DrawHTA();
	}
}
