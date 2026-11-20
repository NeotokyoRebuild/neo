#include "cbase.h"
#include "neo_hud_ammo.h"
#include "neo_hud_draw.h"

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

ConVar cl_neo_hud_ammo_enabled("cl_neo_hud_ammo_enabled", "1", FCVAR_ARCHIVE,
	"Whether the HUD ammo is enabled or not.", true, 0, true, 1);

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

	m_hSmallTextFont = NeoHud::GetFont(pScheme, "NHudPanelText", NeoHud::GetFont(pScheme, "NHudOCRSmall", vgui::INVALID_FONT, false));
	m_hCountFont = NeoHud::GetFont(pScheme, "NHudAmmoCount", m_hSmallTextFont);
	m_hStatusFont = NeoHud::GetFont(pScheme, "NHudAmmoStatus", m_hSmallTextFont);
	m_hLabelFont = NeoHud::GetFont(pScheme, "NHudPanelLabel", m_hSmallTextFont);
	m_hLabelSmallFont = NeoHud::GetFont(pScheme, "NHudPanelLabelSmall", m_hLabelFont);
	m_hBulletFont = NeoHud::GetFont(pScheme, "NHudBullets");
	m_hFireModeFont = NeoHud::GetFont(pScheme, "NHudFireMode", NeoHud::GetFont(pScheme, "NHudBullets", vgui::INVALID_FONT, false));
	m_hThrowableIconFont = NeoHud::GetFont(pScheme, "NHudThrowableIcon", m_hFireModeFont);
	m_hWeaponIconFont = NeoHud::GetFont(pScheme, "NHudWeaponIcon", NeoHud::GetFont(pScheme, "NHudKillfeedIcons"));
	m_hWeaponIconSmallFont = NeoHud::GetFont(pScheme, "NHudWeaponIconSmall", m_hWeaponIconFont);

	static constexpr struct
	{
		wchar_t wchBullet;
		const char *pszFont;
	} BULLET_FONTS[ARRAYSIZE(m_bulletFonts)] = {
		{L'a', "NHudBulletsA"}, {L'b', "NHudBulletsB"}, {L'c', "NHudBullets"},
		{L'd', "NHudBulletsD"}, {L'f', "NHudBulletsF"}, {L'g', "NHudBulletsG"},
	};
	for (int i = 0; i < ARRAYSIZE(m_bulletFonts); ++i)
	{
		m_bulletFonts[i].wchBullet = BULLET_FONTS[i].wchBullet;
		m_bulletFonts[i].hFont = NeoHud::GetFont(pScheme, BULLET_FONTS[i].pszFont, m_hBulletFont);
	}

	static constexpr struct
	{
		wchar_t wchGlyph;
		const char *pszFont;
	} RESERVE_GLYPH_FONTS[ARRAYSIZE(m_reserveGlyphFonts)] = {
		{L'a', "NHudReserveA"}, {L'b', "NHudReserveB"}, {L'c', "NHudReserveC"},
		{L'd', "NHudReserveD"}, {L'm', "NHudReserveM"},
	};
	for (int i = 0; i < ARRAYSIZE(m_reserveGlyphFonts); ++i)
	{
		m_reserveGlyphFonts[i].wchBullet = RESERVE_GLYPH_FONTS[i].wchGlyph;
		m_reserveGlyphFonts[i].hFont = NeoHud::GetFont(pScheme, RESERVE_GLYPH_FONTS[i].pszFont);
	}

	surface()->GetScreenSize(m_resX, m_resY);
	SetBounds(0, 0, m_resX, m_resY);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

vgui::HFont CNEOHud_Ammo::GetBulletFont(const wchar_t wchBullet) const
{
	for (const BulletFont &bulletFont : m_bulletFonts)
	{
		if (bulletFont.wchBullet == wchBullet)
		{
			return bulletFont.hFont;
		}
	}
	return m_hBulletFont;
}

vgui::HFont CNEOHud_Ammo::GetReserveGlyphFont(const wchar_t wchGlyph) const
{
	for (const BulletFont &reserveGlyphFont : m_reserveGlyphFonts)
	{
		if (reserveGlyphFont.wchBullet == wchGlyph)
		{
			return reserveGlyphFont.hFont;
		}
	}
	return vgui::INVALID_FONT;
}

