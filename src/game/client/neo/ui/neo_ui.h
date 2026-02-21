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
 * TODO:
 * - Fonts refactor
 *   - Change to union of anon-struct + array
 *   - Fixated on category (like Colors) rather than relying on eFont/SwapFont
 * - Sizing: Padding/margin
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
static constexpr int MAX_SECTIONS = 6;
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

enum ECopyMenu
{
	COPYMENU_NIL = 0,
	COPYMENU_CUT,
	COPYMENU_COPY,
	COPYMENU_PASTE,

	COPYMENU__TOTAL,
};

enum EBaseButtonType
{
	BASEBUTTONTYPE_TEXT = 0,
	BASEBUTTONTYPE_IMAGE,
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

enum EInternalPopup
{
	INTERNALPOPUP_COLOREDIT = -2,
	INTERNALPOPUP_COPYMENU = -1,
	INTERNALPOPUP_NIL = 0,
};

struct ColorEditInfo
{
	uint8 *r;
	uint8 *g;
	uint8 *b;
	uint8 *a;
};

enum ESectionFlag
{
	SECTIONFLAG_NONE = 0,

	// Sets this section as the focus by default
	SECTIONFLAG_DEFAULTFOCUS = 1 << 0,
	// Allow left-right navigation of widgets
	SECTIONFLAG_ROWWIDGETS = 1 << 1,
	// Do not allow controller to move to this section.
	//   Make sure to have Bind so controllers don't get cut out from being
	//   able to utilize actions from this section.
	SECTIONFLAG_EXCLUDECONTROLLER = 1 << 2,
	// Allow the hover and button click sound to play on buttons
	SECTIONFLAG_PLAYBUTTONSOUNDS = 1 << 3,
	// Internal use only: Mark this section as a popup
	SECTIONFLAG_POPUP = 1 << 4,
};
typedef int ISectionFlags;

enum EKeyHints
{
	KEYHINTS_KEYBOARD = 0,
	KEYHINTS_CONTROLLER,
};

enum EMultiHighlightFlag : uint8
{
	MULTIHIGHLIGHTFLAG_NONE = 0,
	MULTIHIGHLIGHTFLAG_IN_USE = 1 << 0,
	MULTIHIGHLIGHTFLAG_HOT = 1 << 1,
	MULTIHIGHLIGHTFLAG_ACTIVE = 1 << 2,
};

// This is utilized as both lower BeginPopup options flags and upper internal bool flags
enum PopupFlag_
{
	// Public flags, used as BeginPopup options
	POPUPFLAG_NIL = 0,
	POPUPFLAG_FOCUSONOPEN = 1 << 0, // Direct focus to this popup when it opens 
	POPUPFLAG_COLORHOTASACTIVE = 1 << 1, // Color hot widgets as active, mainly for menu-style popups

	// Internal only flags
	POPUPFLAG__EXTERNAL = ((1 << 8) - 1), // Mask of all external/options flags below start of internal
	POPUPFLAG__INPOPUPSECTION = 1 << 8, // Inside a Begin/EndPopup section
	POPUPFLAG__NEWOPENPOPUP = 1 << 9, // The popup just initialized
};
typedef int PopupFlags;

struct Colors
{
	Color normalFg;
	Color normalBg;
	Color hotFg;
	Color hotBg;
	Color hotBorder;
	Color activeFg;
	Color activeBg;
	Color overrideFg;
	Color titleFg;
	Color cursor;
	Color textSelectionBg;
	Color divider;
	Color popupBg;
	Color sectionBg;
	Color scrollbarBg;
	Color scrollbarHandleNormalBg;
	Color scrollbarHandleActiveBg;
	Color sliderNormalBg;
	Color sliderHotBg;
	Color sliderActiveBg;
	Color tabHintsFg;
};

struct Context
{
	Mode eMode;
	ButtonCode_t eCode;
	wchar_t unichar;

	bool bIsOverrideFgColor = false;

