#include "neo_ui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

#include "neo_misc.h"

using namespace vgui;

static const wchar_t *ENABLED_LABELS[] = {
	L"Disabled",
	L"Enabled",
};

namespace NeoUI
{

static inline Context g_emptyCtx;
static Context *g_pCtx = &g_emptyCtx;
static void InternalLabel(const wchar_t *wszText, const bool bCenter);

void GCtxDrawFilledRectXtoX(const int x1, const int y1, const int x2, const int y2)
{
	surface()->DrawFilledRect(g_pCtx->dPanel.x + g_pCtx->iLayoutX + x1,
							  g_pCtx->dPanel.y + g_pCtx->iLayoutY + y1,
							  g_pCtx->dPanel.x + g_pCtx->iLayoutX + x2,
							  g_pCtx->dPanel.y + g_pCtx->iLayoutY + y2);
}

// This is only because it'll be an eye-sore and repetitve with those g_pCtx->dPanel, iLayout, g_pCtx->iRowTall...
void GCtxDrawFilledRectXtoX(const int x1, const int x2)
{
	GCtxDrawFilledRectXtoX(x1, 0, x2, g_pCtx->iRowTall);
}
void GCtxDrawSetTextPos(const int x, const int y)
{
	surface()->DrawSetTextPos(g_pCtx->dPanel.x + x, g_pCtx->dPanel.y + y);
}

void SwapFont(const EFont eFont)
{
	if (g_pCtx->eMode != MODE_PAINT) return;
	g_pCtx->eFont = eFont;
	surface()->DrawSetTextFont(g_pCtx->fonts[g_pCtx->eFont].hdl);
}

void SwapColorNormal(const Color &color)
{
	g_pCtx->normalBgColor = color;
	surface()->DrawSetColor(g_pCtx->normalBgColor);
}

void BeginContext(NeoUI::Context *ctx, const NeoUI::Mode eMode, const wchar_t *wszTitle, const char *pSzCtxName)
{
	g_pCtx = ctx;
	g_pCtx->eMode = eMode;
	g_pCtx->iLayoutY = -(g_pCtx->iYOffset[0] * g_pCtx->iRowTall);
	g_pCtx->iWidget = 0;
	g_pCtx->iSection = 0;
	g_pCtx->iHasMouseInPanel = 0;
	g_pCtx->iHorizontalWidth = 0;
	g_pCtx->iHorizontalMargin = 0;
	g_pCtx->flWgXPerc = 0.4f;
	g_pCtx->bValueEdited = false;
	g_pCtx->eButtonTextStyle = TEXTSTYLE_CENTER;
	g_pCtx->eLabelTextStyle = TEXTSTYLE_LEFT;
	g_pCtx->bTextEditIsPassword = false;
	g_pCtx->selectBgColor = COLOR_NEOPANELSELECTBG;
	g_pCtx->normalBgColor = COLOR_NEOPANELACCENTBG;
	// Different pointer, change context
	if (g_pCtx->pSzCurCtxName != pSzCtxName)
	{
		g_pCtx->htSliders.RemoveAll();
		g_pCtx->pSzCurCtxName = pSzCtxName;
	}

	switch (g_pCtx->eMode)
	{
	case MODE_KEYPRESSED:
		if (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP)
		{
			g_pCtx->iActiveDirection = (g_pCtx->eCode == KEY_UP) ? -1 : +1;
			if (g_pCtx->iActive != FOCUSOFF_NUM)
			{
				g_pCtx->iActive += g_pCtx->iActiveDirection;
			}
			else
			{
				g_pCtx->iActive = 0;
				if (g_pCtx->iHot != FOCUSOFF_NUM)
				{
					g_pCtx->iActive = g_pCtx->iHot;
					g_pCtx->iActive += g_pCtx->iActiveDirection;
				}
			}
			g_pCtx->iHot = g_pCtx->iActive;
		}
		break;
	case MODE_MOUSERELEASED:
		g_pCtx->eMousePressedStart = MOUSEPOS_NONE;
		break;
	case MODE_PAINT:
		for (int i = 0; i < FONT__TOTAL; ++i)
		{
			FontInfo *pFontI = &g_pCtx->fonts[i];
			const int iTall = surface()->GetFontTall(pFontI->hdl);
			pFontI->iYOffset = (g_pCtx->iRowTall / 2) - (iTall / 2);
			{
				int iPrevNextWide, iPrevNextTall;
				surface()->GetTextSize(pFontI->hdl, L"<", iPrevNextWide, iPrevNextTall);
				pFontI->iStartBtnXPos = (g_pCtx->iRowTall / 2) - (iPrevNextWide / 2);
				pFontI->iStartBtnYPos = (g_pCtx->iRowTall / 2) - (iPrevNextTall / 2);
			}
		}

		if (wszTitle)
		{
			SwapFont(FONT_NTHEADING);
			surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
			GCtxDrawSetTextPos(g_pCtx->iMarginX, -g_pCtx->iRowTall + g_pCtx->fonts[FONT_NTHEADING].iYOffset);
			surface()->DrawPrintText(wszTitle, V_wcslen(wszTitle));
		}
		break;
	case MODE_MOUSEMOVED:
		g_pCtx->iHot = FOCUSOFF_NUM;
		g_pCtx->iHotSection = -1;
		break;
	default:
		break;
	}

	SwapFont(FONT_NTNORMAL);
	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
}

void EndContext()
{
	if (g_pCtx->eMode == MODE_MOUSEPRESSED && !g_pCtx->iHasMouseInPanel)
	{
		g_pCtx->eMousePos = MOUSEPOS_NONE;
		g_pCtx->iActiveDirection = 0;
		g_pCtx->iActive = FOCUSOFF_NUM;
		g_pCtx->iActiveSection = -1;
	}

	if (g_pCtx->eMode == MODE_KEYPRESSED)
	{
		if (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP || g_pCtx->eCode == KEY_TAB)
		{
			if (g_pCtx->iActiveSection == -1)
			{
				g_pCtx->iActiveSection = 0;
			}

			if (g_pCtx->eCode == KEY_TAB)
			{
				const int iTotalSection = g_pCtx->iSection;
				int iTally = 0;
				for (int i = 0; i < iTotalSection; ++i)
				{
					iTally += g_pCtx->iSectionCanActive[i];
				}
				if (iTally == 0)
				{
					// There is no valid sections, don't do anything
					g_pCtx->iActiveSection = -1;
				}
				else
				{
					const int iIncr = ((input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT)) ? -1 : +1);
					do
					{
						g_pCtx->iActiveSection += iIncr;
						g_pCtx->iActiveSection = LoopAroundInArray(g_pCtx->iActiveSection, iTotalSection);
					} while (g_pCtx->iSectionCanActive[g_pCtx->iActiveSection] == 0);
				}
			}
			g_pCtx->iHotSection = g_pCtx->iActiveSection;
		}
	}
	else if (g_pCtx->eMode == MODE_MOUSEPRESSED || g_pCtx->eMode == MODE_MOUSEDOUBLEPRESSED)
	{
		g_pCtx->iHot = g_pCtx->iActive;
		g_pCtx->iHotSection = g_pCtx->iActiveSection;
	}
}

