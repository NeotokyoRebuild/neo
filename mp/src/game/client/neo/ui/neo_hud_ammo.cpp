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
#include "inttostr.h"

#include "ammodef.h"

#include "weapon_ghost.h"
#include "weapon_grenade.h"
#include "weapon_neobasecombatweapon.h"
#include "weapon_smokegrenade.h"
#include "weapon_supa7.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Ammo, 0.00695);

using vgui::surface;

ConVar neo_cl_hud_ammo_enabled("neo_cl_hud_ammo_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD ammo is enabled or not.", true, 0, true, 1);
ConVar neo_cl_hud_ammo_pos_x("neo_cl_hud_ammo_pos_x", "5", FCVAR_USERINFO,
	"HUD ammo X offset divisor.", true, 1, false, 0);
ConVar neo_cl_hud_ammo_pos_y("neo_cl_hud_ammo_pos_y", "5", FCVAR_USERINFO,
	"HUD ammo Y offset divisor.", true, 1, false, 0);

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

	m_hSmallTextFont = scheme->GetFont("NHudOCRSmall");
	m_hBulletFont = scheme->GetFont("NHudBullets");
	m_hTextFont = scheme->GetFont("NHudOCR");

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

	wchar_t sampleText[1] = { 'a' };
	surface()->GetTextSize(m_hSmallTextFont, sampleText, m_smallFontWidth, m_smallFontHeight);
	sampleText[0] = { 'h' };
	surface()->GetTextSize(m_hBulletFont, sampleText, m_bulletFontWidth, m_bulletFontHeight);
	m_fontWidth = surface()->GetCharacterWidth(m_hTextFont, '4'); //Widest character
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
	auto textColorTransparent = Color(textColor.r(), textColor.g(), textColor.b(), 127);

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

	const int margin = GetMargin(); //Get this from scheme?
	
	// These are the constant res based scalings of the NT ammo/health box dimensions.
	const int xPos1 = m_resX - neo_cl_hud_ammo_pos_x.GetInt();
	const int yPos1 = m_resY - neo_cl_hud_ammo_pos_y.GetInt();
	const int xPos0 = xPos1 - ((m_resX * 0.2375) + (margin * 2));
	const int yPos0 = yPos1 - (((m_bulletFontHeight * 0.8) + m_smallFontHeight + margin * 2) + (margin * 2));
	
	
	DrawNeoHudRoundedBox(xPos0, yPos0, xPos1, yPos1);

	surface()->DrawSetTextFont(m_hSmallTextFont);
	surface()->DrawSetTextColor(textColorTransparent);
	int weaponNamePixelWidth, weaponNamePixelHeight;
	surface()->GetTextSize(m_hSmallTextFont, unicodeWepName, weaponNamePixelWidth, weaponNamePixelHeight);
	surface()->DrawSetTextPos(xPos1 - ((margin * 2) + weaponNamePixelWidth + m_fontWidth / 2), yPos0 + (margin / 2));
	surface()->DrawPrintText(unicodeWepName, textLen);

	if(dynamic_cast<C_WeaponGhost*> (activeWep))
	{
		return;
	}

	const int maxClip = activeWep->GetMaxClip1();
	if (maxClip != 0 && !activeWep->IsMeleeWeapon())
	{
		const auto ammo = GetAmmoDef()->GetAmmoOfIndex(activeWep->GetPrimaryAmmoType());
		const int ammoCount = activeWep->GetOwner()->GetAmmoCount(ammo->pName);
		const int numClips = ceil(abs((float)ammoCount / activeWep->GetMaxClip1())); // abs because grenades return negative values (???) // casting division to float in case we have a half-empty mag, rounding up to show the half mag as one more mag
		const auto isSupa = dynamic_cast<CWeaponSupa7*>(activeWep);
		
		const int maxLen = 5;
		char clipsText[maxLen]{ '\0' };
		if(isSupa)
		{
			const auto secondaryAmmo = GetAmmoDef()->GetAmmoOfIndex(activeWep->GetSecondaryAmmoType());
			snprintf(clipsText, 10, "%d+%d", ammoCount, activeWep->GetOwner()->GetAmmoCount(secondaryAmmo->pName));
		} else
		{
			snprintf(clipsText, 10, "%d", numClips);
		}
		textLen = V_strlen(clipsText);
		wchar_t unicodeClipsText[maxLen]{ L'\0' };
		g_pVGuiLocalize->ConvertANSIToUnicode(clipsText, unicodeClipsText, sizeof(unicodeClipsText));

		int clipsTextWidth, clipsTextHeight;
		surface()->GetTextSize(m_hTextFont, unicodeClipsText, clipsTextWidth, clipsTextHeight);
		surface()->DrawSetTextFont(m_hTextFont);
		surface()->DrawSetTextPos(xPos1 - (clipsTextWidth + margin), yPos0 + (margin * 2) + m_smallFontHeight);
		surface()->DrawPrintText(unicodeClipsText, textLen);

		const auto neoWep = dynamic_cast<C_NEOBaseCombatWeapon*> (activeWep);
		
		char* ammoChar = nullptr;
		int fireModeWidth = 0, fireModeHeight = 0;
		int magSizeMax = 0;
		int magSizeCurrent = 0;
		
		if (activeWep->UsesClipsForAmmo1())
		{
			char fireModeText[2]{ L'\0' };

			ammoChar = const_cast<char*>(activeWep->GetWpnData().szBulletCharacter);
			magSizeMax = activeWep->GetMaxClip1();
			magSizeCurrent = activeWep->Clip1();
			
			if(neoWep)
			{			
				if(neoWep->IsAutomatic())
					fireModeText[0] = 'j';
				else if(isSupa)
					if(isSupa->SlugLoaded())
						fireModeText[0] = 'h';
					else
						fireModeText[0] = 'l';
				else
					fireModeText[0] = 'h';
				
 
				wchar_t unicodeFireModeText[2]{ L'\0' };
				g_pVGuiLocalize->ConvertANSIToUnicode(fireModeText, unicodeFireModeText, sizeof(unicodeFireModeText));

				surface()->DrawSetTextFont(m_hBulletFont);
				surface()->DrawSetTextPos(xPos0 + margin, yPos0 + ((((yPos1 - margin) - yPos0) / 2) - ((m_bulletFontHeight * 0.8) / 2)));
				surface()->DrawPrintText(unicodeFireModeText, V_strlen(fireModeText));

				surface()->GetTextSize(m_hBulletFont, unicodeFireModeText, fireModeWidth, fireModeHeight);
			}
		} else
		{
			if(dynamic_cast<CWeaponSmokeGrenade*> (activeWep))
			{
				ammoChar = new char[2] { 'f', '\0' };
				magSizeMax = magSizeCurrent = ammoCount;
			} else if(dynamic_cast<CWeaponGrenade*> (activeWep))
			{
				ammoChar = new char[2] { 'g', '\0' };
				magSizeMax = magSizeCurrent = ammoCount;
			}			
		}

		auto maxSpaceAvaliableForBullets = (xPos1 - (margin + max(clipsTextWidth, m_fontWidth))) - (xPos0 + fireModeWidth + (margin * 2));
		auto bulletWidth = surface()->GetCharacterWidth(m_hBulletFont, *ammoChar);
		auto plusWidth = surface()->GetCharacterWidth(m_hBulletFont, '+');
		auto maxBulletsWeCanDisplay = maxSpaceAvaliableForBullets / bulletWidth;
		auto maxBulletsWeCanDisplayWithPlus = (maxSpaceAvaliableForBullets - plusWidth) / bulletWidth;
		auto bulletsOverflowing = maxBulletsWeCanDisplay < magSizeMax;

		if(bulletsOverflowing)
		{
			magSizeMax = maxBulletsWeCanDisplayWithPlus + 1;
		}

		magSizeMax = min(magSizeMax, 64);
		char bullets[64]{ '\0' };
		for(int i = 0; i < magSizeMax; i++)
		{
			bullets[i] = *ammoChar;
		}

		int magAmountToDrawFilled = magSizeCurrent;
		
		if(bulletsOverflowing)
		{
			bullets[magSizeMax - 1] = '+';

			if(maxClip == magSizeCurrent)
			{
				magAmountToDrawFilled = magSizeMax;
			} else if(magSizeMax - 1 < magSizeCurrent)
			{
				magAmountToDrawFilled = magSizeMax - 1;
			} else
			{
				magAmountToDrawFilled = magSizeCurrent;
			}
		}
		
		wchar_t unicodeBullets[64];
		g_pVGuiLocalize->ConvertANSIToUnicode(bullets, unicodeBullets, sizeof(unicodeBullets));
		
		surface()->DrawSetTextFont(m_hBulletFont);
		surface()->DrawSetTextPos(xPos0 + fireModeWidth + (margin * 2), (yPos0 + margin + m_smallFontHeight) - (m_bulletFontHeight * 0.2));
		surface()->DrawPrintText(unicodeBullets, magAmountToDrawFilled);

		if(maxClip > 0)
		{
			surface()->DrawSetColor(textColor);
			surface()->DrawSetTextPos(xPos0 + fireModeWidth + (margin * 2), (yPos0 + margin + m_smallFontHeight) - (m_bulletFontHeight * 0.2));
			surface()->DrawPrintText(unicodeBullets, magSizeMax);
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
