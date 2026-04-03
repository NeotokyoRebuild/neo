#include "neo_ui.h"

#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui/ISystem.h>
#include <vgui_controls/Controls.h>
#include <filesystem.h>
#include <stb_image.h>
#include <materialsystem/imaterial.h>

#include "neo_misc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const wchar_t *ENABLED_LABELS[] = {
	L"Disabled",
	L"Enabled",
};

namespace NeoUI
{

static inline Context g_emptyCtx;
static Context *c = &g_emptyCtx;
const int ROWLAYOUT_TWOSPLIT[] = { 40, -1 };
static constexpr int WDGINFO_ALLOC_STEPS = 64;
static constexpr float FL_BORDER_RATIO = 0.2f;
static constexpr float FL_HEADER_DRAG_RATIO = 0.4f;
static constexpr int I_MIN_COL_WIDE_RATIO = 8;

// NEO JANK (nullsystem): DrawUnicodeChar is buggy, so use DrawPrintText instead, hence
// single character but a (wide-)string instead of (wide-)char
static constexpr wchar_t WSZ_ARROW_UP[] = L"\u2191";
static constexpr wchar_t WSZ_ARROW_DOWN[] = L"\u2193";
static constexpr wchar_t WSZ_ARROW_LEFT[] = L"<";
static constexpr wchar_t WSZ_ARROW_RIGHT[] = L">";
static constexpr wchar_t WSZ_CHECK_MARK[] = L"\u2713";
#define NEOUI_SCROLL_THICKNESS() (c->iMarginX * 3)

#define DEBUG_NEOUI 0 // NEO NOTE (nullsystem): !!! Always flip to 0 on master + PR !!!
#ifndef DEBUG
static_assert(DEBUG_NEOUI == 0);
#endif

void SwapFont(const EFont eFont, const bool bForce)
{
	if (c->eMode != MODE_PAINT || (!bForce && (c->eFont == eFont))) return;
	c->eFont = eFont;
	vgui::surface()->DrawSetTextFont(c->fonts[c->eFont].hdl);
}

void BeginOverrideFgColor(const Color &fgColor)
{
	c->colors.overrideFg = fgColor;
	c->bIsOverrideFgColor = true;
}

void EndOverrideFgColor()
{
	c->bIsOverrideFgColor = false;
}

void BeginMultiWidgetHighlighter(const int iTotalWidgets)
{
	// NEO NOTE (nullsystem): c->iLayoutY instead of c->irWidgetLayoutY
	// as this happens before rather than during widget Begin/End
	if (c->eMode != MODE_PAINT || !IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
	{
		return;
	}

	Assert(c->layout.iRowPartsTotal > 0);
	const int iRowPartsTotal = Max(1, c->layout.iRowPartsTotal);

	// Peek-forward what the area of the multiple widgets will cover without modifying the context
	int iMulLayoutX = c->iLayoutX;
	int iAllXWide = 0;
	int iMulIdxRowParts = c->iIdxRowParts;

	if (iMulIdxRowParts < 0)
	{
		iMulLayoutX = 0;
		iMulIdxRowParts = 0;
	}

	for (int i = 0; i < iTotalWidgets; ++i)
	{
		// NEO TODO (nullsystem): Maybe to not bother with vertical at this stage
		// but this is usually an internal use at the moment

		const bool bNextIsLast = (iMulIdxRowParts + 1) >= iRowPartsTotal;

		int xWide;
		if (bNextIsLast)
		{
			xWide = c->dPanel.wide - iMulLayoutX;
		}
		else
		{
			if (c->layout.iRowParts)
			{
				xWide = (c->layout.iRowParts[iMulIdxRowParts] / 100.0f) * c->dPanel.wide;
			}
			else
			{
				xWide = (1.0f / static_cast<float>(iRowPartsTotal)) * c->dPanel.wide;
			}
		}
		iAllXWide += xWide;

		if (bNextIsLast)
		{
			iMulIdxRowParts = 0;
			AssertMsg(i == (iTotalWidgets - 1), "NeoUI::BeginMultiWidgetHighlighter should not be used for split up widgets");
		}
		else
		{
			if (c->layout.iRowParts)
			{
				iMulLayoutX += (c->layout.iRowParts[iMulIdxRowParts] / 100.0f) * c->dPanel.wide;
			}
			else
			{
				iMulLayoutX += (1.0f / static_cast<float>(iRowPartsTotal)) * c->dPanel.wide;
			}
			++iMulIdxRowParts;
		}
	}
	c->rMultiHighlightArea = vgui::IntRect{
		.x0 = c->dPanel.x + c->iLayoutX,
		.y0 = c->dPanel.y + c->iLayoutY,
		.x1 = c->dPanel.x + c->iLayoutX + iAllXWide,
		.y1 = c->dPanel.y + c->iLayoutY + c->layout.iRowTall,
	};

	// Mostly so the highlight can apply to multi-widget's labels
	const bool bMouseIn = IN_BETWEEN_EQ(c->rMultiHighlightArea.x0, c->iMouseAbsX, c->rMultiHighlightArea.x1 - 1)
			&& IN_BETWEEN_EQ(c->rMultiHighlightArea.y0, c->iMouseAbsY, c->rMultiHighlightArea.y1 - 1);
	if (bMouseIn)
	{
		c->iHot = c->iWidget;
		c->iHotSection = c->iSection;
	}

	// Apply highlight if it's hot/active
	bool bHot = c->iHotSection == c->iSection && IN_BETWEEN_AR(c->iWidget, c->iHot, c->iWidget + iTotalWidgets);
	if (bHot && (c->dimPopup.wide > 0 && c->dimPopup.tall > 0) &&
			!(c->popupFlags & POPUPFLAG__INPOPUPSECTION))
	{
		const Dim &dim = c->dimPopup;
		bHot = !(IN_BETWEEN_EQ(dim.x, c->iMouseAbsX, dim.x + dim.wide) &&
				 IN_BETWEEN_EQ(dim.y, c->iMouseAbsY, dim.y + dim.tall));
		if (!bHot)
		{
			c->iHot = FOCUSOFF_NUM;
			c->iHotSection = -1;
		}
	}

	const bool bActive = c->iActiveSection == c->iSection && IN_BETWEEN_AR(c->iWidget, c->iActive, c->iWidget + iTotalWidgets);
	if (bHot || bActive)
	{
		vgui::surface()->DrawSetColor(bActive ? c->colors.activeBg : c->colors.hotBg);
		vgui::surface()->DrawFilledRectArray(&c->rMultiHighlightArea, 1);
	}

	c->uMultiHighlightFlags = MULTIHIGHLIGHTFLAG_IN_USE;
	if (bHot) c->uMultiHighlightFlags |= MULTIHIGHLIGHTFLAG_HOT;
	if (bActive) c->uMultiHighlightFlags |= MULTIHIGHLIGHTFLAG_ACTIVE;
}

static void DrawBorder(const vgui::IntRect &rect)
{
	const int iMargin = Max(static_cast<int>(FL_BORDER_RATIO * c->iMarginY), 1);
	vgui::surface()->DrawSetColor(c->colors.hotBorder);

	vgui::surface()->DrawFilledRect(rect.x0, rect.y0, rect.x1, rect.y0 + iMargin); // top
	vgui::surface()->DrawFilledRect(rect.x0, rect.y1 - iMargin, rect.x1, rect.y1); // bottom
	vgui::surface()->DrawFilledRect(rect.x0, rect.y0, rect.x0 + iMargin, rect.y1); // left
	vgui::surface()->DrawFilledRect(rect.x1 - iMargin, rect.y0, rect.x1, rect.y1); // right
}

void EndMultiWidgetHighlighter()
{
	if (c->eMode == MODE_PAINT && c->uMultiHighlightFlags & MULTIHIGHLIGHTFLAG_HOT)
	{
		DrawBorder(c->rMultiHighlightArea);
	}
	c->uMultiHighlightFlags = 0;
}

NeoUI::Context *CurrentContext()
{
	return c;
}

void FreeContext(NeoUI::Context *pCtx)
{
	if (pCtx->wdgInfos)
	{
		free(pCtx->wdgInfos);
	}
}

// Just helper bool function to deal with both KB and gamepad inputs

static bool IsKeyEnter()
{
	return c->eCode == KEY_ENTER || c->eCode == KEY_XBUTTON_A ||
		c->eCode == STEAMCONTROLLER_A;
}

static bool IsKeyBack()
{
	return c->eCode == KEY_ESCAPE || c->eCode == KEY_XBUTTON_B ||
		c->eCode == STEAMCONTROLLER_B;
}

static bool IsKeyLeft()
{
	return c->eCode == KEY_LEFT || c->eCode == KEY_XBUTTON_LEFT ||
			c->eCode == KEY_XSTICK1_LEFT || c->eCode == STEAMCONTROLLER_DPAD_LEFT;
}

static bool IsKeyRight()
{
	return c->eCode == KEY_RIGHT || c->eCode == KEY_XBUTTON_RIGHT ||
			c->eCode == KEY_XSTICK1_RIGHT || c->eCode == STEAMCONTROLLER_DPAD_RIGHT;
}

static bool IsKeyLeftRight()
{
	return IsKeyLeft() || IsKeyRight();
}

// The following are not just simply called IsKeyDownUp because
// SECTIONFLAG_ROWWIDGETS can allow left-right direction also

static bool IsKeyDownWidget()
{
	return c->eCode == KEY_DOWN || c->eCode == KEY_XBUTTON_DOWN || 
			c->eCode == KEY_XSTICK1_DOWN || c->eCode == STEAMCONTROLLER_DPAD_DOWN ||
			(c->iCurPopupId != 0 && c->eCode == KEY_TAB) ||
			((c->iSectionFlags & SECTIONFLAG_ROWWIDGETS) && IsKeyRight());
}

static bool IsKeyUpWidget()
{
	const bool bIsShiftDown = (vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT));
	return c->eCode == KEY_UP || c->eCode == KEY_XBUTTON_UP ||
			c->eCode == KEY_XSTICK1_UP || c->eCode == STEAMCONTROLLER_DPAD_UP ||
			(c->iCurPopupId != 0 && c->eCode == KEY_TAB && bIsShiftDown) ||
			((c->iSectionFlags & SECTIONFLAG_ROWWIDGETS) && IsKeyLeft());
}

static bool IsKeyChangeWidgetFocus()
{
	return IsKeyDownWidget() || IsKeyUpWidget();
}

bool BindKeyEnter()
{
	return c->eMode == MODE_KEYPRESSED && IsKeyEnter();
}

bool BindKeyBack()
{
	return c->eMode == MODE_KEYPRESSED && IsKeyBack() &&
			(false == (c->dimPopup.wide > 0 && c->dimPopup.tall > 0));
}

void BeginContext(NeoUI::Context *pNextCtx, const NeoUI::Mode eMode, const wchar_t *wszTitle, const char *pSzCtxName)
{
	c = pNextCtx;
	if (c->wdgInfos == nullptr)
	{
		c->iWdgInfosMax = WDGINFO_ALLOC_STEPS;
		c->wdgInfos = (DynWidgetInfos *)(malloc(sizeof(DynWidgetInfos) * c->iWdgInfosMax));
	}
	c->layout.iRowParts = nullptr;
	c->layout.iRowPartsTotal = 1;
	c->eMode = eMode;
	c->iLayoutX = 0;
	c->iLayoutY = 0;
	c->irWidgetLayoutX = 0;
	c->irWidgetLayoutY = 0;
	c->irWidgetMaxX = 0;
	c->iWidget = 0;
	c->iSection = 0;
	c->iHasMouseInPanel = 0;
	c->bValueEdited = false;
	c->eButtonTextStyle = TEXTSTYLE_CENTER;
	c->eLabelTextStyle = TEXTSTYLE_LEFT;
	c->ibfSectionCanActive = 0;
	c->ibfSectionCanController = 0;
	c->popupFlags &= ~(POPUPFLAG__CTXDONEPOPUP);
	// Different pointer, change context
	c->bFirstCtxUse = (c->pSzCurCtxName != pSzCtxName);
	if (c->bFirstCtxUse)
	{
		c->htSliders.RemoveAll();
		c->pSzCurCtxName = pSzCtxName;
		c->ibfSectionHasXScroll = 0;
		c->ibfSectionHasYScroll = 0;
		c->iCurPopupId = 0;
		V_memset(&c->dimPopup, 0, sizeof(Dim));
		c->colorEditInfo.r = nullptr;
		c->colorEditInfo.g = nullptr;
		c->colorEditInfo.b = nullptr;
		c->colorEditInfo.a = nullptr;
		c->iPrePopupActive = c->iPrePopupHot = FOCUSOFF_NUM;
		c->iPrePopupActiveSection = c->iPrePopupHotSection = -1;
		c->iTextSelStart = c->iTextSelCur = -1;
	}

	switch (c->eMode)
	{
	case MODE_KEYPRESSED:
		if (IsKeyChangeWidgetFocus())
		{
			if (c->iActive == FOCUSOFF_NUM)
			{
				c->iActive = 0;
				if (c->iHot != FOCUSOFF_NUM)
				{
					c->iActive = c->iHot;
				}
			}
			c->iHot = c->iActive;
		}
		c->eKeyHints = (c->eCode <= KEY_LAST) ? KEYHINTS_KEYBOARD : KEYHINTS_CONTROLLER;
		break;
	case MODE_MOUSERELEASED:
		c->eMousePressedStart = MOUSESTART_NONE;
		break;
	case MODE_MOUSEPRESSED:
	case MODE_MOUSEDOUBLEPRESSED:
	case MODE_MOUSEWHEELED:
		c->eKeyHints = KEYHINTS_KEYBOARD;
		break;
	case MODE_PAINT:
		for (int i = 0; i < FONT__TOTAL; ++i)
		{
			FontInfo *pFontI = &c->fonts[i];
			const int iTall = vgui::surface()->GetFontTall(pFontI->hdl);
			pFontI->iYFontOffset = (c->layout.iRowTall / 2) - (iTall / 2);
			{
				int iPrevNextWide, iPrevNextTall;
				vgui::surface()->GetTextSize(pFontI->hdl, L"<", iPrevNextWide, iPrevNextTall);
				pFontI->iStartBtnXPos = (c->layout.iRowTall / 2) - (iPrevNextWide / 2);
				pFontI->iStartBtnYPos = (c->layout.iRowTall / 2) - (iPrevNextTall / 2);
			}
		}

		if (wszTitle)
		{
			SwapFont(FONT_NTLARGE, true);
			vgui::surface()->DrawSetTextColor(c->colors.titleFg);
			vgui::surface()->DrawSetTextPos(c->dPanel.x + c->iMarginX,
											c->dPanel.y + -c->layout.iRowTall + c->fonts[FONT_NTLARGE].iYFontOffset);
			vgui::surface()->DrawPrintText(wszTitle, V_wcslen(wszTitle));
		}
		break;
	default:
		break;
	}

	c->iHotPersist = c->iHot;
	c->iHotPersistSection = c->iHotSection;
	if (c->eMode == MODE_MOUSEMOVED)
	{
		c->iHot = FOCUSOFF_NUM;
		c->iHotSection = -1;
	}

	// Force SwapFont on main to prevent crash on startup
	SwapFont(FONT_NTNORMAL, true);
	c->eButtonTextStyle = TEXTSTYLE_LEFT;
	vgui::surface()->DrawSetTextColor(c->colors.normalFg);
}

void EndContext()
{
	if (BeginPopup(INTERNALPOPUP_TABLEHEADER, POPUPFLAG_COLORHOTASACTIVE))
	{
		for (int i = 0; i < c->iTableVisColsTotal; ++i)
		{
			if (ButtonCheckbox(c->wszTableVisColNamesList[i], (c->piTableVisColsWide[i] > 0)).bPressed)
			{
				// The bounds checking and explicit initialization of headers sizes should
				// mean this shouldn't be zero in the first place.
				Assert(c->piTableVisColsWide[i] != 0);
				if (c->piTableVisColsWide[i] == 0)
				{
					const int iMinColWide = Max(I_MIN_COL_WIDE_RATIO * c->iMarginX, 10);
					c->piTableVisColsWide[i] = iMinColWide;
				}
				else
				{
					c->piTableVisColsWide[i] = -c->piTableVisColsWide[i];
				}
				ClosePopup();
				break;
			}
		}
		EndPopup();
	}

	if (BeginPopup(INTERNALPOPUP_COLOREDIT, POPUPFLAG_FOCUSONOPEN))
	{
		static const int ROWLAYOUT_COLORSPLIT[] = { 30, -1 };
		SetPerRowLayout(2, ROWLAYOUT_COLORSPLIT);
		SliderU8(L"Red", c->colorEditInfo.r, 0, UCHAR_MAX);
		SliderU8(L"Green", c->colorEditInfo.g, 0, UCHAR_MAX);
		SliderU8(L"Blue", c->colorEditInfo.b, 0, UCHAR_MAX);
		SliderU8(L"Alpha", c->colorEditInfo.a, 0, UCHAR_MAX);

		EndPopup();
	}

	if (BeginPopup(INTERNALPOPUP_COPYMENU, POPUPFLAG_COLORHOTASACTIVE))
	{
		if (Button(L"Cut").bPressed)
		{
			c->eRightClickCopyMenuRet = COPYMENU_CUT;
			ClosePopup();
		}
		if (Button(L"Copy").bPressed)
		{
			c->eRightClickCopyMenuRet = COPYMENU_COPY;
			ClosePopup();
		}
		if (Button(L"Paste").bPressed)
		{
			c->eRightClickCopyMenuRet = COPYMENU_PASTE;
			ClosePopup();
		}

		EndPopup();
	}

	if (c->eMode == MODE_MOUSEPRESSED && !c->iHasMouseInPanel)
	{
		c->iActive = FOCUSOFF_NUM;
		c->iActiveSection = -1;
	}

	if (c->eMode == MODE_KEYPRESSED)
	{
		const bool bSwitchSectionController = c->eCode == KEY_XBUTTON_BACK || c->eCode == STEAMCONTROLLER_SELECT;
		const bool bSwitchSectionKey = c->iCurPopupId == 0 && c->eCode == KEY_TAB;
		if (IsKeyChangeWidgetFocus() || bSwitchSectionKey || bSwitchSectionController)
		{
			if (c->iActiveSection == -1)
			{
				c->iActiveSection = 0;
			}

			if (bSwitchSectionKey || bSwitchSectionController)
			{
				const int iTotalSection = c->iSection;
				int iTally = 0;
				for (decltype(Context::ibfSectionCanActive) i = 0; i < iTotalSection; ++i)
				{
					const auto ibfCmp = (decltype(i))1 << i;
					iTally += (c->ibfSectionCanActive & ibfCmp) &&
							(!bSwitchSectionController || (c->ibfSectionCanController & ibfCmp));
				}
				if (iTally == 0)
				{
					// There is no valid sections, don't do anything
					c->iActiveSection = -1;
				}
				else
				{
					const int iIncr = ((vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT)) ? -1 : +1);
					bool bNextCmp = false;
					do
					{
						c->iActiveSection += iIncr;
						c->iActiveSection = LoopAroundInArray(c->iActiveSection, iTotalSection);

						const auto ibfCmp = (decltype(Context::ibfSectionCanActive))1 << c->iActiveSection;
						bNextCmp = !((c->ibfSectionCanActive & ibfCmp) &&
								(!bSwitchSectionController || (c->ibfSectionCanController & ibfCmp)));
					} while (bNextCmp);
				}
			}
			c->iHotSection = c->iActiveSection;
		}
	}
	else if (c->eMode == MODE_MOUSEPRESSED || c->eMode == MODE_MOUSEDOUBLEPRESSED)
	{
		c->iHot = c->iActive;
		c->iHotSection = c->iActiveSection;
	}
}