void BeginSection(const bool bDefaultFocus)
{
	g_pCtx->iPartitionY = 0;
	g_pCtx->iLayoutX = 0;
	g_pCtx->iLayoutY = -(g_pCtx->iYOffset[g_pCtx->iSection] * g_pCtx->iRowTall);
	g_pCtx->iWidget = 0;
	g_pCtx->iCanActives = 0;
	g_pCtx->iWgXPos = static_cast<int>(g_pCtx->dPanel.wide * g_pCtx->flWgXPerc);

	g_pCtx->iMouseRelX = g_pCtx->iMouseAbsX - g_pCtx->dPanel.x;
	g_pCtx->iMouseRelY = g_pCtx->iMouseAbsY - g_pCtx->dPanel.y;
	g_pCtx->bMouseInPanel = (0 <= g_pCtx->iMouseRelX && g_pCtx->iMouseRelX < g_pCtx->dPanel.wide &&
						   0 <= g_pCtx->iMouseRelY && g_pCtx->iMouseRelY < g_pCtx->dPanel.tall);

	g_pCtx->iHasMouseInPanel += g_pCtx->bMouseInPanel;
	if (g_pCtx->bMouseInPanel && g_pCtx->eMode == MODE_MOUSEMOVED)
	{
		// g_pCtx->iRowTall used to shape the square buttons
		if (g_pCtx->iMouseRelX < g_pCtx->iWgXPos)
		{
			g_pCtx->eMousePos = NeoUI::MOUSEPOS_NONE;
		}
		else if (g_pCtx->iMouseRelX < (g_pCtx->iWgXPos + g_pCtx->iRowTall))
		{
			g_pCtx->eMousePos = NeoUI::MOUSEPOS_LEFT;
		}
		else if (g_pCtx->iMouseRelX > (g_pCtx->dPanel.wide - g_pCtx->iRowTall))
		{
			g_pCtx->eMousePos = NeoUI::MOUSEPOS_RIGHT;
		}
		else
		{
			g_pCtx->eMousePos = NeoUI::MOUSEPOS_CENTER;
		}
	}

	switch (g_pCtx->eMode)
	{
	case MODE_PAINT:
		surface()->DrawSetColor(g_pCtx->bgColor);
		surface()->DrawFilledRect(g_pCtx->dPanel.x,
								  g_pCtx->dPanel.y,
								  g_pCtx->dPanel.x + g_pCtx->dPanel.wide,
								  g_pCtx->dPanel.y + g_pCtx->dPanel.tall);

		surface()->DrawSetColor(g_pCtx->normalBgColor);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
		break;
	case MODE_KEYPRESSED:
		if (bDefaultFocus && g_pCtx->iActiveSection == -1 && (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP))
		{
			g_pCtx->iActiveSection = g_pCtx->iSection;
		}
		break;
	}
}

