#include "neo_ui.h"

#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>
#include <filesystem.h>
#include <stb_image.h>
#include <materialsystem/imaterial.h>

#include "neo_misc.h"

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

void SwapFont(const EFont eFont, const bool bForce)
{
	if (c->eMode != MODE_PAINT || (!bForce && (c->eFont == eFont))) return;
	c->eFont = eFont;
	vgui::surface()->DrawSetTextFont(c->fonts[c->eFont].hdl);
}

void SwapColorNormal(const Color &color)
{
	c->normalBgColor = color;
	vgui::surface()->DrawSetColor(c->normalBgColor);
}

void MultiWidgetHighlighter(const int iTotalWidgets)
{
	if (c->eMode == MODE_PAINT)
	{
		// Peek-forward what the area of the multiple widgets will cover without modifying the context
		int iMulLayoutX = c->iLayoutX;
		int iMulLayoutY = c->iLayoutY;
		int iFirstLayoutX = iMulLayoutX;
		int iFirstLayoutY = iMulLayoutY;
		int iMulIdxRowParts = c->iIdxRowParts;
		int iXWide = 0;
		for (int i = 0; i < iTotalWidgets; ++i)
		{
			if (iMulIdxRowParts < 0 || iMulIdxRowParts >= (c->layout.iRowPartsTotal - 1))
			{
				iMulLayoutX = 0;
				iMulLayoutY += (iMulIdxRowParts < 0) ? 0 : c->layout.iRowTall;
				iMulIdxRowParts = 0;
			}
			else
			{
				if (c->layout.iRowParts)
				{
					iMulLayoutX += (c->layout.iRowParts[iMulIdxRowParts] / 100.0f) * c->dPanel.wide;
				}
				else
				{
					iMulLayoutX += (1.0f / static_cast<float>(c->layout.iRowPartsTotal)) * c->dPanel.wide;
				}
				++iMulIdxRowParts;
			}
			if (i == 0)
			{
				iFirstLayoutX = iMulLayoutX;
				iFirstLayoutY = iMulLayoutY;
			}

			iXWide += (iMulIdxRowParts == (c->layout.iRowPartsTotal - 1))
					? c->dPanel.wide - iMulLayoutX
					: (c->layout.iRowParts)
					  ? (c->layout.iRowParts[iMulIdxRowParts] / 100.0f) * c->dPanel.wide
					  : (1.0f / static_cast<float>(c->layout.iRowPartsTotal)) * c->dPanel.wide;
		}
		vgui::IntRect hightlightRect = {
			.x0 = c->dPanel.x + iFirstLayoutX,
			.y0 = c->dPanel.y + iFirstLayoutY,
			.x1 = c->dPanel.x + iFirstLayoutX + iXWide,
			.y1 = c->dPanel.y + iFirstLayoutY + c->layout.iRowTall,
		};

		// Mostly so the highlight can apply to multi-widget's labels
		const bool bMouseIn = IN_BETWEEN_EQ(hightlightRect.x0, c->iMouseAbsX, hightlightRect.x1)
				&& IN_BETWEEN_EQ(hightlightRect.y0, c->iMouseAbsY, hightlightRect.y1);
		if (bMouseIn)
		{
			c->iHot = c->iWidget;
			c->iHotSection = c->iSection;
		}

		// Apply highlight if it's hot/active
		const bool bHot = c->iHotSection == c->iSection && IN_BETWEEN_AR(c->iWidget, c->iHot, c->iWidget + iTotalWidgets);
		const bool bActive = c->iActiveSection == c->iSection && IN_BETWEEN_AR(c->iWidget, c->iActive, c->iWidget + iTotalWidgets);
		if (bHot || bActive)
		{
			vgui::surface()->DrawSetColor(c->selectBgColor);
			if (bActive)
			{
				vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
			}
			vgui::surface()->DrawFilledRectArray(&hightlightRect, 1);
		}
	}
}

