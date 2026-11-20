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

// A round on the rail at full size, and the space between two rows of them
static constexpr int AMMO_ROUND_TALL_PX = 35;
static constexpr int AMMO_ROW_GAP_PX = 3;

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
	m_hMagCountFont = NeoHud::GetFont(pScheme, "NHudMagCount", m_hSmallTextFont);
	m_hMagCountStackFont = NeoHud::GetFont(pScheme, "NHudMagCountStack", m_hSmallTextFont);
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
		{L'd', "NHudReserveD"}, {L'f', "NHudReserveF"}, {L'g', "NHudReserveG"}, {L'm', "NHudReserveM"},
	};
	for (int i = 0; i < ARRAYSIZE(m_reserveGlyphFonts); ++i)
	{
		m_reserveGlyphFonts[i].wchBullet = RESERVE_GLYPH_FONTS[i].wchGlyph;
		m_reserveGlyphFonts[i].hFont = NeoHud::GetFont(pScheme, RESERVE_GLYPH_FONTS[i].pszFont);
	}
	m_hSlugStackFont = NeoHud::GetFont(pScheme, "NHudReserveMStack", GetReserveGlyphFont(L'm'));

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

NeoHud::InkRange CNEOHud_Ammo::DrawGlyphCount(const vgui::HFont hCountFont, const vgui::HFont hGlyphFont, const wchar_t wchGlyph, const wchar_t *pwszCount, const int iRight, const int yBaseline) const
{
	static constexpr int COUNT_GLYPH_GAP_PX = 4;

	const NeoHud::InkRange count = NeoHud::DrawNumberRight(hCountFont, iRight, yBaseline, pwszCount);
	if (hGlyphFont == vgui::INVALID_FONT)
	{
		return count;
	}
	const wchar_t wszGlyph[] = { wchGlyph, L'\0' };
	const NeoHud::InkRange glyph = NeoHud::DrawTextRight(hGlyphFont, count.iLeft - ScaledPx(COUNT_GLYPH_GAP_PX), yBaseline, wszGlyph);
	return NeoHud::InkRange{ glyph.iLeft, count.iRight };
}