	ISectionFlags iSectionFlags;
	EKeyHints eKeyHints;

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

	// Themes
	Colors colors;
	FontInfo fonts[FONT__TOTAL] = {};
	EFont eFont = FONT_NTNORMAL;

	// Multi-widget highlight
	uint8 uMultiHighlightFlags;
	vgui::IntRect rMultiHighlightArea;

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
	int iXOffset[MAX_SECTIONS] = {};
	bool abYMouseDragOffset[MAX_SECTIONS] = {};
	int iStartMouseDragOffset[MAX_SECTIONS] = {};

	// Saved infos for EndSection managing scrolling
	DynWidgetInfos *wdgInfos = nullptr;
	int iWdgInfosMax = 0;

	TextStyle eButtonTextStyle;
	TextStyle eLabelTextStyle;

	// Input management
	int iWidget; // Always increments per widget use
	int iSection;
	uint64_t ibfSectionCanActive = 0;
	uint64_t ibfSectionCanController = 0;

	// Caches/read-ahead this section has scroll is
	// known in previous frame(s)
	uint64_t ibfSectionHasScroll = 0;

	int iHot;
	int iHotSection;

	// vs iHot this persists between Begin/EndContext
	// only used for bNewHot detection
	int iHotPersist;
	int iHotPersistSection;

	int iActive;
	int iActiveSection;
	bool bValueEdited;

	// Saved hot + active when OpenPopup activates
	// Restored when ClosePopup
	int iPrePopupHot;
	int iPrePopupHotSection;

	int iPrePopupActive;
	int iPrePopupActiveSection;

	MouseStart eMousePressedStart;

	const char *pSzCurCtxName;
	CUtlHashtable<const wchar_t *, SliderInfo> htSliders;
	CUtlHashtable<CUtlConstString, int> htTexMap;

	// # Popup mechanism variables

	// Cached popup size to readahead for mouse area checking
	Dim dimPopup = {};
	PopupFlags popupFlags = POPUPFLAG_NIL;

	// 0 = reserved for nil ID, negative numbers = reserved for
	// internal popups, only start using from 1 onwards for
	// popups done externally
	int iCurPopupId = 0;

	// # Internal managed popups variables
	ECopyMenu eRightClickCopyMenuRet = COPYMENU_NIL; // Right-click copy menu
	ColorEditInfo colorEditInfo; // Color editor popup

	// TextEdit text selection
	int iTextSelStart = -1;
	int iTextSelCur = -1;
	int iTextSelDrag = -1;
	int iTextSelDragSection = -1;
	int irTextWidths[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};