void BeginSection(const ISectionFlags iSectionFlags)
{
	if (!(iSectionFlags & SECTIONFLAG_DISABLEOFFSETS))
	{
		// Previous frame(s) known this section does scroll
		if (c->ibfSectionHasYScroll & (1ULL << c->iSection))
		{
			// NEO TODO (nullsystem): Change how dPanel works to enforce setting per BeginSection
			// so don't need to shift around wide on scrollbars and keep usage dPanel "immutable"
			// without extra variable
			c->dPanel.wide -= NEOUI_SCROLL_THICKNESS();
		}
		if (c->ibfSectionHasXScroll & (1ULL << c->iSection))
		{
			c->dPanel.tall -= NEOUI_SCROLL_THICKNESS();
		}
	}

	c->iLayoutX = -c->iXOffset[c->iSection];
	c->iLayoutY = -c->iYOffset[c->iSection];
	c->irWidgetLayoutX = c->iLayoutX;
	c->irWidgetLayoutY = c->iLayoutY;
	c->irWidgetMaxX = 0;
	c->iWidget = 0;
	c->iIdxRowParts = -1;
	c->iIdxVertParts = -1;
	c->iVertLayoutY = 0;
	c->iSectionFlags = iSectionFlags;
	c->uMultiHighlightFlags = MULTIHIGHLIGHTFLAG_NONE;
	c->layout.iTableLabelsSize = 0;
	c->layout.piTableColsWide = nullptr;
	c->curRowFlags = NEXTTABLEROWFLAG_NONE;
	c->bBlockSectionMWheel = false;

	if (iSectionFlags & SECTIONFLAG_POPUP)
	{
		c->popupFlags |= POPUPFLAG__INPOPUPSECTION;
	}
	else
	{
		c->popupFlags &= ~(POPUPFLAG__INPOPUPSECTION);
	}

	c->iMouseRelX = c->iMouseAbsX - c->dPanel.x;
	c->iMouseRelY = c->iMouseAbsY - c->dPanel.y;
	c->bMouseInPanel = IN_BETWEEN_EQ(0, c->iMouseRelX, c->dPanel.wide - 1) && IN_BETWEEN_EQ(0, c->iMouseRelY, c->dPanel.tall - 1);

	c->iHasMouseInPanel += c->bMouseInPanel;

	switch (c->eMode)
	{
	case MODE_PAINT:
		vgui::surface()->DrawSetColor((iSectionFlags & SECTIONFLAG_POPUP)
				? c->colors.popupBg
				: c->colors.sectionBg);
		vgui::surface()->DrawFilledRect(c->dPanel.x,
										c->dPanel.y,
										c->dPanel.x + c->dPanel.wide,
										c->dPanel.y + c->dPanel.tall);

		vgui::surface()->DrawSetColor(c->colors.normalBg);
		vgui::surface()->DrawSetTextColor(c->colors.normalFg);
		break;
	case MODE_KEYPRESSED:
		if (IsKeyChangeWidgetFocus())
		{
			const bool bDefaultFocus = (iSectionFlags & SECTIONFLAG_DEFAULTFOCUS) && (c->iActiveSection == -1);
			const bool bPopupFocus = (iSectionFlags & SECTIONFLAG_POPUP);
			if (bDefaultFocus || bPopupFocus)
			{
				c->iActiveSection = c->iSection;
			}
		}
		break;
	}

	NeoUI::SetPerRowLayout(1, nullptr, c->layout.iDefRowTall);
	NeoUI::SetPerCellVertLayout(0);
}

void EndSection()
{
	// If button pressed inside of the section, but after the final
	// widget, then should also be treated like outside of section
	if (c->eMode == MODE_MOUSEPRESSED && c->bMouseInPanel &&
			(c->irWidgetLayoutY + c->irWidgetTall) < c->iMouseRelY)
	{
		c->iActive = FOCUSOFF_NUM;
		c->iActiveSection = -1;
	}
	const bool bOffsetDisabled = (c->iSectionFlags & SECTIONFLAG_DISABLEOFFSETS);
	if (bOffsetDisabled)
	{
		c->iXOffset[c->iSection] = 0;
		c->iYOffset[c->iSection] = 0;
	}

	const int iAbsLayoutX = c->irWidgetMaxX + c->iXOffset[c->iSection];
	const int iAbsLayoutY = c->irWidgetLayoutY + c->irWidgetTall + c->iYOffset[c->iSection];
	c->wdgInfos[c->iWidget].iXOffsets = iAbsLayoutX;
	c->wdgInfos[c->iWidget].iYOffsets = iAbsLayoutY;

	// Scroll handling
	const int iMWheelJump = c->layout.iDefRowTall;
	const bool bHasXScroll = (false == bOffsetDisabled) && (iAbsLayoutX > c->dPanel.wide);
	const bool bHasYScroll = (false == bOffsetDisabled) && (iAbsLayoutY > c->dPanel.tall);
	const int iScrollThick = NEOUI_SCROLL_THICKNESS();
	const bool bResetXScrollPanelWide = (c->ibfSectionHasXScroll & (1ULL << c->iSection));
	const bool bResetYScrollPanelTall = (c->ibfSectionHasYScroll & (1ULL << c->iSection));

	// Saved for next frame(s) to layout y-axis based on scroll
	if (bHasYScroll)
	{
		c->ibfSectionHasYScroll |= (1ULL << c->iSection);
	}
	else
	{
		c->ibfSectionHasYScroll &= ~(1ULL << c->iSection);
	}

	if (bHasXScroll)
	{
		c->ibfSectionHasXScroll |= (1ULL << c->iSection);
	}
	else
	{
		c->ibfSectionHasXScroll &= ~(1ULL << c->iSection);
	}

	vgui::IntRect rectYScrollArea = {
		.x0 = c->dPanel.x + c->dPanel.wide,
		.y0 = c->dPanel.y,
		.x1 = c->dPanel.x + c->dPanel.wide + (bHasYScroll ? iScrollThick : 0),
		.y1 = c->dPanel.y + c->dPanel.tall - (bHasXScroll ? iScrollThick : 0),
	};
	// NEO TODO (nullsystem): X-axis scrollbar visibility is optional
	vgui::IntRect rectXScrollArea = {
		.x0 = c->dPanel.x,
		.y0 = c->dPanel.y + c->dPanel.tall,
		.x1 = c->dPanel.x + c->dPanel.wide - (bHasYScroll ? iScrollThick : 0),
		.y1 = c->dPanel.y + c->dPanel.tall + (bHasXScroll ? iScrollThick : 0),
	};
	EXYMouseDragOffset eMouseInScrollbar = XYMOUSEDRAGOFFSET_NIL;
	if (bHasYScroll && InRect(rectYScrollArea, c->iMouseAbsX, c->iMouseAbsY))
	{
		eMouseInScrollbar = XYMOUSEDRAGOFFSET_YAXIS;
	}
	else if (bHasXScroll && InRect(rectXScrollArea, c->iMouseAbsX, c->iMouseAbsY))
	{
		eMouseInScrollbar = XYMOUSEDRAGOFFSET_XAXIS;
	}

	const bool bMouseInYWheelable = c->bMouseInPanel || (XYMOUSEDRAGOFFSET_YAXIS == eMouseInScrollbar);
	const bool bMouseInXWheelable = bHasXScroll && ((!bMouseInYWheelable && c->bMouseInPanel) || (XYMOUSEDRAGOFFSET_XAXIS == eMouseInScrollbar));

	// y-axis always have precedence over x-axis on being wheelable
	if (c->eMode == MODE_MOUSEWHEELED && bMouseInYWheelable)
	{
		if (false == c->bBlockSectionMWheel)
		{
			if (!bHasYScroll)
			{
				c->iYOffset[c->iSection] = 0;
			}
			else
			{
				c->iYOffset[c->iSection] += (c->eCode == MOUSE_WHEEL_UP) ? -iMWheelJump : +iMWheelJump;
				c->iYOffset[c->iSection] = clamp(c->iYOffset[c->iSection], 0, iAbsLayoutY - c->dPanel.tall);
			}
		}
	}
	else if (c->eMode == MODE_MOUSEWHEELED && bMouseInXWheelable)
	{
		if (false == c->bBlockSectionMWheel)
		{
			if (!bHasXScroll)
			{
				c->iXOffset[c->iSection] = 0;
			}
			else
			{
				c->iXOffset[c->iSection] += (c->eCode == MOUSE_WHEEL_UP) ? -iMWheelJump : +iMWheelJump;
				c->iXOffset[c->iSection] = clamp(c->iXOffset[c->iSection], 0, iAbsLayoutX - c->dPanel.wide);
			}
		}
	}
	else if (c->eMode == MODE_KEYPRESSED && IsKeyChangeWidgetFocus() &&
			 (c->iActiveSection == c->iSection || c->iHotSection == c->iSection))
	{
		if (c->iActive != FOCUSOFF_NUM && c->iActiveSection == c->iSection)
		{
			const int iActiveDirection = IsKeyUpWidget() ? -1 : +1;
			int activeIdxLoop = 0;
			do // do-while: Deal with if LoopAroundInArray does alter (begin <-> end widgets)
			{
				do // do-while: Skip over widgets that cannot be an active widget
				{
					c->iActive += iActiveDirection;
				} while (IN_BETWEEN_AR(0, c->iActive, c->iWidget) && c->wdgInfos[c->iActive].bCannotActive);
				c->iActive = LoopAroundInArray(c->iActive, c->iWidget);
				++activeIdxLoop;
			} while (c->wdgInfos[c->iActive].bCannotActive && activeIdxLoop < 2); // Loop guard limit only twice
			c->iHot = c->iActive;
			c->iHotSection = c->iActiveSection;
		}

		if (!bHasYScroll)
		{
			// Disable scroll if it doesn't need to
			c->iYOffset[c->iSection] = 0;
		}
		else if (c->wdgInfos[c->iActive].iYOffsets <= c->iYOffset[c->iSection])
		{
			// Scrolling up past visible, re-adjust
			c->iYOffset[c->iSection] = c->wdgInfos[c->iActive].iYOffsets;
		}
		else if ((c->wdgInfos[c->iActive].iYOffsets + c->wdgInfos[c->iActive].iYTall) >= (c->iYOffset[c->iSection] + c->dPanel.tall))
		{
			// Scrolling down post visible, re-adjust
			c->iYOffset[c->iSection] = c->wdgInfos[c->iActive].iYOffsets - c->dPanel.tall + c->wdgInfos[c->iActive].iYTall;
		}
	}

	// Scroll bar area painting and mouse interaction (no keyboard as that's handled by active widgets)
	if (bHasYScroll || bHasXScroll)
	{
		vgui::IntRect rectYHandle = {};
		vgui::IntRect rectXHandle = {};

		if (bHasYScroll)
		{
			const int iYStart = c->iYOffset[c->iSection];
			const int iYEnd = iYStart + c->dPanel.tall;
			const float flYPercStart = iYStart / static_cast<float>(iAbsLayoutY);
			const float flYPercEnd = iYEnd / static_cast<float>(iAbsLayoutY);
			rectYHandle.x0 = c->dPanel.x + c->dPanel.wide;
			rectYHandle.y0 = c->dPanel.y + static_cast<int>(c->dPanel.tall * flYPercStart);
			rectYHandle.x1 = c->dPanel.x + c->dPanel.wide + iScrollThick;
			rectYHandle.y1 = c->dPanel.y + static_cast<int>(c->dPanel.tall * flYPercEnd);
		}
		if (bHasXScroll)
		{
			const int iXStart = c->iXOffset[c->iSection];
			const int iXEnd = iXStart + c->dPanel.wide;
			const float flXPercStart = iXStart / static_cast<float>(iAbsLayoutX);
			const float flXPercEnd = iXEnd / static_cast<float>(iAbsLayoutX);
			rectXHandle.x0 = c->dPanel.x + static_cast<int>(c->dPanel.wide * flXPercStart);
			rectXHandle.y0 = c->dPanel.y + c->dPanel.tall;
			rectXHandle.x1 = c->dPanel.x + static_cast<int>(c->dPanel.wide * flXPercEnd);
			rectXHandle.y1 = c->dPanel.y + c->dPanel.tall + iScrollThick;
		}

		EXYMouseDragOffset eXYAlterOffset = XYMOUSEDRAGOFFSET_NIL;
		switch (c->eMode)
		{
		case MODE_PAINT:
			vgui::surface()->DrawSetColor(c->colors.scrollbarBg);
			if (bHasYScroll) vgui::surface()->DrawFilledRectArray(&rectYScrollArea, 1);
			if (bHasXScroll) vgui::surface()->DrawFilledRectArray(&rectXScrollArea, 1);

			if (bHasYScroll)
			{
				vgui::surface()->DrawSetColor(
						(c->aeXYMouseDragOffset[c->iSection] == XYMOUSEDRAGOFFSET_YAXIS)
							? c->colors.scrollbarHandleActiveBg
							: c->colors.scrollbarHandleNormalBg);
				vgui::surface()->DrawFilledRectArray(&rectYHandle, 1);
			}
			if (bHasXScroll)
			{
				vgui::surface()->DrawSetColor(
						(c->aeXYMouseDragOffset[c->iSection] == XYMOUSEDRAGOFFSET_XAXIS)
							? c->colors.scrollbarHandleActiveBg
							: c->colors.scrollbarHandleNormalBg);
				vgui::surface()->DrawFilledRectArray(&rectXHandle, 1);
			}
			if (bHasYScroll && bHasXScroll)
			{
				vgui::surface()->DrawSetColor(c->colors.sectionBg);
				vgui::surface()->DrawFilledRect(c->dPanel.x + c->dPanel.wide,
												c->dPanel.y + c->dPanel.tall,
												c->dPanel.x + c->dPanel.wide + iScrollThick,
												c->dPanel.y + c->dPanel.tall + iScrollThick);
			}
			break;
		case MODE_MOUSEPRESSED:
			c->aeXYMouseDragOffset[c->iSection] = eMouseInScrollbar;
			if (XYMOUSEDRAGOFFSET_YAXIS == eMouseInScrollbar)
			{
				c->aeXYMouseDragOffset[c->iSection] = XYMOUSEDRAGOFFSET_YAXIS;
				// If not pressed on handle, set the drag at the middle
				const bool bInHandle = InRect(rectYHandle, c->iMouseAbsX, c->iMouseAbsY);
				c->iStartMouseDragOffset[c->iSection] = (bInHandle) ?
							(c->iMouseAbsY - rectYHandle.y0) : ((rectYHandle.y1 - rectYHandle.y0) / 2.0f);
				eXYAlterOffset = XYMOUSEDRAGOFFSET_YAXIS;
			}
			else if (XYMOUSEDRAGOFFSET_XAXIS == eMouseInScrollbar)
			{
				// If not pressed on handle, set the drag at the middle
				const bool bInHandle = InRect(rectXHandle, c->iMouseAbsX, c->iMouseAbsY);
				c->iStartMouseDragOffset[c->iSection] = (bInHandle) ?
							(c->iMouseAbsX - rectXHandle.x0) : ((rectXHandle.x1 - rectXHandle.x0) / 2.0f);
				eXYAlterOffset = XYMOUSEDRAGOFFSET_XAXIS;
			}
			break;
		case MODE_MOUSERELEASED:
			c->aeXYMouseDragOffset[c->iSection] = XYMOUSEDRAGOFFSET_NIL;
			break;
		case MODE_MOUSEMOVED:
			eXYAlterOffset = c->aeXYMouseDragOffset[c->iSection];
			break;
		default:
			break;
		}

		// NEO TODO (nullsystem): Could we deal with overlap/partial if we restrict paint area for
		// each section properly? So it actually paints partially if partial area painted.
		if (eXYAlterOffset == XYMOUSEDRAGOFFSET_YAXIS)
		{
			const float flYPercMouse = static_cast<float>(c->iMouseRelY - c->iStartMouseDragOffset[c->iSection]) / static_cast<float>(c->dPanel.tall);
			// Do not allow smooth scrolling so we don't have to deal with overlap/partial in section widgets
			const int iNextYOffset = clamp(flYPercMouse * iAbsLayoutY, 0, (iAbsLayoutY - c->dPanel.tall));
			if (iNextYOffset >= (iAbsLayoutY - c->dPanel.tall))
			{
				c->iYOffset[c->iSection] = iNextYOffset;
			}
			else
			{
				c->iYOffset[c->iSection] = iNextYOffset - (iNextYOffset % c->layout.iDefRowTall);
			}
		}
		else if (eXYAlterOffset == XYMOUSEDRAGOFFSET_XAXIS)
		{
			const float flXPercMouse = static_cast<float>(c->iMouseRelX - c->iStartMouseDragOffset[c->iSection]) / static_cast<float>(c->dPanel.wide);
			// Allow smooth scrolling as we deal with overlap/partial for x-axis
			// Unlike Y-axis we don't typically layout for X-axis scroll
			const int iNextXOffset = clamp(flXPercMouse * iAbsLayoutX, 0, (iAbsLayoutX - c->dPanel.wide));
			c->iXOffset[c->iSection] = iNextXOffset;
		}

		if (bHasYScroll)
		{
			c->iYOffset[c->iSection] = clamp(c->iYOffset[c->iSection], 0, iAbsLayoutY - c->dPanel.tall);
		}
		if (bHasXScroll)
		{
			c->iXOffset[c->iSection] = clamp(c->iXOffset[c->iSection], 0, iAbsLayoutX - c->dPanel.wide);
		}
	}
	
	if (false == bOffsetDisabled)
	{
		// NEO TODO (nullsystem): Change how dPanel works to enforce setting per BeginSection
		// so don't need to shift around wide on scrollbars and keep usage dPanel "immutable"
		// without extra variable
		if (bResetXScrollPanelWide)
		{
			c->dPanel.tall += iScrollThick;
		}
		if (bResetYScrollPanelTall)
		{
			c->dPanel.wide += iScrollThick;
		}
	}

	++c->iSection;
}