void CNEOHud_Ammo::DrawSupaReserves(const int iShells, const int iSlugs, const int iRight, const int yRailBaseline) const
{
	static constexpr int STACK_LINE_PX = 21;

	wchar_t wszShells[8] = {};
	wchar_t wszSlugs[8] = {};
	V_swprintf_safe(wszShells, L"%d", iShells);
	V_swprintf_safe(wszSlugs, L"%d", iSlugs);

	surface()->DrawSetTextColor(count_color);

	// The two counts stack across the rail's height, the slugs on the rail's baseline and the shells above them.
	// The shells are the gun's own rounds, as the round count shows, so only the slugs carry their glyph.
	NeoHud::DrawNumberRight(m_hMagCountStackFont, iRight, yRailBaseline - ScaledPx(STACK_LINE_PX), wszShells);
	DrawGlyphCount(m_hMagCountStackFont, m_hSlugStackFont, L'm', wszSlugs, iRight, yRailBaseline);
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

void CNEOHud_Ammo::DrawRounds(const wchar_t wchBullet, const int iMagSizeMax, const int iMagSizeCurrent, const int yBaseline, const int iRailTall, const int iRows, const int iBatch) const
{
	const vgui::HFont hBulletFont = GetBulletFont(wchBullet);
	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(hBulletFont, wchBullet, a, b, c);
	const int iBulletWide = a + b + c;
	// The rail runs from the first round's left edge to the round count's right edge
	const int iRailLeft = digit_xpos + xpos;
	const int iRailWide = count_xpos - digit_xpos;
	if (iBulletWide <= 0 || iMagSizeMax <= 0 || iRailWide <= 0)
	{
		return;
	}

	// A space after each batch of rounds, as a share of a round's width, so the rail can be counted at a glance
	static constexpr float BATCH_GAP = 0.4f;
	// A row's width in rounds, spaces included
	const auto RowWide = [iBatch](const int iRounds)
	{
		return iRounds + ((iBatch > 0) ? (((iRounds - 1) / iBatch) * BATCH_GAP) : 0.0f);
	};

	// One row stands the whole mag at full size, or at the largest size that fits the rail. Two rows share
	// the height under the top row, each ending on a whole batch, and the lower row empties first. A round in
	// two rows stands this share of its row's height, which leaves room above each row.
	static constexpr float TWO_ROW_ROUND_SHARE = 0.8f;
	int iPerRow = iMagSizeMax;
	int iRowStep = 0;
	float flScale = 1.0f;
	if (iRows == 2 && iMagSizeMax > 1)
	{
		iPerRow = (iMagSizeMax + 1) / 2;
		if (iBatch > 0)
		{
			iPerRow = Min(iMagSizeMax, ((iPerRow + iBatch - 1) / iBatch) * iBatch);
		}
		const float flRowTall = (iRailTall - ScaledPx(AMMO_ROW_GAP_PX)) / 2.0f;
		flScale = Min(1.0f, flRowTall * TWO_ROW_ROUND_SHARE / ScaledPx(AMMO_ROUND_TALL_PX));
		iRowStep = RoundFloatToInt(flRowTall) + ScaledPx(AMMO_ROW_GAP_PX);
	}
	flScale = Min(flScale, iRailWide / (RowWide(iPerRow) * iBulletWide));
	// Smaller rounds stand on whole pixels, so every round lands on the pixel grid the same way: each round's ink
	// is a whole number of pixels wide, with a gap after it and a wider one after each batch. When a row would run
	// past the rail, the batch gaps close first, then the gaps between rounds, then the rounds narrow.
	static constexpr int SMALL_ROUND_GAP_PX = 2;
	static constexpr int SMALL_BATCH_GAP_PX = 4;
	int iPitch = iBulletWide;
	int iBatchGap = 0;
	if (flScale < 1.0f)
	{
		const int iBatchGaps = (iBatch > 0) ? ((iPerRow - 1) / iBatch) : 0;
		int iInkWide = Max(1, RoundFloatToInt(b * flScale));
		int iGap = ScaledPx(SMALL_ROUND_GAP_PX);
		iBatchGap = (iBatch > 0) ? ScaledPx(SMALL_BATCH_GAP_PX) : 0;
		const auto RowWideNeeded = [&]()
		{
			return ((iPerRow - 1) * (iInkWide + iGap)) + (iBatchGaps * iBatchGap) + iInkWide;
		};
		while (RowWideNeeded() > iRailWide)
		{
			if (iBatchGap > 1)
			{
				--iBatchGap;
			}
			else if (iGap > 1)
			{
				--iGap;
			}
			else if (iInkWide > 1)
			{
				--iInkWide;
			}
			else
			{
				break;
			}
		}
		iPitch = iInkWide + iGap;
		flScale = Min(flScale, static_cast<float>(iInkWide) / Max(1, b));
	}
	// One row of smaller rounds stands in the middle of the height a full-size round takes
	const int iSingleRowLift = (iRowStep == 0) ? RoundFloatToInt(ScaledPx(AMMO_ROUND_TALL_PX) * (1.0f - flScale) / 2.0f) : 0;
	const int y = NeoHud::GetTextY(hBulletFont, yBaseline);
	const float flAscent = static_cast<float>(yBaseline - y);

	surface()->DrawSetTextFont(hBulletFont);
	for (int i = 0; i < iMagSizeMax; ++i)
	{
		const int iColumn = (i < iPerRow) ? i : (i - iPerRow);
		surface()->DrawSetTextColor((i < iMagSizeCurrent) ? ammo_color : emptied_ammo_color);
		if (flScale >= 1.0f)
		{
			const float flLeft = iRailLeft + (iPitch * (RowWide(iColumn + 1) - 1.0f));
			surface()->DrawSetTextPos(RoundFloatToInt(flLeft) - a, y);
			NeoHud::DrawUnicodeChar(wchBullet);
			continue;
		}

		// The surface hands out the quad it would draw, which can be drawn at another size
		const int yRowBaseline = ((i < iPerRow) ? (yBaseline - iRowStep) : yBaseline) - iSingleRowLift;
		surface()->DrawSetTextPos(0, 0);
		vgui::CharRenderInfo info = {};
		if (!surface()->DrawGetUnicodeCharRenderInfo(wchBullet, info))
		{
			continue;
		}
		// The quad keeps whole-pixel corners and size, so each round samples the glyph alike
		const int xInk = iRailLeft + (iColumn * iPitch) + ((iBatch > 0) ? ((iColumn / iBatch) * iBatchGap) : 0);
		const Vector2D vecTopLeft = info.verts[0].m_Position;
		const Vector2D vecBottomRight = info.verts[1].m_Position;
		const int x0 = xInk + RoundFloatToInt((vecTopLeft.x - a) * flScale);
		const int y0 = yRowBaseline + RoundFloatToInt((vecTopLeft.y - flAscent) * flScale);
		info.verts[0].m_Position.Init(x0, y0);
		info.verts[1].m_Position.Init(x0 + RoundFloatToInt((vecBottomRight.x - vecTopLeft.x) * flScale),
			y0 + RoundFloatToInt((vecBottomRight.y - vecTopLeft.y) * flScale));
		surface()->DrawRenderCharFromInfo(info);
		if (NeoHud::IsDebugActive())
		{
			NeoHud::DebugScaledGlyph(hBulletFont, wchBullet, flScale, xInk - RoundFloatToInt(a * flScale), yRowBaseline,
				NeoHud::InkRange{ xInk, xInk + RoundFloatToInt(b * flScale) });
		}
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
	NeoHud::DebugBox("mag column", count_xpos + xpos, ypos, mag_xpos + xpos, ypos + tall);

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

	// The round count stands at the right of the top row, on the panel's right content edge where the mag count
	// ends, with the gun's round glyph before it. It keeps room for a full mag, so the name's room holds still
	// as it falls: a digit cell is the same width for every digit, so a full mag's ink starts that many cells
	// left of this count's.
	const bool bShowsCount = !bBalc && !bRoundsAsNumber && ammoChar != nullptr;
	int iNameRoomRight = count_xpos + xpos;
	if (bShowsCount)
	{
		wchar_t wszCount[8] = {};
		wchar_t wszFullCount[8] = {};
		V_swprintf_safe(wszCount, L"%d", magSizeCurrent);
		V_swprintf_safe(wszFullCount, L"%d", magSizeMax);
		surface()->DrawSetTextColor(count_color);
		const wchar_t wchRound = static_cast<wchar_t>(*ammoChar);
		const NeoHud::InkRange count = DrawGlyphCount(m_hCountFont, GetReserveGlyphFont(wchRound), wchRound, wszCount, mag_xpos + xpos, iTopRowBaseline);
		iNameRoomRight = count.iLeft - (NeoHud::GetNumberWide(m_hCountFont, wszFullCount) - NeoHud::GetNumberWide(m_hCountFont, wszCount));
	}

	// The rail has the height between the two baselines, less the room the top row's descent takes
	static constexpr int TOP_ROW_CLEAR_PX = 4;
	const int iRailTall = iRailBaseline - iTopRowBaseline - ScaledPx(TOP_ROW_CLEAR_PX);
	const bool bBulleted = activeWep->UsesClipsForAmmo1() && !(iNeoWepBits & NEO_WEP_THROWABLE);
	const int iRailRows = bBulleted ? activeWep->GetNEOWpnData().iHudRailRows : 1;
	const int iRailBatch = bBulleted ? activeWep->GetNEOWpnData().iHudRailBatch : 0;

	// The mag count stands at the end of the rack, level with the middle of the rail
	if (bHasAmmo && activeWep->UsesClipsForAmmo1() && !(iNeoWepBits & NEO_WEP_DETPACK))
	{
		if (iNeoWepBits & NEO_WEP_SUPA7)
		{
			DrawSupaReserves(ammoCount, activeWep->m_iSecondaryAmmoCount.Get(), mag_xpos + xpos, iRailBaseline);
		}
		else
		{
			// NOCR's numerals stand this share of the font's ascent tall
			static constexpr float NUMERAL_TALL = 0.74f;
			const int iRailMiddle = iRailBaseline - (((iRailRows == 2) ? iRailTall : ScaledPx(AMMO_ROUND_TALL_PX)) / 2);
			const int iNumeralTall = RoundFloatToInt(surface()->GetFontAscent(m_hMagCountFont, L'0') * NUMERAL_TALL);
			// Rounded up, so a part-filled mag shows as one more mag
			const int numClips = Ceil2Int(static_cast<float>(ammoCount) / maxClip);
			wchar_t unicodeClipsText[8]{ L'\0' };
			V_swprintf_safe(unicodeClipsText, L"%d", numClips);
			surface()->DrawSetTextColor(count_color);
			NeoHud::DrawNumberRight(m_hMagCountFont, mag_xpos + xpos, iRailMiddle + (iNumeralTall / 2), unicodeClipsText);
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
		DrawRounds(static_cast<wchar_t>(*ammoChar), magSizeMax, magSizeCurrent, iRailBaseline, iRailTall, iRailRows, iRailBatch);
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
		NeoHud::DrawTextRight(m_hStatusFont, mag_xpos + xpos, yBaseline, L"OVERHEAT", NeoHud::GetReadoutTracking(m_hStatusFont));
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