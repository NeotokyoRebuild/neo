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

#include "ammodef.h"

#include "weapon_ghost.h"
#include "weapon_grenade.h"
#include "weapon_neobasecombatweapon.h"
#include "weapon_smokegrenade.h"
#include "weapon_supa7.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

ConVar cl_neo_hud_ammo_enabled("cl_neo_hud_ammo_enabled", "1", FCVAR_USERINFO,
	"Whether the HUD ammo is enabled or not.", true, 0, true, 1);

ConVar cl_neo_hud_debug_ammo_color_r("cl_neo_hud_debug_ammo_color_r", "190", FCVAR_USERINFO | FCVAR_CHEAT,
	"Red color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_ammo_color_g("cl_neo_hud_debug_ammo_color_g", "185", FCVAR_USERINFO | FCVAR_CHEAT,
	"Green color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_ammo_color_b("cl_neo_hud_debug_ammo_color_b", "205", FCVAR_USERINFO | FCVAR_CHEAT,
	"Blue value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);
ConVar cl_neo_hud_debug_ammo_color_a("cl_neo_hud_debug_ammo_color_a", "255", FCVAR_USERINFO | FCVAR_CHEAT,
	"Alpha color value of the ammo, in range 0 - 255.", true, 0.0f, true, 255.0f);

DECLARE_NAMED_HUDELEMENT(CNEOHud_Ammo, NHudWeapon);

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(Ammo, 0.00695);

CNEOHud_Ammo::CNEOHud_Ammo(const char* pElementName, vgui::Panel* parent)
	: CHudElement(pElementName), EditablePanel(parent, pElementName)
{
	SetAutoDelete(true);
	m_iHideHudElementNumber = NEO_HUD_ELEMENT_AMMO;

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

	SetVisible(cl_neo_hud_ammo_enabled.GetBool());

	SetHiddenBits(HIDEHUD_HEALTH | HIDEHUD_PLAYERDEAD | HIDEHUD_NEEDSUIT | HIDEHUD_WEAPONSELECTION);
}

void CNEOHud_Ammo::Paint()
{
	PaintNeoElement();
	BaseClass::Paint();
}

void CNEOHud_Ammo::UpdateStateForNeoHudElementDraw()
{
	Assert(C_NEO_Player::GetLocalNEOPlayer());
}

void CNEOHud_Ammo::ApplySchemeSettings(vgui::IScheme* pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);

	LoadControlSettings("scripts/HudLayout.res");

	m_hSmallTextFont = pScheme->GetFont("NHudOCRSmall");
	m_hBulletFont = pScheme->GetFont("NHudBullets");
	m_hTextFont = pScheme->GetFont("NHudOCR");

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_Ammo::DrawAmmo() const
{
	Assert(C_NEO_Player::GetLocalNEOPlayer());

	C_NEOBaseCombatWeapon* activeWep = dynamic_cast<C_NEOBaseCombatWeapon*>(C_NEO_Player::GetLocalNEOPlayer()->GetActiveWeapon());
	if (!activeWep)
		return;

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

	DrawNeoHudRoundedBox(xpos, ypos, xpos + wide, ypos + tall, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);

	surface()->DrawSetTextFont(m_hSmallTextFont);
	surface()->DrawSetTextColor(ammo_text_color);
	int fontWidth, fontHeight;
	surface()->GetTextSize(m_hSmallTextFont, unicodeWepName, fontWidth, fontHeight);
	surface()->DrawSetTextPos((text_xpos + xpos) - fontWidth, text_ypos + ypos);
	surface()->DrawPrintText(unicodeWepName, textLen);

	if(activeWep->IsGhost())
		return;

	if (activeWep->GetNeoWepBits() & NEO_WEP_BALC)
	{
		DrawHeatMeter(activeWep);
		return;
	}

	const int maxClip = activeWep->GetMaxClip1();
	if (maxClip == 0 || activeWep->IsMeleeWeapon())
		return;

	const int ammoCount = activeWep->m_iPrimaryAmmoCount;
	const int numClips = ceil(abs((float)ammoCount / activeWep->GetMaxClip1())); // abs because grenades return negative values (???) // casting division to float in case we have a half-empty mag, rounding up to show the half mag as one more mag
	const bool isSupa = activeWep->GetNeoWepBits() & NEO_WEP_SUPA7;
		
	if (activeWep->UsesClipsForAmmo1() && !(activeWep->GetNeoWepBits() & NEO_WEP_DETPACK)) {
		const int maxLen = 5;
		char clipsText[maxLen]{ '\0' };
		if(isSupa)
		{
			V_sprintf_safe(clipsText, "%d+%d", ammoCount, activeWep->m_iSecondaryAmmoCount.Get());
		} else
		{
			V_sprintf_safe(clipsText, "%d", numClips);
		}

		wchar_t unicodeClipsText[maxLen]{ L'\0' };
		g_pVGuiLocalize->ConvertANSIToUnicode(clipsText, unicodeClipsText, sizeof(unicodeClipsText));

		int clipsTextWidth, clipsTextHeight;
		surface()->GetTextSize(m_hTextFont, unicodeClipsText, clipsTextWidth, clipsTextHeight);
		surface()->DrawSetTextFont(m_hTextFont);

		surface()->GetTextSize(m_hTextFont, unicodeClipsText, fontWidth, fontHeight);
		surface()->DrawSetTextPos(digit2_xpos + xpos - fontWidth, digit2_ypos + ypos);
		surface()->DrawPrintText(unicodeClipsText, V_wcslen(unicodeClipsText));
	}

	const char* ammoChar = nullptr;
	int fireModeWidth = 0, fireModeHeight = 0;
	int magSizeMax = 0;
	int magSizeCurrent = 0;
		
	if (activeWep->UsesClipsForAmmo1() && !(activeWep->GetNeoWepBits() & NEO_WEP_THROWABLE)) 
	{
		char fireModeText[2]{ '\0' };

		ammoChar = activeWep->GetWpnData().szBulletCharacter;
		magSizeMax = activeWep->GetMaxClip1();
		magSizeCurrent = activeWep->Clip1();
			
		if(activeWep)
		{			
			if(activeWep->IsAutomatic())
				fireModeText[0] = 'j';
			else if(isSupa)
				if(dynamic_cast<CWeaponSupa7*>(activeWep)->SlugLoaded())
					fireModeText[0] = 'h';
				else
					fireModeText[0] = 'l';
			else
				fireModeText[0] = 'h';
				
 
			wchar_t unicodeFireModeText[2]{ L'\0' };
			g_pVGuiLocalize->ConvertANSIToUnicode(fireModeText, unicodeFireModeText, sizeof(unicodeFireModeText));

			surface()->DrawSetTextFont(m_hBulletFont);
			surface()->DrawSetTextPos(icon_xpos + xpos, icon_ypos + ypos);
			surface()->DrawPrintText(unicodeFireModeText, V_wcslen(unicodeFireModeText));

			surface()->GetTextSize(m_hBulletFont, unicodeFireModeText, fireModeWidth, fireModeHeight);
		}
	} else 
	{
		if(activeWep->GetNeoWepBits() & NEO_WEP_SMOKE_GRENADE)
		{
			ammoChar = "f";
			magSizeMax = magSizeCurrent = ammoCount;
		} else if(activeWep->GetNeoWepBits() & NEO_WEP_FRAG_GRENADE)
		{
			ammoChar = "g";
			magSizeMax = magSizeCurrent = ammoCount;
		}			
	}

	surface()->DrawSetTextColor(ammo_color);
	if (digit_as_number && activeWep->UsesClipsForAmmo1())
	{ // Draw bullets in magazine in number form
		surface()->DrawSetTextFont(m_hBulletFont);
		surface()->DrawSetTextPos(digit_xpos + xpos, digit_ypos + ypos);
		wchar_t bullets[22];
		V_swprintf_safe(bullets, L"%i/%i", magSizeCurrent, magSizeMax);
		surface()->DrawPrintText(bullets, (int)(magSizeCurrent == 0 ? 1 : log10(magSizeCurrent) + 1) + (int)(log10(magSizeMax) + 1) + 1);
		return;
	}

	if (ammoChar == nullptr)
		return;

	const int maxSpaceAvailableForBullets = digit_max_width;
	const int bulletWidth = surface()->GetCharacterWidth(m_hBulletFont, *ammoChar);
	const int plusWidth = surface()->GetCharacterWidth(m_hBulletFont, '+');
	const int maxBulletsWeCanDisplay = bulletWidth == 0 ? 0 : (maxSpaceAvailableForBullets / bulletWidth);

	if (maxBulletsWeCanDisplay == 0)
		return;

	const int maxBulletsWeCanDisplayWithPlus = bulletWidth == 0 ? 0 : ((maxSpaceAvailableForBullets - plusWidth) / bulletWidth);
	const bool bulletsOverflowing = maxBulletsWeCanDisplay < magSizeMax;

	if(bulletsOverflowing)
	{
		magSizeMax = maxBulletsWeCanDisplayWithPlus + 1;
	}

	constexpr auto maxBullets = 100; // PZ Mag Size

	char bullets[maxBullets + 1];
	magSizeMax = min(magSizeMax, sizeof(bullets)-1);
	int i;
	for(i = 0; i < magSizeMax; i++)
	{
		bullets[i] = *ammoChar;
	}
	bullets[i] = '\0';

	int magAmountToDrawFilled = magSizeCurrent;
		
	if(bulletsOverflowing)
	{
		if (magSizeMax > 0)
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
		
	wchar_t unicodeBullets[maxBullets + 1];
	g_pVGuiLocalize->ConvertANSIToUnicode(bullets, unicodeBullets, sizeof(unicodeBullets));
		
	if (magAmountToDrawFilled > 0)
	{
		surface()->DrawSetTextFont(m_hBulletFont);
		surface()->DrawSetTextPos(digit_xpos + xpos, digit_ypos + ypos);
		surface()->DrawPrintText(unicodeBullets, magAmountToDrawFilled);
	}

	if(maxClip > 0)
	{
		if (magSizeMax > 0) {
			surface()->DrawSetTextColor(emptied_ammo_color);
			surface()->DrawSetTextPos(digit_xpos + xpos + (bulletWidth * magAmountToDrawFilled), digit_ypos + ypos);
			surface()->DrawPrintText(&unicodeBullets[magAmountToDrawFilled], magSizeMax - magAmountToDrawFilled);
		}
	}
}

void CNEOHud_Ammo::DrawNeoHudElement()
{
	auto *localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!ShouldDraw() || (!localPlayer || localPlayer->IsObserver()))
	{
		return;
	}

	if (cl_neo_hud_ammo_enabled.GetBool())
	{
		DrawAmmo();
	}
}

void CNEOHud_Ammo::DrawHeatMeter(C_NEOBaseCombatWeapon* activeWep) const
{
	float flHeatAmount = (1.0f - (activeWep->GetPrimaryAmmoCount() / (float)activeWep->GetDefaultClip1()));
	Color heatColorLerp = LerpColor(ammo_color,heat_color, flHeatAmount);
	
	if (activeWep->GetPrimaryAmmoCount() == 0)
	{
		surface()->DrawSetTextFont(m_hBulletFont);
		surface()->DrawSetTextPos(icon_xpos + xpos, icon_ypos + ypos);
		surface()->DrawPrintText(L"HOT", 3);
	}

	surface()->DrawSetColor(heatColorLerp);
	surface()->DrawFilledRect(
		heatbar_xpos + xpos,
		heatbar_ypos + ypos,
		heatbar_xpos + xpos + (heatbar_w * flHeatAmount),
		heatbar_ypos + ypos + heatbar_h);

	surface()->DrawSetColor(ammo_text_color);
	surface()->DrawOutlinedRect(
		heatbar_xpos + xpos,
		heatbar_ypos + ypos,
		heatbar_xpos + xpos + heatbar_w,
		heatbar_ypos + ypos + heatbar_h);
}