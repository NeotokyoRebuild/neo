#pragma once

#include <utlhashtable.h>
#include <vgui/ISurface.h>

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
 *         NeoUI::SetPerRowLayout(2, NeoUI::ROWLAYOUT_TWOSPLIT);
 *         NeoUI::RingBoxBool(L"Example boolean ringbox", &bTest);
 *         NeoUI::SliderInt(L"Example int slider", &iTest, 0, 150);
 *     }
 *     NeoUI::EndSection();
 * }
 * NeoUI::EndContext();
 *
 * For a better example, just take a look at the CNeoRoot source code.
 *
 * NEO TODO (nullsystem)
 * - Change how styling works
 * 		- Colors
 * 		- Padding/margins
 * - Cut/copy/paste for text edits
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
enum MouseStart
{
	MOUSESTART_NONE = 0,
	MOUSESTART_SLIDER,
};

enum LayoutMode
{
	LAYOUT_VERTICAL = 0,
	LAYOUT_HORIZONTAL,
};
enum TextStyle
{
	TEXTSTYLE_LEFT = 0,
	TEXTSTYLE_CENTER,
	TEXTSTYLE_RIGHT,
};

static constexpr int FOCUSOFF_NUM = -1000;
static constexpr int MAX_SECTIONS = 5;
static constexpr int SIZEOF_SECTIONS = sizeof(int) * MAX_SECTIONS;
static constexpr int MAX_TEXTINPUT_U8BYTES_LIMIT = 256;

extern const int ROWLAYOUT_TWOSPLIT[];

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

struct DynWidgetInfos
{
	int iYOffsets;
	int iYTall;
	bool bCannotActive;
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

	// NEO NOTE (nullsystem):
	// If iRowParts = nullptr, iRowPartsTotal will be used of an equal proportions split
	struct Layout
	{
		int iRowPartsTotal;
		const int *iRowParts;
		int iRowTall;
		int iDefRowTall;

		int iVertPartsTotal;
		const int *iVertParts;
	};
	Layout layout;

	// Absolute positioning/wide/tall of the widget
	vgui::IntRect rWidgetArea;
	int irWidgetWide;
	int irWidgetTall;
	int irWidgetLayoutY;

	// Layout management
	// "Static" sizing
	Dim dPanel;
	int iMarginX;
	int iMarginY;

	// Active layouting
	int iIdxRowParts;
	int iIdxVertParts;
	int iLayoutX;
	int iLayoutY;
	int iVertLayoutY;
	int iYOffset[MAX_SECTIONS] = {};
	bool abYMouseDragOffset[MAX_SECTIONS] = {};
	int iStartMouseDragOffset[MAX_SECTIONS] = {};

	// Saved infos for EndSection managing scrolling
	DynWidgetInfos *wdgInfos = nullptr;
	int iWdgInfosMax = 0;

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
	int iActiveSection;
	bool bValueEdited;

	MouseStart eMousePressedStart;

	const char *pSzCurCtxName;
	CUtlHashtable<const wchar_t *, SliderInfo> htSliders;
	CUtlHashtable<CUtlConstString, int> htTexMap;
};

struct GetMouseinFocusedRet
{
	bool bActive;
	bool bHot;
};

struct RetButton
{
	bool bPressed;
	bool bKeyPressed;
	bool bMousePressed;
	bool bMouseHover;
	bool bMouseDoublePressed;
};

struct LabelExOpt
{
	TextStyle eTextStyle;
	EFont eFont;
};

// NEO TODO (nullsystem): Depreciate fixed colors, instead define outside of NeoUI and give proper way to
// define color schemes
// !!!Also the change to COLOR_NEOPANELACCENTBG makes element invisible! Should never been set to fully transparent!!!
#define COLOR_NEOPANELNORMALBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELSELECTBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELACCENTBG Color(0, 0, 0, 0) // TODO: Why is this invisible now?
#define COLOR_NEOPANELTEXTNORMAL Color(255, 255, 255, 255)//Color(200, 200, 200, 255)
#define COLOR_NEOPANELTEXTBRIGHT Color(255, 255, 255, 255)
#define COLOR_NEOPANELPOPUPBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELFRAMEBG Color(0, 0, 0, 170)
#define COLOR_NEOTITLE Color(255, 255, 255, 255)//Color(200, 200, 200, 255)
#define COLOR_NEOPANELBAR Color(20, 20, 20, 255)
#define COLOR_NEOPANELMICTEST Color(30, 90, 30, 255)

