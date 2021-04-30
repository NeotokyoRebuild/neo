#include "cbase.h"
#include "neo_hud_ammo.h"

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

#include "ammodef.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Ammo, 0.00695);

using vgui::surface;

ConVar neo_cl_hud_ammo_enabled("neo_cl_hud_ammo_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD ammo is enabled or not.", true, 0, true, 1);
ConVar neo_cl_hud_ammo_pos_x("neo_cl_hud_ammo_pos_x", "15", FCVAR_USERINFO,
	"HUD ammo X offset divisor.", true, 1, false, 100);
ConVar neo_cl_hud_ammo_pos_y("neo_cl_hud_ammo_pos_y", "20", FCVAR_USERINFO,
	"HUD ammo Y offset divisor.", true, 1, false, 100);

ConVar neo_cl_hud_debug_ammo_color_r("neo_cl_hud_debug_ammo_color_r", "190", FCVAR_USERINFO | FCVAR_CHEAT,
	"Red color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_ammo_color_g("neo_cl_hud_debug_ammo_color_g", "185", FCVAR_USERINFO | FCVAR_CHEAT,
	"Green color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_ammo_color_b("neo_cl_hud_debug_ammo_color_b", "205", FCVAR_USERINFO | FCVAR_CHEAT,
	"Blue value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar neo_cl_hud_debug_ammo_color_a("neo_cl_hud_debug_ammo_color_a", "255", FCVAR_USERINFO | FCVAR_CHEAT,
	"Alpha color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);

CNEOHud_Ammo::CNEOHud_Ammo(const char* pElementName, vgui::Panel* parent)
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

	m_hTextFont = scheme->GetFont("NHudOCRSmall");
	m_hBulletFont = scheme->GetFont("NHudBullets");

	SetVisible(neo_cl_hud_ammo_enabled.GetBool());

	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION);
}

void CNEOHud_Ammo::Paint()
{
	PaintNeoElement();

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	BaseClass::Paint();
}

void CNEOHud_Ammo::UpdateStateForNeoHudElementDraw()
{
	Assert(C_NEO_Player::GetLocalNEOPlayer());
}

void CNEOHud_Ammo::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);

	SetRoundedCorners(PANEL_ROUND_CORNER_ALL); // FIXME
}

void CNEOHud_Ammo::DrawAmmo() const
{
	Assert(C_NEO_Player::GetLocalNEOPlayer());

	auto activeWep = C_NEO_Player::GetLocalNEOPlayer()->GetActiveWeapon();
	if (!activeWep)
	{
		return;
	}

	const Color textColor = COLOR_WHITE;

	const size_t maxWepnameLen = 64;
	char wepName[maxWepnameLen]{ '\0' };
	wchar_t unicodeWepName[maxWepnameLen]{ L'\0' };
	V_strcpy_safe(wepName, activeWep->GetPrintName());
	int textLen;
	for (textLen = 0; textLen < sizeof(wepName); ++textLen) {
		if (wepName[textLen] == 0)
			break;
		wepName[textLen] = toupper(wepName[textLen]);
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(wepName, unicodeWepName, sizeof(unicodeWepName));

	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hTextFont, unicodeWepName, fontWidth, fontHeight);

	// These are the constant res based scalings of the NT ammo/health box dimensions.
	const int xpos = m_resX - (m_resX * 0.2375);
	const int ypos = m_resY - (m_resY * (0.1 / 1.5));

	surface()->DrawSetColor(Color(116, 116, 116, 200));
	surface()->DrawFilledRect(xpos, ypos, m_resX, m_resY);

	surface()->DrawSetTextFont(m_hTextFont);
	surface()->DrawSetTextColor(textColor);
	surface()->DrawSetTextPos(m_resX - fontWidth * 1.5, ypos + fontHeight * 0.5);
	surface()->DrawPrintText(unicodeWepName, textLen);

	const int maxClip = activeWep->GetMaxClip1();
	if (maxClip != 0)
	{
		const auto ammo = GetAmmoDef()->GetAmmoOfIndex(activeWep->GetPrimaryAmmoType());
		const int ammoCount = activeWep->GetOwner()->GetAmmoCount(ammo->pName);
		const int numClips = abs(ammoCount / activeWep->GetMaxClip1()); // abs because grenades return negative values (???)
		
		// Render amount of clips remaining. Sort of an approximation, should revisit once unfinished mag "reload dumping" is implemented.
		if (numClips != 0)
		{
			const int maxLen = 4; // support a max of '999' clips, plus '\0'
			char clipsText[maxLen]{ '\0' };
			itoa(numClips, clipsText, 10);
			textLen = V_strlen(clipsText);
			wchar_t unicodeClipsText[maxLen]{ L'\0' };
			g_pVGuiLocalize->ConvertANSIToUnicode(clipsText, unicodeClipsText, sizeof(unicodeClipsText));

			surface()->DrawSetTextPos(m_resX - fontWidth * 1.5, ypos + fontHeight * 2.5);
			surface()->DrawPrintText(unicodeClipsText, textLen);
		}

		// Render the bullet icons representing the amount of bullets in current clip.
		if (ammoCount != 0 && activeWep->UsesClipsForAmmo1())
		{
			const int maxBulletsInClip = 63 + 1;
			char bullets[maxBulletsInClip]{ '\0' };
			for (int i = 0, numBulletsInCurClip = activeWep->Clip1(); i < maxBulletsInClip && numBulletsInCurClip != 0; ++i) {
				V_strcat_safe(bullets, "a");
				--numBulletsInCurClip;
			}
			wchar_t unicodeBullets[maxBulletsInClip]{ L'\0' };
			g_pVGuiLocalize->ConvertANSIToUnicode(bullets, unicodeBullets, sizeof(unicodeBullets));

			surface()->DrawSetTextFont(m_hBulletFont);
			surface()->DrawSetTextPos(xpos + 10, ypos + 10); // TODO: resolution scaling for the offsets here
			surface()->DrawPrintText(unicodeBullets, activeWep->Clip1());
		}
	}
}

void CNEOHud_Ammo::DrawNeoHudElement()
{
	if (!ShouldDraw())
	{
		return;
	}

	if (neo_cl_hud_ammo_enabled.GetBool())
	{
		DrawAmmo();
	}
}