void CNEOHud_Ammo::DrawWeaponIcon(const C_NEOBaseCombatWeapon *activeWep, const int yBaseline) const
{
	const NEO_WEP_BITS_UNDERLYING_TYPE iNeoWepBits = activeWep->GetNeoWepBits();

	wchar_t wchIcon = L'\0';
	vgui::HFont hIconFont = m_hFireModeFont;
	if (activeWep->IsGhost())
	{
		// Uses the kill feed icon below
	}
	else if ((activeWep->UsesClipsForAmmo1() && !(iNeoWepBits & NEO_WEP_THROWABLE)) || (iNeoWepBits & NEO_WEP_BALC))
	{
		if (activeWep->IsAutomatic())
		{
			wchIcon = L'j';
		}
		else if ((iNeoWepBits & NEO_WEP_SUPA7) && !assert_cast<const CWeaponSupa7 *>(activeWep)->SlugLoaded())
		{
			wchIcon = L'l';
		}
		else
		{
			wchIcon = L'h';
		}
	}
	else if (iNeoWepBits & NEO_WEP_SMOKE_GRENADE)
	{
		wchIcon = L'f';
		hIconFont = m_hThrowableIconFont;
	}
	else if (iNeoWepBits & NEO_WEP_FRAG_GRENADE)
	{
		wchIcon = L'g';
		hIconFont = m_hThrowableIconFont;
	}

	const int iColumnX = icon_xpos + xpos;
	surface()->DrawSetTextColor(ammo_text_color);

	if (wchIcon != L'\0')
	{
		// Fire mode icons sit on the rail's baseline. Grenades are shorter, so they are centered in the panel instead.
		static constexpr int FRAG_GRENADE_ICON_DROP_PX = -4;
		static constexpr int SMOKE_GRENADE_ICON_DROP_PX = -6;
		int iIconDrop = 0;
		switch (wchIcon)
		{
		case L'g': iIconDrop = ScaledPx(FRAG_GRENADE_ICON_DROP_PX); break;
		case L'f': iIconDrop = ScaledPx(SMOKE_GRENADE_ICON_DROP_PX); break;
		default: break;
		}
		NeoHud::DrawGlyphInColumn(hIconFont, iColumnX, icon_wide, yBaseline + iIconDrop, wchIcon);
		return;
	}

	// Knife, ghost, detpack: the same icon as the kill feed. Its widest icons need a smaller size to fit the column.
	wchar_t wszDeathIcon[2] = {};
	Q_UTF8ToUnicode(CNEOBaseCombatWeapon::GetDeathIcon(activeWep, DEATHICONTYPE_IDX), wszDeathIcon, sizeof(wszDeathIcon));
	if (wszDeathIcon[0] == L'\0')
	{
		return;
	}
	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(m_hWeaponIconFont, wszDeathIcon[0], a, b, c);
	if (b > icon_wide)
	{
		hIconFont = m_hWeaponIconSmallFont;
		surface()->GetCharABCwide(hIconFont, wszDeathIcon[0], a, b, c);
	}
	else
	{
		hIconFont = m_hWeaponIconFont;
	}
	// Centered in the panel, since these are far shorter than the column. Their middle is about a quarter of the font's height above the baseline.
	const int iIconBaseline = ypos + (tall / 2) + (surface()->GetFontTall(hIconFont) / 4);
	NeoHud::DrawGlyphInColumn(hIconFont, iColumnX, icon_wide, iIconBaseline, wszDeathIcon[0]);
}

int CNEOHud_Ammo::DrawReserve(const wchar_t wchGlyph, const wchar_t *pwszCount, const int iRight, const int yBaseline) const
{
	static constexpr int RESERVE_GLYPH_GAP_PX = 3;

	const NeoHud::InkRange count = NeoHud::DrawNumberRight(m_hSmallTextFont, iRight, yBaseline, pwszCount);

	const vgui::HFont hGlyphFont = GetReserveGlyphFont(wchGlyph);
	if (hGlyphFont == vgui::INVALID_FONT)
	{
		return count.iLeft;
	}
	const wchar_t wszGlyph[] = { wchGlyph, L'\0' };
	return NeoHud::DrawTextRight(hGlyphFont, count.iLeft - ScaledPx(RESERVE_GLYPH_GAP_PX), yBaseline, wszGlyph).iLeft;
}

int CNEOHud_Ammo::DrawSupaReserves(const int iShells, const int iSlugs, const int iRight, const int yBaseline) const
{
	wchar_t wszShells[8] = {};
	wchar_t wszSlugs[8] = {};
	V_swprintf_safe(wszShells, L"x%d", iShells);
	V_swprintf_safe(wszSlugs, L"x%d", iSlugs);

	surface()->DrawSetTextColor(count_color);

	// Shells take the place of other weapons' mag counts, with the slugs a count gap to their left
	const int iShellsLeft = DrawReserve(L'd', wszShells, iRight, yBaseline);
	return DrawReserve(L'm', wszSlugs, iShellsLeft - count_gap, yBaseline);
}