void OpenPopup(const int iPopupId, const Dim &dimPopupInit)
{
	if (c->iCurPopupId != 0)
	{
		ClosePopup();
	}

	c->iCurPopupId = iPopupId;
	c->popupFlags |= POPUPFLAG__NEWOPENPOPUP;

	c->iPrePopupHot = c->iHot;
	c->iPrePopupHotSection = c->iHotSection;

	c->iPrePopupActive = c->iActive;
	c->iPrePopupActiveSection = c->iActiveSection;

	V_memcpy(&c->dimPopup, &dimPopupInit, sizeof(Dim));
}

void ClosePopup()
{
	c->iCurPopupId = 0;

	c->iHot = c->iPrePopupHot;
	c->iHotSection = c->iPrePopupHotSection;

	c->iActive = c->iPrePopupActive;
	c->iActiveSection = c->iPrePopupActiveSection;

	V_memset(&c->dimPopup, 0, sizeof(Dim));
}

bool BeginPopup(const int iPopupId, const PopupFlags flags)
{
	if (iPopupId != c->iCurPopupId || (c->popupFlags & POPUPFLAG__CTXDONEPOPUP))
	{
		return false;
	}

	const Dim &dim = c->dimPopup;
	const bool bPressOutsidePopup = (c->eMode == MODE_MOUSEPRESSED && c->eCode == MOUSE_LEFT &&
			!(c->popupFlags & POPUPFLAG__NEWOPENPOPUP) &&
			!( IN_BETWEEN_EQ(dim.x, c->iMouseAbsX, dim.x + dim.wide)
			&& IN_BETWEEN_EQ(dim.y, c->iMouseAbsY, dim.y + dim.tall)));
	if (bPressOutsidePopup || (c->eMode == MODE_KEYPRESSED && IsKeyBack()))
	{
		ClosePopup();
		return false;
	}

	c->popupFlags &= ~(POPUPFLAG__EXTERNAL);
	c->popupFlags |= (flags & POPUPFLAG__EXTERNAL) | POPUPFLAG__CTXDONEPOPUP;

	V_memcpy(&c->dPanel, &dim, sizeof(Dim));
	BeginSection(SECTIONFLAG_POPUP);
	return true;
}

void EndPopup()
{
	EndSection();
	c->popupFlags &= ~(POPUPFLAG__NEWOPENPOPUP);
}

int CurrentPopup()
{
	return c->iCurPopupId;
}

int SuitableWideByWStr(const wchar_t *pwszStr, const ESuitableWide eWideType)
{
	const auto *pFontI = &c->fonts[c->eFont];
	switch (eWideType)
	{
	case SUITABLEWIDE_POPUP:
	{
		// Rough-estimate, suitable for popup menus
		const int iWszSize = V_wcslen(pwszStr);
		const int iChWidth = vgui::surface()->GetCharacterWidth(pFontI->hdl, 'A');
		return (c->iMarginX * 2) + (iWszSize * iChWidth);
	} break;
	case SUITABLEWIDE_TABLE:
	{
		// More precise wide, suitable for fixed-tables
		[[maybe_unused]] int iWide = 0, iTall = 0;
		vgui::surface()->GetTextSize(pFontI->hdl, pwszStr, iWide, iTall);
		return (c->iMarginX * 2) + iWide;
	} break;
	}
	Assert(false); // Should not be able to get here
	return 0;
}

void SetPerRowLayout(const int iColTotal, const int *iColProportions, const int iRowTall)
{
	// Bump y-axis with previous row layout before applying new row layout
	if (c->iWidget > 0 && c->iIdxRowParts > 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutX = -c->iXOffset[c->iSection];
		c->iLayoutY += c->layout.iRowTall;
		c->irWidgetLayoutY = c->iLayoutY;
	}

	// NEO NOTE (nullsystem): Setting iColTotal must be at least 1 or more, the layout
	// is always mainly a per row based
	Assert(iColTotal > 0);
	if (iColTotal <= 0)
	{
		// Really shouldn't ever happen but just in-case to enforce 1
		c->layout.iRowPartsTotal = 1;
		c->layout.iRowParts = nullptr;
	}
	else
	{
		c->layout.iRowPartsTotal = iColTotal;
		c->layout.iRowParts = iColProportions;
	}
	if (iRowTall > 0) c->layout.iRowTall = iRowTall;
	c->iIdxRowParts = -1;
}

void SetPerCellVertLayout(const int iRowTotal, const int *iRowProportions)
{
	// NEO NOTE (nullsystem): iRowTotal = 0 is allowed to switch off vertical layout
	Assert(iRowTotal >= 0);
	c->layout.iVertPartsTotal = iRowTotal;
	c->layout.iVertParts = iRowProportions;
	c->iIdxVertParts = -1;
	c->iVertLayoutY = 0;
}

CurrentWidgetState BeginWidget(const WidgetFlag eWidgetFlag)
{
	if (c->iWidget >= c->iWdgInfosMax)
	{
		c->iWdgInfosMax += WDGINFO_ALLOC_STEPS;
		c->wdgInfos = (DynWidgetInfos *)(realloc(c->wdgInfos, sizeof(DynWidgetInfos) * c->iWdgInfosMax));
	}

	const int iWdgXOffset = (c->bIgnoreXOffset) ? 0 : c->iXOffset[c->iSection];
	const bool bInTable = (c->layout.iTableLabelsSize > 0 && c->layout.piTableColsWide);

	if (c->iIdxRowParts < 0)
	{
		// NEO NOTE (nullsystem): Don't need to bump c->iLayoutY
		// because any reset of iIdxRowParts either don't need to
		// or already done it
		c->iLayoutX = -iWdgXOffset;
		c->iIdxRowParts = 0;
	}

	int iVertYOffset = 0;
	int iWdgTall = c->layout.iRowTall;
	bool bShiftIdxRowParts = true;

	const bool bVertMode = c->layout.iVertPartsTotal > 0;
	if (bVertMode)
	{
		if (c->iIdxVertParts < 0)
		{
			c->iVertLayoutY = 0;
			c->iIdxVertParts = 0;
		}

		if (c->layout.iVertParts)
		{
			iWdgTall = (c->layout.iVertParts[c->iIdxVertParts] / 100.0f) * c->layout.iRowTall;
		}
		else
		{
			iWdgTall = (1.0f / static_cast<float>(c->layout.iVertPartsTotal)) * c->layout.iRowTall;
		}
		c->iVertLayoutY += iWdgTall;
		iVertYOffset = c->iVertLayoutY - iWdgTall;

		if ((c->iIdxVertParts + 1) >= c->layout.iVertPartsTotal)
		{
			c->iVertLayoutY = 0;
			c->iIdxVertParts = 0;
		}
		else
		{
			bShiftIdxRowParts = false;
			++c->iIdxVertParts;
		}
	}

	Assert(c->layout.iRowPartsTotal > 0);
	const int iRowPartsTotal = (bInTable)
			? Max(1, c->layout.iTableLabelsSize)
			: Max(1, c->layout.iRowPartsTotal);
	const bool bNextIsLast = (c->iIdxRowParts + 1) >= iRowPartsTotal;

	// NEO NOTE (nullsystem): If very last partition of the row, use what's left
	// instead so whatever pixels don't get left out
	int xWide;
	if (bNextIsLast && !bInTable)
	{
		xWide = c->dPanel.wide - c->iLayoutX;
	}
	else
	{
		if (bInTable)
		{
			// If this is less than/equal 0, then the widgets should just check visibility
			// and skip over
			xWide = Max(0, c->layout.piTableColsWide[c->iIdxRowParts]);
		}
		else if (c->layout.iRowParts)
		{
			xWide = (c->layout.iRowParts[c->iIdxRowParts] / 100.0f) * c->dPanel.wide;
		}
		else
		{
			xWide = (1.0f / static_cast<float>(iRowPartsTotal)) * c->dPanel.wide;
		}
	}

	c->rWidgetArea = vgui::IntRect{
		.x0 = c->dPanel.x + c->iLayoutX,
		.y0 = c->dPanel.y + c->iLayoutY + iVertYOffset,
		.x1 = c->dPanel.x + c->iLayoutX + xWide,
		.y1 = c->dPanel.y + c->iLayoutY + iWdgTall,
	};
	c->irWidgetWide = c->rWidgetArea.x1 - c->rWidgetArea.x0;
	c->irWidgetTall = c->rWidgetArea.y1 - c->rWidgetArea.y0;
	c->irWidgetLayoutX = c->iLayoutX;
	c->irWidgetLayoutY = c->iLayoutY;
	c->irWidgetMaxX = Max(c->irWidgetMaxX, c->irWidgetLayoutX + c->irWidgetWide);
	c->wdgInfos[c->iWidget].iYOffsets = c->iLayoutY + c->iYOffset[c->iSection];
	c->wdgInfos[c->iWidget].iYTall = c->irWidgetTall;
	c->wdgInfos[c->iWidget].bCannotActive = (eWidgetFlag & WIDGETFLAG_SKIPACTIVE);

	if (bShiftIdxRowParts)
	{
		if (bNextIsLast)
		{
			c->iLayoutX = -iWdgXOffset;
			c->iLayoutY += c->layout.iRowTall;
			c->iIdxRowParts = 0;
		}
		else
		{
			c->iLayoutX += xWide;
			++c->iIdxRowParts;
		}
	}

	bool bHot = false;
	bool bActive = false;
	bool bNotCurHot = false;
	// Only deal with Y-axis, X-axis only have smooth scrolling and isn't
	// as reliable to check if in view or not
	const bool bInView = (xWide > 0) && IN_BETWEEN_AR(0, c->irWidgetLayoutY, c->dPanel.tall);
	const bool bInteract = bInView && IN_BETWEEN_AR(0, c->irWidgetLayoutY + c->layout.iRowTall - 1,
			c->dPanel.tall);

	// Check mouse hot/active states if WIDGETFLAG_MOUSE flag set
	if (eWidgetFlag & WIDGETFLAG_MOUSE && bInteract)
	{
		bNotCurHot = (c->iHotPersist != c->iWidget || c->iHotPersistSection != c->iSection);
		const bool bMouseIn = IN_BETWEEN_EQ(c->rWidgetArea.x0, c->iMouseAbsX, c->rWidgetArea.x1 - 1)
				&& IN_BETWEEN_EQ(c->rWidgetArea.y0, c->iMouseAbsY, c->rWidgetArea.y1 - 1);
		if (bMouseIn)
		{
			c->iHot = c->iWidget;
			c->iHotSection = c->iSection;
		}
		bHot = (c->iHot == c->iWidget && c->iHotSection == c->iSection);
		if (bHot && (c->dimPopup.wide > 0 && c->dimPopup.tall > 0) &&
				!(c->popupFlags & POPUPFLAG__INPOPUPSECTION))
		{
			const Dim &dim = c->dimPopup;
			bHot = !(IN_BETWEEN_EQ(dim.x, c->iMouseAbsX, dim.x + dim.wide) &&
					 IN_BETWEEN_EQ(dim.y, c->iMouseAbsY, dim.y + dim.tall));
			if (!bHot)
			{
				c->iHot = FOCUSOFF_NUM;
				c->iHotSection = -1;
			}
		}

		bActive = (c->iWidget == c->iActive && c->iSection == c->iActiveSection);
	}

	static const PopupFlags POPUPFLAGS_FIRSTACTIVE = POPUPFLAG_FOCUSONOPEN | POPUPFLAG__NEWOPENPOPUP | POPUPFLAG__INPOPUPSECTION;
	const bool bIsPopupFirstActive =
			(((c->popupFlags & POPUPFLAGS_FIRSTACTIVE) == POPUPFLAGS_FIRSTACTIVE)
			&& (eWidgetFlag & WIDGETFLAG_MARKACTIVE));

	if ((eWidgetFlag & WIDGETFLAG_FORCEACTIVE) || bIsPopupFirstActive)
	{
		c->iActive = c->iHot = c->iWidget;
		c->iActiveSection = c->iHotSection = c->iSection;
		bHot = true;
		bActive = true;
		if (bIsPopupFirstActive)
		{
			c->popupFlags &= ~(POPUPFLAG_FOCUSONOPEN);
		}
	}

	// Mark this section this widget under as able to be active
	if (eWidgetFlag & WIDGETFLAG_MARKACTIVE)
	{
		c->ibfSectionCanActive |= (decltype(Context::ibfSectionCanActive))1 << c->iSection;
		if (!(c->iSectionFlags & SECTIONFLAG_EXCLUDECONTROLLER))
		{
			c->ibfSectionCanController |= (decltype(Context::ibfSectionCanController))1 << c->iSection;
		}
	}

	// Deal with inital widget colors, could also color active/hot by mutli-highlighting
	static const PopupFlags POPUPFLAGS_COLORHOTASACTIVE = POPUPFLAG_COLORHOTASACTIVE | POPUPFLAG__INPOPUPSECTION;
	const bool bColorHot = bHot || (c->uMultiHighlightFlags & MULTIHIGHLIGHTFLAG_HOT);
	const bool bColorActive = ((bActive || (c->uMultiHighlightFlags & MULTIHIGHLIGHTFLAG_ACTIVE)) ||
			(((c->popupFlags & POPUPFLAGS_COLORHOTASACTIVE) == POPUPFLAGS_COLORHOTASACTIVE) && bColorHot));
	if (bColorActive || (bInTable && (c->curRowFlags & NEXTTABLEROWFLAG_SELECTED)))
	{
		vgui::surface()->DrawSetColor(c->colors.activeBg);
		vgui::surface()->DrawSetTextColor(c->colors.activeFg);
	}
	else if (bColorHot || (bInTable && (c->curRowFlags & NEXTTABLEROWFLAG__HOT)))
	{
		vgui::surface()->DrawSetColor(c->colors.hotBg);
		vgui::surface()->DrawSetTextColor(c->colors.hotFg);
	}
	else
	{
		vgui::surface()->DrawSetColor(c->colors.normalBg);
		vgui::surface()->DrawSetTextColor(c->colors.normalFg);
	}
	if (c->bIsOverrideFgColor)
	{
		vgui::surface()->DrawSetTextColor(c->colors.overrideFg);
	}

	return CurrentWidgetState{
		.bActive = bActive,
		.bHot = bHot,
		.bNewHot = bNotCurHot && bHot,
		.bInView = bInView,
		.eWidgetFlag = eWidgetFlag,
	};
}

void EndWidget(const CurrentWidgetState &wdgState)
{
	if (c->eMode == MODE_PAINT &&
			!(wdgState.eWidgetFlag & WIDGETFLAG_NOHOTBORDER) &&
			!(c->uMultiHighlightFlags & MULTIHIGHLIGHTFLAG_IN_USE) &&
			wdgState.bHot)
	{
		DrawBorder(c->rWidgetArea);
	}

	++c->iWidget;
}