void FreeContext(NeoUI::Context *pCtx)
{
	if (pCtx->wdgInfos)
	{
		free(pCtx->wdgInfos);
	}
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
	c->iWidget = 0;
	c->iSection = 0;
	c->iHasMouseInPanel = 0;
	c->bValueEdited = false;
	c->eButtonTextStyle = TEXTSTYLE_CENTER;
	c->eLabelTextStyle = TEXTSTYLE_LEFT;
	c->bTextEditIsPassword = false;
	c->selectBgColor = COLOR_NEOPANELSELECTBG;
	c->normalBgColor = COLOR_NEOPANELACCENTBG;
	// Different pointer, change context
	if (c->pSzCurCtxName != pSzCtxName)
	{
		c->htSliders.RemoveAll();
		c->pSzCurCtxName = pSzCtxName;
	}

	switch (c->eMode)
	{
	case MODE_KEYPRESSED:
		if (c->eCode == KEY_DOWN || c->eCode == KEY_UP)
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
		break;
	case MODE_MOUSERELEASED:
		c->eMousePressedStart = MOUSESTART_NONE;
		break;
	case MODE_PAINT:
		for (int i = 0; i < FONT__TOTAL; ++i)
		{
			FontInfo *pFontI = &c->fonts[i];
			const int iTall = vgui::surface()->GetFontTall(pFontI->hdl);
			pFontI->iYOffset = (c->layout.iRowTall / 2) - (iTall / 2);
			{
				int iPrevNextWide, iPrevNextTall;
				vgui::surface()->GetTextSize(pFontI->hdl, L"<", iPrevNextWide, iPrevNextTall);
				pFontI->iStartBtnXPos = (c->layout.iRowTall / 2) - (iPrevNextWide / 2);
				pFontI->iStartBtnYPos = (c->layout.iRowTall / 2) - (iPrevNextTall / 2);
			}
		}

		if (wszTitle)
		{
			SwapFont(FONT_NTHEADING);
			vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
			vgui::surface()->DrawSetTextPos(c->dPanel.x + c->iMarginX,
											c->dPanel.y + -c->layout.iRowTall + c->fonts[FONT_NTHEADING].iYOffset);
			vgui::surface()->DrawPrintText(wszTitle, V_wcslen(wszTitle));
		}
		break;
	case MODE_MOUSEMOVED:
		c->iHot = FOCUSOFF_NUM;
		c->iHotSection = -1;
		break;
	default:
		break;
	}

	// Force SwapFont on main to prevent crash on startup
	SwapFont(FONT_NTNORMAL, true);
	c->eButtonTextStyle = TEXTSTYLE_LEFT;
	vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
}

