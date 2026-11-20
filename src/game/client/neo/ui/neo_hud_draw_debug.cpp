#include "cbase.h"
#include "neo_hud_draw_debug.h"
#include "neo_hud_draw.h"

#include <vgui/ISurface.h>
#include <vgui_controls/Controls.h>

#include "filesystem.h"
#include "tier1/utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using vgui::surface;

enum NeoHudDebugDraw
{
	NEO_HUD_DEBUG_DRAW_OFF = 0,
	NEO_HUD_DEBUG_DRAW_LINES,	// Line boxes, baselines, ink edges and boxes
	NEO_HUD_DEBUG_DRAW_GLYPHS,	// And the ink span of each character

	NEO_HUD_DEBUG_DRAW__TOTAL
};

// Not a cheat: it outlines what the HUD already shows and reveals nothing else
ConVar cl_neo_hud_debug_draw("cl_neo_hud_debug_draw", "0", FCVAR_CLIENTDLL,
	"Outline HUD text and boxes. 0: off, 1: each text's line box, baseline and ink edges, and each box, 2: also each character's ink span. "
	"Cyan is the font's line box, red its top edge, which glyphs are cut at, magenta the baseline, yellow the ink edges the text was placed by, green a box.",
	true, NEO_HUD_DEBUG_DRAW_OFF, true, NEO_HUD_DEBUG_DRAW__TOTAL - 1);