// Internal API: Figure out X position from text, font, and text style
static int XPosFromText(const wchar_t *wszText, const FontInfo *pFontI, const TextStyle eTextStyle)
{
	int iFontTextWidth = 0, iFontTextHeight = 0;
	if (eTextStyle != TEXTSTYLE_LEFT)
	{
		vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontTextWidth, iFontTextHeight);
	}
	int x = 0;
	switch (eTextStyle)
	{
	break; case TEXTSTYLE_LEFT:
		x = c->iMarginX;
	break; case TEXTSTYLE_CENTER:
		x = (c->irWidgetWide / 2) - (iFontTextWidth / 2);
	break; case TEXTSTYLE_RIGHT:
		x = c->irWidgetWide - c->iMarginX - iFontTextWidth;
	}
	return x;
}

void Pad()
{
	auto _ = BeginWidget();
}

void Divider(const wchar_t *wszText)
{
	Layout tmp;
	V_memcpy(&tmp, &c->layout, sizeof(Layout));

	SetPerRowLayout(1, nullptr, tmp.iRowTall);
	c->eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE);
	if (wdgState.bInView && c->eMode == MODE_PAINT)
	{
		const int iDividerTall = c->iMarginY / 2;
		const int iCenter = c->rWidgetArea.y0 + ((c->rWidgetArea.y1 - c->rWidgetArea.y0) - iDividerTall) / 2;

		vgui::surface()->DrawSetColor(c->colors.divider);

		if (wszText)
		{
			int iFontTextWidth = 0, iFontTextHeight = 0;
			constexpr int iPadding = 16;

			// Text
			const auto *pFontI = &c->fonts[c->eFont];
			const int x = XPosFromText(wszText, pFontI, c->eLabelTextStyle);
			const int y = pFontI->iYFontOffset;
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + x, c->rWidgetArea.y0 + y);
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));

			vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontTextWidth, iFontTextHeight);

			// Left Bar
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, iCenter,
				(c->rWidgetArea.x0 + x) - iPadding, iCenter + iDividerTall);

			// Right Bar
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + x + iFontTextWidth + iPadding,
				iCenter, c->rWidgetArea.x1, iCenter + iDividerTall);
		}
		else
		{
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, iCenter,
				c->rWidgetArea.x1, iCenter + iDividerTall);
		}
		vgui::surface()->DrawSetColor(Color(0, 0, 0, 0));
	}
	EndWidget(wdgState);

	c->eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
	SetPerRowLayout(tmp.iRowPartsTotal, tmp.iRowParts, tmp.iRowTall);
}

void LabelWrap(const wchar_t *wszText)
{
	if (!wszText || wszText[0] == '\0')
	{
		Pad();
		return;
	}

	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE);
	int iLines = 0;
	if (wdgState.bInView)
	{
		const auto *pFontI = &c->fonts[c->eFont];
		const int iWszSize = V_wcslen(wszText);
		int iSumWidth = 0;
		int iStart = 0;
		int iYOffset = 0;
		int iLastSpace = 0;
		for (int i = 0; i < iWszSize; ++i)
		{
			const wchar_t ch = wszText[i];
			if (ch == L' ')
			{
				iLastSpace = i;
			}
			const int chWidth = vgui::surface()->GetCharacterWidth(pFontI->hdl, ch);
			iSumWidth += chWidth;
			if (iSumWidth >= c->dPanel.wide)
			{
				if (c->eMode == MODE_PAINT)
				{
					vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + c->iMarginX,
													c->rWidgetArea.y0 + pFontI->iYFontOffset + iYOffset);
					vgui::surface()->DrawPrintText(wszText + iStart, iLastSpace - iStart);
				}
				++iLines;

				iYOffset += c->layout.iRowTall;
				iSumWidth = 0;
				iStart = iLastSpace + 1;
				i = iLastSpace;
			}
		}
		if (iSumWidth > 0)
		{
			if (c->eMode == MODE_PAINT)
			{
				vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + c->iMarginX,
												c->rWidgetArea.y0 + pFontI->iYFontOffset + iYOffset);
				vgui::surface()->DrawPrintText(wszText + iStart, iWszSize - iStart);
			}
			++iLines;
		}
	}

	c->iLayoutY += (c->layout.iRowTall * iLines);
	c->irWidgetLayoutX = c->iLayoutX;
	c->irWidgetLayoutY = c->iLayoutY;
	c->irWidgetMaxX = Max(c->irWidgetMaxX, c->irWidgetLayoutX + c->irWidgetWide);

	EndWidget(wdgState);
}

void HeadingLabel(const wchar_t *wszText)
{
	Layout tmp;
	V_memcpy(&tmp, &c->layout, sizeof(Layout));

	SetPerRowLayout(1, nullptr, tmp.iRowTall);
	c->eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

	Label(wszText);

	c->eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
	SetPerRowLayout(tmp.iRowPartsTotal, tmp.iRowParts, tmp.iRowTall);
}

void Label(const wchar_t *wszText, const bool bNotWidget)
{
	CurrentWidgetState wdgState = {};
	if (bNotWidget)
	{
		wdgState.bInView = IN_BETWEEN_AR(0, c->irWidgetLayoutY, c->dPanel.tall);
	}
	else
	{
		wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE);
	}
	if (wdgState.bInView && c->eMode == MODE_PAINT)
	{
		const int iLDiff = Max(c->dPanel.x - c->rWidgetArea.x0, 0);
		const int iRDiff = Max(c->rWidgetArea.x1 - (c->dPanel.x + c->dPanel.wide), 0);
		Dim viewportDim = {};
		const bool bViewportPanel = (bNotWidget || (c->iSectionFlags & SECTIONFLAG_LABELPANELVIEWPORT));
		if (bViewportPanel)
		{
			viewportDim.x = c->dPanel.x;
			viewportDim.y = c->dPanel.y;
			viewportDim.wide = c->dPanel.wide;
			viewportDim.tall = c->dPanel.tall;
		}
		else
		{
			viewportDim.x = Max(c->rWidgetArea.x0, c->dPanel.x);
			viewportDim.y = c->rWidgetArea.y0;
			viewportDim.wide = c->irWidgetWide - iLDiff - iRDiff;
			viewportDim.tall = c->irWidgetTall;
		}
		// NEO NOTE (nullsystem): Only paint text if the viewport is in a valid region
		// Viewport can be invalidated if X-axis offset turns the label to be off the
		// section's panel's area, so can be expected to not pass this check.
		if (IN_BETWEEN_EQ(c->dPanel.x, viewportDim.x, c->dPanel.x + c->dPanel.wide)
				&& IN_BETWEEN_EQ(c->dPanel.y, viewportDim.y, c->dPanel.y + c->dPanel.tall)
				&& IN_BETWEEN_EQ(c->dPanel.y, viewportDim.y + viewportDim.tall, c->dPanel.y + c->dPanel.tall)
				&& IN_BETWEEN_EQ(1, viewportDim.wide, c->dPanel.wide)
				&& IN_BETWEEN_EQ(1, viewportDim.tall, c->dPanel.tall))
		{
			vgui::surface()->SetFullscreenViewport(
					viewportDim.x, viewportDim.y,
					viewportDim.wide, viewportDim.tall);
			vgui::surface()->PushFullscreenViewport();

			const auto *pFontI = &c->fonts[c->eFont];
			int x = XPosFromText(wszText, pFontI, c->eLabelTextStyle);
			int y = pFontI->iYFontOffset;
			if (bViewportPanel)
			{
				vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 - c->dPanel.x + x, c->rWidgetArea.y0 - c->dPanel.y + y);
			}
			else
			{
				vgui::surface()->DrawSetTextPos(x - iLDiff, y);
			}
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));

			vgui::surface()->PopFullscreenViewport();
			vgui::surface()->SetFullscreenViewport(0, 0, 0, 0);
		}
	}
	if (!bNotWidget)
	{
		EndWidget(wdgState);
	}
}

void Label(const wchar_t *wszText, const LabelExOpt &opt)
{
	const LabelExOpt bkup{
		.eTextStyle = c->eLabelTextStyle,
		.eFont = c->eFont,
	};

	SwapFont(opt.eFont);
	c->eLabelTextStyle = opt.eTextStyle;
	Label(wszText);
	c->eLabelTextStyle = bkup.eTextStyle;
	SwapFont(bkup.eFont);
}

void Label(const wchar_t *wszLabel, const wchar_t *wszText)
{
	Label(wszLabel);
	Label(wszText);
}

NeoUI::RetButton BaseButton(const wchar_t *wszText, const char *szTexturePath,
		const char *szTextureGroup, const EBaseButtonType eType, const bool bVal,
		const ButtonFlags flags, const float flScrollStart)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);

	RetButton ret = {};
	ret.bMouseHover = wdgState.bHot;

	if (wdgState.bInView)
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			switch (eType)
			{
			case BASEBUTTONTYPE_TEXT:
			case BASEBUTTONTYPE_TOGGLE:
			{
				if (eType == BASEBUTTONTYPE_TOGGLE)
				{
					vgui::surface()->DrawSetColor(bVal ? c->colors.activeBg : c->colors.normalBg);
					vgui::surface()->DrawSetTextColor(bVal ? c->colors.activeFg : c->colors.normalFg);
				}
				vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);

				const auto *pFontI = &c->fonts[c->eFont];
				const int x = XPosFromText(wszText, pFontI, c->eButtonTextStyle);
				const int y = pFontI->iYFontOffset;
				const int wszTextLen = V_wcslen(wszText);

				bool drawn = false;
				if (flags & BUTTONFLAG_SCROLLTEXT)
				{
					int textWidth = 0, textHeight = 0;
					vgui::surface()->GetTextSize(pFontI->hdl, wszText, textWidth, textHeight);
					const int textWidthOver = textWidth - c->dPanel.wide;
					if (textWidthOver > 0)
					{ 
						// Draw with scrolling text // NEO TODO (Adam) could be useful for plain labels too
						vgui::surface()->SetFullscreenViewport(c->dPanel.x, c->dPanel.y, c->dPanel.wide, c->dPanel.tall);
						vgui::surface()->PushFullscreenViewport();

						// Scrolling text
						{
							textWidth += textHeight; // Separation between two draws of wszText
							const float PAUSE_BETWEEN_SCROLLS_SECONDS = 2.5f;
							const float PAUSE_BETWEEN_SCROLLS_PIXELS = PAUSE_BETWEEN_SCROLLS_SECONDS * textHeight;
							const float SCROLL_SPEED_TEXT_HEIGHT_PER_SECOND = 1.0f;
							const float SCROLL_SPEED_PIXELS = SCROLL_SPEED_TEXT_HEIGHT_PER_SECOND * textHeight;
							const int timeForScrollInPixels = textWidth + PAUSE_BETWEEN_SCROLLS_PIXELS;
							// 1 pixel per second scroll speed if using just time, scale by size of font so text scrolls relatively fast even on large displays.
							float timeSpentOnCurrentScrollInPixels = fmod((gpGlobals->realtime - flScrollStart) * SCROLL_SPEED_PIXELS, timeForScrollInPixels);
							if (timeSpentOnCurrentScrollInPixels < PAUSE_BETWEEN_SCROLLS_PIXELS)
							{
								vgui::surface()->DrawSetTextPos(x, y);
								vgui::surface()->DrawPrintText(wszText, wszTextLen);
							}
							else
							{
								timeSpentOnCurrentScrollInPixels -= PAUSE_BETWEEN_SCROLLS_PIXELS;
								const int x1 = x - timeSpentOnCurrentScrollInPixels;
								vgui::surface()->DrawSetTextPos(x1, y);
								vgui::surface()->DrawPrintText(wszText, wszTextLen);

								const int x2 = x1 + textWidth;
								vgui::surface()->DrawSetTextPos(x2, y);
								vgui::surface()->DrawPrintText(wszText, wszTextLen);
							}
						}
					
						vgui::surface()->PopFullscreenViewport();
						vgui::surface()->SetFullscreenViewport(0, 0, 0, 0);

						drawn = true;
					}
				}

				if (!drawn)
				{
					vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + x, c->rWidgetArea.y0 + y);
					vgui::surface()->DrawPrintText(wszText, wszTextLen);
				}
			} break;
			case BASEBUTTONTYPE_IMAGE:
			{
				vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
												c->rWidgetArea.x1, c->rWidgetArea.y1);
				if (wszText && wszText[0])
				{
					// NEO TODO (nullsystem): Currently only top-center texture,
					// bottom-center label style but if wanted can tweak texture-label styling
					const int iTexYTall = c->irWidgetTall * 0.75f;
					Texture(szTexturePath, c->rWidgetArea.x0, c->rWidgetArea.y0,
							c->irWidgetWide, iTexYTall,
							szTextureGroup);

					const auto *pFontI = &c->fonts[c->eFont];
					const int x = XPosFromText(wszText, pFontI, TEXTSTYLE_CENTER);
					vgui::surface()->DrawSetTextPos(
							c->rWidgetArea.x0 + x,
							c->rWidgetArea.y0 + iTexYTall - pFontI->iYFontOffset);
					vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
				}
				else
				{
					Texture(szTexturePath, c->rWidgetArea.x0, c->rWidgetArea.y0,
							c->irWidgetWide, c->irWidgetTall,
							szTextureGroup);
				}
			} break;
			case BASEBUTTONTYPE_CHECKBOX:
			{
				vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);

				const auto *pFontI = &c->fonts[c->eFont];
				int iFontTextWidth = 0, iFontTextHeight = 0;
				vgui::surface()->GetTextSize(pFontI->hdl, WSZ_CHECK_MARK, iFontTextWidth, iFontTextHeight);

				const int x = (c->layout.iRowTall / 2) - (iFontTextWidth / 2);
				const int y = pFontI->iYFontOffset;

				if (bVal)
				{
					vgui::surface()->DrawSetTextPos(
							c->rWidgetArea.x0 + x,
							c->rWidgetArea.y0 + y);
					vgui::surface()->DrawPrintText(WSZ_CHECK_MARK, 1);
				}

				vgui::surface()->DrawSetTextPos(
						c->rWidgetArea.x0 + c->layout.iRowTall + c->iMarginX,
						c->rWidgetArea.y0 + y);
				vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
			} break;
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
			ret.bMouseRightPressed = (ret.bMouseHover && c->eCode == MOUSE_RIGHT);
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			ret.bMouseDoublePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			ret.bKeyEnterPressed = ret.bPressed = (wdgState.bActive && IsKeyEnter());
		}
		break;
		default:
			break;
		}

		if (ret.bPressed)
		{
			c->iActive = c->iWidget;
			c->iActiveSection = c->iSection;
		}
	}

	if (c->iSectionFlags & SECTIONFLAG_PLAYBUTTONSOUNDS)
	{
		if (ret.bPressed)
		{
			vgui::surface()->PlaySound(c->pszSoundBtnPressed);
		}
        if (wdgState.bNewHot &&
                ((c->eMode == MODE_MOUSEMOVED && ret.bMouseHover) ||
                 (c->eMode == MODE_KEYPRESSED && !ret.bPressed)))
		{
			vgui::surface()->PlaySound(c->pszSoundBtnRollover);
		}
	}

	EndWidget(wdgState);
	return ret;
}

NeoUI::RetButton Button(const wchar_t *wszText)
{
	return BaseButton(wszText, "", "", BASEBUTTONTYPE_TEXT);
}

NeoUI::RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	const NeoUI::RetButton ret = Button(wszText);
	EndMultiWidgetHighlighter();
	return ret;
}

void ImageTexture(const char *szTexturePath, const wchar_t *wszErrorMsg, const char *szTextureGroup)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE);
	if (wdgState.bInView && c->eMode == MODE_PAINT)
	{
		const bool bHasTexture = Texture(szTexturePath,
										 c->rWidgetArea.x0, c->rWidgetArea.y0,
										 c->irWidgetWide, c->irWidgetTall,
										 szTextureGroup);
		if (!bHasTexture)
		{
			Label(wszErrorMsg, true);
		}
	}
	EndWidget(wdgState);
}