void EndContext()
{
	if (c->eMode == MODE_MOUSEPRESSED && !c->iHasMouseInPanel)
	{
		c->iActive = FOCUSOFF_NUM;
		c->iActiveSection = -1;
	}

	if (c->eMode == MODE_KEYPRESSED)
	{
		if (c->eCode == KEY_DOWN || c->eCode == KEY_UP || c->eCode == KEY_TAB)
		{
			if (c->iActiveSection == -1)
			{
				c->iActiveSection = 0;
			}

			if (c->eCode == KEY_TAB)
			{
				const int iTotalSection = c->iSection;
				int iTally = 0;
				for (int i = 0; i < iTotalSection; ++i)
				{
					iTally += c->iSectionCanActive[i];
				}
				if (iTally == 0)
				{
					// There is no valid sections, don't do anything
					c->iActiveSection = -1;
				}
				else
				{
					const int iIncr = ((vgui::input()->IsKeyDown(KEY_LSHIFT) || vgui::input()->IsKeyDown(KEY_RSHIFT)) ? -1 : +1);
					do
					{
						c->iActiveSection += iIncr;
						c->iActiveSection = LoopAroundInArray(c->iActiveSection, iTotalSection);
					} while (c->iSectionCanActive[c->iActiveSection] == 0);
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

void BeginSection(const bool bDefaultFocus)
{
	c->iLayoutX = 0;
	c->iLayoutY = -c->iYOffset[c->iSection];
	c->iWidget = 0;
	c->iCanActives = 0;
	c->iIdxRowParts = -1;

	c->iMouseRelX = c->iMouseAbsX - c->dPanel.x;
	c->iMouseRelY = c->iMouseAbsY - c->dPanel.y;
	c->bMouseInPanel = IN_BETWEEN_EQ(0, c->iMouseRelX, c->dPanel.wide) && IN_BETWEEN_EQ(0, c->iMouseRelY, c->dPanel.tall);

	c->iHasMouseInPanel += c->bMouseInPanel;

	switch (c->eMode)
	{
	case MODE_PAINT:
		vgui::surface()->DrawSetColor(c->bgColor);
		vgui::surface()->DrawFilledRect(c->dPanel.x,
										c->dPanel.y,
										c->dPanel.x + c->dPanel.wide,
										c->dPanel.y + c->dPanel.tall);

		vgui::surface()->DrawSetColor(c->normalBgColor);
		vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
		break;
	case MODE_KEYPRESSED:
		if (bDefaultFocus && c->iActiveSection == -1 && (c->eCode == KEY_DOWN || c->eCode == KEY_UP))
		{
			c->iActiveSection = c->iSection;
		}
		break;
	}

	NeoUI::SetPerRowLayout(1, nullptr, c->layout.iDefRowTall);
}

void EndSection()
{
	if (c->eMode == MODE_MOUSEPRESSED && c->bMouseInPanel &&
			c->iLayoutY < c->iMouseRelY)
	{
		c->iActive = FOCUSOFF_NUM;
		c->iActiveSection = -1;
	}
	const int iAbsLayoutY = c->iLayoutY + c->irWidgetTall + c->iYOffset[c->iSection];
	c->wdgInfos[c->iWidget].iYOffsets= iAbsLayoutY;

	// Scroll handling
	const int iScrollThick = c->iMarginX * 4;
	const int iMWheelJump = c->layout.iDefRowTall;
	const bool bHasScroll = (iAbsLayoutY > c->dPanel.tall);
	vgui::IntRect rectScrollArea = {
		.x0 = c->dPanel.x + c->dPanel.wide,
		.y0 = c->dPanel.y,
		.x1 = c->dPanel.x + c->dPanel.wide + iScrollThick,
		.y1 = c->dPanel.y + c->dPanel.tall,
	};
	const bool bMouseInScrollbar = bHasScroll && InRect(rectScrollArea, c->iMouseAbsX, c->iMouseAbsY);
	const bool bMouseInWheelable = c->bMouseInPanel || bMouseInScrollbar;

	if (c->eMode == MODE_MOUSEWHEELED && bMouseInWheelable)
	{
		if (!bHasScroll)
		{
			c->iYOffset[c->iSection] = 0;
		}
		else
		{
			c->iYOffset[c->iSection] += (c->eCode == MOUSE_WHEEL_UP) ? -iMWheelJump : +iMWheelJump;
			c->iYOffset[c->iSection] = clamp(c->iYOffset[c->iSection], 0, iAbsLayoutY - c->dPanel.tall);
		}
	}
	else if (c->eMode == MODE_KEYPRESSED && (c->eCode == KEY_DOWN || c->eCode == KEY_UP) &&
			 (c->iActiveSection == c->iSection || c->iHotSection == c->iSection))
	{
		if (c->iActive != FOCUSOFF_NUM && c->iActiveSection == c->iSection)
		{
			const int iActiveDirection = (c->eCode == KEY_UP) ? -1 : +1;
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

		if (!bHasScroll)
		{
			// Disable scroll if it doesn't need to
			c->iYOffset[c->iSection] = 0;
		}
		else if (c->wdgInfos[c->iActive].iYOffsets <= c->iYOffset[c->iSection])
		{
			// Scrolling up past visible, re-adjust
			c->iYOffset[c->iSection] = c->wdgInfos[c->iActive].iYOffsets;
		}
		else if (c->wdgInfos[c->iActive].iYOffsets >= (c->iYOffset[c->iSection] + c->dPanel.tall))
		{
			// Scrolling down post visible, re-adjust
			c->iYOffset[c->iSection] = c->wdgInfos[c->iActive].iYOffsets - c->dPanel.tall + c->wdgInfos[c->iActive].iYTall;
		}
	}

	// Scroll bar area painting and mouse interaction (no keyboard as that's handled by active widgets)
	if (bHasScroll)
	{
		const int iYStart = c->iYOffset[c->iSection];
		const int iYEnd = iYStart + c->dPanel.tall;
		const float flYPercStart = iYStart / static_cast<float>(iAbsLayoutY);
		const float flYPercEnd = iYEnd / static_cast<float>(iAbsLayoutY);
		vgui::IntRect rectHandle{
			c->dPanel.x + c->dPanel.wide,
			c->dPanel.y + static_cast<int>(c->dPanel.tall * flYPercStart),
			c->dPanel.x + c->dPanel.wide + iScrollThick,
			c->dPanel.y + static_cast<int>(c->dPanel.tall * flYPercEnd)
		};

		bool bAlterOffset = false;
		switch (c->eMode)
		{
		case MODE_PAINT:
			vgui::surface()->DrawSetColor(c->bgColor);
			vgui::surface()->DrawFilledRectArray(&rectScrollArea, 1);
			vgui::surface()->DrawSetColor(c->abYMouseDragOffset[c->iSection] ? c->selectBgColor : COLOR_NEOPANELNORMALBG);//c->normalBgColor);
			vgui::surface()->DrawFilledRectArray(&rectHandle, 1);
			break;
		case MODE_MOUSEPRESSED:
			c->abYMouseDragOffset[c->iSection] = bMouseInScrollbar;
			if (bMouseInScrollbar)
			{
				// If not pressed on handle, set the drag at the middle
				const bool bInHandle = InRect(rectHandle, c->iMouseAbsX, c->iMouseAbsY);
				c->iStartMouseDragOffset[c->iSection] = (bInHandle) ?
							(c->iMouseAbsY - rectHandle.y0) :((rectHandle.y1 - rectHandle.y0) / 2.0f);
				bAlterOffset = true;
			}
			break;
		case MODE_MOUSERELEASED:
			c->abYMouseDragOffset[c->iSection] = false;
			break;
		case MODE_MOUSEMOVED:
			bAlterOffset = c->abYMouseDragOffset[c->iSection];
			break;
		default:
			break;
		}

		if (bAlterOffset)
		{
			const float flXPercMouse = static_cast<float>(c->iMouseRelY - c->iStartMouseDragOffset[c->iSection]) / static_cast<float>(c->dPanel.tall);
			c->iYOffset[c->iSection] = clamp(flXPercMouse * iAbsLayoutY, 0, (iAbsLayoutY - c->dPanel.tall));
		}
	}

	c->iSectionCanActive[c->iSection] = c->iCanActives;
	++c->iSection;
}

void SetPerRowLayout(const int iColTotal, const int *iColProportions, const int iRowTall)
{
	// Bump y-axis with previous row layout before applying new row layout
	if (c->iWidget > 0 && c->iIdxRowParts >= 0 && c->iIdxRowParts < c->layout.iRowPartsTotal)
	{
		c->iLayoutY += c->layout.iRowTall;
	}

	c->layout.iRowPartsTotal = iColTotal;
	c->layout.iRowParts = iColProportions;
	if (iRowTall > 0) c->layout.iRowTall = iRowTall;
	c->iIdxRowParts = -1;
}

static GetMouseinFocusedRet InternalGetMouseinFocused()
{
	const bool bMouseIn = IN_BETWEEN_EQ(c->rWidgetArea.x0, c->iMouseAbsX, c->rWidgetArea.x1)
			&& IN_BETWEEN_EQ(c->rWidgetArea.y0, c->iMouseAbsY, c->rWidgetArea.y1);
	if (bMouseIn)
	{
		c->iHot = c->iWidget;
		c->iHotSection = c->iSection;
	}
	const bool bHot = c->iHot == c->iWidget && c->iHotSection == c->iSection;
	const bool bActive = c->iWidget == c->iActive && c->iSection == c->iActiveSection;
	if (bActive || bHot)
	{
		vgui::surface()->DrawSetColor(c->selectBgColor);
		if (bActive) vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	}
	return GetMouseinFocusedRet{
		.bActive = bActive,
		.bHot = bHot,
	};
}

void BeginWidget(const WidgetFlag eWidgetFlag)
{
	if (c->iWidget >= c->iWdgInfosMax)
	{
		c->iWdgInfosMax += WDGINFO_ALLOC_STEPS;
		c->wdgInfos = (DynWidgetInfos *)(realloc(c->wdgInfos, sizeof(DynWidgetInfos) * c->iWdgInfosMax));
	}

	if (c->iIdxRowParts < 0 || c->iIdxRowParts >= (c->layout.iRowPartsTotal - 1))
	{
		// Shift to the next row once we're out of partition
		c->iLayoutX = 0;
		c->iLayoutY += (c->iIdxRowParts < 0) ? 0 : c->layout.iRowTall;
		c->iIdxRowParts = 0;
	}
	else
	{
		if (c->layout.iRowParts)
		{
			c->iLayoutX += (c->layout.iRowParts[c->iIdxRowParts] / 100.0f) * c->dPanel.wide;
		}
		else
		{
			c->iLayoutX += (1.0f / static_cast<float>(c->layout.iRowPartsTotal)) * c->dPanel.wide;
		}
		++c->iIdxRowParts;
	}

	// NEO NOTE (nullsystem): If very last partition of the row, use what's left
	// instead so whatever pixels don't get left out
	const int xWide = (c->iIdxRowParts == (c->layout.iRowPartsTotal - 1))
			? c->dPanel.wide - c->iLayoutX
			: (c->layout.iRowParts)
			  ? (c->layout.iRowParts[c->iIdxRowParts] / 100.0f) * c->dPanel.wide
			  : (1.0f / static_cast<float>(c->layout.iRowPartsTotal)) * c->dPanel.wide;
	c->rWidgetArea = vgui::IntRect{
		.x0 = c->dPanel.x + c->iLayoutX,
		.y0 = c->dPanel.y + c->iLayoutY,
		.x1 = c->dPanel.x + c->iLayoutX + xWide,
		.y1 = c->dPanel.y + c->iLayoutY + c->layout.iRowTall,
	};
	c->irWidgetWide = c->rWidgetArea.x1 - c->rWidgetArea.x0;
	c->irWidgetTall = c->rWidgetArea.y1 - c->rWidgetArea.y0;
	c->wdgInfos[c->iWidget].iYOffsets = c->iLayoutY + c->iYOffset[c->iSection];
	c->wdgInfos[c->iWidget].iYTall = c->irWidgetTall;
	c->wdgInfos[c->iWidget].bCannotActive = (eWidgetFlag & WIDGETFLAG_SKIPACTIVE);
}

void EndWidget(const GetMouseinFocusedRet wdgState)
{
	++c->iWidget;
	// NEO TODO (nullsystem): Will refactor when dealing with styling/color refactor
	if (wdgState.bActive || wdgState.bHot)
	{
		vgui::surface()->DrawSetColor(c->normalBgColor);
		vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
	}
}

void Pad()
{
	BeginWidget();
}

void LabelWrap(const wchar_t *wszText)
{
	if (!wszText || wszText[0] == '\0')
	{
		Pad();
		return;
	}

	BeginWidget(WIDGETFLAG_SKIPACTIVE);
	int iLines = 0;
	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
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
													c->rWidgetArea.y0 + pFontI->iYOffset + iYOffset);
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
												c->rWidgetArea.y0 + pFontI->iYOffset + iYOffset);
				vgui::surface()->DrawPrintText(wszText + iStart, iWszSize - iStart);
			}
			++iLines;
		}
	}

	c->iLayoutY += (c->layout.iRowTall * iLines);

	EndWidget(GetMouseinFocusedRet{true, true});
}

void HeadingLabel(const wchar_t *wszText)
{
	Context::Layout tmp;
	V_memcpy(&tmp, &c->layout, sizeof(Context::Layout));

	SetPerRowLayout(1, nullptr, tmp.iRowTall);
	c->eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

	Label(wszText);

	c->eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
	SetPerRowLayout(tmp.iRowPartsTotal, tmp.iRowParts, tmp.iRowTall);
}

void Label(const wchar_t *wszText, const bool bNotWidget)
{
	if (!bNotWidget)
	{
		BeginWidget(WIDGETFLAG_SKIPACTIVE);
	}
	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall) && c->eMode == MODE_PAINT)
	{
		const auto *pFontI = &c->fonts[c->eFont];
		int iFontTextWidth = 0, iFontTextHeight = 0;
		const bool bCenter = c->eLabelTextStyle == TEXTSTYLE_CENTER;
		if (bCenter)
		{
			vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontTextWidth, iFontTextHeight);
		}
		const int x = ((bCenter) ? ((c->irWidgetWide / 2) - (iFontTextWidth / 2)) : c->iMarginX);
		const int y = pFontI->iYOffset;
		vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + x, c->rWidgetArea.y0 + y);
		vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
	}
	if (!bNotWidget)
	{
		EndWidget(GetMouseinFocusedRet{true, true});
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

NeoUI::RetButton Button(const wchar_t *wszText)
{
	BeginWidget();
	RetButton ret = {};
	const auto wdgState = InternalGetMouseinFocused();
	ret.bMouseHover = wdgState.bHot;

	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			const auto *pFontI = &c->fonts[c->eFont];
			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);
			const int xMargin = c->eButtonTextStyle == TEXTSTYLE_CENTER ?
						((c->irWidgetWide / 2) - (iFontWide / 2)) : c->iMarginX;

			vgui::surface()->DrawFilledRectArray(&c->rWidgetArea, 1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + xMargin, c->rWidgetArea.y0 + pFontI->iYOffset);
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			ret.bMouseDoublePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			ret.bKeyPressed = ret.bPressed = (wdgState.bActive && c->eCode == KEY_ENTER);
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

	++c->iCanActives;
	EndWidget(wdgState);
	return ret;
}

