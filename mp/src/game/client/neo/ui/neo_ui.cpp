#include "neo_ui.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>
#include <filesystem.h>

#include "stb_image.h"
#include "stb_image_resize2.h"
#include "stb_dxt.h"

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
	g_pCtx->iWgXPos = static_cast<int>(g_pCtx->dPanel.wide * 0.4f);
	g_pCtx->iSection = 0;
	g_pCtx->iHasMouseInPanel = 0;
	g_pCtx->iHorizontalWidth = 0;
	g_pCtx->iHorizontalMargin = 0;
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
				InternalLabel(wszLeftLabel, false);
				GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->dPanel.wide);
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

uint8 *ConvertToVTF(char (*szRetTexPath)[VTF_PATH_MAX], const char *szFullpath)
{
	if (!szFullpath)
	{
		return nullptr;
	}

	int width, height, stride;
	uint8 *data = stbi_load(szFullpath, &width, &height, &stride, 0);
	if (!data || width <= 0 || height <= 0)
	{
		return nullptr;
	}

	// Convert to RGBA
	if (stride == 3)
	{
		uint8 *rgbaData = (uint8 *)(calloc(width * height, sizeof(uint8) * 4));

#if 0
		const int iSrcEnd = width * height * stride;

		for (int srcOffset = 0, dstOffset = 0;
			 srcOffset < iSrcEnd;
			 srcOffset += stride, dstOffset += 4)
		{
			V_memcpy(rgbaData + dstOffset, data + srcOffset, stride);
			(rgbaData + dstOffset)[4] = 255;
		}
#else
		bool bConverted = ImageLoader::ConvertImageFormat(data, IMAGE_FORMAT_RGB888,
														  rgbaData, IMAGE_FORMAT_RGBA8888,
														  width, height);
#endif

		stbi_image_free(data);
		data = rgbaData;
		stride = 4;
	}

	// Crop to square
	if (width != height)
	{
		const int targetWH = min(width, height);
		uint8 *croppedData = (uint8 *)(calloc(targetWH * targetWH, sizeof(uint8) * stride));

		const int dstYLines = targetWH * stride;
		const int dstYEnd = dstYLines * targetWH;
		const int dstXOffset = (width < targetWH) ? (targetWH - width) : 0;
		const int dstYOffset = (height < targetWH) ? (targetWH - height) : 0;
		const int dstOffsetStart = (dstYOffset * dstYLines) + (dstXOffset * stride);

		const int srcYLines = width * stride;
		const int srcYEnd = srcYLines * height;
		const int srcXOffset = (width > targetWH) ? ((width / 2) - (targetWH / 2)) : 0;
		const int srcYOffset = (height > targetWH) ? ((height / 2) - (targetWH / 2)) : 0;
		const int srcOffsetStart = (srcYOffset * srcYLines) + (srcXOffset * stride);

		const int xTakeMem = ((width > targetWH) ? targetWH : width) * stride;

		for (int srcOffset = srcOffsetStart, dstOffset = dstOffsetStart;
			 (srcOffset < srcYEnd) && (dstOffset < dstYEnd);
			 srcOffset += srcYLines, dstOffset += dstYLines)
		{
			V_memcpy(croppedData + dstOffset, data + srcOffset, xTakeMem);
		}

		stbi_image_free(data);
		data = croppedData;
		width = targetWH;
		height = targetWH;
	}

	// Downscale to 256x256
	if (width != 256 || height != 256)
	{
		uint8 *downData = (uint8 *)(calloc(256 * 256, sizeof(uint8) * stride));
		stbir_resize_uint8_linear(data, width, height, 0,
								  downData, 256, 256, 0,
								  STBIR_4CHANNEL);//STBIR_RGBA);

		stbi_image_free(data);
		data = downData;
		width = 256;
		height = 256;
	}

#if 0
	const char *pLastSlash = V_strrchr(szFullpath, '/');
	const char *pszBaseName = pLastSlash ? pLastSlash + 1 : szFullpath;
	char *pszDot = strchr((char *)(pszBaseName), '.');
	if (pszDot)
	{
		*pszDot = '\0';
	}
#else
	const char *pszBaseName = "spray";
#endif
	V_sprintf_safe(*szRetTexPath, "materials/vgui/logos/%s", pszBaseName);
	filesystem->CreateDirHierarchy("materials/vgui/logos");
	filesystem->CreateDirHierarchy("materials/vgui/logos/ui");
	char szFullFilePath[PATH_MAX];

#if 0
	From VPC: https://developer.valvesoftware.com/wiki/VTF_(Valve_Texture_Format)
	typedef struct tagVTFHEADER // 7.1
	{
		char            signature[4];       // File signature ("VTF\0"). (or as little-endian integer, 0x00465456)
		unsigned int    version[2];         // version[0].version[1] (currently 7.2).
		unsigned int    headerSize;         // Size of the header struct  (16 byte aligned; currently 80 bytes) + size of the resources dictionary (7.3+).
		unsigned short  width;              // Width of the largest mipmap in pixels. Must be a power of 2.
		unsigned short  height;             // Height of the largest mipmap in pixels. Must be a power of 2.
		unsigned int    flags;              // VTF flags.
		unsigned short  frames;             // Number of frames, if animated (1 for no animation).
		unsigned short  firstFrame;         // First frame in animation (0 based). Can be -1 in environment maps older than 7.5, meaning there are 7 faces, not 6.
		unsigned char   padding0[4];        // reflectivity padding (16 byte alignment).
		float           reflectivity[3];    // reflectivity vector.
		unsigned char   padding1[4];        // reflectivity padding (8 byte packing).
		float           bumpmapScale;       // Bumpmap scale.
		int             highResImageFormat; // High resolution image format.
		unsigned char   mipmapCount;        // Number of mipmaps.
		int             lowResImageFormat;  // Low resolution image format (Usually DXT1).
		unsigned char   lowResImageWidth;   // Low resolution image width.
		unsigned char   lowResImageHeight;  // Low resolution image height.
	} VTFHEADER;
#endif

#define DXT_OUT
#define VER_71
//#define VER_74

#if 1
	// Create and initialize a 256x256 VTF texture
	CUtlBuffer buffer(0, 0, 0);

	// Write out the headers first
	{
		// Signature
		buffer.PutString("VTF");

		// Version 7.1
#ifndef VER_74
		buffer.PutUnsignedInt(7);
		buffer.PutUnsignedInt(1);
#else
		buffer.PutUnsignedInt(7);
		buffer.PutUnsignedInt(4);
#endif

		// Header size
#ifdef VER_74
		buffer.PutUnsignedInt(88);
#else
		buffer.PutUnsignedInt(64);
#endif

		// Width + height
		buffer.PutUnsignedShort(256);
		buffer.PutUnsignedShort(256);

		// Flags
#ifdef DXT_OUT
		const int iFlags = TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_NOLOD;
#else
		const int iFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOLOD;
#endif
		buffer.PutUnsignedInt(iFlags);

		// Frames
		buffer.PutUnsignedShort(1);

		// First frame
		buffer.PutUnsignedShort(0);

		// padding0 u8[4];
		for (int i = 0; i < 4; ++i) buffer.PutUnsignedChar(0);

		// reflectivity float[3] (aka vector)
		buffer.PutFloat(0.0f);
		buffer.PutFloat(0.0f);
		buffer.PutFloat(0.0f);

		// padding1 u8[4];
		for (int i = 0; i < 4; ++i) buffer.PutUnsignedChar(0);

		// bumpmapScale
#ifdef DXT_OUT
		buffer.PutFloat(0.0f);
#else
#ifdef VER_74
		buffer.PutFloat(1.0f);
#else
		buffer.PutFloat(0.0f);
#endif
#endif

		// highResImageFormat
#ifdef DXT_OUT
		buffer.PutInt(IMAGE_FORMAT_DXT1);
#else
		buffer.PutInt(IMAGE_FORMAT_RGBA8888);
#endif

		// mipmapCount
		buffer.PutUnsignedChar(4);

		// lowResImageFormat
		buffer.PutInt(IMAGE_FORMAT_DXT1);

		// lowResImageWidth + Height
		buffer.PutUnsignedChar(0);
		buffer.PutUnsignedChar(0);

#ifdef VER_71
		buffer.PutUnsignedChar(1); // ???
#endif

#ifdef VER_74
		// 7.2
		// depth
		buffer.PutUnsignedShort(1);


		// 7.3+
		// padding
		for (int i = 0; i < 3; ++i) buffer.PutUnsignedChar(0);

		// Number of resources
		buffer.PutUnsignedInt(1);

		// padding
		for (int i = 0; i < 8; ++i) buffer.PutUnsignedChar(0);
#endif
	}
	{
#ifdef VER_74
		// Resource infos
		/*
		struct ResourceEntryInfo
		{
			unsigned char	tag[3]; 		// A three-byte "tag" that identifies what this resource is.
			unsigned char	flags;			// Resource entry flags. The only known flag is 0x2, which indicates that no data chunk corresponds to this resource.
			unsigned int	offset;			// The offset of this resource's data in the file.
		};
		 */
		// tag
		buffer.PutUnsignedChar('\x30');
		buffer.PutUnsignedChar('\0');
		buffer.PutUnsignedChar('\0');

		// flag
		buffer.PutUnsignedChar(0);

		// offset
		buffer.PutUnsignedInt(88);
#endif
	}

	// Start of image data
	// Usually low res starts here but it's empty so skipped to high res
	// For each frame, there's only 1
	//     For each face, there's only 1
	//         For each Z slice (mipmaps), from 1 to 4 (smallest to highest res)
	int mipWidth = 32;
	int mipHeight = 32;
	uchar *mipData = reinterpret_cast<uchar *>(malloc(256 * 256 * 4));
	// In VTF, mip starts from the smallest resolution (32px for 4-MIP-256px) to the highest one (256px)
	for (int mip = 4; mip >= 1; --mip, mipWidth *= 2, mipHeight *= 2)
	{
		Msg("Mip map gen: %d (%d x %d)\n", mip, mipWidth, mipHeight);
		// Copy/downscale the image data to fit into the mipData
		if (mip == 1)
		{
			V_memcpy(mipData, data, 256 * 256 * 4);
		}
		else
		{
			V_memset(mipData, 0, 256 * 256 * 4);
			stbir_resize_uint8_linear(data, 256, 256, 0,
									  mipData, mipWidth, mipHeight, 0,
									  STBIR_4CHANNEL);//STBIR_RGBA);
		}

#ifndef DXT_OUT
		// RGBA8888 data
		buffer.Put(mipData, mipWidth * mipHeight * 4);
#else
		// Then do the DXT1 blocks
		const int mipStride = mipWidth * 4;
		for (int cy = 0; cy < mipHeight; cy += 4)
		{
			for (int cx = 0; cx < mipWidth; cx += 4)
			{
#if 1
				int inOffset = 0;
				unsigned char ucBufIn[64];
				for (int by = 0; by < 4; ++by)
				{
					const int y = cy + by;
					for (int bx = 0; bx < 4; ++bx)
					{
						const int x = cx + bx;
						V_memcpy(ucBufIn + inOffset,
								 mipData + ((y * mipStride) + (x * 4)),
								 4);
						inOffset += 4;
					}
				}
				Assert(inOffset == 64);

				unsigned char ucBufOut[8];
				stb_compress_dxt_block(ucBufOut, ucBufIn, 0, STB_DXT_HIGHQUAL);
				buffer.Put(ucBufOut, 8);
#else
				unsigned char ucBufIn[64];
				int inOffset = 0;

				unsigned char rgbMin[3] = {0xFF, 0xFF, 0xFF};
				unsigned char rgbMax[3] = {0x00, 0x00, 0x00};

				for (int by = 0; by < 4; ++by)
				{
					const int y = cy + by;
					for (int bx = 0; bx < 4; ++bx)
					{
						const int x = cx + bx;
						V_memcpy(ucBufIn + inOffset,
								 mipData + ((y * mipStride) + (x * 4)),
								 4);
						unsigned char *ucThisColor = ucBufIn + inOffset;
						unsigned char r = ucThisColor[0];
						unsigned char g = ucThisColor[1];
						unsigned char b = ucThisColor[2];
						if (r < rgbMin[0]) rgbMin[0] = r;
						if (g < rgbMin[1]) rgbMin[1] = g;
						if (b < rgbMin[2]) rgbMin[2] = b;
						if (r > rgbMax[0]) rgbMax[0] = r;
						if (g > rgbMax[1]) rgbMax[1] = g;
						if (b < rgbMax[2]) rgbMax[2] = b;

						inOffset += 4;
					}
				}
				Assert(inOffset == 64);

				// Write RGB565 color0 and color1
				static constexpr unsigned char INSET_SHIFT = 4;
				unsigned char inset[3];
				inset[0] = (rgbMax[0] - rgbMin[0]) >> INSET_SHIFT;
				inset[1] = (rgbMax[1] - rgbMin[1]) >> INSET_SHIFT;
				inset[2] = (rgbMax[2] - rgbMin[2]) >> INSET_SHIFT;
				rgbMin[0] = (rgbMin[0] + inset[0] <= 255) ? rgbMin[0] + inset[0] : 255;
				rgbMin[1] = (rgbMin[1] + inset[1] <= 255) ? rgbMin[1] + inset[1] : 255;
				rgbMin[2] = (rgbMin[2] + inset[2] <= 255) ? rgbMin[2] + inset[2] : 255;
				rgbMax[0] = (rgbMax[0] >= inset[0]) ? rgbMax[0] - inset[0] : 0;
				rgbMax[1] = (rgbMax[1] >= inset[1]) ? rgbMax[1] - inset[1] : 0;
				rgbMax[2] = (rgbMax[2] >= inset[2]) ? rgbMax[2] - inset[2] : 0;

				static auto rgb888_r = [](const unsigned int rgb888) -> unsigned char {
					return (rgb888 >> 16) & 0xFF;
				};
				static auto rgb888_g = [](const unsigned int rgb888) -> unsigned char {
					return (rgb888 >> 8) & 0xFF;
				};
				static auto rgb888_b = [](const unsigned int rgb888) -> unsigned char {
					return rgb888 & 0xFF;
				};

				static auto rgb888Interp = [](unsigned short f0, unsigned int c0, unsigned short f1, unsigned int c1) -> unsigned int {
					unsigned int r = (f0 * rgb888_r(c0) + f1 * rgb888_r(c1)) / (f0 + f1);
					unsigned int g = (f0 * rgb888_g(c0) + f1 * rgb888_g(c1)) / (f0 + f1);
					unsigned int b = (f0 * rgb888_b(c0) + f1 * rgb888_b(c1)) / (f0 + f1);
					return (r << 16) | (g << 8) | b;
				};

				unsigned char *rgb0 = rgbMax;
				unsigned char *rgb1 = rgbMin;
				unsigned int rgb888Color0 = (rgbMax[0] << 16) | (rgbMax[1] << 8) | rgbMax[2];
				unsigned int rgb888Color1 = (rgbMin[0] << 16) | (rgbMin[1] << 8) | rgbMin[2];
				unsigned int rgb888Color2 = rgb888Interp(2, rgb888Color0, 1, rgb888Color1);
				unsigned int rgb888Color3 = rgb888Interp(1, rgb888Color0, 2, rgb888Color1);

				static auto rgb888Dist = [](const unsigned int c1, const unsigned int c2) -> unsigned short {
					short r1 = rgb888_r(c1), g1 = rgb888_g(c1), b1 = rgb888_b(c1);
					short r2 = rgb888_r(c2), g2 = rgb888_g(c2), b2 = rgb888_b(c2);
					return abs(r1 - r2) + abs(g1 - g1) + abs(b1 - b2);
				};

				static auto rgb888Closests = [](const unsigned int c,
						const unsigned int c0, const unsigned int c1, const unsigned int c2, const unsigned int c3) -> unsigned char {
					unsigned short d0 = rgb888Dist(c, c0);
					unsigned short d1 = rgb888Dist(c, c1);
					unsigned short d2 = rgb888Dist(c, c2);
					unsigned short d3 = rgb888Dist(c, c3);
					int b0 = d0 > d3;
					int b1 = d1 > d2;
					int b2 = d0 > d2;
					int b3 = d1 > d3;
					int b4 = d2 > d3;
					int x0 = b1 & b2;
					int x1 = b0 & b3;
					int x2 = b0 & b4;
					return ((x0 | x1) << 1) | x2;
				};

				unsigned int bits = 0;
				for (int by = 0; by < 4; ++by)
				{
					const int y = cy + by;
					for (int bx = 0; bx < 4; ++bx)
					{
						const int x = cx + bx;
						unsigned char *rgbPix = mipData + ((y * mipStride) + (x * 4));
						unsigned int rgb888BitColor = (rgbPix[0] << 16) | (rgbPix[1] << 8) | rgbPix[2];
						unsigned int idx = rgb888Closests(rgb888BitColor,
														  rgb888Color0, rgb888Color1, rgb888Color2, rgb888Color3);
						bits = (idx << 30) | (bits >> 2);
					}
				}

				static auto rgb888to565 = [](const unsigned char *ucRgb888) -> unsigned short {
					return ((ucRgb888[0] >> 3) << 11) | ((ucRgb888[1] >> 2) << 5) | (ucRgb888[2] >> 3);
				};
				unsigned short rgb565Color0 = rgb888to565(rgb0);
				unsigned short rgb565Color1 = rgb888to565(rgb1);
				buffer.PutUnsignedShort(rgb565Color0);
				buffer.PutUnsignedShort(rgb565Color1);
				buffer.PutUnsignedInt(bits);
#endif
			}
		}
#endif
	}

	free(mipData);

#endif

#if 1
	CUtlBuffer vtfWBuffer;
	bool bWriteResult = false;
	IVTFTexture *pVTFTexture = CreateVTFTexture();
	const int nFlags = TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT | TEXTUREFLAGS_NOLOD;// | TEXTUREFLAGS_ONEBITALPHA;
	if (pVTFTexture->Init(256, 256, 1, IMAGE_FORMAT_RGBA8888, nFlags, 1, 4))
	{
		uint8 *pDstData = pVTFTexture->ImageData();
		V_memcpy(pDstData, data, width * height * 4);
		pVTFTexture->SetReflectivity(Vector{0.0f, 0.0f, 0.0f});
		pVTFTexture->InitLowResImage(0, 0, IMAGE_FORMAT_DXT1);
		pVTFTexture->GenerateMipmaps();
#if 1
		pVTFTexture->ConvertImageFormat(IMAGE_FORMAT_DXT1, false);
		pVTFTexture->PostProcess(false);
#endif

		// Allocate output buffer
		const int iFileSize = pVTFTexture->FileSize() * 2;
		void *pVTF = malloc(iFileSize);
		vtfWBuffer.SetExternalBuffer(pVTF, iFileSize, 0);

		// Serialize to the buffer
		bWriteResult = pVTFTexture->Serialize(vtfWBuffer);

		// Free the VTF texture
		DestroyVTFTexture(pVTFTexture);
	}
	else
	{
		bWriteResult = false;
	}

	//stbi_image_free(data);

	if (!bWriteResult)
	{
		return nullptr;
	}
#endif

	V_sprintf_safe(szFullFilePath, "%s.vtf", *szRetTexPath);
	if (!filesystem->WriteFile(szFullFilePath, nullptr, buffer))
	{
		// TODO ERROR
	}

	// TODO: Windows use back-slash
	char szStrBuffer[1024];
	{
		V_sprintf_safe(szStrBuffer, R"VMT(
LightmappedGeneric
{
	"$basetexture"	"vgui/logos/%s"
	"$translucent" "1"
	"$decal" "1"
	"$decalscale" "0.250"
}
)VMT", pszBaseName);

		CUtlBuffer bufVmt(0, 0, CUtlBuffer::TEXT_BUFFER);
		bufVmt.PutString(szStrBuffer);

		V_sprintf_safe(szFullFilePath, "%s.vmt", *szRetTexPath);
		if (!filesystem->WriteFile(szFullFilePath, nullptr, bufVmt))
		{
			// TODO ERROR
		}
	}

	V_memset(szStrBuffer, 0, sizeof(szStrBuffer));
	{
		V_sprintf_safe(szStrBuffer, R"VMT(
"UnlitGeneric"
{
	// Original shader: BaseTimesVertexColorAlphaBlendNoOverbright
	"$translucent" 1
	"$basetexture" "VGUI/logos/%s"
	"$vertexcolor" 1
	"$vertexalpha" 1
	"$no_fullbright" 1
	"$ignorez" 1
}
)VMT", pszBaseName);

		CUtlBuffer bufVmt(0, 0, CUtlBuffer::TEXT_BUFFER);
		bufVmt.PutString(szStrBuffer);

		V_sprintf_safe(szFullFilePath, "materials/vgui/logos/ui/%s.vmt", pszBaseName);
		if (!filesystem->WriteFile(szFullFilePath, nullptr, bufVmt))
		{
			// TODO ERROR
		}
	}

	return data;
}

}  // namespace NeoUI