bool Texture(const char *szTexturePath, const int x, const int y, const int width, const int height,
			 const char *szTextureGroup, const TextureOptFlags texFlags)
{
	auto hdl = c->htTexMap.Find(szTexturePath);
	if (hdl == c->htTexMap.InvalidHandle() && c->eMode == MODE_PAINT)
	{
		bool bApplied = false;
		int iTex = -1;
		if (V_striEndsWith(szTexturePath, ".png") || V_striEndsWith(szTexturePath, ".jpg") ||
				V_striEndsWith(szTexturePath, ".jpeg"))
		{
			// General images decoded via stb_image
			int width, height, channels;
			char szFullTexturePath[MAX_PATH] = {};
#ifdef _WIN32
			if (V_isalpha(szTexturePath[0])
					&& szTexturePath[1] == ':'
					&& (szTexturePath[2] == '\\' || szTexturePath[2] == '/'))
#else
			if (szTexturePath[0] == '/')
#endif
			{
				V_strcpy_safe(szFullTexturePath, szTexturePath);
			}
			else
			{
				filesystem->RelativePathToFullPath_safe(szTexturePath, szTextureGroup, szFullTexturePath);
			}
			uint8 *data = stbi_load(szFullTexturePath, &width, &height, &channels, 0);
			if (data)
			{
				if (channels == 3)
				{
					uint8 *rgbaData = reinterpret_cast<uint8 *>(calloc(width * height, sizeof(uint8) * 4));
					ImageLoader::ConvertImageFormat(data, IMAGE_FORMAT_RGB888,
													rgbaData, IMAGE_FORMAT_RGBA8888,
													width, height);
					stbi_image_free(data);
					data = rgbaData;
					channels = 4;
				}
				else if (channels != 4)
				{
					Assert(false);
				}
				iTex = vgui::surface()->CreateNewTextureID(true);
				vgui::surface()->DrawSetTextureRGBAEx(iTex, data, width, height, IMAGE_FORMAT_RGBA8888);
				stbi_image_free(data);
				bApplied = true;
			}
		}
		else if (V_striEndsWith(szTexturePath, ".vtf"))
		{
			// Direct vtf file
			CUtlBuffer buf(0, 0, CUtlBuffer::READ_ONLY);
			if (filesystem->ReadFile(szTexturePath, nullptr, buf))
			{
				IVTFTexture *pVTFTexture = CreateVTFTexture();
				if (pVTFTexture)
				{
					if (pVTFTexture->Unserialize(buf))
					{
						pVTFTexture->ConvertImageFormat(IMAGE_FORMAT_RGBA8888, false);
						iTex = vgui::surface()->CreateNewTextureID(true);
						vgui::surface()->DrawSetTextureRGBAEx(iTex, pVTFTexture->ImageData(0, 0, 0),
														pVTFTexture->Width(), pVTFTexture->Height(),
														IMAGE_FORMAT_RGBA8888);
						bApplied = true;
					}
					DestroyVTFTexture(pVTFTexture);
				}
			}
		}
		else if (!IsErrorMaterial(materials->FindMaterial(szTexturePath, szTextureGroup)))
		{
			// Direct texture determined by vmt (without extension)
			iTex = vgui::surface()->CreateNewTextureID(true);
			vgui::surface()->DrawSetTextureFile(iTex, szTexturePath, false, true);
			bApplied = true;
		}
		hdl = c->htTexMap.Insert(szTexturePath, (bApplied) ? iTex : -1);
	}

	if (hdl != c->htTexMap.InvalidHandle())
	{
		const int iTex = c->htTexMap.Element(hdl);
		if (vgui::surface()->IsTextureIDValid(iTex) && iTex != -1)
		{
			if (c->eMode == MODE_PAINT)
			{
				// Letterboxing
				int iTexWide, iTexTall;
				vgui::surface()->DrawGetTextureSize(iTex, iTexWide, iTexTall);

				int iDispWide, iDispTall;
				if (iTexWide > iTexTall)
				{
					iDispWide = width;
					iDispTall = width * (iTexTall / iTexWide);
				}
				else if (iTexWide < iTexTall)
				{
					iDispWide = height * (iTexWide / iTexTall);
					iDispTall = height;
				}
				else
				{
					iDispWide = Min(width, height);
					iDispTall = iDispWide;
				}

				const int iStartX = x + (width / 2) - (iDispWide / 2);
				const int iStartY = y + (height / 2) - (iDispTall / 2);

				vgui::surface()->DrawSetColor(COLOR_WHITE);
				vgui::surface()->DrawSetTexture(iTex);
				if (texFlags & TEXTUREOPTFLAGS_DONOTCROPTOPANEL)
				{
					vgui::surface()->DrawTexturedRect(
						iStartX,
						iStartY,
						iStartX + iDispWide,
						iStartY + iDispTall);
				}
				else
				{
					// Only about bottom part since top always set to partition's Y position
					const int iImgEnd = iStartY + iDispTall;
					const int iEndOfPanelY = c->dPanel.y + c->dPanel.tall;

					float flPartialShow = 1.0f;
					if (iImgEnd > iEndOfPanelY)
					{
						const int iExtra = iImgEnd - iEndOfPanelY;
						flPartialShow = 1.0f - (iExtra / static_cast<float>(iDispTall));
					}

					vgui::surface()->DrawTexturedSubRect(
						iStartX,
						iStartY,
						iStartX + iDispWide,
						iStartY + (iDispTall * flPartialShow),
						0.0f, 0.0f, 1.0f, flPartialShow);
				}
				vgui::surface()->DrawSetColor(c->colors.normalBg);
			}
			return true;
		}
	}
	return false;
}

NeoUI::RetButton ButtonTexture(const char *szTexturePath, const char *szTextureGroup,
		const wchar_t *wszText)
{
	return BaseButton(wszText, szTexturePath, szTextureGroup, BASEBUTTONTYPE_IMAGE);
}

NeoUI::RetButton ButtonCheckbox(const wchar_t *wszText, const bool bVal)
{
	return BaseButton(wszText, "", "", BASEBUTTONTYPE_CHECKBOX, bVal);
}

NeoUI::RetButton ButtonToggle(const wchar_t *wszText, const bool bVal, const ButtonFlags flags, const float flScrollStart)
{
	return BaseButton(wszText, "", "", BASEBUTTONTYPE_TOGGLE, bVal, flags, flScrollStart);
}

void ResetTextures()
{
	CUtlHashtable<CUtlConstString, int> *pHtTexMap = &(c->htTexMap);
	for (auto hdl = pHtTexMap->FirstHandle(); hdl != pHtTexMap->InvalidHandle(); hdl = pHtTexMap->NextHandle(hdl))
	{
		const int iTex = pHtTexMap->Element(hdl);
		vgui::surface()->DeleteTextureByID(iTex);
	}
	pHtTexMap->Purge();
}

void RingBoxFlag(const int iToggleFlag, int *iFlags, const wchar_t **wszLabelsCustomList)
{
	const bool bWasOnFlag = (*iFlags & iToggleFlag);
	bool bNowOnFlag = bWasOnFlag;
	RingBoxBool(&bNowOnFlag, wszLabelsCustomList);
	if (bNowOnFlag != bWasOnFlag)
	{
		if (bNowOnFlag)
		{
			*iFlags |= iToggleFlag;
		}
		else
		{
			*iFlags &= ~(iToggleFlag);
		}
	}
}

void RingBoxFlag(const wchar_t *wszLeftLabel, const int iToggleFlag, int *iFlags, const wchar_t **wszLabelsCustomList)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	RingBoxFlag(iToggleFlag, iFlags, wszLabelsCustomList);
	EndMultiWidgetHighlighter();
}

void RingBoxBool(bool *bChecked, const wchar_t **wszLabelsCustomList)
{
	int iIndex = static_cast<int>(*bChecked);
	RingBox((wszLabelsCustomList) ? wszLabelsCustomList : ENABLED_LABELS, 2, &iIndex);
	*bChecked = static_cast<bool>(iIndex);
}

void RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked, const wchar_t **wszLabelsCustomList)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	RingBoxBool(bChecked, wszLabelsCustomList);
	EndMultiWidgetHighlighter();
}

void RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	RingBox(wszLabelsList, iLabelsSize, iIndex);
	EndMultiWidgetHighlighter();
}

void RingBox(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);

	// Sanity check if iLabelsSize dynamically changes
	*iIndex = clamp(*iIndex, 0, iLabelsSize - 1);

	if (wdgState.bInView)
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			if (wdgState.bHot)
			{
				vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);
			}
			const auto *pFontI = &c->fonts[c->eFont];

			// Get text size
			const wchar_t *wszText = wszLabelsList[*iIndex];
			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);

			// Center-text label
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + ((c->irWidgetWide / 2) - (iFontWide / 2)),
											c->rWidgetArea.y0 + pFontI->iYFontOffset);
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));

			// Left-side "<" prev button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x0 + c->layout.iRowTall, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_LEFT, 1);

			// Right-side ">" next button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x1 - c->layout.iRowTall, c->rWidgetArea.y0,
											c->rWidgetArea.x1, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - c->layout.iRowTall + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_RIGHT, 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot)
			{
				c->iActive = c->iWidget;
				c->iActiveSection = c->iSection;

				if (c->eCode == MOUSE_LEFT)
				{
					MousePos eMousePos = MOUSEPOS_NONE;
					if (IN_BETWEEN_EQ(c->rWidgetArea.x0, c->iMouseAbsX, c->rWidgetArea.x0 + c->layout.iRowTall))
					{
						eMousePos = MOUSEPOS_LEFT;
					}
					else if (IN_BETWEEN_EQ(c->rWidgetArea.x1 - c->layout.iRowTall, c->iMouseAbsX, c->rWidgetArea.x1))
					{
						eMousePos = MOUSEPOS_RIGHT;
					}

					if (eMousePos == MOUSEPOS_LEFT || eMousePos == MOUSEPOS_RIGHT)
					{
						*iIndex += (eMousePos == MOUSEPOS_LEFT) ? -1 : +1;
						*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
						c->bValueEdited = true;
					}
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive && IsKeyLeftRight())
			{
				*iIndex += (IsKeyLeft()) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				c->bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	EndWidget(wdgState);
}

void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex,
		const TabsFlags flags, TabsState *pState)
{
	Assert(iLabelsSize > 0);

	// This is basically a ringbox but different UI
	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE | WIDGETFLAG_MOUSE | WIDGETFLAG_NOHOTBORDER);

	// Sanity check if iLabelsSize dynamically changes
	*iIndex = clamp(*iIndex, 0, iLabelsSize - 1);

	if (wdgState.bInView)
	{
		const int iTabWideEqual = (c->dPanel.wide / iLabelsSize);
		int iTabWide = 0;
		int iTabOffset = 0;
		if (pState)
		{
			const bool bThisYScroll = (c->ibfSectionHasYScroll & (1ULL << c->iSection));
			if (c->bFirstCtxUse
					|| pState->iWide <= 0
					|| pState->bInYScroll != bThisYScroll)
			{
				for (int i = 0; i < iLabelsSize; ++i)
				{
					int iCurTabWide, iTabTall;
					vgui::surface()->GetTextSize(c->fonts[c->eFont].hdl, wszLabelsList[i], iCurTabWide, iTabTall);
					iTabWide = Max(iCurTabWide, iTabWide);
				}
				iTabWide += 2 * c->iMarginX;
				// Just go use equal tab spacing if the text + X-margins fits
				iTabWide = Max(iTabWide, iTabWideEqual);
				pState->iWide = iTabWide;
				pState->iOffset = 0;
				pState->bInYScroll = bThisYScroll;
				Assert(iTabWide > 0);
			}
			iTabWide = pState->iWide;
			iTabOffset = pState->iOffset;
		}
		else
		{
			iTabWide = iTabWideEqual;
		}

		const int iOverflowBtnX = (iTabWide > (c->dPanel.wide / iLabelsSize))
				? c->layout.iRowTall : 0;
		
		const int iTotalTabsWidth = (iTabWide * iLabelsSize);
		const int iTabOffsetMax = iTotalTabsWidth - c->dPanel.wide + (iOverflowBtnX * 2);
		bool bResetSectionStates = false;
		bool bShiftOffset = false;

		const bool bHotLeft = wdgState.bHot
				&& IN_BETWEEN_EQ(c->rWidgetArea.x0,
						c->iMouseAbsX,
						c->rWidgetArea.x0 + iOverflowBtnX);
		const bool bHotRight = wdgState.bHot
				&& IN_BETWEEN_EQ(c->rWidgetArea.x1 - iOverflowBtnX,
						c->iMouseAbsX,
						c->rWidgetArea.x1);

		const int iMWheelJump = iTabWide * 0.2;
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			const int iXViewportStart = c->rWidgetArea.x0 + iOverflowBtnX;
			const int iXViewportWide = c->irWidgetWide - (iOverflowBtnX * 2);
			vgui::surface()->SetFullscreenViewport(
					iXViewportStart,
					c->rWidgetArea.y0,
					iXViewportWide,
					c->irWidgetTall);
			const int iXViewportEnd = iXViewportStart + iXViewportWide;
			vgui::surface()->PushFullscreenViewport();

			const auto *pFontI = &c->fonts[c->eFont];
			for (int i = 0, iXPosTab = 0; i < iLabelsSize; ++i, iXPosTab += iTabWide)
			{
				vgui::IntRect tabRect = {
					.x0 = iTabOffset + iXPosTab,
					.y0 = 0,
					.x1 = iTabOffset + iXPosTab + iTabWide,
					.y1 = c->irWidgetTall,
				};
				// If in equal sizing wide, last tab, and no offset, make sure it expands
				// fully to the right edge
				if (i == (iLabelsSize - 1) && iTabWideEqual == iTabWide && 0 == iTabOffset)
				{
					tabRect.x1 = iXViewportWide;
				}
				const bool bHotTab = IN_BETWEEN_EQ(iXViewportStart, c->iMouseAbsX, iXViewportEnd)
						&& IN_BETWEEN_EQ(tabRect.x0 + iXViewportStart, c->iMouseAbsX, tabRect.x1 + iXViewportStart)
						&& IN_BETWEEN_EQ(tabRect.y0 + c->rWidgetArea.y0, c->iMouseAbsY, tabRect.y1 + c->rWidgetArea.y0);
				const bool bActiveTab = i == *iIndex;
				if (bHotTab || bActiveTab)
				{
					vgui::surface()->DrawSetColor(bActiveTab ? c->colors.activeBg : c->colors.hotBg);
					vgui::surface()->DrawFilledRectArray(&tabRect, 1);
				}
				const wchar_t *wszText = wszLabelsList[i];
				int wide, tall;
				vgui::surface()->GetTextSize(c->fonts[c->eFont].hdl, wszText, wide, tall);
				vgui::surface()->DrawSetTextPos(iTabOffset + iXPosTab + ((iTabWide - wide) * 0.5), pFontI->iYFontOffset);
				vgui::surface()->DrawSetTextColor(bActiveTab ? c->colors.activeFg : bHotTab ? c->colors.hotFg : c->colors.normalFg);
				vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
				if (bHotTab)
				{
					DrawBorder(tabRect);
				}
			}
			vgui::surface()->PopFullscreenViewport();
			vgui::surface()->SetFullscreenViewport(0, 0, 0, 0);

			if (iOverflowBtnX > 0)
			{
				const bool bIndicateLeft = iTabOffset < 0;
				const bool bIndicateRight = iTabOffset > -iTabOffsetMax;

				// Show left button
				vgui::surface()->DrawSetColor(bHotLeft ? c->colors.activeBg : c->colors.normalBg);
				vgui::surface()->DrawSetTextColor(bHotLeft ? c->colors.activeFg : c->colors.normalFg);
				vgui::surface()->DrawFilledRect(
						c->rWidgetArea.x0,
						c->rWidgetArea.y0,
						c->rWidgetArea.x0 + iOverflowBtnX,
						c->rWidgetArea.y1);

				vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + pFontI->iStartBtnXPos,
												c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
				vgui::surface()->DrawPrintText(bIndicateLeft ? WSZ_ARROW_LEFT : L"|", 1);

				// Show right button
				vgui::surface()->DrawSetColor(bHotRight ? c->colors.activeBg : c->colors.normalBg);
				vgui::surface()->DrawSetTextColor(bHotRight ? c->colors.activeFg : c->colors.normalFg);
				vgui::surface()->DrawFilledRect(
						c->rWidgetArea.x1 - iOverflowBtnX,
						c->rWidgetArea.y0,
						c->rWidgetArea.x1,
						c->rWidgetArea.y1);

				vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - iOverflowBtnX + pFontI->iStartBtnXPos,
												c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
				vgui::surface()->DrawPrintText(bIndicateRight ? WSZ_ARROW_RIGHT : L"|", 1);
			}

			if (!(flags & TABFLAG_NOSIDEKEYS))
			{
				// Draw the side-hints text
				// NEO NOTE (nullsystem): F# as 1 is thinner than 3/not monospaced font
				int iFontWidth, iFontHeight;
				vgui::surface()->GetTextSize(c->fonts[c->eFont].hdl, L"F #", iFontWidth, iFontHeight);
				const int iHintYPos = c->rWidgetArea.y0 + pFontI->iYFontOffset;

				vgui::surface()->DrawSetTextColor(c->colors.tabHintsFg);
				vgui::surface()->DrawSetTextPos(c->dPanel.x - c->iMarginX - iFontWidth, iHintYPos);
				vgui::surface()->DrawPrintText((c->eKeyHints == KEYHINTS_KEYBOARD) ? L"F 1" : L"L 1", 3);
				vgui::surface()->DrawSetTextPos(c->dPanel.x + c->dPanel.wide + c->iMarginX, iHintYPos);
				vgui::surface()->DrawPrintText((c->eKeyHints == KEYHINTS_KEYBOARD) ? L"F 3" : L"R 1", 3);
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot && c->eCode == MOUSE_LEFT)
			{
				if (bHotLeft || bHotRight)
				{
					*iIndex = clamp(*iIndex + ((bHotLeft) ? -1 : +1), 0, iLabelsSize - 1);
					bShiftOffset = true;
				}
				else if (c->iMouseAbsX >= (c->rWidgetArea.x0 + iOverflowBtnX)
						&& c->iMouseAbsX <= c->rWidgetArea.x0 + iOverflowBtnX + (iTabWide * iLabelsSize))
				{
					const int iNextIndex = ((-iTabOffset + c->iMouseAbsX - c->rWidgetArea.x0 - iOverflowBtnX) / iTabWide);
					if (iNextIndex != *iIndex)
					{
						*iIndex = clamp(iNextIndex, 0, iLabelsSize - 1);
						bResetSectionStates = true;
						bShiftOffset = true;
					}
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (!(flags & TABFLAG_NOSIDEKEYS))
			{
				const bool bLeftKey = c->eCode == KEY_F1 || c->eCode == KEY_XBUTTON_LEFT_SHOULDER ||
						c->eCode == STEAMCONTROLLER_LEFT_TRIGGER;
				const bool bRightKey = c->eCode == KEY_F3 || c->eCode == KEY_XBUTTON_RIGHT_SHOULDER ||
						c->eCode == STEAMCONTROLLER_RIGHT_TRIGGER;
				if (bLeftKey || bRightKey) // Global keybind
				{
					*iIndex += (bLeftKey) ? -1 : +1;
					*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
					bShiftOffset = true;
					bResetSectionStates = true;
				}
			}
		}
		break;
		case MODE_MOUSEWHEELED:
			if (wdgState.bHot && iOverflowBtnX > 0)
			{
				iTabOffset += (c->eCode == MOUSE_WHEEL_UP) ? iMWheelJump : -iMWheelJump;
				iTabOffset = clamp(iTabOffset, -iTabOffsetMax, 0);
				c->bBlockSectionMWheel = true;
			}
			break;
		default:
			break;
		}

		if (bShiftOffset)
		{
			const int distanceToStartOfButton = *iIndex * iTabWide;
			if (-iTabOffset > distanceToStartOfButton)
			{
				iTabOffset = -distanceToStartOfButton;
			}
			const int distanceToEndOfButton = (*iIndex + 1) * iTabWide;
			if (-iTabOffset + c->irWidgetWide < distanceToEndOfButton)
			{
				iTabOffset = c->irWidgetWide - distanceToEndOfButton - (iOverflowBtnX * 2);
			}
		}
		if (bResetSectionStates && !(flags & TABFLAG_NOSTATERESETS))
		{
			V_memset(c->iYOffset, 0, sizeof(c->iYOffset));
			V_memset(c->iXOffset, 0, sizeof(c->iXOffset));
			c->iActive = FOCUSOFF_NUM;
			c->iActiveSection = -1;
			c->iHot = FOCUSOFF_NUM;
			c->iHotSection = -1;
			iTabOffset = 0;
		}
		if (pState)
		{
			pState->iOffset = iTabOffset;
		}
	}

	EndWidget(wdgState);
}