void EndSection()
{
	if (g_pCtx->eMode == MODE_MOUSEPRESSED && g_pCtx->bMouseInPanel &&
			g_pCtx->iLayoutY < g_pCtx->iMouseRelY)
	{
		g_pCtx->eMousePos = MOUSEPOS_NONE;
		g_pCtx->iActiveDirection = 0;
		g_pCtx->iActive = FOCUSOFF_NUM;
		g_pCtx->iActiveSection = -1;
	}
	if (g_pCtx->iActive != FOCUSOFF_NUM && g_pCtx->iActiveSection == g_pCtx->iSection)
	{
		g_pCtx->iActive = LoopAroundInArray(g_pCtx->iActive, g_pCtx->iWidget);
	}

	// Scroll handling
	const int iRowsInScreen = g_pCtx->dPanel.tall / g_pCtx->iRowTall;
	const bool bHasScroll = (g_pCtx->iPartitionY > iRowsInScreen);
	vgui::IntRect rectScrollArea = (bHasScroll) ?
			vgui::IntRect{
				.x0 = g_pCtx->dPanel.x + g_pCtx->dPanel.wide,
				.y0 = g_pCtx->dPanel.y,
				.x1 = g_pCtx->dPanel.x + g_pCtx->dPanel.wide + g_pCtx->iRowTall,
				.y1 = g_pCtx->dPanel.y + g_pCtx->dPanel.tall,
			} : vgui::IntRect{};
	const bool bMouseInScrollbar = bHasScroll && InRect(rectScrollArea, g_pCtx->iMouseAbsX, g_pCtx->iMouseAbsY);
	const bool bMouseInWheelable = g_pCtx->bMouseInPanel || bMouseInScrollbar;

	if (g_pCtx->eMode == MODE_MOUSEWHEELED && bMouseInWheelable)
	{
		if (!bHasScroll)
		{
			g_pCtx->iYOffset[g_pCtx->iSection] = 0;
		}
		else
		{
			g_pCtx->iYOffset[g_pCtx->iSection] += (g_pCtx->eCode == MOUSE_WHEEL_UP) ? -1 : +1;
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iYOffset[g_pCtx->iSection], 0, g_pCtx->iPartitionY - iRowsInScreen);
		}
	}
	else if (g_pCtx->eMode == MODE_KEYPRESSED && (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP) &&
			 (g_pCtx->iActiveSection == g_pCtx->iSection || g_pCtx->iHotSection == g_pCtx->iSection))
	{
		if (!bHasScroll)
		{
			// Disable scroll if it doesn't need to
			g_pCtx->iYOffset[g_pCtx->iSection] = 0;
		}
		else if (g_pCtx->iActive < (g_pCtx->iYOffset[g_pCtx->iSection]))
		{
			// Scrolling up past visible, re-adjust
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iActive, 0, g_pCtx->iWidget - iRowsInScreen);
		}
		else if (g_pCtx->iActive >= (g_pCtx->iYOffset[g_pCtx->iSection] + iRowsInScreen))
		{
			// Scrolling down post visible, re-adjust
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iActive - iRowsInScreen + 1, 0, g_pCtx->iWidget - iRowsInScreen);
		}
	}

	// Scroll bar area painting and mouse interaction (no keyboard as that's handled by active widgets)
	if (bHasScroll)
	{
		const int iYStart = g_pCtx->iYOffset[g_pCtx->iSection];
		const int iYEnd = iYStart + iRowsInScreen;
		const float flYPercStart = iYStart / static_cast<float>(g_pCtx->iWidget);
		const float flYPercEnd = iYEnd / static_cast<float>(g_pCtx->iWidget);
		vgui::IntRect rectHandle{
			g_pCtx->dPanel.x + g_pCtx->dPanel.wide,
			g_pCtx->dPanel.y + static_cast<int>(g_pCtx->dPanel.tall * flYPercStart),
			g_pCtx->dPanel.x + g_pCtx->dPanel.wide + g_pCtx->iRowTall,
			g_pCtx->dPanel.y + static_cast<int>(g_pCtx->dPanel.tall * flYPercEnd)
		};

		bool bAlterOffset = false;
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
			surface()->DrawSetColor(g_pCtx->bgColor);
			surface()->DrawFilledRectArray(&rectScrollArea, 1);
			surface()->DrawSetColor(g_pCtx->abYMouseDragOffset[g_pCtx->iSection] ? g_pCtx->selectBgColor : g_pCtx->normalBgColor);
			surface()->DrawFilledRectArray(&rectHandle, 1);
			break;
		case MODE_MOUSEPRESSED:
			g_pCtx->abYMouseDragOffset[g_pCtx->iSection] = bMouseInScrollbar;
			if (bMouseInScrollbar)
			{
				// If not pressed on handle, set the drag at the middle
				const bool bInHandle = InRect(rectHandle, g_pCtx->iMouseAbsX, g_pCtx->iMouseAbsY);
				g_pCtx->iStartMouseDragOffset[g_pCtx->iSection] = (bInHandle) ?
							(g_pCtx->iMouseAbsY - rectHandle.y0) :((rectHandle.y1 - rectHandle.y0) / 2.0f);
				bAlterOffset = true;
			}
			break;
		case MODE_MOUSERELEASED:
			g_pCtx->abYMouseDragOffset[g_pCtx->iSection] = false;
			break;
		case MODE_MOUSEMOVED:
			bAlterOffset = g_pCtx->abYMouseDragOffset[g_pCtx->iSection];
			break;
		default:
			break;
		}

		if (bAlterOffset)
		{
			const float flXPercMouse = static_cast<float>(g_pCtx->iMouseRelY - g_pCtx->iStartMouseDragOffset[g_pCtx->iSection]) / static_cast<float>(g_pCtx->dPanel.tall);
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(flXPercMouse * g_pCtx->iWidget, 0, (g_pCtx->iWidget - iRowsInScreen));
		}
	}

	g_pCtx->iSectionCanActive[g_pCtx->iSection] = g_pCtx->iCanActives;
	++g_pCtx->iSection;
}