	// Sound paths
	const char *pszSoundBtnPressed = "ui/buttonclickrelease.wav";
	const char *pszSoundBtnRollover = "ui/buttonrollover.wav";
};

struct RetButton
{
	bool bPressed;
	bool bKeyPressed;
	bool bMousePressed;
	bool bMouseHover;
	bool bMouseDoublePressed;
	bool bMouseRightPressed;
};

struct LabelExOpt
{
	TextStyle eTextStyle;
	EFont eFont;
};

enum WidgetFlag_
{
	WIDGETFLAG_NONE = 0,
	WIDGETFLAG_SKIPACTIVE = 1 << 0,
	WIDGETFLAG_MOUSE = 1 << 1,
	WIDGETFLAG_MARKACTIVE = 1 << 2, // Mark section as having an active widget
	WIDGETFLAG_NOHOTBORDER = 1 << 3,
	WIDGETFLAG_FORCEACTIVE = 1 << 4, // Make this widget active no matter what
};
typedef int WidgetFlag;

struct CurrentWidgetState
{
	bool bActive;
	bool bHot;
	bool bNewHot;
	bool bInView;
	WidgetFlag eWidgetFlag;
};

// Current (global) context - Useful if wanting to implement a new widget externally
NeoUI::Context *CurrentContext();
void FreeContext(NeoUI::Context *pCtx);

void BeginContext(NeoUI::Context *pNextCtx, const NeoUI::Mode eMode, const wchar_t *wszTitle, const char *pSzCtxName);
void EndContext();
void BeginSection(const ISectionFlags iSectionFlags = SECTIONFLAG_NONE);
void EndSection();

void OpenPopup(const int iPopupId, const Dim &dimPopupInit);
void ClosePopup();
[[nodiscard]] bool BeginPopup(const int iPopupId, const PopupFlags flags = POPUPFLAG_NIL);
void EndPopup();

// Get a suitable wide size for a popup by the longest text in the popup
int PopupWideByStr(const char *pszStr);

[[nodiscard]] CurrentWidgetState BeginWidget(const WidgetFlag eWidgetFlag = WIDGETFLAG_NONE);
void EndWidget(const CurrentWidgetState &wdgState);

void SetPerRowLayout(const int iColTotal, const int *iColProportions = nullptr, const int iRowHeight = -1);
// Layout a vertical within the (horizontal) column, iRowTotal = 0 to disable
void SetPerCellVertLayout(const int iRowTotal, const int *iRowProportions = nullptr);

void SwapFont(const EFont eFont, const bool bForce = false);
void BeginMultiWidgetHighlighter(const int iTotalWidgets);
void EndMultiWidgetHighlighter();

void BeginOverrideFgColor(const Color &fgColor);
void EndOverrideFgColor();

// Widgets
/*1W*/ void Pad();
/*SW*/ void Divider(const wchar_t *wszText = nullptr);
/*1W*/ void LabelWrap(const wchar_t *wszText);
/*SW*/ void HeadingLabel(const wchar_t *wszText);
/*1W*/ void Label(const wchar_t *wszText, const bool bNotWidget = false);
/*1W*/ void Label(const wchar_t *wszText, const LabelExOpt &opt);
/*2W*/ void Label(const wchar_t *wszLabel, const wchar_t *wszText);

// If piTabWide == nullptr, fills the whole layout width, otherwise only length of the tabs.
// Initialize the variable to -1 and NeoUI::Tabs will auto-figure it out.
enum TabsFlag_
{
	TABFLAG_DEFAULT = 0,
	TABFLAG_NOSIDEKEYS = 1 << 0,  // Don't show hints and don't recognize keys EX: F1-F3/L-R on left-right side
	TABFLAG_NOSTATERESETS = 1 << 1, // Don't reset scroll/hot/active states
};
typedef int TabsFlags;
/*1W*/ void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex,
		const TabsFlags flags = TABFLAG_DEFAULT, int *piTabWide = nullptr);
/*1W*/ RetButton BaseButton(const wchar_t *wszText, const char *szTexturePath, const EBaseButtonType eType);
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
/*1W*/ void ColorEdit(uint8 *r, uint8 *g, uint8 *b, uint8 *a);
/*2W*/ void ColorEdit(const wchar_t *wszLeftLabel, uint8 *r, uint8 *g, uint8 *b, uint8 *a);

enum TextEditFlag_
{
	TEXTEDITFLAG_NONE = 0,
	TEXTEDITFLAG_PASSWORD = 1 << 0, // Replace all characters with "*"
	TEXTEDITFLAG_FORCEACTIVE = 1 << 1, // Enforce hot+active focusing in this text edit widget all time, only suitable for single-textedit popups
};
typedef int TextEditFlags;
/*1W*/ void TextEdit(wchar_t *wszText, const int iMaxWszTextSize, const TextEditFlags flags = TEXTEDITFLAG_NONE);
/*2W*/ void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxWszTextSize, const TextEditFlags flags = TEXTEDITFLAG_NONE);
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
bool Bind(const ButtonCode_t *peCode, const int ieCodeSize);
bool BindKeyEnter();
bool BindKeyBack();
void OpenURL(const char *szBaseUrl, const char *szPath);

// Convience function to return wszKey or wszController depending on c->eKeyHints mode 
const wchar_t *HintAlt(const wchar *wszKey, const wchar *wszController);
}