void CNEOHud_Ammo::DrawWeaponName(const wchar_t *pwszName, const int yBaseline, const int iRoomRight) const
{
	// The name matches the counts' size, unless it would come closer to them than the gap between counts
	const int iLeft = text_xpos + xpos;
	const int iRight = iRoomRight - count_gap;

	vgui::HFont hFont = m_hLabelFont;
	const NeoHud::InkRange ink = NeoHud::GetTextInk(hFont, pwszName, NeoHud::GetReadoutTracking(hFont));
	if (iLeft + (ink.iRight - ink.iLeft) > iRight)
	{
		hFont = m_hLabelSmallFont;
	}
	surface()->DrawSetTextColor(ammo_text_color);
	NeoHud::DrawTextLeft(hFont, iLeft, yBaseline, pwszName, NeoHud::GetReadoutTracking(hFont));
}

void CNEOHud_Ammo::DrawRounds(const wchar_t wchBullet, const int iMagSizeMax, const int iMagSizeCurrent, const int yBaseline) const
{
	const vgui::HFont hBulletFont = GetBulletFont(wchBullet);
	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(hBulletFont, wchBullet, a, b, c);
	const int iBulletWide = a + b + c;
	if (iBulletWide <= 0 || iMagSizeMax <= 0)
	{
		return;
	}

	// The rail runs from the first round's left edge to the round count's right edge
	const int iRailLeft = digit_xpos + xpos;
	const int iRailRight = count_xpos + xpos;
	const int iSlots = (iRailRight - iRailLeft) / iBulletWide;
	if (iSlots <= 0)
	{
		return;
	}

	// Rounds keep their natural spacing, so the rail's length shows the mag's size.
	// Only mags with more rounds than slots fill the row, by proportion.
	const bool bOverflow = iMagSizeMax > iSlots;
	const int iRounds = Min(iMagSizeMax, iSlots);
	const int iFilled = bOverflow ? Ceil2Int(iMagSizeCurrent * iSlots / static_cast<float>(iMagSizeMax)) : iMagSizeCurrent;

	const int iStartX = iRailLeft - a;
	const int iLastX = iRailRight - (a + b);
	const float flPitch = (bOverflow && iRounds > 1) ? (iLastX - iStartX) / static_cast<float>(iRounds - 1) : iBulletWide;
	const int y = NeoHud::GetTextY(hBulletFont, yBaseline);

	surface()->DrawSetTextFont(hBulletFont);
	for (int i = 0; i < iRounds; ++i)
	{
		surface()->DrawSetTextColor((i < iFilled) ? ammo_color : emptied_ammo_color);
		surface()->DrawSetTextPos(iStartX + RoundFloatToInt(flPitch * i), y);
		NeoHud::DrawUnicodeChar(wchBullet);
	}
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
	V_strcpy_safe(wepName, activeWep->GetNEOWpnData().szHudName);
	for (int textLen = 0; textLen < sizeof(wepName); ++textLen) {
		if (wepName[textLen] == 0)
			break;
		wepName[textLen] = toupper(wepName[textLen]);
	}
	g_pVGuiLocalize->ConvertANSIToUnicode(wepName, unicodeWepName, sizeof(unicodeWepName));

	DrawNeoHudRoundedBox(xpos, ypos, xpos + wide, ypos + tall, box_color, top_left_corner, top_right_corner, bottom_left_corner, bottom_right_corner);
	NeoHud::DebugBox("icon column", icon_xpos + xpos, ypos, icon_xpos + xpos + icon_wide, ypos + tall);
	NeoHud::DebugBox("text column", text_xpos + xpos, ypos, count_xpos + xpos, ypos + tall);

	// These baselines match the first and third rows of the vitals panel, whose text sits on the bottom of its bars
	const int iTopRowBaseline = ypos + text_ypos + row_h;
	const int iRailBaseline = ypos + digit_ypos + row_h;

	DrawWeaponIcon(activeWep, iRailBaseline);

	const NEO_WEP_BITS_UNDERLYING_TYPE iNeoWepBits = activeWep->GetNeoWepBits();
	const int maxClip = activeWep->GetMaxClip1();
	const bool bHasAmmo = !activeWep->IsGhost() && maxClip != 0 && !activeWep->IsMeleeWeapon();
	const int ammoCount = activeWep->m_iPrimaryAmmoCount;
	const bool bBalc = iNeoWepBits & NEO_WEP_BALC;
	const bool bRoundsAsNumber = digit_as_number && activeWep->UsesClipsForAmmo1();

	const char* ammoChar = nullptr;
	int magSizeMax = 0;
	int magSizeCurrent = 0;

	if (bHasAmmo)
	{
		if ((activeWep->UsesClipsForAmmo1() && !(iNeoWepBits & NEO_WEP_THROWABLE)) || bBalc)
		{
			ammoChar = activeWep->GetNEOWpnData().szBulletCharacter;
			magSizeMax = maxClip;
			magSizeCurrent = activeWep->Clip1();
		}
		else if (iNeoWepBits & NEO_WEP_SMOKE_GRENADE)
		{
			ammoChar = "f";
			magSizeMax = magSizeCurrent = ammoCount;
		}
		else if (iNeoWepBits & NEO_WEP_FRAG_GRENADE)
		{
			ammoChar = "g";
			magSizeMax = magSizeCurrent = ammoCount;
		}
	}

	// The counts stand at the right of the top row, the round count first, with an equal gap between each.
	// The round count keeps room for a full mag, so the others hold still as it falls: a digit cell is the
	// same width for every digit, so a full mag's ink starts that many cells left of this count's.
	const bool bShowsCount = !bBalc && !bRoundsAsNumber && ammoChar != nullptr;
	int iNameRoomRight = count_xpos + xpos;
	int iReservesRight = iNameRoomRight;
	if (bShowsCount)
	{
		wchar_t wszCount[8] = {};
		wchar_t wszFullCount[8] = {};
		V_swprintf_safe(wszCount, L"%d", magSizeCurrent);
		V_swprintf_safe(wszFullCount, L"%d", magSizeMax);
		surface()->DrawSetTextColor(count_color);
		const NeoHud::InkRange count = NeoHud::DrawNumberRight(m_hCountFont, count_xpos + xpos, iTopRowBaseline, wszCount);
		iNameRoomRight = count.iLeft - (NeoHud::GetNumberWide(m_hCountFont, wszFullCount) - NeoHud::GetNumberWide(m_hCountFont, wszCount));
		iReservesRight = iNameRoomRight - count_gap;
	}
	if (bHasAmmo && activeWep->UsesClipsForAmmo1() && !(iNeoWepBits & NEO_WEP_DETPACK))
	{
		if (iNeoWepBits & NEO_WEP_SUPA7)
		{
			iNameRoomRight = DrawSupaReserves(ammoCount, activeWep->m_iSecondaryAmmoCount.Get(), iReservesRight, iTopRowBaseline);
		}
		else
		{
			// Rounded up, so a part-filled mag shows as one more mag
			const int numClips = Ceil2Int(static_cast<float>(ammoCount) / maxClip);
			wchar_t unicodeClipsText[8]{ L'\0' };
			V_swprintf_safe(unicodeClipsText, L"x%d", numClips);
			surface()->DrawSetTextColor(count_color);
			iNameRoomRight = DrawReserve(static_cast<wchar_t>(*activeWep->GetNEOWpnData().szBulletCharacter), unicodeClipsText, iReservesRight, iTopRowBaseline);
		}
	}
	DrawWeaponName(unicodeWepName, iTopRowBaseline, iNameRoomRight);

	if (!bHasAmmo)
		return;

	if (bBalc)
	{
		DrawHeatMeter(activeWep, iTopRowBaseline);
		return;
	}

	if (bRoundsAsNumber)
	{ // Draw bullets in magazine in number form
		surface()->DrawSetTextColor(ammo_color);
		surface()->DrawSetTextFont(m_hBulletFont);
		surface()->DrawSetTextPos(digit_xpos + xpos, NeoHud::GetTextY(m_hBulletFont, iRailBaseline));
		wchar_t bullets[22];
		V_swprintf_safe(bullets, L"%i/%i", magSizeCurrent, magSizeMax);
		NeoHud::DrawPrintText(bullets, V_wcslen(bullets));
		return;
	}

	if (bShowsCount)
	{
		DrawRounds(static_cast<wchar_t>(*ammoChar), magSizeMax, magSizeCurrent, iRailBaseline);
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

void CNEOHud_Ammo::DrawHeatMeter(C_NEOBaseCombatWeapon* activeWep, const int yBaseline) const
{
	float flHeatAmount = (1.0f - (activeWep->GetPrimaryAmmoCount() / (float)activeWep->GetDefaultClip1()));
	Color heatColorLerp = LerpColor(ammo_color,heat_color, flHeatAmount);
	
	if (activeWep->GetPrimaryAmmoCount() == 0)
	{
		surface()->DrawSetTextColor(heat_color);
		NeoHud::DrawTextRight(m_hStatusFont, count_xpos + xpos, yBaseline, L"OVERHEAT", NeoHud::GetReadoutTracking(m_hStatusFont));
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