void BeginHorizontal(const int iHorizontalWidth, const int iHorizontalMargin)
{
	g_pCtx->iHorizontalWidth = iHorizontalWidth;
	g_pCtx->iHorizontalMargin = iHorizontalMargin;
	g_pCtx->iLayoutX = 0;
}

void EndHorizontal()
{
	g_pCtx->iHorizontalWidth = 0;
	g_pCtx->iHorizontalMargin = 0;
	g_pCtx->iLayoutX = 0;
	++g_pCtx->iPartitionY;
	g_pCtx->iLayoutY += g_pCtx->iRowTall;
}

static void InternalLabel(const wchar_t *wszText, const bool bCenter)
{
	if (g_pCtx->eMode != MODE_PAINT) return;
	const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];

	int iFontTextWidth = 0, iFontTextHeight = 0;
	if (bCenter)
	{
		surface()->GetTextSize(pFontI->hdl, wszText, iFontTextWidth, iFontTextHeight);
	}
	GCtxDrawSetTextPos(((bCenter) ? ((g_pCtx->dPanel.wide / 2) - (iFontTextWidth / 2)) : g_pCtx->iMarginX),
					   g_pCtx->iLayoutY + pFontI->iYOffset);
	surface()->DrawPrintText(wszText, V_wcslen(wszText));
}

struct GetMouseinFocusedRet
{
	bool bActive;
	bool bHot;
};

static GetMouseinFocusedRet InternalGetMouseinFocused()
{
	bool bMouseIn = (g_pCtx->bMouseInPanel && ((g_pCtx->iYOffset[g_pCtx->iSection] + (g_pCtx->iMouseRelY / g_pCtx->iRowTall)) == g_pCtx->iPartitionY));
	if (bMouseIn && g_pCtx->iHorizontalWidth)
	{
		bMouseIn = IN_BETWEEN_AR(g_pCtx->iLayoutX, g_pCtx->iMouseRelX, g_pCtx->iLayoutX + g_pCtx->iHorizontalWidth);
	}
	if (bMouseIn && g_pCtx->eMode == MODE_MOUSEMOVED)
	{
		g_pCtx->iHot = g_pCtx->iWidget;
		g_pCtx->iHotSection = g_pCtx->iSection;
	}
	const bool bHot = g_pCtx->iHot == g_pCtx->iWidget && g_pCtx->iHotSection == g_pCtx->iSection;
	const bool bActive = g_pCtx->iWidget == g_pCtx->iActive && g_pCtx->iSection == g_pCtx->iActiveSection;
	if (bActive || bHot)
	{
		surface()->DrawSetColor(g_pCtx->selectBgColor);
		if (bActive) surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	}
	return GetMouseinFocusedRet{
		.bActive = bActive,
		.bHot = bHot,
	};
}

static void InternalUpdatePartitionState(const GetMouseinFocusedRet wdgState)
{
	NeoUI::Pad();
	++g_pCtx->iWidget;
	if (wdgState.bActive || wdgState.bHot)
	{
		surface()->DrawSetColor(g_pCtx->normalBgColor);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
	}
}

void Pad()
{
	if (g_pCtx->iHorizontalWidth)
	{
		g_pCtx->iLayoutX += g_pCtx->iHorizontalWidth + g_pCtx->iHorizontalMargin;
	}
	else
	{
		++g_pCtx->iPartitionY;
		g_pCtx->iLayoutY += g_pCtx->iRowTall;
	}
}

void GCtxSkipActive()
{
	if (g_pCtx->iWidget == g_pCtx->iActive && g_pCtx->iSection == g_pCtx->iActiveSection)
	{
		g_pCtx->iActive += g_pCtx->iActiveDirection;
	}
}