static float ClampAndLimitDp(const float curValue, const float flMin, const float flMax, const int iDp)
{
	const float flDpMult = pow(10, iDp);
	float nextValue = curValue;
	nextValue = clamp(nextValue, flMin, flMax);
	nextValue = roundf(nextValue * flDpMult) / flDpMult;
	return nextValue;
}

void Progress(const float flValue, const float flMin, const float flMax)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE);
	if (wdgState.bInView && c->eMode == MODE_PAINT)
	{
		vgui::surface()->DrawSetColor(c->colors.progressBarBg);
		const float flPerc = (flValue - flMin) / (flMax - flMin);
		vgui::surface()->DrawFilledRect(c->rWidgetArea.x0,
										c->rWidgetArea.y0,
										c->rWidgetArea.x0 + (flPerc * c->irWidgetWide),
										c->rWidgetArea.y1);
	}
	EndWidget(wdgState);
}

void ProgressDrag(float *flValue, const float flMin, const float flMax)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);
	if (wdgState.bInView)
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			vgui::surface()->DrawSetColor(c->colors.progressBarBg);
			const float flPerc = (*flValue - flMin) / (flMax - flMin);
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0,
											c->rWidgetArea.y0,
											c->rWidgetArea.x0 + (flPerc * c->irWidgetWide),
											c->rWidgetArea.y1);
		} break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot && c->eCode == MOUSE_LEFT)
			{
				c->iActive = c->iWidget;
				c->iActiveSection = c->iSection;
				c->eMousePressedStart = MOUSESTART_SLIDER;
			}
		} [[fallthrough]];
		case MODE_MOUSEMOVED:
		{
			if (wdgState.bActive
					&& vgui::input()->IsMouseDown(MOUSE_LEFT)
					&& c->eMousePressedStart == MOUSESTART_SLIDER)
			{
				const int iMouseRelXWidget = c->iMouseAbsX - c->rWidgetArea.x0;
				const float flPerc = static_cast<float>(iMouseRelXWidget) / static_cast<float>(c->irWidgetWide);
				*flValue = flMin + (flPerc * (flMax - flMin));
				*flValue = ClampAndLimitDp(*flValue, flMin, flMax, 2);
				c->bValueEdited = true;
			}
		} break;
		default:
			break;
		}
	}
	EndWidget(wdgState);
}

void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp, const float flStep, const wchar_t *wszSpecialText)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);

	if (wdgState.bInView)
	{
		auto hdl = c->htSliders.Find(wszLeftLabel);
		if (hdl == -1 || c->htSliders.Element(hdl).flCachedValue != *flValue ||
				c->htSliders.Element(hdl).bActive != wdgState.bActive)
		{
			// Update string/cached value
			wchar_t wszFormat[32];
			V_swprintf_safe(wszFormat, L"%%0.%df", iDp);

			if (hdl == -1)
			{
				SliderInfo sInfoNew = {};
				hdl = c->htSliders.Insert(wszLeftLabel, sInfoNew);
			}
			SliderInfo *pSInfo = &c->htSliders.Element(hdl);

			V_swprintf_safe(pSInfo->wszText, wszFormat, *flValue);
			pSInfo->flCachedValue = *flValue;
			wchar_t wszTmpTest[ARRAYSIZE(pSInfo->wszText)];
			const int iStrMinLen = V_swprintf_safe(wszTmpTest, wszFormat, flMin);
			const int iStrMaxLen = V_swprintf_safe(wszTmpTest, wszFormat, flMax);
			pSInfo->iMaxStrSize = Max(iStrMinLen, iStrMaxLen);
			pSInfo->bActive = wdgState.bActive;
		}
		SliderInfo *pSInfo = &c->htSliders.Element(hdl);

		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			const auto *pFontI = &c->fonts[c->eFont];
			if (wdgState.bHot)
			{
				vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);
			}

			// Left-side "<" prev button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x0 + c->layout.iRowTall, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_LEFT, 1);

			// Right-side ">" next button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x1 - c->layout.iRowTall, c->rWidgetArea.y0,
											c->rWidgetArea.x1, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - c->layout.iRowTall + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_RIGHT, 1);

			// Center-text text dimensions
			const bool bSpecial = (wszSpecialText && !wdgState.bActive && *flValue == 0.0f);
			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, bSpecial ? wszSpecialText : pSInfo->wszText, iFontWide, iFontTall);
			const int iFontStartX = ((c->irWidgetWide / 2) - (iFontWide / 2));
			const int iFontEndX = ((c->irWidgetWide / 2) + (iFontWide / 2));
			const int iFontStartPadX = iFontStartX - c->iMarginX;
			const int iFontEndPadX = iFontEndX + c->iMarginX;

			// Background bars render
			const float flPerc = (((iDp == 0) ? (int)(*flValue) : *flValue) - flMin) / (flMax - flMin);
			const float flBarMaxWide = static_cast<float>(c->irWidgetWide - (2 * c->layout.iRowTall));
			const int iBarEndX = c->layout.iRowTall + static_cast<int>(flPerc * flBarMaxWide);
			const int iBarMarginY = static_cast<int>(1.5f * c->iMarginY);
			vgui::surface()->DrawSetColor(wdgState.bActive ? c->colors.sliderActiveBg : wdgState.bHot ? c->colors.sliderHotBg : c->colors.sliderNormalBg);
			if (flPerc > 0.0f)
			{
				vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + c->layout.iRowTall,
												c->rWidgetArea.y0 + iBarMarginY,
												c->rWidgetArea.x0 + Min(iBarEndX, iFontStartPadX),
												c->rWidgetArea.y1 - iBarMarginY);
			}
			if (iFontEndX < iBarEndX)
			{
				vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + iFontEndPadX,
												c->rWidgetArea.y0 + iBarMarginY,
												c->rWidgetArea.x0 + iBarEndX,
												c->rWidgetArea.y1 - iBarMarginY);
			}

			// Center-text text render
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + iFontStartX,
											c->rWidgetArea.y0 + pFontI->iYFontOffset);
			vgui::surface()->DrawPrintText(bSpecial ? wszSpecialText : pSInfo->wszText,
										   bSpecial ? V_wcslen(wszSpecialText) : V_wcslen(pSInfo->wszText));

			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					const int iMarkX = iFontStartX + iFontWide;
					vgui::surface()->DrawSetColor(c->colors.cursor);
					vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + iMarkX,
													c->rWidgetArea.y0 + pFontI->iYFontOffset,
													c->rWidgetArea.x0 + iMarkX + c->iMarginX,
													c->rWidgetArea.y0 + pFontI->iYFontOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot)
			{
				c->iActive = c->iWidget;
				c->iActiveSection = c->iSection;
				if (c->eCode == MOUSE_LEFT)
				{
					MousePos eMousePos = MOUSEPOS_CENTER;
					if (IN_BETWEEN_EQ(c->rWidgetArea.x0, c->iMouseAbsX, c->rWidgetArea.x0 + c->layout.iRowTall))
					{
						eMousePos = MOUSEPOS_LEFT;
					}
					else if (IN_BETWEEN_EQ(c->rWidgetArea.x1 - c->layout.iRowTall, c->iMouseAbsX, c->rWidgetArea.x1))
					{
						eMousePos = MOUSEPOS_RIGHT;
					}

					if (eMousePos == MOUSEPOS_LEFT || eMousePos == MOUSEPOS_RIGHT)
					{
						*flValue += (eMousePos == MOUSEPOS_LEFT) ? -flStep : +flStep;
						*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
						c->bValueEdited = true;
					}
					else // MOUSEPOS_CENTER
					{
						c->iActive = c->iWidget;
						c->iActiveSection = c->iSection;
						c->eMousePressedStart = MOUSESTART_SLIDER;
					}
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive)
			{
				if (IsKeyLeftRight())
				{
					*flValue += (IsKeyLeft()) ? -flStep : +flStep;
					*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
					c->bValueEdited = true;
				}
				else if (IsKeyEnter())
				{
					c->iActive = FOCUSOFF_NUM;
					c->iActiveSection = -1;
				}
				else if (c->eCode == KEY_BACKSPACE)
				{
					const int iTextSize = V_wcslen(pSInfo->wszText);
					if (iTextSize > 0)
					{
						pSInfo->wszText[iTextSize - 1] = L'\0';
						if (iTextSize > 1)
						{
							char szText[ARRAYSIZE(pSInfo->wszText)];
							g_pVGuiLocalize->ConvertUnicodeToANSI(pSInfo->wszText, szText, sizeof(szText));
							const float flFromTextValue = atof(szText);
							if (IN_BETWEEN_EQ(flMin, flFromTextValue, flMax))
							{
								*flValue = flFromTextValue;
								pSInfo->flCachedValue = *flValue;
								c->bValueEdited = true;
							}
						}
					}
				}
			}
			else if (wdgState.bHot)
			{
				if (IsKeyEnter())
				{
					c->iActive = c->iWidget;
					c->iActiveSection = c->iSection;
				}
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (wdgState.bActive && vgui::input()->IsMouseDown(MOUSE_LEFT) && c->eMousePressedStart == MOUSESTART_SLIDER)
			{
				const int iMouseRelXWidget = c->iMouseAbsX - c->rWidgetArea.x0;
				const float flPerc = static_cast<float>(iMouseRelXWidget - c->layout.iRowTall) / static_cast<float>(c->irWidgetWide - (2 * c->layout.iRowTall));
				*flValue = flMin + (flPerc * (flMax - flMin));
				*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
				c->bValueEdited = true;
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive)
			{
				const int iTextSize = V_wcslen(pSInfo->wszText);
				const wchar_t uc = c->unichar;
				if (iTextSize < pSInfo->iMaxStrSize &&
						(iswdigit(uc)
						 || (uc == L'.' && iDp > 0 && !wcsrchr(pSInfo->wszText, L'.'))
						 || (uc == L'-' && iTextSize == 0 && flMin < 0.0f)))
				{
					pSInfo->wszText[iTextSize] = uc;
					pSInfo->wszText[iTextSize + 1] = L'\0';
					char szText[ARRAYSIZE(pSInfo->wszText)];
					g_pVGuiLocalize->ConvertUnicodeToANSI(pSInfo->wszText, szText, sizeof(szText));
					const float flFromTextValue = atof(szText);
					if (IN_BETWEEN_EQ(flMin, flFromTextValue, flMax))
					{
						*flValue = flFromTextValue;
						pSInfo->flCachedValue = *flValue;
						c->bValueEdited = true;
					}
				}
			}
		}
		break;
		default:
			break;
		}
	}

	EndWidget(wdgState);
	EndMultiWidgetHighlighter();
}

void SliderInt(const wchar_t *wszLeftLabel, int *iValue, const int iMin, const int iMax, const int iStep, const wchar_t *wszSpecialText)
{
	const float flOrigValue = *iValue;
	float flValue = flOrigValue;
	Slider(wszLeftLabel, &flValue, static_cast<float>(iMin), static_cast<float>(iMax), 0, static_cast<float>(iStep), wszSpecialText);
	if (flValue != flOrigValue)
	{
		*iValue = RoundFloatToInt(flValue);
	}
}

void SliderU8(const wchar_t *wszLeftLabel, uint8 *ucValue, const uint8 iMin, const uint8 iMax, const uint8 iStep, const wchar_t *wszSpecialText)
{
	const float flOrigValue = *ucValue;
	float flValue = flOrigValue;
	Slider(wszLeftLabel, &flValue, static_cast<float>(iMin), static_cast<float>(iMax), 0, static_cast<float>(iStep), wszSpecialText);
	if (flValue != flOrigValue)
	{
		*ucValue = static_cast<uint8>(RoundFloatToInt(flValue));
	}
}

void ColorEdit(uint8 *r, uint8 *g, uint8 *b, uint8 *a)
{
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);

	if (wdgState.bInView)
	{
		bool bShowPopup = false;

		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			vgui::surface()->DrawSetColor(*r, *g, *b, *a);
			vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			bShowPopup = (wdgState.bHot && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			bShowPopup = (wdgState.bActive && IsKeyEnter());
		}
		break;
		default:
			break;
		}

		if (bShowPopup)
		{
			OpenPopup(INTERNALPOPUP_COLOREDIT, Dim{
						.x = c->rWidgetArea.x0,
						.y = c->rWidgetArea.y1,
						.wide = c->irWidgetWide,
						.tall = c->layout.iDefRowTall * 4,
					});
			c->colorEditInfo.r = r;
			c->colorEditInfo.g = g;
			c->colorEditInfo.b = b;
			c->colorEditInfo.a = a;
		}
	}

	EndWidget(wdgState);
}

void ColorEdit(const wchar_t *wszLeftLabel, uint8 *r, uint8 *g, uint8 *b, uint8 *a)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	ColorEdit(r, g, b, a);
	EndMultiWidgetHighlighter();
}

static int TextEditChIdxFromMouse(const int iWszTextSize)
{
	if (iWszTextSize <= 0)
	{
		return 0;
	}

	const int iMouseOnXWidth = c->iMouseAbsX - (c->rWidgetArea.x0 + c->iMarginX);
	int iChIdx = -1;
	for (int i = 0; i < iWszTextSize; ++i)
	{
		if (c->irTextWidths[i] > iMouseOnXWidth)
		{
			// Check if left or right of middle of character
			int iMidPoint = 0;
			if (i == 0)
			{
				const int iChWidth = c->irTextWidths[i];
				iMidPoint = iChWidth / 2;
			}
			else
			{
				const int iChWidth = c->irTextWidths[i] - c->irTextWidths[i - 1];
				iMidPoint = c->irTextWidths[i - 1] + (iChWidth / 2);
			}
			iChIdx = (iMouseOnXWidth >= iMidPoint) ? i + 1 : i;
			break;
		}
	}
	if (iChIdx == -1)
	{
		if (iMouseOnXWidth >= c->irTextWidths[iWszTextSize - 1])
		{
			iChIdx = iWszTextSize;
		}
		else
		{
			iChIdx = 0;
		}
	}
	return iChIdx;
}

// NEO NOTE (nullsystem): Build up irTextWidths, hot spot so do only when necessary
//
// wszVisText could be a password for visuals so a separate iWszTextSize
// which is always set by V_wcslen(wszText) instead
static void BuildUpIrTextWidths(const wchar_t *wszVisText, const int iWszTextSize)
{
	Assert(iWszTextSize < MAX_TEXTINPUT_U8BYTES_LIMIT);

	wchar_t wszMutVisText[MAX_TEXTINPUT_U8BYTES_LIMIT];
	V_wcscpy_safe(wszMutVisText, wszVisText);

	const auto *pFontI = &c->fonts[c->eFont];

	V_memset(c->irTextWidths, 0, sizeof(c->irTextWidths));
	for (int i = 0; i < iWszTextSize; ++i)
	{
		const wchar_t wchTemp = wszMutVisText[i + 1];
		wszMutVisText[i + 1] = '\0';
		int iFontWide, iFontTall;
		vgui::surface()->GetTextSize(pFontI->hdl, wszMutVisText, iFontWide, iFontTall);
		c->irTextWidths[i] = iFontWide;
		wszMutVisText[i + 1] = wchTemp;
	}
}

void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxWszTextSize, const TextEditFlags flags)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	TextEdit(wszText, iMaxWszTextSize, flags);
	EndMultiWidgetHighlighter();
}

