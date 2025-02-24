#pragma once

#include <utlhashtable.h>

/*
 * NEO NOTE (nullsystem):
 * NeoUI - Immediate-mode GUI on top of VGUI2 for NT;RE
 *
 * Custom GUI API based on the IMGUI-style of GUI API design. This routes VGUI2
 * panel events over to NeoUI::Context and allows for UI to be written in a
 * non-OOP, procedual, and more direct way. The current GUI design and layouts
 * is fairly specific to how NT;RE main menu is designed.
 *
 * To use this, Panel calls must be re-routed to NeoUI::Context with a NeoUI::Mode
 * enum assigned. In general all those Panel::OnMousePressed, OnMouseWheeled,
 * OnCursorMoved, etc... code should first update the context, then re-route
 * into a single common function that takes a NeoUI::Mode enum.
 *
 * The general layout looks like this:
 *
 * inline NeoUI::Context g_uiCtx;
 * static bool bTest = false;
 * static int iTest = 0;
 * NeoUI::BeginContext(&g_uiCtx, eMode);
 * {
 *     g_ctx.dPanel.x = ...; // goes for: x, y, wide, tall...
 *     g_ctx.bgColor = Color(40, 40, 40, 255);
 *     NeoUI::BeginSection();
 *     {
 *         NeoUI::Label(L"Example label");
 *         if (NeoUI::Button(L"Example button").bPressed)
 *         {
 *             // Do things here...
 *         }
 *         NeoUI::RingBoxBool(L"Example boolean ringbox", &bTest);
 *         NeoUI::SliderInt(L"Example int slider", &iTest, 0, 150);
 *     }
 *     NeoUI::EndSection();
 * }
 * NeoUI::EndContext();
 *
 * For a better example, just take a look at the CNeoRoot source code.
 */

#define SZWSZ_LEN(wlabel) ((sizeof(wlabel) / sizeof(wlabel[0])) - 1)

namespace NeoUI
{
enum Mode
{
	MODE_PAINT = 0,
	MODE_MOUSEPRESSED,
	MODE_MOUSERELEASED,
	MODE_MOUSEDOUBLEPRESSED,
	MODE_MOUSEMOVED,
	MODE_MOUSEWHEELED,
	MODE_KEYPRESSED,
	MODE_KEYTYPED,
};
enum MousePos
{
	MOUSEPOS_NONE = 0,
	MOUSEPOS_LEFT,
	MOUSEPOS_CENTER,
	MOUSEPOS_RIGHT,
};
enum LayoutMode
{
	LAYOUT_VERTICAL = 0,
	LAYOUT_HORIZONTAL,
};
enum TextStyle
{
	TEXTSTYLE_CENTER = 0,
	TEXTSTYLE_LEFT,
};

static constexpr int FOCUSOFF_NUM = -1000;
static constexpr int MAX_SECTIONS = 5;
static constexpr int SIZEOF_SECTIONS = sizeof(int) * MAX_SECTIONS;
static constexpr int MAX_TEXTINPUT_U8BYTES_LIMIT = 256;

struct Dim
{
	int x;
	int y;
	int wide;
	int tall;
};

struct FontInfo
{
	vgui::HFont hdl;
	int iYOffset;
	int iStartBtnXPos;
	int iStartBtnYPos;
};

enum EFont
{
	FONT_NTNORMAL,
	FONT_NTHEADING,
	FONT_NTHORIZSIDES,
	FONT_LOGO,
	FONT_LOGOSMALL,
	FONT_NTLARGE,

	FONT__TOTAL,
};

struct SliderInfo
{
	wchar_t wszText[33];
	float flCachedValue;
	int iMaxStrSize;
	bool bActive;
};

struct Context
{
	Mode eMode;
	ButtonCode_t eCode;
	wchar_t unichar;
	Color bgColor;
	Color selectBgColor;
	Color normalBgColor;

	// Mouse handling
	int iMouseAbsX;
	int iMouseAbsY;
	int iMouseRelX;
	int iMouseRelY;
	bool bMouseInPanel;
	int iHasMouseInPanel;