void LabelWrap(const wchar_t *wszText)
{
	if (!wszText || wszText[0] == '\0')
	{
		NeoUI::Pad();
		return;
	}

	GCtxSkipActive();
	int iLines = 0;
	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
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
			const int chWidth = surface()->GetCharacterWidth(pFontI->hdl, ch);
			iSumWidth += chWidth;
			if (iSumWidth >= g_pCtx->dPanel.wide)
			{
				if (g_pCtx->eMode == MODE_PAINT)
				{
					GCtxDrawSetTextPos(g_pCtx->iMarginX, g_pCtx->iLayoutY + pFontI->iYOffset + iYOffset);
					surface()->DrawPrintText(wszText + iStart, iLastSpace - iStart);
				}
				++iLines;

				iYOffset += g_pCtx->iRowTall;
				iSumWidth = 0;
				iStart = iLastSpace + 1;
				i = iLastSpace;
			}
		}
		if (iSumWidth > 0)
		{
			if (g_pCtx->eMode == MODE_PAINT)
			{
				GCtxDrawSetTextPos(g_pCtx->iMarginX, g_pCtx->iLayoutY + pFontI->iYOffset + iYOffset);
				surface()->DrawPrintText(wszText + iStart, iWszSize - iStart);
			}
			++iLines;
		}
	}

	g_pCtx->iPartitionY += iLines;
	g_pCtx->iLayoutY += (g_pCtx->iRowTall * iLines);

	++g_pCtx->iWidget;
	surface()->DrawSetColor(g_pCtx->normalBgColor);
	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
}

void Label(const wchar_t *wszText)
{
	GCtxSkipActive();
	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall) && g_pCtx->eMode == MODE_PAINT)
	{
		InternalLabel(wszText, g_pCtx->eLabelTextStyle == TEXTSTYLE_CENTER);
	}
	InternalUpdatePartitionState(GetMouseinFocusedRet{true, true});
}

void Label(const wchar_t *wszLabel, const wchar_t *wszText)
{
	GCtxSkipActive();
	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall) && g_pCtx->eMode == MODE_PAINT)
	{
		const int iTmpMarginX = g_pCtx->iMarginX;
		InternalLabel(wszLabel, g_pCtx->eLabelTextStyle == TEXTSTYLE_CENTER);
		g_pCtx->iMarginX = g_pCtx->iWgXPos + g_pCtx->iMarginX;
		InternalLabel(wszText, g_pCtx->eLabelTextStyle == TEXTSTYLE_CENTER);
		g_pCtx->iMarginX = iTmpMarginX;
	}
	InternalUpdatePartitionState(GetMouseinFocusedRet{true, true});
}

NeoUI::RetButton Button(const wchar_t *wszText)
{
	return Button(nullptr, wszText);
}

NeoUI::RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText)
{
	RetButton ret = {};
	const auto wdgState = InternalGetMouseinFocused();
	ret.bMouseHover = wdgState.bHot && (!wszLeftLabel || (wszLeftLabel && g_pCtx->eMousePos != MOUSEPOS_NONE));

	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		const int iBtnWidth = g_pCtx->iHorizontalWidth ? g_pCtx->iHorizontalWidth : g_pCtx->dPanel.wide;
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
			int iFontWide, iFontTall;
			surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);

			if (wszLeftLabel)
			{
				if (wdgState.bHot)
				{
					GCtxDrawFilledRectXtoX(0, g_pCtx->dPanel.wide);
				}
				InternalLabel(wszLeftLabel, false);
				const int xMargin = g_pCtx->eButtonTextStyle == TEXTSTYLE_CENTER ?
							(((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2)) : g_pCtx->iMarginX;
				GCtxDrawSetTextPos(g_pCtx->iWgXPos + xMargin,
								   g_pCtx->iLayoutY + pFontI->iYOffset);
			}
			else
			{
				// No label, fill the whole partition
				GCtxDrawFilledRectXtoX(0, iBtnWidth);
				const int xMargin = g_pCtx->eButtonTextStyle == TEXTSTYLE_CENTER ?
							((iBtnWidth / 2) - (iFontWide / 2)) : g_pCtx->iMarginX;
				GCtxDrawSetTextPos(g_pCtx->iLayoutX + xMargin, g_pCtx->iLayoutY + pFontI->iYOffset);
			}
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && g_pCtx->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			ret.bMouseDoublePressed = ret.bPressed = (ret.bMouseHover && g_pCtx->eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			ret.bKeyPressed = ret.bPressed = (wdgState.bActive && g_pCtx->eCode == KEY_ENTER);
		}
		break;
		default:
			break;
		}

		if (ret.bPressed)
		{
			g_pCtx->iActive = g_pCtx->iWidget;
			g_pCtx->iActiveSection = g_pCtx->iSection;
			g_pCtx->iActiveDirection = 0;
		}
	}

	++g_pCtx->iCanActives;
	InternalUpdatePartitionState(wdgState);
	return ret;
}

void RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked)
{
	int iIndex = static_cast<int>(*bChecked);
	RingBox(wszLeftLabel, ENABLED_LABELS, 2, &iIndex);
	*bChecked = static_cast<bool>(iIndex);
}

void RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			if (wdgState.bHot)
			{
				GCtxDrawFilledRectXtoX(0, g_pCtx->dPanel.wide);
			}
			const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
			InternalLabel(wszLeftLabel, false);

			// Center-text label
			const wchar_t *wszText = wszLabelsList[*iIndex];
			int iFontWide, iFontTall;
			surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + (((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2)),
							   g_pCtx->iLayoutY + pFontI->iYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->iWgXPos + g_pCtx->iRowTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + pFontI->iStartBtnXPos, g_pCtx->iLayoutY + pFontI->iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_pCtx->dPanel.wide - g_pCtx->iRowTall, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->dPanel.wide - g_pCtx->iRowTall + pFontI->iStartBtnXPos, g_pCtx->iLayoutY + pFontI->iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (wdgState.bHot)
			{
				g_pCtx->iActive = g_pCtx->iWidget;
				g_pCtx->iActiveSection = g_pCtx->iSection;
				g_pCtx->iActiveDirection = 0;
			}
			if (wdgState.bHot && g_pCtx->eCode == MOUSE_LEFT && (g_pCtx->eMousePos == MOUSEPOS_LEFT || g_pCtx->eMousePos == MOUSEPOS_RIGHT))
			{
				*iIndex += (g_pCtx->eMousePos == MOUSEPOS_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive && (g_pCtx->eCode == KEY_LEFT || g_pCtx->eCode == KEY_RIGHT))
			{
				*iIndex += (g_pCtx->eCode == KEY_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	++g_pCtx->iCanActives;
	InternalUpdatePartitionState(wdgState);
}

void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	SwapFont(FONT_NTHORIZSIDES);
	// This is basically a ringbox but different UI
	if (g_pCtx->iWidget == g_pCtx->iActive && g_pCtx->iSection == g_pCtx->iActiveSection) g_pCtx->iActive += g_pCtx->iActiveDirection;
	const auto wdgState = InternalGetMouseinFocused();
	const int iTabWide = (g_pCtx->dPanel.wide / iLabelsSize);
	bool bResetActiveHot = false;

	switch (g_pCtx->eMode)
	{
	case MODE_PAINT:
	{
		const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
		for (int i = 0, iXPosTab = 0; i < iLabelsSize; ++i, iXPosTab += iTabWide)
		{
			const bool bHoverTab = (wdgState.bHot && IN_BETWEEN_AR(iXPosTab, g_pCtx->iMouseRelX, iXPosTab + iTabWide));
			if (bHoverTab || i == *iIndex)
			{
				// NEO NOTE (nullsystem): On the final tab, just expand it to the end width as iTabWide isn't always going
				// to give a properly aligned width
				surface()->DrawSetColor(bHoverTab ? COLOR_NEOPANELSELECTBG : g_pCtx->normalBgColor);
				GCtxDrawFilledRectXtoX(iXPosTab, (i == (iLabelsSize - 1)) ? (g_pCtx->dPanel.wide) : (iXPosTab + iTabWide));
			}
			const wchar_t *wszText = wszLabelsList[i];
			GCtxDrawSetTextPos(iXPosTab + g_pCtx->iMarginX, g_pCtx->iLayoutY + pFontI->iYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}

		// Draw the side-hints text
		// NEO NOTE (nullsystem): F# as 1 is thinner than 3/not monospaced font
		int iFontWidth, iFontHeight;
		surface()->GetTextSize(g_pCtx->fonts[g_pCtx->eFont].hdl, L"F##", iFontWidth, iFontHeight);
		const int iHintYPos = g_pCtx->dPanel.y + (iFontHeight / 2);

		surface()->DrawSetTextPos(g_pCtx->dPanel.x - g_pCtx->iMarginX - iFontWidth, iHintYPos);
		surface()->DrawPrintText(L"F 1", 3);
		surface()->DrawSetTextPos(g_pCtx->dPanel.x + g_pCtx->dPanel.wide + g_pCtx->iMarginX, iHintYPos);
		surface()->DrawPrintText(L"F 3", 3);
	}
	break;
	case MODE_MOUSEPRESSED:
	{
		if (wdgState.bHot && g_pCtx->eCode == MOUSE_LEFT)
		{
			const int iNextIndex = (g_pCtx->iMouseRelX / iTabWide);
			if (iNextIndex != *iIndex)
			{
				*iIndex = clamp(iNextIndex, 0, iLabelsSize);
				V_memset(g_pCtx->iYOffset, 0, sizeof(g_pCtx->iYOffset));
				bResetActiveHot = true;
			}
		}
	}
	break;
	case MODE_KEYPRESSED:
	{
		if (g_pCtx->eCode == KEY_F1 || g_pCtx->eCode == KEY_F3) // Global keybind
		{
			*iIndex += (g_pCtx->eCode == KEY_F1) ? -1 : +1;
			*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
			V_memset(g_pCtx->iYOffset, 0, sizeof(g_pCtx->iYOffset));
			bResetActiveHot = true;
		}
	}
	break;
	default:
		break;
	}

	if (bResetActiveHot)
	{
		g_pCtx->eMousePos = MOUSEPOS_NONE;
		g_pCtx->iActiveDirection = 0;
		g_pCtx->iActive = FOCUSOFF_NUM;
		g_pCtx->iActiveSection = -1;
		g_pCtx->iHot = FOCUSOFF_NUM;
		g_pCtx->iHotSection = -1;
	}

	InternalUpdatePartitionState(wdgState);
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
	GCtxSkipActive();
	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall) && g_pCtx->eMode == MODE_PAINT)
	{
		const float flPerc = (flValue - flMin) / (flMax - flMin);
		GCtxDrawFilledRectXtoX(0, flPerc * g_pCtx->dPanel.wide);
	}
	InternalUpdatePartitionState(GetMouseinFocusedRet{true, true});
}

void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp, const float flStep, const wchar_t *wszSpecialText)
{
	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		auto hdl = g_pCtx->htSliders.Find(wszLeftLabel);
		if (hdl == -1 || g_pCtx->htSliders.Element(hdl).flCachedValue != *flValue ||
				g_pCtx->htSliders.Element(hdl).bActive != wdgState.bActive)
		{
			// Update string/cached value
			wchar_t wszFormat[32];
			V_swprintf_safe(wszFormat, L"%%0.%df", iDp);

			if (hdl == -1)
			{
				SliderInfo sInfoNew = {};
				hdl = g_pCtx->htSliders.Insert(wszLeftLabel, sInfoNew);
			}
			SliderInfo *pSInfo = &g_pCtx->htSliders.Element(hdl);

			V_swprintf_safe(pSInfo->wszText, wszFormat, *flValue);
			pSInfo->flCachedValue = *flValue;
			wchar_t wszTmpTest[ARRAYSIZE(pSInfo->wszText)];
			const int iStrMinLen = V_swprintf_safe(wszTmpTest, wszFormat, flMin);
			const int iStrMaxLen = V_swprintf_safe(wszTmpTest, wszFormat, flMax);
			pSInfo->iMaxStrSize = max(iStrMinLen, iStrMaxLen);
			pSInfo->bActive = wdgState.bActive;
		}
		SliderInfo *pSInfo = &g_pCtx->htSliders.Element(hdl);

		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
			if (wdgState.bHot)
			{
				GCtxDrawFilledRectXtoX(0, g_pCtx->dPanel.wide);
			}
			InternalLabel(wszLeftLabel, false);

			// Background bar
			const float flPerc = (((iDp == 0) ? (int)(*flValue) : *flValue) - flMin) / (flMax - flMin);
			const float flBarMaxWide = static_cast<float>(g_pCtx->dPanel.wide - g_pCtx->iWgXPos - (2 * g_pCtx->iRowTall));
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos + g_pCtx->iRowTall, g_pCtx->iWgXPos + g_pCtx->iRowTall + static_cast<int>(flPerc * flBarMaxWide));

			// Center-text label
			const bool bSpecial = (wszSpecialText && !wdgState.bActive && *flValue == 0.0f);
			int iFontWide, iFontTall;
			surface()->GetTextSize(pFontI->hdl, bSpecial ? wszSpecialText : pSInfo->wszText, iFontWide, iFontTall);
			const int iFontStartX = g_pCtx->iWgXPos + (((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2));
			GCtxDrawSetTextPos(iFontStartX, g_pCtx->iLayoutY + pFontI->iYOffset);
			surface()->DrawPrintText(bSpecial ? wszSpecialText : pSInfo->wszText,
									 bSpecial ? V_wcslen(wszSpecialText) : V_wcslen(pSInfo->wszText));

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->iWgXPos + g_pCtx->iRowTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + pFontI->iStartBtnXPos, g_pCtx->iLayoutY + pFontI->iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_pCtx->dPanel.wide - g_pCtx->iRowTall, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->dPanel.wide - g_pCtx->iRowTall + pFontI->iStartBtnXPos, g_pCtx->iLayoutY + pFontI->iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);

			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					const int iMarkX = iFontStartX + iFontWide;
					surface()->DrawSetColor(COLOR_NEOPANELTEXTBRIGHT);
					GCtxDrawFilledRectXtoX(iMarkX, pFontI->iYOffset,
										   iMarkX + g_pCtx->iMarginX, pFontI->iYOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (wdgState.bHot)
			{
				g_pCtx->iActive = g_pCtx->iWidget;
				g_pCtx->iActiveSection = g_pCtx->iSection;
				g_pCtx->iActiveDirection = 0;
				if (g_pCtx->eCode == MOUSE_LEFT)
				{
					if (g_pCtx->eMousePos == MOUSEPOS_LEFT || g_pCtx->eMousePos == MOUSEPOS_RIGHT)
					{
						*flValue += (g_pCtx->eMousePos == MOUSEPOS_LEFT) ? -flStep : +flStep;
						*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
						g_pCtx->bValueEdited = true;
					}
					else if (g_pCtx->eMousePos == MOUSEPOS_CENTER)
					{
						g_pCtx->iActiveDirection = 0;
						g_pCtx->iActive = g_pCtx->iWidget;
						g_pCtx->iActiveSection = g_pCtx->iSection;
					}
					g_pCtx->eMousePressedStart = g_pCtx->eMousePos;
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive)
			{
				if (g_pCtx->eCode == KEY_LEFT || g_pCtx->eCode == KEY_RIGHT)
				{
					*flValue += (g_pCtx->eCode == KEY_LEFT) ? -flStep : +flStep;
					*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
					g_pCtx->bValueEdited = true;
				}
				else if (g_pCtx->eCode == KEY_ENTER)
				{
					g_pCtx->iActiveDirection = 0;
					g_pCtx->iActive = FOCUSOFF_NUM;
					g_pCtx->iActiveSection = -1;
				}
				else if (g_pCtx->eCode == KEY_BACKSPACE)
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
								g_pCtx->bValueEdited = true;
							}
						}
					}
				}
			}
			else if (wdgState.bHot)
			{
				if (g_pCtx->eCode == KEY_ENTER)
				{
					g_pCtx->iActiveDirection = 0;
					g_pCtx->iActive = g_pCtx->iWidget;
					g_pCtx->iActiveSection = g_pCtx->iSection;
				}
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (wdgState.bActive && input()->IsMouseDown(MOUSE_LEFT) && g_pCtx->eMousePressedStart == MOUSEPOS_CENTER)
			{
				const int iBase = g_pCtx->iRowTall + g_pCtx->iWgXPos;
				const float flPerc = static_cast<float>(g_pCtx->iMouseRelX - iBase) / static_cast<float>(g_pCtx->dPanel.wide - g_pCtx->iRowTall - iBase);
				*flValue = flMin + (flPerc * (flMax - flMin));
				*flValue = ClampAndLimitDp(*flValue, flMin, flMax, iDp);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive)
			{
				const int iTextSize = V_wcslen(pSInfo->wszText);
				const wchar_t uc = g_pCtx->unichar;
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
						g_pCtx->bValueEdited = true;
					}
				}
			}
		}
		break;
		default:
			break;
		}
	}

	++g_pCtx->iCanActives;
	InternalUpdatePartitionState(wdgState);
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
	static wchar_t staticWszPasswordChars[256] = {};
	if (staticWszPasswordChars[0] == L'\0')
	{
		for (int i = 0; i < (ARRAYSIZE(staticWszPasswordChars) - 1); ++i)
		{
			staticWszPasswordChars[i] = L'*';
		}
	}

	const auto wdgState = InternalGetMouseinFocused();

	if (IN_BETWEEN_AR(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		const int iBtnWidth = g_pCtx->iHorizontalWidth ? g_pCtx->iHorizontalWidth : g_pCtx->dPanel.wide;
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			if (wdgState.bHot)
			{
				GCtxDrawFilledRectXtoX(0, g_pCtx->dPanel.wide);
			}
			const auto *pFontI = &g_pCtx->fonts[g_pCtx->eFont];
			InternalLabel(wszLeftLabel, false);
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + g_pCtx->iMarginX, g_pCtx->iLayoutY + pFontI->iYOffset);
			const int iWszTextSize = V_wcslen(wszText);
			if (g_pCtx->bTextEditIsPassword)
			{
				surface()->DrawPrintText(staticWszPasswordChars, iWszTextSize);
			}
			else
			{
				surface()->DrawPrintText(wszText, iWszTextSize);
			}
			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					int iFontWide, iFontTall;
					if (g_pCtx->bTextEditIsPassword)
					{
						staticWszPasswordChars[iWszTextSize] = L'\0';
						surface()->GetTextSize(pFontI->hdl, staticWszPasswordChars, iFontWide, iFontTall);
						staticWszPasswordChars[iWszTextSize] = L'*';
					}
					else
					{
						surface()->GetTextSize(pFontI->hdl, wszText, iFontWide, iFontTall);
					}
					const int iMarkX = g_pCtx->iWgXPos + g_pCtx->iMarginX + iFontWide;
					surface()->DrawSetColor(COLOR_NEOPANELTEXTBRIGHT);
					GCtxDrawFilledRectXtoX(iMarkX, pFontI->iYOffset,
										   iMarkX + g_pCtx->iMarginX, pFontI->iYOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (wdgState.bHot && g_pCtx->iWgXPos <= g_pCtx->iMouseRelX)
			{
				g_pCtx->iActive = g_pCtx->iWidget;
				g_pCtx->iActiveSection = g_pCtx->iSection;
				g_pCtx->iActiveDirection = 0;
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive && g_pCtx->eCode == KEY_BACKSPACE)
			{
				const int iTextSize = V_wcslen(wszText);
				if (iTextSize > 0)
				{
					wszText[iTextSize - 1] = L'\0';
					g_pCtx->bValueEdited = true;
				}
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive && iswprint(g_pCtx->unichar))
			{
				static char szTmpANSICheck[MAX_TEXTINPUT_U8BYTES_LIMIT];
				const int iTextInU8Bytes = g_pVGuiLocalize->ConvertUnicodeToANSI(wszText, szTmpANSICheck, sizeof(szTmpANSICheck));
				if (iTextInU8Bytes < iMaxBytes)
				{
					int iTextSize = V_wcslen(wszText);
					wszText[iTextSize++] = g_pCtx->unichar;
					wszText[iTextSize] = L'\0';
					g_pCtx->bValueEdited = true;
				}
			}
		}
		break;
		default:
			break;
		}
	}

	++g_pCtx->iCanActives;
	InternalUpdatePartitionState(wdgState);
}

bool Bind(const ButtonCode_t eCode)
{
	return g_pCtx->eMode == MODE_KEYPRESSED && g_pCtx->eCode == eCode;
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