void TextEdit(wchar_t *wszText, const int iMaxWszTextSize, const TextEditFlags flags)
{
	WidgetFlag wdgFlags = WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE;
	if (flags & TEXTEDITFLAG_FORCEACTIVE) wdgFlags |= WIDGETFLAG_FORCEACTIVE;
	const auto wdgState = BeginWidget(wdgFlags);

	static wchar_t staticWszPasswordChars[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
	if (staticWszPasswordChars[0] == L'\0')
	{
		for (int i = 0; i < (ARRAYSIZE(staticWszPasswordChars) - 1); ++i)
		{
			staticWszPasswordChars[i] = L'*';
		}
	}

	if (wdgState.bInView)
	{
		ECopyMenu iCopyAction = COPYMENU_NIL;

		if (IN_BETWEEN_AR(COPYMENU_CUT, c->eRightClickCopyMenuRet, COPYMENU__TOTAL) &&
				c->iPrePopupActive == c->iWidget &&
				c->iPrePopupActiveSection == c->iSection)
		{
			iCopyAction = c->eRightClickCopyMenuRet;
			c->eRightClickCopyMenuRet = COPYMENU_NIL;
		}

		const bool bHasTextSel = wdgState.bActive;
		const bool bCurRightStart = (c->iTextSelCur >= c->iTextSelStart);

		// NEO NOTE (nullsystem): Only use iWszTextSize when not changing it
		// EX: paint, cursor/selection only change actions, or just before
		// changing it. Otherwise use V_wcslen directly after changing wszText
		// contents.
		const int iWszTextSize = V_wcslen(wszText);

		// Caused by forced input, set it to the end of the text
		if (bHasTextSel && c->iTextSelStart == -1 && c->iTextSelCur == -1)
		{
			c->iTextSelStart = c->iTextSelCur = iWszTextSize;
		}

		const int iTextSelMin = bCurRightStart ? c->iTextSelStart : c->iTextSelCur;
		const int iTextSelMax = bCurRightStart ? c->iTextSelCur : c->iTextSelStart;

		const bool bIsSelection = bHasTextSel && iTextSelMin < iTextSelMax;
		const bool bIsCursor = bHasTextSel && iTextSelMin == iTextSelMax;
		const bool bFromEnd = bHasTextSel && (iTextSelMin == iTextSelMax && iWszTextSize <= iTextSelMin);

		const bool bIsPassword = flags & TEXTEDITFLAG_PASSWORD;
		if (bIsPassword)
		{
			staticWszPasswordChars[iWszTextSize + 1] = L'\0';
		}

		bool bRebuildIrTextWidths = false;

		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
			const auto *pFontI = &c->fonts[c->eFont];
			if (wdgState.bHot)
			{
				vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
												c->rWidgetArea.x1, c->rWidgetArea.y1);
			}
#if defined(DEBUG) && DEBUG_NEOUI // Right-aligned text to show debug info
			vgui::surface()->DrawSetTextColor(Color(32, 32, 180, 255));
			wchar_t wszDbgText[MAX_TEXTINPUT_U8BYTES_LIMIT];
			V_swprintf_safe(wszDbgText, L"%d %d %d %d",
					iTextSelMin, iTextSelMax, c->iTextSelStart, c->iTextSelCur);
			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, wszDbgText, iFontWide, iFontTall);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - c->iMarginX - iFontWide,
											c->rWidgetArea.y0 + pFontI->iYFontOffset);
			vgui::surface()->DrawPrintText(wszDbgText, V_wcslen(wszDbgText));
			vgui::surface()->DrawSetTextColor(c->colors.normalFg);
#endif // defined(DEBUG) && DEBUG_NEOUI
			if (bHasTextSel && (bIsSelection || bEditBlinkShow))
			{
				int iXStart;
				if (iTextSelMin <= 0)
				{
					iXStart = 0;
				}
				else if (iTextSelMin >= iWszTextSize)
				{
					iXStart = c->irTextWidths[iWszTextSize - 1];
				}
				else
				{
					iXStart = c->irTextWidths[iTextSelMin - 1];
				}

				// If bIsCursor, render cursor-like, otherwise selection
				int iXEnd;
				if (bIsCursor)
				{
					iXEnd = iXStart + (c->iMarginX / 2);
				}
				else
				{
					if (iTextSelMax <= 0)
					{
						iXEnd = 0;
					}
					else if (iTextSelMax >= iWszTextSize)
					{
						iXEnd = c->irTextWidths[iWszTextSize - 1];
					}
					else
					{
						iXEnd = c->irTextWidths[iTextSelMax - 1];
					}
				}

				if (iXStart != iXEnd)
				{
					vgui::surface()->DrawSetColor(
							bIsCursor ? c->colors.cursor : c->colors.textSelectionBg);
					vgui::surface()->DrawFilledRect(
							c->rWidgetArea.x0 + c->iMarginX + iXStart,
							c->rWidgetArea.y0 + ((bIsCursor) ? pFontI->iYFontOffset : 0),
							c->rWidgetArea.x0 + c->iMarginX + iXEnd,
							(bIsCursor) ? c->rWidgetArea.y0 + pFontI->iYFontOffset + vgui::surface()->GetFontTall(pFontI->hdl) : c->rWidgetArea.y1);
					vgui::surface()->DrawSetColor(c->colors.normalBg);
				}
			}
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + c->iMarginX,
											c->rWidgetArea.y0 + pFontI->iYFontOffset);
			const wchar_t *pwszPrintText = (bIsPassword) ? staticWszPasswordChars : wszText;
			vgui::surface()->DrawPrintText(pwszPrintText, iWszTextSize);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (!IN_BETWEEN_AR(COPYMENU_CUT, iCopyAction, COPYMENU__TOTAL) && wdgState.bHot)
			{
				c->iActive = c->iWidget;
				c->iActiveSection = c->iSection;

				switch (c->eCode)
				{
				case MOUSE_LEFT:
				{
					BuildUpIrTextWidths(bIsPassword ? staticWszPasswordChars : wszText, iWszTextSize);
					const int iStartPos = TextEditChIdxFromMouse(iWszTextSize);

					// Selection start point
					c->iTextSelStart = iStartPos;
					c->iTextSelCur = iStartPos;
					c->iTextSelDrag = c->iWidget;
					c->iTextSelDragSection = c->iSection;
				} break;
				case MOUSE_RIGHT:
				{
					// Open right-click menu
					OpenPopup(INTERNALPOPUP_COPYMENU, Dim{
								.x = c->iMouseAbsX,
								.y = c->iMouseAbsY,
								.wide = SuitableWideByWStr(L"Paste", SUITABLEWIDE_POPUP),
								.tall = c->layout.iDefRowTall * 3,
							});
					c->eRightClickCopyMenuRet = COPYMENU_NIL;
				} break;
				default:
					break;
				}
			}
		}
		break;
		case MODE_MOUSERELEASED:
		{
			c->iTextSelDrag = -1;
			c->iTextSelDragSection = -1;
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (c->iTextSelDrag == c->iWidget && c->iTextSelDragSection == c->iSection)
			{
				c->iTextSelCur = TextEditChIdxFromMouse(iWszTextSize);
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			const bool bIsCtrlDown = (vgui::input()->IsKeyDown(KEY_LCONTROL) || vgui::input()->IsKeyDown(KEY_RCONTROL));
			if (bIsCtrlDown && bHasTextSel)
			{
				switch (c->eCode)
				{
				break; case KEY_X: if (bIsSelection) iCopyAction = COPYMENU_CUT;
				break; case KEY_C: if (bIsSelection) iCopyAction = COPYMENU_COPY;
				break; case KEY_V: iCopyAction = COPYMENU_PASTE;
				break; default: break;
				}
			}
			if (!IN_BETWEEN_AR(COPYMENU_CUT, iCopyAction, COPYMENU__TOTAL) && wdgState.bActive)
			{
				const bool bIsShiftDown = (vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT));
				switch (c->eCode)
				{
				case KEY_HOME:
				case KEY_END:
				{
					c->iTextSelCur = (c->eCode == KEY_HOME) ? 0 : iWszTextSize;
					if (!bIsShiftDown)
					{
						c->iTextSelStart = c->iTextSelCur;
					}
				} break;
				case KEY_LEFT:
				case KEY_RIGHT:
				{
					if (bIsCtrlDown)
					{
						// NEO NOTE (nullsystem): This matches vgui behavior
						// which is always start of word even on CTRL+right
						// and only space used for delimiter
						int iNextCur = (c->eCode == KEY_LEFT) ? 0 : iWszTextSize;
						const int iIncr = ((c->eCode == KEY_LEFT) ? -1 : +1);

						for (int i = c->iTextSelCur + iIncr;
								IN_BETWEEN_AR(1, i, iWszTextSize);
								i += iIncr)
						{
							if (wszText[i - 1] == L' ' && wszText[i] != L' ')
							{
								iNextCur = i;
								break;
							}
						}
						c->iTextSelCur = iNextCur;
					}
					else
					{
						c->iTextSelCur = clamp(c->iTextSelCur + ((c->eCode == KEY_LEFT) ? -1 : +1), 0, iWszTextSize);
					}

					if (!bIsShiftDown)
					{
						c->iTextSelStart = c->iTextSelCur;
					}
				} break;
				case KEY_BACKSPACE:
				case KEY_DELETE:
				{
					if (iWszTextSize > 0)
					{
						bool bIsValueEdited = true;
						const bool bIsLeftDelete = (c->eCode == KEY_BACKSPACE);
						// Write to temporary first otherwise wszText -> wszText can corrupt
						static wchar_t wszStaticTmpText[MAX_TEXTINPUT_U8BYTES_LIMIT];

						if (bFromEnd && bIsLeftDelete)
						{
							// Cursor - At the end
							wszText[iWszTextSize - 1] = L'\0';
							--c->iTextSelStart;
						}
						else if (bIsCursor)
						{
							if (iTextSelMin > 0 && bIsLeftDelete)
							{
								// Cursor - delete character before cursor, non-zero, cursor shift left
								V_swprintf_safe(wszStaticTmpText, L"%.*ls%ls", iTextSelMin - 1, wszText, wszText + iTextSelMin);
								V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
								--c->iTextSelStart;
							}
							else if (iTextSelMin < iWszTextSize && !bIsLeftDelete)
							{
								// Cursor - delete character after cursor, non-end
								V_swprintf_safe(wszStaticTmpText, L"%.*ls%ls", iTextSelMin, wszText, wszText + iTextSelMin + 1);
								V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
							}
						}
						else if (bIsSelection)
						{
							// Selection - delete selected texts
							wszText[iTextSelMin] = L'\0';
							V_swprintf_safe(wszStaticTmpText, L"%ls%ls", wszText, wszText + iTextSelMax);
							V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
							c->iTextSelStart = iTextSelMin;
						}
						else
						{
							bIsValueEdited = false;
						}

						if (bIsValueEdited)
						{
							bRebuildIrTextWidths = true;
							c->iTextSelCur = c->iTextSelStart;
						}
					}
				} break;
				default:
					break;
				}
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive && iswprint(c->unichar) && iWszTextSize < iMaxWszTextSize)
			{
				if (bFromEnd)
				{
					wszText[iWszTextSize] = c->unichar;
					wszText[iWszTextSize + 1] = L'\0';
				}
				else if (bIsCursor)
				{
					for (int i = (iWszTextSize - 1); i >= iTextSelMin; --i)
					{
						wszText[i + 1] = wszText[i];
					}
					wszText[iTextSelMin] = c->unichar;
					wszText[iWszTextSize + 1] = L'\0';
				}
				else if (bIsSelection)
				{
					// Has multi selection - delete selected texts then insert text
					static wchar_t wszStaticTmpText[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
					V_swprintf_safe(wszStaticTmpText, L"%.*ls%lc%ls", iTextSelMin, wszText, c->unichar, wszText + iTextSelMax);
					V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
				}
				bRebuildIrTextWidths = true;
				// Any character typed devolves back to cursor on iTextSelMin
				c->iTextSelStart = Min(iTextSelMin + 1, V_wcslen(wszText));
				c->iTextSelCur = c->iTextSelStart;
			}
		}
		break;
		default:
			break;
		}

		switch (iCopyAction)
		{
		case COPYMENU_CUT:
		case COPYMENU_COPY:
		{
			if (iTextSelMin < iTextSelMax)
			{
				const wchar_t wchTmp = wszText[iTextSelMax];
				wszText[iTextSelMax] = L'\0';

				wchar_t wszCopyBuf[MAX_TEXTINPUT_U8BYTES_LIMIT];
				V_wcscpy_safe(wszCopyBuf, wszText + iTextSelMin);

				wszText[iTextSelMax] = wchTmp;

				char szCopyBuf[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
				g_pVGuiLocalize->ConvertUnicodeToANSI(wszCopyBuf, szCopyBuf, sizeof(szCopyBuf));

				vgui::system()->SetClipboardText(szCopyBuf, V_strlen(szCopyBuf));

				const bool bIsCut = iCopyAction == COPYMENU_CUT;
				if (bIsCut)
				{
					wszText[iTextSelMin] = L'\0';
					V_snwprintf(wszText, iMaxWszTextSize, L"%ls%ls", wszText, wszText + iTextSelMax);
					bRebuildIrTextWidths = true;
					c->iTextSelStart = iTextSelMin;
					c->iTextSelCur = iTextSelMin;
				}
#if defined(DEBUG) && DEBUG_NEOUI
				Msg("COPYMENU_%s: %s\n", (bIsCut) ? "CUT" : "COPY", szCopyBuf);
#endif // defined(DEBUG) && DEBUG_NEOUI
			}
		} break;	
		case COPYMENU_PASTE:
		{
			if (vgui::system()->GetClipboardTextCount() > 0)
			{
				wchar_t wszClipboard[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
				const int iClipboardWSZBytes = vgui::system()->GetClipboardText(0, wszClipboard, sizeof(wszClipboard));
				if (iClipboardWSZBytes > 0)
				{
					// Write to temporary first otherwise wszText -> wszText can corrupt
					static wchar_t wszStaticTmpText[MAX_TEXTINPUT_U8BYTES_LIMIT];
					if (bFromEnd)
					{
						V_wcsncat(wszText, wszClipboard, (size_t)iMaxWszTextSize + 1);
						c->iTextSelCur = V_wcslen(wszText);
					}
					else if (bIsCursor)
					{
						// Single selection - append clipboard on cursor
						if (iTextSelMin > 0)
						{
							V_swprintf_safe(wszStaticTmpText, L"%.*ls%ls%ls", iTextSelMin, wszText, wszClipboard, wszText + iTextSelMin);
						}
						else
						{
							V_swprintf_safe(wszStaticTmpText, L"%ls%ls", wszClipboard, wszText);
						}
						V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
						c->iTextSelCur = Min(c->iTextSelCur + V_wcslen(wszClipboard), V_wcslen(wszText));
					}
					else if (bIsSelection)
					{
						// Multi selection - clipboard replaces selection
						V_swprintf_safe(wszStaticTmpText, L"%.*ls%ls%ls", iTextSelMin, wszText, wszClipboard, wszText + iTextSelMax);
						V_wcsncpy(wszText, wszStaticTmpText, sizeof(wchar_t) * iMaxWszTextSize);
						c->iTextSelCur = Min(iTextSelMin + V_wcslen(wszClipboard), V_wcslen(wszText));
					}
					// All devolves to cursor
					c->iTextSelStart = c->iTextSelCur;
					bRebuildIrTextWidths = true;
#if defined(DEBUG) && DEBUG_NEOUI
					char szClipboard[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
					g_pVGuiLocalize->ConvertUnicodeToANSI(wszClipboard, szClipboard, sizeof(szClipboard));
					char szText[MAX_TEXTINPUT_U8BYTES_LIMIT] = {};
					g_pVGuiLocalize->ConvertUnicodeToANSI(wszText, szText, sizeof(szText));
					Msg("COPYMENU_PASTE: %s -> %s\n", szClipboard, szText);
#endif // defined(DEBUG) && DEBUG_NEOUI
				}
			}
		} break;
		default:
			break;
		}

		if (bIsPassword)
		{
			// iWszTextSize instead of V_wcslen because it's altered
			// before the changes happens.
			staticWszPasswordChars[iWszTextSize + 1] = L'*';
		}
		if (bRebuildIrTextWidths)
		{
			BuildUpIrTextWidths(bIsPassword ? staticWszPasswordChars : wszText, V_wcslen(wszText));
			c->bValueEdited = true;
		}
	}

	EndWidget(wdgState);
}

TableHeaderModFlags TableHeader(const wchar_t **wszColNamesList, const int iColsTotal,
		int *piColsWide, int *piSortIndex, bool *pbSortDescending,
		const int iRelSyncSectionXOffset)
{
	TableHeaderModFlags modFlags = 0;

	const auto wdgState = BeginWidget(WIDGETFLAG_SKIPACTIVE | WIDGETFLAG_MOUSE | WIDGETFLAG_NOHOTBORDER);
	const int iDragWide = Max(static_cast<int>(FL_HEADER_DRAG_RATIO * c->iMarginY), 2);

	// Sanity check if iLabelsSize dynamically changes
	*piSortIndex = clamp(*piSortIndex, 0, iColsTotal - 1);

	if (wdgState.bInView)
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			bool bHotDrag = false;
			int prevX1 = c->rWidgetArea.x0 - c->iXOffset[c->iSection + iRelSyncSectionXOffset];
			const auto *pFontI = &c->fonts[c->eFont];
			for (int i = 0; i < iColsTotal; ++i)
			{
				if (piColsWide[i] <= 0)
				{
					continue;
				}

				vgui::IntRect headerCellRect = {
					.x0 = prevX1,
					.y0 = c->rWidgetArea.y0,
					.x1 = prevX1 + piColsWide[i],
					.y1 = c->rWidgetArea.y0 + c->irWidgetTall,
				};
				prevX1 = headerCellRect.x1;

				// Header cell background section
				const bool bHotCell =
						IN_BETWEEN_EQ(headerCellRect.x0, c->iMouseAbsX, headerCellRect.x1 - iDragWide)
						&& IN_BETWEEN_EQ(headerCellRect.y0, c->iMouseAbsY, headerCellRect.y1);
				const bool bSortCell = (i == *piSortIndex);

				// Drag area section
				bHotDrag = bHotDrag ||
						(  IN_BETWEEN_EQ(headerCellRect.x1 - iDragWide, c->iMouseAbsX, headerCellRect.x1)
						&& IN_BETWEEN_EQ(headerCellRect.y0, c->iMouseAbsY, headerCellRect.y1));

				const int iLDiff = Max(c->rWidgetArea.x0 - headerCellRect.x0, 0);
				const int iRDiff = Max(headerCellRect.x1 - c->rWidgetArea.x1, 0);
				const int iXTotalWide = headerCellRect.x1 - headerCellRect.x0;
				const int iXWide = Max(iXTotalWide - iLDiff - iRDiff, 0);
				if (iXWide > 0)
				{
					vgui::surface()->SetFullscreenViewport(
							Max(headerCellRect.x0, c->rWidgetArea.x0),
							headerCellRect.y0,
							iXWide,
							c->irWidgetTall);
					vgui::surface()->PushFullscreenViewport();

					// Rendering
					if (bHotCell || bSortCell)
					{
						vgui::surface()->DrawSetColor(bSortCell ? c->colors.activeBg : c->colors.hotBg);
						vgui::surface()->DrawFilledRect(0, 0, iXWide - ((iRDiff > 0) ? 0 : iDragWide), c->irWidgetTall);
					}

					vgui::surface()->DrawSetColor((c->iInDrag == (i + 1))
							? c->colors.headerDragActiveBg
							: c->colors.headerDragNormalBg);
					vgui::surface()->DrawFilledRect(iXTotalWide - iLDiff - iDragWide, 0, iXTotalWide - iLDiff, c->irWidgetTall);

					// Text section
					const wchar_t *wszText = wszColNamesList[i];
					vgui::surface()->DrawSetTextPos(c->iMarginX - iLDiff, pFontI->iYFontOffset);
					vgui::surface()->DrawSetTextColor(bSortCell ? c->colors.activeFg : bHotCell ? c->colors.hotFg : c->colors.normalFg);
					vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
					if (bSortCell)
					{
						const auto *pFontI = &c->fonts[c->eFont];
						vgui::surface()->DrawSetColor(c->colors.tableHeaderSortIndicatorBg);

						const int iSortIndX = iXWide - c->layout.iRowTall + pFontI->iStartBtnXPos;
						const int iSortIndY = pFontI->iStartBtnYPos;
						vgui::surface()->DrawSetTextPos(iSortIndX, iSortIndY);

						vgui::surface()->DrawPrintText((*pbSortDescending) ? WSZ_ARROW_UP : WSZ_ARROW_DOWN, 1);
					}
					if (bHotCell)
					{
						DrawBorder(vgui::IntRect{0, 0, iXWide - ((iRDiff > 0) ? 0 : iDragWide), c->irWidgetTall});
					}

					vgui::surface()->PopFullscreenViewport();
					vgui::surface()->SetFullscreenViewport(0, 0, 0, 0);
				}
			}
			vgui::surface()->SetCursor((bHotDrag || c->iInDrag > 0)
					? vgui::dc_sizewe
					: vgui::dc_arrow);
		}
		break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot) 
			{
				if (c->eCode == MOUSE_LEFT)
				{
					int cellX0 = 0;
					int cellX1 = c->rWidgetArea.x0 - c->iXOffset[c->iSection + iRelSyncSectionXOffset];
					for (int i = 0; i < iColsTotal; ++i)
					{
						if (piColsWide[i] <= 0)
						{
							continue;
						}

						// y-axis already covered by wdgState.bHot, just check x-axis
						cellX0 = cellX1;
						cellX1 = cellX0 + piColsWide[i];

						const bool bMouseInCell = IN_BETWEEN_EQ(cellX0, c->iMouseAbsX, cellX1);
						if (bMouseInCell)
						{
							const bool bMouseInDrag = IN_BETWEEN_EQ(cellX1 - iDragWide, c->iMouseAbsX, cellX1);
							if (bMouseInDrag)
							{
								c->iInDrag = i + 1;
							}
							else
							{
								const bool bSortCell = (i == *piSortIndex);
								if (bSortCell)
								{
									*pbSortDescending = !*pbSortDescending;
									modFlags |= TABLEHEADERMODFLAG_DESCENDINGCHANGED;
								}
								else
								{
									*piSortIndex = i;
									modFlags |= TABLEHEADERMODFLAG_INDEXCHANGED;
								}
								c->bValueEdited = true;
							}
							break;
						}
					}
				}
				else if (c->eCode == MOUSE_RIGHT)
				{
					int iWidestIdx = -1;
					int iWidestWide = 0;
					for (int i = 0; i < iColsTotal; ++i)
					{
						const int iCurWide = V_wcslen(wszColNamesList[i]);
						if (iCurWide > iWidestWide)
						{
							iWidestIdx = i;
							iWidestWide = iCurWide;
						}
					}

					Assert(iWidestIdx >= 0);
					if (iWidestIdx >= 0)
					{
						c->wszTableVisColNamesList = wszColNamesList;
						c->iTableVisColsTotal = iColsTotal;
						c->piTableVisColsWide = piColsWide;
						OpenPopup(INTERNALPOPUP_TABLEHEADER, Dim{
									.x = c->iMouseAbsX,
									.y = c->iMouseAbsY,
									.wide = NeoUI::SuitableWideByWStr(L"__", NeoUI::SUITABLEWIDE_POPUP) // Offset by checkmark
											+ NeoUI::SuitableWideByWStr(wszColNamesList[iWidestIdx], NeoUI::SUITABLEWIDE_POPUP),
									.tall = c->layout.iDefRowTall * iColsTotal,
								});
					}
				}
			}
		}
		break;
		case MODE_MOUSERELEASED:
		{
			c->iInDrag = 0;
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (c->iInDrag > 0)
			{
				const int iDragIdx = c->iInDrag - 1;
				int iAccWide = 0;
				for (int i = 0; i < iColsTotal; ++i)
				{
					if (piColsWide[i] <= 0)
					{
						continue;
					}

					iAccWide += piColsWide[i];
					if (iDragIdx == i)
					{
						const int iMinColWide = Max(I_MIN_COL_WIDE_RATIO * c->iMarginX, 10);
						// Alter by the difference in drag
						const int iAbsColX1 = c->rWidgetArea.x0 + iAccWide -
								c->iXOffset[c->iSection + iRelSyncSectionXOffset];
						const int iDiff = c->iMouseAbsX - iAbsColX1;
						piColsWide[i] = Max(piColsWide[i] + iDiff, iMinColWide);
						iAccWide += iDiff;

						// If dragging the end, expand/retract offset
						// This is assuming c->dPanel.wide on next section equals to this
						if (iDragIdx == (iColsTotal - 1))
						{
							c->iXOffset[c->iSection + iRelSyncSectionXOffset] = Max(iAccWide - c->dPanel.wide, 0);
						}
						break;
					}
				}
			}
		}
		break;
		default:
			break;
		}
	}

	EndWidget(wdgState);
	return modFlags;
}