enum WidgetFlag
{
	WIDGETFLAG_NONE = 0,
	WIDGETFLAG_SKIPACTIVE = 1 << 0,
};

void FreeContext(NeoUI::Context *pCtx);

void BeginContext(NeoUI::Context *pNextCtx, const NeoUI::Mode eMode, const wchar_t *wszTitle, const char *pSzCtxName);
void EndContext();
void BeginSection(const bool bDefaultFocus = false);
void EndSection();
void BeginWidget(const WidgetFlag eWidgetFlag = WIDGETFLAG_NONE);
void EndWidget(const GetMouseinFocusedRet wdgState);

void SetPerRowLayout(const int iColTotal, const int *iColProportions = nullptr, const int iRowHeight = -1);
// Layout a vertical within the (horizontal) column, iRowTotal = 0 to disable
void SetPerCellVertLayout(const int iRowTotal, const int *iRowProportions = nullptr);

void SwapFont(const EFont eFont, const bool bForce = false);
void SwapColorNormal(const Color &color);
void MultiWidgetHighlighter(const int iTotalWidgets);

// Widgets
/*1W*/ void Pad();
/*1W*/ void LabelWrap(const wchar_t *wszText);
/*SW*/ void HeadingLabel(const wchar_t *wszText);
/*1W*/ void Label(const wchar_t *wszText, const bool bNotWidget = false);
/*1W*/ void Label(const wchar_t *wszText, const LabelExOpt &opt);
/*2W*/ void Label(const wchar_t *wszLabel, const wchar_t *wszText);
/*1W*/ void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex);
/*1W*/ RetButton Button(const wchar_t *wszText);
/*2W*/ RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText);
/*1W*/ RetButton ButtonTexture(const char *szTexturePath);
/*1W*/ void RingBoxBool(bool *bChecked);
/*2W*/ void RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked);
/*1W*/ void RingBox(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex);
/*2W*/ void RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex);
/*1W*/ void Progress(const float flValue, const float flMin, const float flMax);
// Sliders relies on wszLeftLabel to cache value string, so only two-widgets variants
/*2W*/ void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp = 2, const float flStep = 1.0f, const wchar_t *wszSpecialText = nullptr);
/*2W*/ void SliderInt(const wchar_t *wszLeftLabel, int *iValue, const int iMin, const int iMax, const int iStep = 1,
					  const wchar_t *wszSpecialText = nullptr);
/*2W*/ void SliderU8(const wchar_t *wszLeftLabel, uint8 *ucValue, const uint8 iMin, const uint8 iMax, const uint8 iStep = 1,
					 const wchar_t *wszSpecialText = nullptr);
// NEO NOTE (nullsystem): iMaxBytes as in when the wchar_t fits into back into a UTF-8 char
/*1W*/ void TextEdit(wchar_t *wszText, const int iMaxBytes);
/*2W*/ void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxBytes);
/*SW*/ void ImageTexture(const char *szTexturePath, const wchar_t *wszErrorMsg = L"", const char *szTextureGroup = "");

// NeoUI::Texture is non-widget, but utilizes NeoUI's image/texture handling
enum TextureOptFlags
{
	TEXTUREOPTFLAGS_NONE = 0,
	TEXTUREOPTFLAGS_DONOTCROPTOPANEL = 1, // Disable cropping based on panel's dimension
};
bool Texture(const char *szTexturePath, const int x, const int y, const int width, const int height,
			 const char *szTextureGroup = "", const TextureOptFlags texFlags = TEXTUREOPTFLAGS_NONE);
void ResetTextures();

// Non-widgets/convenience functions
bool Bind(const ButtonCode_t eCode);
void OpenURL(const char *szBaseUrl, const char *szPath);
}