NeoUI::RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText)
{
	MultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	return Button(wszText);
}

void ImageTexture(const char *szTexturePath, const wchar_t *wszErrorMsg, const char *szTextureGroup)
{
	BeginWidget(WIDGETFLAG_SKIPACTIVE);
	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall) && c->eMode == MODE_PAINT)
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
	EndWidget(GetMouseinFocusedRet{true, true});
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
			uint8 *data = stbi_load(szTexturePath, &width, &height, &channels, 0);
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
					iDispWide = min(width, height);
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
				vgui::surface()->DrawSetColor(c->normalBgColor);
			}
			return true;
		}
	}
	return false;
}

NeoUI::RetButton ButtonTexture(const char *szTexturePath)
{
	BeginWidget();
	RetButton ret = {};
	const auto wdgState = InternalGetMouseinFocused();
	ret.bMouseHover = wdgState.bHot;

	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x1, c->rWidgetArea.y1);
			Texture(szTexturePath, c->rWidgetArea.x0, c->rWidgetArea.y0,
					c->irWidgetWide, c->irWidgetTall);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			ret.bMouseDoublePressed = ret.bPressed = (ret.bMouseHover && c->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			ret.bKeyPressed = ret.bPressed = (wdgState.bActive && c->eCode == KEY_ENTER);
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

	++c->iCanActives;
	EndWidget(wdgState);
	return ret;
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

void RingBoxBool(bool *bChecked)
{
	int iIndex = static_cast<int>(*bChecked);
	RingBox(ENABLED_LABELS, 2, &iIndex);
	*bChecked = static_cast<bool>(iIndex);
}

void RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked)
{
	MultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	RingBoxBool(bChecked);
}

void RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	MultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	RingBox(wszLabelsList, iLabelsSize, iIndex);
}

void RingBox(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	BeginWidget();
	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
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
											c->rWidgetArea.y0 + pFontI->iYOffset);
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));

			// Left-side "<" prev button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x0 + c->layout.iRowTall, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x1 - c->layout.iRowTall, c->rWidgetArea.y0,
											c->rWidgetArea.x1, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - c->layout.iRowTall + pFontI->iStartBtnXPos,
											c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
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
			if (wdgState.bActive && (c->eCode == KEY_LEFT || c->eCode == KEY_RIGHT))
			{
				*iIndex += (c->eCode == KEY_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				c->bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	++c->iCanActives;
	EndWidget(wdgState);
}

void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	// This is basically a ringbox but different UI
	BeginWidget(WIDGETFLAG_SKIPACTIVE);
	const auto wdgState = InternalGetMouseinFocused();

	SwapFont(FONT_NTHORIZSIDES);
	const int iTabWide = (c->dPanel.wide / iLabelsSize);
	bool bResetActiveHot = false;

	switch (c->eMode)
	{
	case MODE_PAINT:
	{
		const auto *pFontI = &c->fonts[c->eFont];
		for (int i = 0, iXPosTab = 0; i < iLabelsSize; ++i, iXPosTab += iTabWide)
		{
			vgui::IntRect tabRect = {
				.x0 = c->rWidgetArea.x0 + iXPosTab,
				.y0 = c->rWidgetArea.y0,
				.x1 = (i == (iLabelsSize - 1))
						? (c->rWidgetArea.x1)
						: (c->rWidgetArea.x0 + iXPosTab + iTabWide),
				.y1 = c->rWidgetArea.y1,
			};
			const bool bHoverTab = IN_BETWEEN_EQ(tabRect.x0, c->iMouseAbsX, tabRect.x1) &&
					IN_BETWEEN_EQ(tabRect.y0, c->iMouseAbsY, tabRect.y1);
			if (bHoverTab || i == *iIndex)
			{
				// NEO NOTE (nullsystem): On the final tab, just expand it to the end width as iTabWide isn't always going
				// to give a properly aligned width
#if 0
				vgui::surface()->DrawSetColor(bHoverTab ? COLOR_NEOPANELSELECTBG : c->normalBgColor);
#else
				// NEO TODO (nullsystem): The altered color scheme makes some element invisible,
				// so will refactor this when I get to the styling refactoring
				vgui::surface()->DrawSetColor(COLOR_NEOPANELSELECTBG);
#endif
				vgui::surface()->DrawFilledRectArray(&tabRect, 1);
			}
			const wchar_t *wszText = wszLabelsList[i];
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + iXPosTab + c->iMarginX,
											c->rWidgetArea.y0 + pFontI->iYOffset);
			vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}

		// Draw the side-hints text
		// NEO NOTE (nullsystem): F# as 1 is thinner than 3/not monospaced font
		int iFontWidth, iFontHeight;
		vgui::surface()->GetTextSize(c->fonts[c->eFont].hdl, L"F##", iFontWidth, iFontHeight);
		const int iHintYPos = c->dPanel.y + (iFontHeight / 2);

		vgui::surface()->DrawSetTextPos(c->dPanel.x - c->iMarginX - iFontWidth, iHintYPos);
		vgui::surface()->DrawPrintText(L"F 1", 3);
		vgui::surface()->DrawSetTextPos(c->dPanel.x + c->dPanel.wide + c->iMarginX, iHintYPos);
		vgui::surface()->DrawPrintText(L"F 3", 3);
	}
	break;
	case MODE_MOUSEPRESSED:
	{
		if (wdgState.bHot && c->eCode == MOUSE_LEFT && c->iMouseAbsX >= c->rWidgetArea.x0)
		{
			const int iNextIndex = ((c->iMouseAbsX - c->rWidgetArea.x0) / iTabWide);
			if (iNextIndex != *iIndex)
			{
				*iIndex = clamp(iNextIndex, 0, iLabelsSize);
				V_memset(c->iYOffset, 0, sizeof(c->iYOffset));
				bResetActiveHot = true;
			}
		}
	}
	break;
	case MODE_KEYPRESSED:
	{
		if (c->eCode == KEY_F1 || c->eCode == KEY_F3) // Global keybind
		{
			*iIndex += (c->eCode == KEY_F1) ? -1 : +1;
			*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
			V_memset(c->iYOffset, 0, sizeof(c->iYOffset));
			bResetActiveHot = true;
		}
	}
	break;
	default:
		break;
	}

	if (bResetActiveHot)
	{
		c->iActive = FOCUSOFF_NUM;
		c->iActiveSection = -1;
		c->iHot = FOCUSOFF_NUM;
		c->iHotSection = -1;
	}

	EndWidget(wdgState);
	SwapFont(FONT_NTNORMAL);
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
	BeginWidget(WIDGETFLAG_SKIPACTIVE);
	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall) && c->eMode == MODE_PAINT)
	{
		const float flPerc = (flValue - flMin) / (flMax - flMin);
		vgui::surface()->DrawFilledRect(c->rWidgetArea.x0,
										c->rWidgetArea.y0,
										c->rWidgetArea.x0 + (flPerc * c->irWidgetWide),
										c->rWidgetArea.y1);
	}
	EndWidget(GetMouseinFocusedRet{true, true});
}