// NEO NOTE (nullsystem): It's done like this so that the highlighter
// border goes over rather than under the content of the row
static void TableHighlightPrevRow()
{
	if (c->curRowFlags & NEXTTABLEROWFLAG__HOT && MODE_PAINT == c->eMode)
	{
		vgui::IntRect rRowArea = {
			.x0 = c->dPanel.x + c->iLayoutX + c->iXOffset[c->iSection],
			.y0 = c->dPanel.y + c->iLayoutY - c->layout.iRowTall,
			.x1 = c->dPanel.x + c->iLayoutX + c->iXOffset[c->iSection] + c->dPanel.wide,
			.y1 = c->dPanel.y + c->iLayoutY,
		};
		const bool bFullyVisible = (rRowArea.y0 >= c->dPanel.y)
				&& (rRowArea.y1 <= (c->dPanel.y + c->dPanel.tall));
		if (bFullyVisible)
		{
			DrawBorder(rRowArea);
		}
	}
}

void BeginTable(const int *piColsWide, const int iLabelsSize)
{
	// Bump y-axis with previous row layout before applying table layout
	if (c->iWidget > 0 && c->iIdxRowParts > 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutX = -c->iXOffset[c->iSection];
		c->iLayoutY += c->layout.iRowTall;
		c->irWidgetLayoutY = c->iLayoutY;
	}

	Assert(0 == c->layout.iTableLabelsSize);
	Assert(nullptr == c->layout.piTableColsWide);

	Assert(iLabelsSize > 0);
	Assert(piColsWide);

	c->layout.iTableLabelsSize = iLabelsSize;
	c->layout.piTableColsWide = piColsWide;
	c->curRowFlags = NEXTTABLEROWFLAG_NONE;
	c->iLastTableRowWidget = 0;

	c->iIdxRowParts = -1;
}

NeoUI::RetButton EndTable()
{
	if (c->iWidget > 0 && c->iIdxRowParts > 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutX = -c->iXOffset[c->iSection];
		c->iLayoutY += c->layout.iRowTall;
		c->irWidgetLayoutY = c->iLayoutY;
	}
	TableHighlightPrevRow();

	RetButton ret = {};

	if (c->iWidget > 0 && c->iIdxRowParts > 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutX = -c->iXOffset[c->iSection];
		c->iLayoutY += c->layout.iRowTall;
		c->irWidgetLayoutY = c->iLayoutY;
	}

	if ((c->curRowFlags & NEXTTABLEROWFLAG__HASROWSELECTABLES)
			&& (false == (c->curRowFlags & NEXTTABLEROWFLAG__HASROWSELECTED))
			&& (c->eMode == MODE_KEYPRESSED))
	{
		ret.bKeyUpPressed = IsKeyUpWidget();
		ret.bKeyDownPressed = IsKeyDownWidget();
		if (ret.bKeyUpPressed || ret.bKeyDownPressed)
		{
			c->iActiveSection = c->iSection;
			c->iActive = ret.bKeyUpPressed ? c->iLastTableRowWidget : 0;
		}
	}

	c->layout.iTableLabelsSize = 0;
	c->layout.piTableColsWide = nullptr;
	c->curRowFlags = NEXTTABLEROWFLAG_NONE;

	c->iIdxRowParts = -1;
	return ret;
}

NeoUI::RetButton NextTableRow(const NextTableRowFlags flags)
{
	Assert(c->layout.iTableLabelsSize > 0 && c->layout.piTableColsWide);

	if (c->iWidget > 0 && c->iIdxRowParts > 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutX = -c->iXOffset[c->iSection];
		c->iLayoutY += c->layout.iRowTall;
		c->irWidgetLayoutY = c->iLayoutY;
	}
	TableHighlightPrevRow();

	RetButton ret = {};
	c->curRowFlags = ((flags & NEXTTABLEROWFLAG__EXTERNAL) | (c->curRowFlags & NEXTTABLEROWFLAG__PERSISTS));

	if (flags & NEXTTABLEROWFLAG_SELECTABLE)
	{
		vgui::IntRect rRowArea = {
			.x0 = c->dPanel.x + c->iLayoutX + c->iXOffset[c->iSection],
			.y0 = c->dPanel.y + c->iLayoutY,
			.x1 = c->dPanel.x + c->iLayoutX + c->iXOffset[c->iSection] + c->dPanel.wide,
			.y1 = c->dPanel.y + c->iLayoutY + c->layout.iRowTall,
		};

		// Not a full Begin/EndWidget, but just fills in wdgInfos mainly
		// for key-pressed up/down Y-axis slider adjustments
		c->irWidgetTall = c->layout.iRowTall;
		c->wdgInfos[c->iWidget].iYOffsets = c->iLayoutY + c->iYOffset[c->iSection];
		c->wdgInfos[c->iWidget].iYTall = c->irWidgetTall;
		c->wdgInfos[c->iWidget].bCannotActive = false;

		bool bMouseIn = false;
		const bool bFullyVisible = (rRowArea.y0 >= c->dPanel.y) && (rRowArea.y1 <= (c->dPanel.y + c->dPanel.tall));
		if (bFullyVisible)
		{
			bMouseIn = IN_BETWEEN_EQ(rRowArea.x0, c->iMouseAbsX, rRowArea.x1 - 1)
					&& IN_BETWEEN_EQ(rRowArea.y0, c->iMouseAbsY, rRowArea.y1 - 1);
			if (bMouseIn && (c->dimPopup.wide > 0 && c->dimPopup.tall > 0) &&
					!(c->popupFlags & POPUPFLAG__INPOPUPSECTION))
			{
				const Dim &dim = c->dimPopup;
				bMouseIn = !(IN_BETWEEN_EQ(dim.x, c->iMouseAbsX, dim.x + dim.wide) &&
						 IN_BETWEEN_EQ(dim.y, c->iMouseAbsY, dim.y + dim.tall));
			}
			if (bMouseIn)
			{
				c->curRowFlags |= NEXTTABLEROWFLAG__HOT;
			}
		}
		ret.bMouseHover = bMouseIn;

		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			if (bFullyVisible)
			{
				Color color;
				if (c->curRowFlags & NEXTTABLEROWFLAG_SELECTED)
				{
					color = c->colors.activeBg;
				}
				else if (c->curRowFlags & NEXTTABLEROWFLAG__HOT)
				{
					color = c->colors.hotBg;
				}

				if (color.a() > 0)
				{
					vgui::surface()->DrawSetColor(color);
					vgui::surface()->DrawFilledRectArray(&rRowArea, 1);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
			ret.bMouseRightPressed = (ret.bMouseHover && c->eCode == MOUSE_RIGHT);
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			ret.bMouseDoublePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (c->curRowFlags & NEXTTABLEROWFLAG_SELECTED)
			{
				ret.bKeyEnterPressed = ret.bPressed = IsKeyEnter();
				if (false == (c->curRowFlags & NEXTTABLEROWFLAG__UPDOWNPROCESSED))
				{
					ret.bKeyUpPressed = IsKeyUpWidget();
					ret.bKeyDownPressed = IsKeyDownWidget();
				}
				if (ret.bKeyUpPressed || ret.bKeyDownPressed)
				{
					c->curRowFlags |= NEXTTABLEROWFLAG__UPDOWNPROCESSED;
					c->iActiveSection = c->iSection;
					c->iActive = c->iWidget;
				}
			}
		}
		break;
		default:
			break;
		}

		if (ret.bPressed)
		{
			c->iActive = c->iWidget;
			c->iActiveSection = c->iSection;
		}

		if (c->curRowFlags & NEXTTABLEROWFLAG_SELECTED)
		{
			c->curRowFlags |= NEXTTABLEROWFLAG__HASROWSELECTED;
		}
		c->curRowFlags |= NEXTTABLEROWFLAG__HASROWSELECTABLES;
		c->iLastTableRowWidget = c->iWidget;

		++c->iWidget;
	}

	return ret;
}

void PadTableXScroll(const int *piColsWide, const int iLabelsSize)
{
	int iTotalX = 0;
	for (int i = 0; i < iLabelsSize; ++i)
	{
		iTotalX += piColsWide[i];
	}
	c->irWidgetMaxX = Max(c->irWidgetMaxX, iTotalX - c->iXOffset[c->iSection]);
}

void BeginIgnoreXOffset()
{
	c->bIgnoreXOffset = true;
}

void EndIgnoreXOffset()
{
	c->bIgnoreXOffset = false;
}

bool Bind(const ButtonCode_t eCode)
{
	return c->eMode == MODE_KEYPRESSED && c->eCode == eCode;
}

bool Bind(const ButtonCode_t *peCode, const int ieCodeSize)
{
	if (c->eMode == MODE_KEYPRESSED)
	{
		for (int i = 0; i < ieCodeSize; ++i)
		{
			if (c->eCode == peCode[i])
			{
				return true;
			}
		}
	}
	return false;
}

void OpenURL(const char *szBaseUrl, const char *szPath)
{
	Assert(szPath && szPath[0] == '/');
	if (!szPath || szPath[0] != '/')
	{
		// Must start with / otherwise don't open URL
		return;
	}

	static constexpr char CMD[] =
#ifdef _WIN32
		"start"
#else
		"xdg-open"
#endif
		;
	char syscmd[512] = {};
	V_sprintf_safe(syscmd, "%s %s%s", CMD, szBaseUrl, szPath);
	[[maybe_unused]] const auto sysRet = system(syscmd);
#ifdef LINUX
	// Retvals are implementation-defined.
	// Linux may declare system with "warn_unused_result", so we gotta check to avoid a warning.
	constexpr auto linuxErrRet = -1;
	if (sysRet == linuxErrRet)
	{
		Warning("%s system call failed: \"%s\"\n", __FUNCTION__, syscmd);
		Assert(false);
	}
#endif
}

const wchar_t *HintAlt(const wchar *wszKey, const wchar *wszController)
{
	return (c->eKeyHints == NeoUI::KEYHINTS_KEYBOARD) ? wszKey : wszController;
}

}  // namespace NeoUI