namespace NeoHud
{
namespace
{
enum DebugKind
{
	DEBUG_KIND_BOX = 0,
	DEBUG_KIND_TEXT,
	DEBUG_KIND_GLYPH,
};

// A longer run is cut to this in the records
constexpr int DEBUG_TEXT_LENGTH = 48;
// A run is measured and outlined whole up to this length, and only its record is cut
constexpr int DEBUG_RUN_LENGTH = 256;

// One rectangle of the frame, in screen pixels
struct DebugRecord
{
	DebugKind eKind;
	char szElement[32];
	char szName[32];
	vgui::HFont hFont;
	int x0, y0, x1, y1;
	int yBaseline;
	int iInkLeft, iInkRight;
	wchar_t wszText[DEBUG_TEXT_LENGTH];
};

struct DebugFontName
{
	vgui::HFont hFont;
	char szName[40];
};

const Color COLOR_DEBUG_LINE_BOX(0, 255, 255, 140);
const Color COLOR_DEBUG_CUT_EDGE(255, 48, 48, 230);
const Color COLOR_DEBUG_BASELINE(255, 0, 255, 200);
const Color COLOR_DEBUG_INK_EDGE(255, 255, 0, 220);
const Color COLOR_DEBUG_GLYPH_EVEN(255, 150, 0, 230);
const Color COLOR_DEBUG_GLYPH_ODD(120, 255, 0, 230);
const Color COLOR_DEBUG_BOX(0, 255, 0, 150);

CUtlVector<DebugRecord> s_records;
CUtlVector<DebugFontName> s_fontNames;
char s_szElement[32] = "";
int s_xPanel = 0;
int s_yPanel = 0;
// The frame the records belong to
int s_iRecordFrame = -1;
// A dump waits for the first whole frame after the command
bool s_bDumpWaiting = false;
int s_iDumpAskedFrame = -1;
char s_szDumpFile[MAX_PATH] = "";

const char *GetFontSchemeName(const vgui::HFont hFont)
{
	for (const DebugFontName &fontName : s_fontNames)
	{
		if (fontName.hFont == hFont)
		{
			return fontName.szName;
		}
	}
	return "-";
}

void WriteDump()
{
	FileHandle_t hFile = g_pFullFileSystem->Open(s_szDumpFile, "wt", "MOD");
	if (hFile == FILESYSTEM_INVALID_HANDLE)
	{
		Warning("NeoHud: could not open %s for the HUD debug dump\n", s_szDumpFile);
		return;
	}

	int iScreenWide = 0, iScreenTall = 0;
	surface()->GetScreenSize(iScreenWide, iScreenTall);
	g_pFullFileSystem->FPrintf(hFile, "# screen\t%d\t%d\tframe\t%d\n", iScreenWide, iScreenTall, s_iRecordFrame);
	g_pFullFileSystem->FPrintf(hFile, "# kind\telement\tname\tscheme_font\tfont_family\tfont_tall\tfont_ascent\tx0\ty0\tx1\ty1\tbaseline\tink_left\tink_right\ttext\n");
	for (const DebugRecord &record : s_records)
	{
		static constexpr const char *KIND_NAMES[] = { "box", "text", "glyph" };
		char szText[ARRAYSIZE(record.wszText) * 4] = "";
		V_UnicodeToUTF8(record.wszText, szText, sizeof(szText));
		for (char *pch = szText; *pch; ++pch)
		{
			if (*pch == '\t' || *pch == '\n' || *pch == '\r')
			{
				*pch = ' ';
			}
		}
		const bool bHasFont = record.eKind != DEBUG_KIND_BOX;
		g_pFullFileSystem->FPrintf(hFile, "%s\t%s\t%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n",
			KIND_NAMES[record.eKind], record.szElement[0] ? record.szElement : "-", record.szName[0] ? record.szName : "-",
			bHasFont ? GetFontSchemeName(record.hFont) : "-",
			bHasFont ? surface()->GetFontName(record.hFont) : "-",
			bHasFont ? surface()->GetFontTall(record.hFont) : 0,
			bHasFont ? surface()->GetFontAscent(record.hFont, L'0') : 0,
			record.x0, record.y0, record.x1, record.y1, record.yBaseline, record.iInkLeft, record.iInkRight, szText);
	}
	g_pFullFileSystem->Close(hFile);
	Msg("NeoHud: wrote %d HUD draws of frame %d to %s\n", s_records.Count(), s_iRecordFrame, s_szDumpFile);
}

// Starts a new frame's records when the frame has changed, and writes a waiting dump once it holds a whole frame
void UpdateFrame()
{
	const int iFrame = gpGlobals->framecount;
	if (iFrame == s_iRecordFrame)
	{
		return;
	}
	if (s_bDumpWaiting && s_iRecordFrame > s_iDumpAskedFrame)
	{
		WriteDump();
		s_bDumpWaiting = false;
	}
	s_records.RemoveAll();
	s_iRecordFrame = iFrame;
}

DebugRecord &AddRecord(const DebugKind eKind, const int x0, const int y0, const int x1, const int y1)
{
	DebugRecord &record = s_records[s_records.AddToTail()];
	record = {};
	record.eKind = eKind;
	V_strcpy_safe(record.szElement, s_szElement);
	record.x0 = x0 + s_xPanel;
	record.y0 = y0 + s_yPanel;
	record.x1 = x1 + s_xPanel;
	record.y1 = y1 + s_yPanel;
	return record;
}

void DrawHorizontal(const Color &color, const int x0, const int x1, const int y)
{
	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(x0, y, x1, y + 1);
}

void DrawVertical(const Color &color, const int x, const int y0, const int y1)
{
	surface()->DrawSetColor(color);
	surface()->DrawFilledRect(x, y0, x + 1, y1);
}
}  // namespace

bool IsDebugActive()
{
	return s_bDumpWaiting || cl_neo_hud_debug_draw.GetInt() != NEO_HUD_DEBUG_DRAW_OFF;
}

void SetDebugElement(const char *pszName, const int xPanel, const int yPanel)
{
	if (!IsDebugActive())
	{
		return;
	}
	UpdateFrame();
	V_strcpy_safe(s_szElement, pszName ? pszName : "");
	s_xPanel = xPanel;
	s_yPanel = yPanel;
}

void NameDebugFont(const vgui::HFont font, const char *pszName)
{
	if (font == vgui::INVALID_FONT)
	{
		return;
	}
	for (DebugFontName &fontName : s_fontNames)
	{
		if (fontName.hFont == font)
		{
			V_strcpy_safe(fontName.szName, pszName);
			return;
		}
	}
	DebugFontName &fontName = s_fontNames[s_fontNames.AddToTail()];
	fontName.hFont = font;
	V_strcpy_safe(fontName.szName, pszName);
}

void DebugText(const vgui::HFont font, const int xPen, const int yBaseline, const wchar_t *pwszText, const float flTracking, const InkRange &ink, const int iDigitCellWide)
{
	if (!IsDebugActive())
	{
		return;
	}
	Assert(pwszText);
	UpdateFrame();

	const int iMode = cl_neo_hud_debug_draw.GetInt();
	const int yTop = GetTextY(font, yBaseline);
	const int yBottom = yTop + surface()->GetFontTall(font);

	// Each character's pen position follows the same rules the draw used
	const int iTextRecord = s_records.Count();
	AddRecord(DEBUG_KIND_TEXT, xPen, yTop, xPen, yBottom);
	int xAdvance = 0;
	for (int i = 0; pwszText[i]; ++i)
	{
		const wchar_t wch = pwszText[i];
		int a = 0, b = 0, c = 0;
		surface()->GetCharABCwide(font, wch, a, b, c);
		const bool bInDigitCell = iDigitCellWide > 0 && wch >= L'0' && wch <= L'9';
		const int iWide = bInDigitCell ? iDigitCellWide : (a + b + c);
		const int xChar = xPen + xAdvance + RoundFloatToInt(flTracking * i);
		const int xCharInk = xChar + (bInDigitCell ? ((iDigitCellWide - b) / 2) : a);

		DebugRecord &glyph = AddRecord(DEBUG_KIND_GLYPH, xChar, yTop, xChar + iWide, yBottom);
		glyph.hFont = font;
		glyph.yBaseline = yBaseline + s_yPanel;
		glyph.iInkLeft = xCharInk + s_xPanel;
		glyph.iInkRight = xCharInk + b + s_xPanel;
		glyph.wszText[0] = wch;

		if (iMode >= NEO_HUD_DEBUG_DRAW_GLYPHS)
		{
			DrawHorizontal((i % 2) ? COLOR_DEBUG_GLYPH_ODD : COLOR_DEBUG_GLYPH_EVEN, xCharInk, xCharInk + b, yBottom - 1);
		}
		xAdvance += iWide;
	}
	const int xPenEnd = xPen + xAdvance + RoundFloatToInt(flTracking * Max(0, V_wcslen(pwszText) - 1));

	DebugRecord &text = s_records[iTextRecord];
	text.hFont = font;
	text.x1 = xPenEnd + s_xPanel;
	text.yBaseline = yBaseline + s_yPanel;
	text.iInkLeft = ink.iLeft + s_xPanel;
	text.iInkRight = ink.iRight + s_xPanel;
	V_wcsncpy(text.wszText, pwszText, sizeof(text.wszText));

	if (iMode == NEO_HUD_DEBUG_DRAW_OFF)
	{
		return;
	}
	surface()->DrawSetColor(COLOR_DEBUG_LINE_BOX);
	surface()->DrawOutlinedRect(xPen, yTop, xPenEnd, yBottom);
	DrawHorizontal(COLOR_DEBUG_CUT_EDGE, xPen, xPenEnd, yTop);
	DrawHorizontal(COLOR_DEBUG_BASELINE, xPen, xPenEnd, yBaseline);
	DrawVertical(COLOR_DEBUG_INK_EDGE, ink.iLeft, yTop, yBottom);
	DrawVertical(COLOR_DEBUG_INK_EDGE, ink.iRight - 1, yTop, yBottom);
}

namespace
{
// Reads the font and position the surface is about to print with, and reports the text as one run
void DebugSurfaceText(const wchar_t *pwszText, const int iLength)
{
	wchar_t wszText[DEBUG_RUN_LENGTH] = {};
	V_wcsncpy(wszText, pwszText, Min(sizeof(wszText), (iLength + 1) * sizeof(wchar_t)));

	// The surface has no getter for its font, but a character's render info names it. Space has no glyph in
	// some fonts, so ask about the first character that does.
	int x = 0, y = 0;
	surface()->DrawGetTextPos(x, y);
	vgui::HFont hFont = vgui::INVALID_FONT;
	for (int i = 0; wszText[i] && hFont == vgui::INVALID_FONT; ++i)
	{
		vgui::CharRenderInfo info = {};
		if (surface()->DrawGetUnicodeCharRenderInfo(wszText[i], info))
		{
			hFont = info.currentFont;
		}
	}
	surface()->DrawSetTextPos(x, y);
	if (hFont == vgui::INVALID_FONT)
	{
		return;
	}

	int a = 0, b = 0, c = 0;
	surface()->GetCharABCwide(hFont, wszText[0], a, b, c);
	InkRange ink = { x + a, x };
	for (int i = 0; wszText[i]; ++i)
	{
		surface()->GetCharABCwide(hFont, wszText[i], a, b, c);
		ink.iRight += a + b + c;
	}
	ink.iRight -= c;
	DebugText(hFont, x, y + surface()->GetFontAscent(hFont, L'0'), wszText, 0.0f, ink);
}
}  // namespace

void DrawPrintText(const wchar_t *pwszText, const int iLength)
{
	if (IsDebugActive() && pwszText && iLength > 0)
	{
		DebugSurfaceText(pwszText, iLength);
	}
	surface()->DrawPrintText(pwszText, iLength);
}

void DrawUnicodeChar(const wchar_t wch)
{
	if (IsDebugActive())
	{
		DebugSurfaceText(&wch, 1);
	}
	surface()->DrawUnicodeChar(wch);
}

void DebugBox(const char *pszName, const int x0, const int y0, const int x1, const int y1)
{
	if (!IsDebugActive())
	{
		return;
	}
	UpdateFrame();

	DebugRecord &box = AddRecord(DEBUG_KIND_BOX, x0, y0, x1, y1);
	V_strcpy_safe(box.szName, pszName);

	if (cl_neo_hud_debug_draw.GetInt() != NEO_HUD_DEBUG_DRAW_OFF)
	{
		surface()->DrawSetColor(COLOR_DEBUG_BOX);
		surface()->DrawOutlinedRect(x0, y0, x1, y1);
	}
}
}  // namespace NeoHud

CON_COMMAND_F(cl_neo_hud_debug_dump, "Write every HUD text and box of the next frame to hud_debug_<label>.txt in the game folder, in screen pixels. Usage: cl_neo_hud_debug_dump [label]", FCVAR_CLIENTDLL)
{
	// The label is the only part of the file name the caller chooses, and it stays a plain word
	char szLabel[32] = "frame";
	if (args.ArgC() > 1)
	{
		V_strcpy_safe(szLabel, args[1]);
	}
	for (char *pch = szLabel; *pch; ++pch)
	{
		const bool bPlain = (*pch >= 'a' && *pch <= 'z') || (*pch >= 'A' && *pch <= 'Z') || (*pch >= '0' && *pch <= '9') || *pch == '_' || *pch == '-';
		if (!bPlain)
		{
			*pch = '_';
		}
	}
	V_sprintf_safe(NeoHud::s_szDumpFile, "hud_debug_%s.txt", szLabel);
	NeoHud::s_iDumpAskedFrame = gpGlobals->framecount;
	NeoHud::s_bDumpWaiting = true;
}