	// Layout management
	// "Static" sizing
	Dim dPanel;
	int iRowTall;
	int iMarginX;
	int iMarginY;

	// Active layouting
	int iPartitionY; // Only increments when Y-pos goes down
	int iLayoutX;
	int iLayoutY;
	float flWgXPerc;
	int iWgXPos;
	int iYOffset[MAX_SECTIONS] = {};
	bool abYMouseDragOffset[MAX_SECTIONS] = {};
	int iStartMouseDragOffset[MAX_SECTIONS] = {};

	int iHorizontalWidth;
	int iHorizontalMargin;

	TextStyle eButtonTextStyle;
	TextStyle eLabelTextStyle;
	bool bTextEditIsPassword;

	FontInfo fonts[FONT__TOTAL] = {};
	EFont eFont = FONT_NTNORMAL;

	// Input management
	int iWidget; // Always increments per widget use
	int iSection;
	int iCanActives; // Only increment if widget can be activated
	int iSectionCanActive[MAX_SECTIONS] = {};

	int iHot;
	int iHotSection;

	int iActive;
	int iActiveDirection;
	int iActiveSection;
	bool bValueEdited;

	MousePos eMousePos; // label | prev | center | next split
	MousePos eMousePressedStart;

	const char *pSzCurCtxName;
	CUtlHashtable<const wchar_t *, SliderInfo> htSliders;
};

#define COLOR_NEOPANELNORMALBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELSELECTBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELACCENTBG Color(0, 0, 0, 0)
#define COLOR_NEOPANELTEXTNORMAL Color(255, 255, 255, 255)//Color(200, 200, 200, 255)
#define COLOR_NEOPANELTEXTBRIGHT Color(255, 255, 255, 255)
#define COLOR_NEOPANELPOPUPBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELFRAMEBG Color(0, 0, 0, 170)
#define COLOR_NEOTITLE Color(255, 255, 255, 255)//Color(200, 200, 200, 255)
#define COLOR_NEOPANELBAR Color(20, 20, 20, 255)
#define COLOR_NEOPANELMICTEST Color(30, 90, 30, 255)

void GCtxDrawFilledRectXtoX(const int x1, const int y1, const int x2, const int y2);
void GCtxDrawFilledRectXtoX(const int x1, const int x2);
void GCtxDrawSetTextPos(const int x, const int y);
void GCtxSkipActive();

void BeginContext(NeoUI::Context *ctx, const NeoUI::Mode eMode, const wchar_t *wszTitle, const char *pSzCtxName);
void EndContext();
void BeginSection(const bool bDefaultFocus = false);
void EndSection();
void BeginHorizontal(const int iHorizontalWidth, const int iHorizontalMargin = 0);
void EndHorizontal();

void SwapFont(const EFont eFont);
void SwapColorNormal(const Color &color);

struct RetButton
{
	bool bPressed;
	bool bKeyPressed;
	bool bMousePressed;
	bool bMouseHover;
	bool bMouseDoublePressed;
};
void Pad();
void LabelWrap(const wchar_t *wszText);
void Label(const wchar_t *wszText);
void Label(const wchar_t *wszLabel, const wchar_t *wszText);
void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex);
RetButton Button(const wchar_t *wszText);
RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText);
void RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked);
void RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex);
void Progress(const float flValue, const float flMin, const float flMax);
void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
			const int iDp = 2, const float flStep = 1.0f, const wchar_t *wszSpecialText = nullptr);
void SliderInt(const wchar_t *wszLeftLabel, int *iValue, const int iMin, const int iMax, const int iStep = 1,
			   const wchar_t *wszSpecialText = nullptr);
void SliderU8(const wchar_t *wszLeftLabel, uint8 *ucValue, const uint8 iMin, const uint8 iMax, const uint8 iStep = 1,
			   const wchar_t *wszSpecialText = nullptr);
// NEO NOTE (nullsystem): iMaxBytes as in when the wchar_t fits into back into a UTF-8 char
void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxBytes);
bool Bind(const ButtonCode_t eCode);
void OpenURL(const char *szBaseUrl, const char *szPath);
}
