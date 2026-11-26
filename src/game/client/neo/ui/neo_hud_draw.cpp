#include "cbase.h"
#include "neo_hud_draw.h"
#include "neo_hud_draw_debug.h"

#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

namespace NeoHud
{
namespace
{
// The readout style's extra space between characters, as a share of the font's ascent
constexpr float READOUT_TRACKING = 0.09f;
// The reference resolution. Sizes in the HUD are written as they are at this screen height
constexpr int REFERENCE_SCREEN_TALL = 1080;

// The widest digit's advance and ink, which every digit is drawn in
struct DigitCell
{
	int iWide;
	int iInkWide;
};

DigitCell GetDigitCell(vgui::HFont font)
{
	DigitCell cell = {};
	for (wchar_t wch = L'0'; wch <= L'9'; ++wch)
	{
		int a = 0, b = 0, c = 0;
		surface()->GetCharABCwide(font, wch, a, b, c);
		cell.iWide = Max(cell.iWide, a + b + c);
		cell.iInkWide = Max(cell.iInkWide, b);
	}
	return cell;
}

bool IsDigit(const wchar_t wch)
{
	return wch >= L'0' && wch <= L'9';
}

InkRange Offset(const InkRange &ink, const int x)
{
	return InkRange{ ink.iLeft + x, ink.iRight + x };
}

int GetNumberWide(vgui::HFont font, const DigitCell &cell, const wchar_t *pwszNumber, const float flTracking)
{
	int iWide = 0;
	int iLength = 0;
	for (const wchar_t *pwch = pwszNumber; *pwch; ++pwch, ++iLength)
	{
		if (IsDigit(*pwch))
		{
			iWide += cell.iWide;
		}
		else
		{
			int a = 0, b = 0, c = 0;
			surface()->GetCharABCwide(font, *pwch, a, b, c);
			iWide += a + b + c;
		}
	}
	return iWide + RoundFloatToInt(flTracking * Max(0, iLength - 1));
}

// The visible span of a number, taking the widest digit in place of the digit at each end,
// so the edge of a column holds still whatever the digit is
InkRange GetNumberInk(vgui::HFont font, const DigitCell &cell, const wchar_t *pwszNumber, const float flTracking)
{
	InkRange ink = {};
	if (!pwszNumber[0])
	{
		return ink;
	}
	const int iLastLen = V_wcslen(pwszNumber) - 1;
	const int iWide = GetNumberWide(font, cell, pwszNumber, flTracking);

	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(font, pwszNumber[0], a, b, c);
	ink.iLeft = IsDigit(pwszNumber[0]) ? (cell.iWide - cell.iInkWide) / 2 : a;

	surface()->GetCharABCwide(font, pwszNumber[iLastLen], a, b, c);
	ink.iRight = IsDigit(pwszNumber[iLastLen])
		? iWide - cell.iWide + ((cell.iWide + cell.iInkWide) / 2)
		: iWide - c;
	return ink;
}

// Draws from the pen position x, which stands a left bearing short of the first ink
void DrawText(vgui::HFont font, const int x, const int yBaseline, const wchar_t *pwszText, const float flTracking)
{
	if (IsDebugActive())
	{
		DebugText(font, x, yBaseline, pwszText, flTracking, Offset(GetTextInk(font, pwszText, flTracking), x));
	}
	surface()->DrawSetTextFont(font);
	const int y = GetTextY(font, yBaseline);
	if (flTracking == 0.0f)
	{
		surface()->DrawSetTextPos(x, y);
		surface()->DrawPrintText(pwszText, V_wcslen(pwszText));
		return;
	}
	int xPen = x;
	for (int i = 0; pwszText[i]; ++i)
	{
		surface()->DrawSetTextPos(xPen + RoundFloatToInt(flTracking * i), y);
		surface()->DrawUnicodeChar(pwszText[i]);
		int a = 0, b = 0, c = 0;
		surface()->GetCharABCwide(font, pwszText[i], a, b, c);
		xPen += a + b + c;
	}
}

void DrawNumber(vgui::HFont font, const DigitCell &cell, const int xStart, const int yBaseline, const wchar_t *pwszNumber, const float flTracking)
{
	if (IsDebugActive())
	{
		DebugText(font, xStart, yBaseline, pwszNumber, flTracking, Offset(GetNumberInk(font, cell, pwszNumber, flTracking), xStart), cell.iWide);
	}
	const int y = GetTextY(font, yBaseline);
	surface()->DrawSetTextFont(font);
	int x = xStart;
	for (int i = 0; pwszNumber[i]; ++i)
	{
		const wchar_t wch = pwszNumber[i];
		const int xPen = x + RoundFloatToInt(flTracking * i);
		int a = 0, b = 0, c = 0;
		surface()->GetCharABCwide(font, wch, a, b, c);
		if (IsDigit(wch))
		{
			// DrawUnicodeChar applies the glyph's left bearing, so the pen stands that far short of the ink
			surface()->DrawSetTextPos(xPen + ((cell.iWide - b) / 2) - a, y);
			surface()->DrawUnicodeChar(wch);
			x += cell.iWide;
		}
		else
		{
			surface()->DrawSetTextPos(xPen, y);
			surface()->DrawUnicodeChar(wch);
			x += a + b + c;
		}
	}
}
}  // namespace

vgui::HFont GetFont(vgui::IScheme *pScheme, const char *pszName, const vgui::HFont hFallback, const bool bProportional)
{
	const vgui::HFont hFont = pScheme->GetFont(pszName, bProportional);
	if (hFont == vgui::INVALID_FONT)
	{
		Warning("NeoHud: ClientScheme.res has no font %s%s\n", pszName,
				(hFallback == vgui::INVALID_FONT) ? ", and it has no fallback" : ", using its fallback");
		return hFallback;
	}
	NameDebugFont(hFont, pszName);
	return hFont;
}

int ScaledPx(const int iReferencePx)
{
	int iScreenWide = 0, iScreenTall = 0;
	surface()->GetScreenSize(iScreenWide, iScreenTall);
	const int iScaled = (iReferencePx * iScreenTall) / REFERENCE_SCREEN_TALL;
	if (iScaled == 0 && iReferencePx != 0)
	{
		// A hairline stays a pixel at any resolution
		return (iReferencePx > 0) ? 1 : -1;
	}
	return iScaled;
}

int GetTextY(vgui::HFont font, const int yBaseline)
{
	return yBaseline - surface()->GetFontAscent(font, L'0');
}

int GetTextWide(vgui::HFont font, const wchar_t *pwszText, const float flTracking)
{
	Assert(pwszText);
	if (flTracking == 0.0f)
	{
		int iWide = 0, iTall = 0;
		surface()->GetTextSize(font, pwszText, iWide, iTall);
		return iWide;
	}
	int iWide = 0;
	const int iLength = V_wcslen(pwszText);
	for (int i = 0; i < iLength; ++i)
	{
		int a = 0, b = 0, c = 0;
		surface()->GetCharABCwide(font, pwszText[i], a, b, c);
		iWide += a + b + c;
	}
	return iWide + RoundFloatToInt(flTracking * Max(0, iLength - 1));
}

InkRange GetTextInk(vgui::HFont font, const wchar_t *pwszText, const float flTracking)
{
	Assert(pwszText);
	InkRange ink = {};
	if (!pwszText[0])
	{
		return ink;
	}
	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(font, pwszText[0], a, b, c);
	ink.iLeft = a;

	surface()->GetCharABCwide(font, pwszText[V_wcslen(pwszText) - 1], a, b, c);
	ink.iRight = GetTextWide(font, pwszText, flTracking) - c;
	return ink;
}

InkRange DrawTextLeft(vgui::HFont font, const int xInkLeft, const int yBaseline, const wchar_t *pwszText, const float flTracking)
{
	Assert(pwszText);
	const InkRange ink = GetTextInk(font, pwszText, flTracking);
	const int x = xInkLeft - ink.iLeft;
	DrawText(font, x, yBaseline, pwszText, flTracking);
	return Offset(ink, x);
}

InkRange DrawTextRight(vgui::HFont font, const int xInkRight, const int yBaseline, const wchar_t *pwszText, const float flTracking)
{
	Assert(pwszText);
	const InkRange ink = GetTextInk(font, pwszText, flTracking);
	const int x = xInkRight - ink.iRight;
	DrawText(font, x, yBaseline, pwszText, flTracking);
	return Offset(ink, x);
}

InkRange DrawTextCentered(vgui::HFont font, const int xCenter, const int yBaseline, const wchar_t *pwszText, const float flTracking)
{
	Assert(pwszText);
	const InkRange ink = GetTextInk(font, pwszText, flTracking);
	const int x = xCenter - ((ink.iLeft + ink.iRight) / 2);
	DrawText(font, x, yBaseline, pwszText, flTracking);
	return Offset(ink, x);
}

InkRange DrawGlyphInColumn(vgui::HFont font, const int xColumn, const int iColumnWide, const int yBaseline, const wchar_t wch)
{
	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(font, wch, a, b, c);
	const wchar_t wszGlyph[] = { wch, L'\0' };
	return DrawTextLeft(font, xColumn + ((iColumnWide - b) / 2), yBaseline, wszGlyph);
}

int GetNumberWide(vgui::HFont font, const wchar_t *pwszNumber, const float flTracking)
{
	Assert(pwszNumber);
	return GetNumberWide(font, GetDigitCell(font), pwszNumber, flTracking);
}

InkRange GetNumberInk(vgui::HFont font, const wchar_t *pwszNumber, const float flTracking)
{
	Assert(pwszNumber);
	return GetNumberInk(font, GetDigitCell(font), pwszNumber, flTracking);
}

InkRange DrawNumberLeft(vgui::HFont font, const int xInkLeft, const int yBaseline, const wchar_t *pwszNumber, const float flTracking)
{
	Assert(pwszNumber);
	const DigitCell cell = GetDigitCell(font);
	const InkRange ink = GetNumberInk(font, cell, pwszNumber, flTracking);
	const int x = xInkLeft - ink.iLeft;
	DrawNumber(font, cell, x, yBaseline, pwszNumber, flTracking);
	return Offset(ink, x);
}

InkRange DrawNumberRight(vgui::HFont font, const int xInkRight, const int yBaseline, const wchar_t *pwszNumber, const float flTracking)
{
	Assert(pwszNumber);
	const DigitCell cell = GetDigitCell(font);
	const InkRange ink = GetNumberInk(font, cell, pwszNumber, flTracking);
	const int x = xInkRight - ink.iRight;
	DrawNumber(font, cell, x, yBaseline, pwszNumber, flTracking);
	return Offset(ink, x);
}

InkRange DrawNumberCentered(vgui::HFont font, const int xCenter, const int yBaseline, const wchar_t *pwszNumber, const float flTracking)
{
	Assert(pwszNumber);
	const DigitCell cell = GetDigitCell(font);
	const InkRange ink = GetNumberInk(font, cell, pwszNumber, flTracking);
	const int x = xCenter - ((ink.iLeft + ink.iRight) / 2);
	DrawNumber(font, cell, x, yBaseline, pwszNumber, flTracking);
	return Offset(ink, x);
}

float GetReadoutTracking(vgui::HFont font)
{
	return surface()->GetFontAscent(font, L'H') * READOUT_TRACKING;
}
}  // namespace NeoHud
