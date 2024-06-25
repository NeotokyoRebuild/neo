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
#include "neo_version_info.h"

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
	m_hFontBuildInfo = scheme->GetFont("Default");

	if (!g_pVGuiLocalize->ConvertANSIToUnicode(GIT_HASH, m_wszBuildInfo, sizeof(m_wszBuildInfo)))
	{
		m_wszBuildInfo[0] = L'\0';
	}

	InvalidateLayout();

	SetVisible(neo_cl_hud_hta_enabled.GetBool());

	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION);
	engine->ClientCmd("hud_reloadscheme"); //NEO FIXME this reloads the scheme of all elements, is there a way to do it just for this one?
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

void CNEOHud_HTA::DrawBuildInfo() const
{
	surface()->DrawSetTextFont(m_hFontBuildInfo);
	int charWidth, charHeight;
	surface()->GetTextSize(m_hFontBuildInfo, L"a", charWidth, charHeight);

	const int x = xpos + 0.1f * charWidth;
	const int y = ypos - charHeight;

	// DIY text shadow for contrast
	surface()->DrawSetTextColor(COLOR_BLACK);
	surface()->DrawSetTextPos(x + 2, y + 2);
	surface()->DrawPrintText(m_wszBuildInfo, ARRAYSIZE(m_wszBuildInfo) - 1);

	surface()->DrawSetTextColor(COLOR_WHITE);
	surface()->DrawSetTextPos(x, y);
	surface()->DrawPrintText(m_wszBuildInfo, ARRAYSIZE(m_wszBuildInfo) - 1);
}

void CNEOHud_HTA::DrawHTA() const
{
	auto player = C_NEO_Player::GetLocalNEOPlayer();
	Assert(player);

	const Color textColor = COLOR_WHITE;
	auto textColorTransparent = Color(textColor.r(), textColor.g(), textColor.b(), 127);

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

	V_sprintf_safe(value_Integrity, "%d", health);
	if (playerIsNotSupport)
	{
		V_sprintf_safe(value_ThermOptic, "%d", thermopticValue);
		V_sprintf_safe(value_Aux, "%d", aux);
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

	Color box_color = Color(box_color_r, box_color_g, box_color_b, box_color_a);
	Color healthColor = Color(health_color_r, health_color_g, health_color_b, health_color_a);
	Color camoColor = Color(camo_color_r, camo_color_g, camo_color_b, camo_color_a);
	Color sprintColor = Color(sprint_color_r, sprint_color_g, sprint_color_b, sprint_color_a);
	DrawNeoHudRoundedBox(xpos, ypos, xpos + wide, ypos + tall, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);

	surface()->DrawSetTextFont(m_hFont);
	surface()->DrawSetTextColor(healthColor);
	surface()->DrawSetTextPos(healthtext_xpos + xpos, healthtext_ypos + ypos);
	surface()->DrawPrintText(L"INTEGRITY", 9);
	if (playerIsNotSupport)
	{
		surface()->DrawSetTextColor(camoColor);
		surface()->DrawSetTextPos(camotext_xpos + xpos, camotext_ypos + ypos);
		surface()->DrawPrintText(L"THERM-OPTIC", 11);
		surface()->DrawSetTextColor(sprintColor);
		surface()->DrawSetTextPos(sprinttext_xpos + xpos, sprinttext_ypos + ypos);
		surface()->DrawPrintText(L"AUX", 3);
	}

	int fontWidth, fontHeight;
	surface()->DrawSetTextColor(healthColor);
	surface()->GetTextSize(m_hFont, unicodeValue_Integrity, fontWidth, fontHeight);
	surface()->DrawSetTextPos(healthnum_xpos + xpos - fontWidth, healthnum_ypos + ypos);
	surface()->DrawPrintText(unicodeValue_Integrity, valLen_Integrity);
	if (playerIsNotSupport)
	{
		surface()->DrawSetTextColor(camoColor);
		surface()->GetTextSize(m_hFont, unicodeValue_ThermOptic, fontWidth, fontHeight);
		surface()->DrawSetTextPos(camonum_xpos + xpos - fontWidth, camonum_ypos + ypos);
		surface()->DrawPrintText(unicodeValue_ThermOptic, valLen_ThermOptic);
		surface()->DrawSetTextColor(sprintColor);
		surface()->GetTextSize(m_hFont, unicodeValue_Aux, fontWidth, fontHeight);
		surface()->DrawSetTextPos(sprintnum_xpos + xpos - fontWidth, sprintnum_ypos + ypos);
		surface()->DrawPrintText(unicodeValue_Aux, valLen_Aux);
	}

	// Integrity progress bar
	surface()->DrawSetColor(healthColor);
	surface()->DrawFilledRect(
		healthbar_xpos + xpos,
		healthbar_ypos + ypos,
		healthbar_xpos + xpos + (healthbar_w * (health / 100.0)),
		healthbar_ypos + ypos + healthbar_h);

	if (playerIsNotSupport)
	{
		// ThermOptic progress bar
		surface()->DrawSetColor(camoColor);
		surface()->DrawFilledRect(
			camobar_xpos + xpos,
			camobar_ypos + ypos,
			camobar_xpos + xpos + (camobar_w * (thermopticPercent / 100.0)),
			camobar_ypos + ypos + camobar_h);

		// AUX progress bar
		surface()->DrawSetColor(sprintColor);
		surface()->DrawFilledRect(
			sprintbar_xpos + xpos,
			sprintbar_ypos + ypos,
			sprintbar_xpos + xpos + (sprintbar_w * (aux / 100.0)),
			sprintbar_ypos + ypos + sprintbar_h);

		surface()->DrawSetColor(textColorTransparent);
		surface()->DrawOutlinedRect(
			camobar_xpos + xpos,
			camobar_ypos + ypos,
			camobar_xpos + xpos + camobar_w,
			camobar_ypos + ypos + camobar_h);

		surface()->DrawOutlinedRect(
			sprintbar_xpos + xpos,
			sprintbar_ypos + ypos,
			sprintbar_xpos + xpos + sprintbar_w,
			sprintbar_ypos + ypos + sprintbar_h);
	}

	surface()->DrawSetColor(textColorTransparent);
	surface()->DrawOutlinedRect(
		healthbar_xpos + xpos,
		healthbar_ypos + ypos,
		healthbar_xpos + xpos + healthbar_w,
		healthbar_ypos + ypos + healthbar_h);
}

void CNEOHud_HTA::DrawNeoHudElement()
{
	DrawBuildInfo(); // Always draw build info

	if (!ShouldDraw())
	{
		return;
	}

	if (neo_cl_hud_hta_enabled.GetBool())
	{
		DrawHTA();
	}
}