void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp, const float flStep, const wchar_t *wszSpecialText)
{
	MultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	BeginWidget();
	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
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
			pSInfo->iMaxStrSize = max(iStrMinLen, iStrMaxLen);
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

			// Background bar
			const float flPerc = (((iDp == 0) ? (int)(*flValue) : *flValue) - flMin) / (flMax - flMin);
			const float flBarMaxWide = static_cast<float>(c->irWidgetWide - (2 * c->layout.iRowTall));
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x0 + static_cast<int>(flPerc * flBarMaxWide), c->rWidgetArea.y1);

			// Center-text label
			const bool bSpecial = (wszSpecialText && !wdgState.bActive && *flValue == 0.0f);
			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, bSpecial ? wszSpecialText : pSInfo->wszText, iFontWide, iFontTall);
			const int iFontStartX = ((c->irWidgetWide / 2) - (iFontWide / 2));
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + iFontStartX,
											c->rWidgetArea.y0 + pFontI->iYOffset);
			vgui::surface()->DrawPrintText(bSpecial ? wszSpecialText : pSInfo->wszText,
										   bSpecial ? V_wcslen(wszSpecialText) : V_wcslen(pSInfo->wszText));

			// Left-side "<" prev button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
											c->rWidgetArea.x0 + c->layout.iRowTall, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + pFontI->iStartBtnXPos,
										   c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			vgui::surface()->DrawFilledRect(c->rWidgetArea.x1 - c->layout.iRowTall, c->rWidgetArea.y0,
											c->rWidgetArea.x1, c->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x1 - c->layout.iRowTall + pFontI->iStartBtnXPos,
										   c->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(L">", 1);

			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					const int iMarkX = iFontStartX + iFontWide;
					vgui::surface()->DrawSetColor(COLOR_NEOPANELTEXTBRIGHT);
					vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + iMarkX,
													c->rWidgetArea.y0 + pFontI->iYOffset,
													c->rWidgetArea.x0 + iMarkX + c->iMarginX,
													c->rWidgetArea.y0 + pFontI->iYOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
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
				if (c->eCode == KEY_LEFT || c->eCode == KEY_RIGHT)
				{
					*flValue += (c->eCode == KEY_LEFT) ? -flStep : +flStep;
					*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
					c->bValueEdited = true;
				}
				else if (c->eCode == KEY_ENTER)
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
				if (c->eCode == KEY_ENTER)
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

	++c->iCanActives;
	EndWidget(wdgState);
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

void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxBytes)
{
	MultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	TextEdit(wszText, iMaxBytes);
}

void TextEdit(wchar_t *wszText, const int iMaxBytes)
{
	BeginWidget();
	static wchar_t staticWszPasswordChars[256] = {};
	if (staticWszPasswordChars[0] == L'\0')
	{
		for (int i = 0; i < (ARRAYSIZE(staticWszPasswordChars) - 1); ++i)
		{
			staticWszPasswordChars[i] = L'*';
		}
	}

	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, c->iLayoutY, c->dPanel.tall))
	{
		switch (c->eMode)
		{
		case MODE_PAINT:
		{
			if (wdgState.bHot)
			{
				vgui::surface()->DrawFilledRect(c->rWidgetArea.x0, c->rWidgetArea.y0,
												c->rWidgetArea.x1, c->rWidgetArea.y1);
			}
			const auto *pFontI = &c->fonts[c->eFont];
			vgui::surface()->DrawSetTextPos(c->rWidgetArea.x0 + c->iMarginX,
											c->rWidgetArea.y0 + pFontI->iYOffset);
			const int iWszTextSize = V_wcslen(wszText);
			if (c->bTextEditIsPassword)
			{
				vgui::surface()->DrawPrintText(staticWszPasswordChars, iWszTextSize);
			}
			else
			{
				vgui::surface()->DrawPrintText(wszText, iWszTextSize);
			}
			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					int iFontWide, iFontTall;
					if (c->bTextEditIsPassword)
					{
						staticWszPasswordChars[iWszTextSize] = L'\0';
						vgui::surface()->GetTextSize(pFontI->hdl, staticWszPasswordChars, iFontWide, iFontTall);
						staticWszPasswordChars[iWszTextSize] = L'*';
					}
					else
					{
						vgui::surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);
					}
					const int iMarkX = c->iMarginX + iFontWide;
					vgui::surface()->DrawSetColor(COLOR_NEOPANELTEXTBRIGHT);
					vgui::surface()->DrawFilledRect(c->rWidgetArea.x0 + iMarkX,
													c->rWidgetArea.y0 + pFontI->iYOffset,
													c->rWidgetArea.x0 + iMarkX + c->iMarginX,
													c->rWidgetArea.y0 + pFontI->iYOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (wdgState.bHot)
			{
				c->iActive = c->iWidget;
				c->iActiveSection = c->iSection;
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive && c->eCode == KEY_BACKSPACE)
			{
				const int iTextSize = V_wcslen(wszText);
				if (iTextSize > 0)
				{
					wszText[iTextSize - 1] = L'\0';
					c->bValueEdited = true;
				}
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive && iswprint(c->unichar))
			{
				static char szTmpANSICheck[MAX_TEXTINPUT_U8BYTES_LIMIT];
				const int iTextInU8Bytes = g_pVGuiLocalize->ConvertUnicodeToANSI(wszText, szTmpANSICheck, sizeof(szTmpANSICheck));
				if (iTextInU8Bytes < iMaxBytes)
				{
					int iTextSize = V_wcslen(wszText);
					wszText[iTextSize++] = c->unichar;
					wszText[iTextSize] = L'\0';
					c->bValueEdited = true;
				}
			}
		}
		break;
		default:
			break;
		}
	}

	++c->iCanActives;
	EndWidget(wdgState);
}

bool Bind(const ButtonCode_t eCode)
{
	return c->eMode == MODE_KEYPRESSED && c->eCode == eCode;
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
	system(syscmd);
}

}  // namespace NeoUI
