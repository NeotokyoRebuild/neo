#include "neo_ui.h"

#include <vgui/ISurface.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

using namespace vgui;

static const wchar_t *ENABLED_LABELS[] = {
	L"Disabled",
	L"Enabled",
};

#define IN_BETWEEN(min, cmp, max) (((min) <= (cmp)) && ((cmp) < (max)))

[[nodiscard]] static int LoopAroundMinMax(const int iValue, const int iMin, const int iMax)
{
	if (iValue < iMin)
	{
		return iMax;
	}
	else if (iValue > iMax)
	{
		return iMin;
	}
	return iValue;
}

[[nodiscard]] static int LoopAroundInArray(const int iValue, const int iSize)
{
	return LoopAroundMinMax(iValue, 0, iSize - 1);
}

namespace NeoUI
{

static inline Context g_emptyCtx;
static Context *g_pCtx = &g_emptyCtx;

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

void BeginContext(NeoUI::Context *ctx, const NeoUI::Mode eMode)
{
	g_pCtx = ctx;
	g_pCtx->eMode = eMode;
	g_pCtx->iLayoutY = -(g_pCtx->iYOffset[0] * g_pCtx->iRowTall);
	g_pCtx->iWidget = 0;
	g_pCtx->iFontTall = surface()->GetFontTall(g_pCtx->font);
	g_pCtx->iFontYOffset = (g_pCtx->iRowTall / 2) - (g_pCtx->iFontTall / 2);
	g_pCtx->iWgXPos = static_cast<int>(g_pCtx->dPanel.wide * 0.4f);
	g_pCtx->iSection = 0;
	g_pCtx->iHasMouseInPanel = 0;
	g_pCtx->iHorizontalWidth = 0;
	g_pCtx->bValueEdited = false;
	g_pCtx->eButtonTextStyle = TEXTSTYLE_CENTER;
	g_pCtx->eLabelTextStyle = TEXTSTYLE_LEFT;

	switch (g_pCtx->eMode)
	{
	case MODE_KEYPRESSED:
		if (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP)
		{
			g_pCtx->iFocusDirection = (g_pCtx->eCode == KEY_UP) ? -1 : +1;
			g_pCtx->iFocus = (g_pCtx->iFocus == FOCUSOFF_NUM) ? 0 : (g_pCtx->iFocus + g_pCtx->iFocusDirection);
		}
		break;
	default:
		break;
	}
}

void EndContext()
{
	if (g_pCtx->eMode == MODE_MOUSEMOVED && !g_pCtx->iHasMouseInPanel)
	{
		g_pCtx->eMousePos = MOUSEPOS_NONE;
		g_pCtx->iFocusDirection = 0;
		g_pCtx->iFocus = FOCUSOFF_NUM;
		g_pCtx->iFocusSection = -1;
	}

	if (g_pCtx->eMode == MODE_KEYPRESSED && g_pCtx->iFocusSection == -1 && (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP))
	{
		g_pCtx->iFocusSection = 0;
	}
}

void BeginSection(const bool bDefaultFocus)
{
	g_pCtx->iPartitionY = 0;
	g_pCtx->iLayoutY = -(g_pCtx->iYOffset[g_pCtx->iSection] * g_pCtx->iRowTall);
	g_pCtx->iWidget = 0;

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
		g_pCtx->iFocusDirection = 0;
		g_pCtx->iFocus = g_pCtx->iMouseRelY / g_pCtx->iRowTall; // TODO: Maybe this should just be per widget?
		g_pCtx->iFocusSection = g_pCtx->iSection;
	}

	switch (g_pCtx->eMode)
	{
	case MODE_PAINT:
		surface()->DrawSetColor(g_pCtx->bgColor);
		surface()->DrawFilledRect(g_pCtx->dPanel.x,
								  g_pCtx->dPanel.y,
								  g_pCtx->dPanel.x + g_pCtx->dPanel.wide,
								  g_pCtx->dPanel.y + g_pCtx->dPanel.tall);

		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
		break;
	case MODE_KEYPRESSED:
		if (bDefaultFocus && g_pCtx->iFocusSection == -1 && (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP))
		{
			g_pCtx->iFocusSection = g_pCtx->iSection;
		}
		break;
	}
}

void EndSection()
{
	if (g_pCtx->eMode == MODE_MOUSEMOVED && g_pCtx->bMouseInPanel &&
			g_pCtx->iLayoutY < g_pCtx->iMouseRelY)
	{
		g_pCtx->eMousePos = MOUSEPOS_NONE;
		g_pCtx->iFocusDirection = 0;
		g_pCtx->iFocus = FOCUSOFF_NUM;
		g_pCtx->iFocusSection = -1;
	}
	if (g_pCtx->iFocus != FOCUSOFF_NUM && g_pCtx->iFocusSection == g_pCtx->iSection)
	{
		g_pCtx->iFocus = LoopAroundInArray(g_pCtx->iFocus, g_pCtx->iWidget);
	}

	// Scroll handling
	const int iRowsInScreen = g_pCtx->dPanel.tall / g_pCtx->iRowTall;
	if (g_pCtx->eMode == MODE_MOUSEWHEELED)
	{
		if (g_pCtx->iWidget <= iRowsInScreen)
		{
			g_pCtx->iYOffset[g_pCtx->iSection] = 0;
		}
		else
		{
			g_pCtx->iYOffset[g_pCtx->iSection] += (g_pCtx->eCode == MOUSE_WHEEL_UP) ? -1 : +1;
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iYOffset[g_pCtx->iSection], 0, g_pCtx->iWidget - iRowsInScreen);
		}
	}
	else if (g_pCtx->eMode == MODE_KEYPRESSED && (g_pCtx->eCode == KEY_DOWN || g_pCtx->eCode == KEY_UP))
	{
		if (g_pCtx->iWidget <= iRowsInScreen)
		{
			// Disable scroll if it doesn't need to
			g_pCtx->iYOffset[g_pCtx->iSection] = 0;
		}
		else if (g_pCtx->iFocus < (g_pCtx->iYOffset[g_pCtx->iSection]))
		{
			// Scrolling up past visible, re-adjust
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iFocus, 0, g_pCtx->iWidget - iRowsInScreen);
		}
		else if (g_pCtx->iFocus >= (g_pCtx->iYOffset[g_pCtx->iSection] + iRowsInScreen))
		{
			// Scrolling down post visible, re-adjust
			g_pCtx->iYOffset[g_pCtx->iSection] = clamp(g_pCtx->iFocus - iRowsInScreen + 1, 0, g_pCtx->iWidget - iRowsInScreen);
		}
	}

	++g_pCtx->iSection;
}

void BeginHorizontal(const int iHorizontalWidth)
{
	g_pCtx->iHorizontalWidth = iHorizontalWidth;
	g_pCtx->iLayoutX = 0;
	if (g_pCtx->eMode == MODE_MOUSEMOVED && g_pCtx->bMouseInPanel &&
			IN_BETWEEN(g_pCtx->iLayoutY, g_pCtx->iMouseRelY, g_pCtx->iLayoutY + g_pCtx->iRowTall))
	{
		g_pCtx->iFocusDirection = 0;
		g_pCtx->iFocus = FOCUSOFF_NUM;
		g_pCtx->iFocusSection = -1;
	}
}

void EndHorizontal()
{
	g_pCtx->iHorizontalWidth = 0;
	g_pCtx->iLayoutX = 0;
	++g_pCtx->iPartitionY;
	g_pCtx->iLayoutY += g_pCtx->iRowTall;
}

static void InternalLabel(const wchar_t *wszText, const bool bCenter)
{
	int iFontTextWidth = 0, iFontTextHeight = 0;
	if (bCenter)
	{
		surface()->GetTextSize(g_pCtx->font, wszText, iFontTextWidth, iFontTextHeight);
	}
	GCtxDrawSetTextPos(((bCenter) ? ((g_pCtx->dPanel.wide / 2) - (iFontTextWidth / 2)) : g_pCtx->iMarginX),
					   g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
	surface()->DrawPrintText(wszText, V_wcslen(wszText));
}

struct GetMouseinFocusedRet
{
	bool bMouseIn;
	bool bFocused;
};

static GetMouseinFocusedRet InternalGetMouseinFocused()
{
	bool bMouseIn = (g_pCtx->bMouseInPanel && ((g_pCtx->iYOffset[g_pCtx->iSection] + (g_pCtx->iMouseRelY / g_pCtx->iRowTall)) == g_pCtx->iPartitionY));
	if (bMouseIn && g_pCtx->iHorizontalWidth)
	{
		bMouseIn = IN_BETWEEN(g_pCtx->iLayoutX, g_pCtx->iMouseRelX, g_pCtx->iLayoutX + g_pCtx->iHorizontalWidth);
	}
	const bool bFocused = g_pCtx->iWidget == g_pCtx->iFocus && g_pCtx->iSection == g_pCtx->iFocusSection;
	if (bFocused)
	{
		surface()->DrawSetColor(COLOR_NEOPANELSELECTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	}
	return GetMouseinFocusedRet{
		.bMouseIn = bMouseIn,
		.bFocused = bFocused,
	};
}

static void InternalUpdatePartitionState(const bool bMouseIn, const bool bFocused)
{
	NeoUI::Pad();
	++g_pCtx->iWidget;
	if (bMouseIn || bFocused)
	{
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
	}
}

void Pad()
{
	if (g_pCtx->iHorizontalWidth)
	{
		g_pCtx->iLayoutX += g_pCtx->iHorizontalWidth;
	}
	else
	{
		++g_pCtx->iPartitionY;
		g_pCtx->iLayoutY += g_pCtx->iRowTall;
	}
}

void Label(const wchar_t *wszText)
{
	if (g_pCtx->iWidget == g_pCtx->iFocus && g_pCtx->iSection == g_pCtx->iFocusSection) g_pCtx->iFocus += g_pCtx->iFocusDirection;
	if (IN_BETWEEN(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		InternalLabel(wszText, g_pCtx->eLabelTextStyle == TEXTSTYLE_CENTER);
	}
	InternalUpdatePartitionState(true, true);
}

NeoUI::RetButton Button(const wchar_t *wszText)
{
	return Button(nullptr, wszText);
}

NeoUI::RetButton Button(const wchar_t *wszLeftLabel, const wchar_t *wszText)
{
	RetButton ret = {};
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();
	ret.bMouseHover = bMouseIn && (!wszLeftLabel || (wszLeftLabel && g_pCtx->eMousePos != MOUSEPOS_NONE));

	if (IN_BETWEEN(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		const int iBtnWidth = g_pCtx->iHorizontalWidth ? g_pCtx->iHorizontalWidth : g_pCtx->dPanel.wide;
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_pCtx->font, wszText, iFontWide, iFontTall);

			if (wszLeftLabel)
			{
				InternalLabel(wszLeftLabel, false);
				GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->dPanel.wide);
				const int xMargin = g_pCtx->eButtonTextStyle == TEXTSTYLE_CENTER ?
							(((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2)) : g_pCtx->iMarginX;
				GCtxDrawSetTextPos(g_pCtx->iWgXPos + xMargin,
								   g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
			}
			else
			{
				// No label, fill the whole partition
				GCtxDrawFilledRectXtoX(0, iBtnWidth);
				const int xMargin = g_pCtx->eButtonTextStyle == TEXTSTYLE_CENTER ?
							((iBtnWidth / 2) - (iFontWide / 2)) : g_pCtx->iMarginX;
				GCtxDrawSetTextPos(g_pCtx->iLayoutX + xMargin, g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
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
			ret.bKeyPressed = ret.bPressed = (bFocused && g_pCtx->eCode == KEY_ENTER);
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (bMouseIn)
			{
				g_pCtx->iFocus = g_pCtx->iWidget;
				g_pCtx->iFocusSection = g_pCtx->iSection;
				g_pCtx->iFocusDirection = 0;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
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
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();

	if (IN_BETWEEN(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			InternalLabel(wszLeftLabel, false);

			// Center-text label
			const wchar_t *wszText = wszLabelsList[*iIndex];
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_pCtx->font, wszText, iFontWide, iFontTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + (((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2)),
							   g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));

			// TODO: Generate once?
			int iStartBtnXPos, iStartBtnYPos;
			{
				int iPrevNextWide, iPrevNextTall;
				surface()->GetTextSize(g_pCtx->font, L"<", iPrevNextWide, iPrevNextTall);
				iStartBtnXPos = (g_pCtx->iRowTall / 2) - (iPrevNextWide / 2);
				iStartBtnYPos = (g_pCtx->iRowTall / 2) - (iPrevNextTall / 2);
			}

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->iWgXPos + g_pCtx->iRowTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + iStartBtnXPos, g_pCtx->iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_pCtx->dPanel.wide - g_pCtx->iRowTall, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->dPanel.wide - g_pCtx->iRowTall + iStartBtnXPos, g_pCtx->iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (bMouseIn && g_pCtx->eCode == MOUSE_LEFT && (g_pCtx->eMousePos == MOUSEPOS_LEFT || g_pCtx->eMousePos == MOUSEPOS_RIGHT))
			{
				*iIndex += (g_pCtx->eMousePos == MOUSEPOS_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (bFocused && (g_pCtx->eCode == KEY_LEFT || g_pCtx->eCode == KEY_RIGHT))
			{
				*iIndex += (g_pCtx->eCode == KEY_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (bMouseIn)
			{
				g_pCtx->iFocus = g_pCtx->iWidget;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

void Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	// This is basically a ringbox but different UI
	if (g_pCtx->iWidget == g_pCtx->iFocus && g_pCtx->iSection == g_pCtx->iFocusSection) g_pCtx->iFocus += g_pCtx->iFocusDirection;
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();
	const int iTabWide = (g_pCtx->dPanel.wide / iLabelsSize);

	switch (g_pCtx->eMode)
	{
	case MODE_PAINT:
	{
		for (int i = 0, iXPosTab = 0; i < iLabelsSize; ++i, iXPosTab += iTabWide)
		{
			const bool bHoverTab = (bMouseIn && IN_BETWEEN(iXPosTab, g_pCtx->iMouseRelX, iXPosTab + iTabWide));
			if (bHoverTab || i == *iIndex)
			{
				surface()->DrawSetColor(bHoverTab ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
				GCtxDrawFilledRectXtoX(iXPosTab, iXPosTab + iTabWide);
			}
			const wchar_t *wszText = wszLabelsList[i];
			GCtxDrawSetTextPos(iXPosTab + g_pCtx->iMarginX, g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}
	}
	break;
	case MODE_MOUSEPRESSED:
	{
		if (bMouseIn && g_pCtx->eCode == MOUSE_LEFT)
		{
			const int iNextIndex = (g_pCtx->iMouseRelX / iTabWide);
			if (iNextIndex != *iIndex)
			{
				*iIndex = clamp(iNextIndex, 0, iLabelsSize);
				g_pCtx->iYOffset[g_pCtx->iSection] = 0;
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
			g_pCtx->iYOffset[g_pCtx->iSection] = 0;
		}
	}
	break;
	default:
		break;
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

#if 0	// NEO TODO (nullsystem): Implement back the slider text edit code
	if (sl->iWszCacheLabelSize < sl->iChMax)
	{
		// Prevent minus usage if minimum is a positive
		// Prevent dot usage if flMulti = 1.0f, aka integer direct
		const bool bHasMinus = (sl->iValMin >= 0) || sl->wszCacheLabel[0] == L'-';
		const bool bHasDot = (sl->flMulti == 1.0f) || wmemchr(sl->wszCacheLabel, L'.', sl->iWszCacheLabelSize) != nullptr;
		if (iswdigit(unichar) || (!bHasDot && unichar == L'.') || (!bHasMinus && unichar == L'-'))
		{
			sl->wszCacheLabel[sl->iWszCacheLabelSize++] = unichar;
			sl->wszCacheLabel[sl->iWszCacheLabelSize] = '\0';
			char szCacheLabel[CNeoDataSlider::LABEL_MAX + 1] = {};
			g_pVGuiLocalize->ConvertUnicodeToANSI(sl->wszCacheLabel, szCacheLabel, sizeof(szCacheLabel));
			if (const float flVal = atof(szCacheLabel))
			{
				const int iVal = static_cast<int>(flVal * sl->flMulti);
				if (iVal >= sl->iValMin && iVal <= sl->iValMax)
				{
					sl->iValCur = iVal;
					m_bModified = true;
				}
			}
		}
#endif

void Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp, const float flStep)
{
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();

	if (IN_BETWEEN(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			InternalLabel(wszLeftLabel, false);

			wchar_t wszFormat[32];
			V_swprintf_safe(wszFormat, L"%%0.%df", iDp);

			// Background bar
			const float flPerc = (((iDp == 0) ? (int)(*flValue) : *flValue) - flMin) / (flMax - flMin);
			const float flBarMaxWide = static_cast<float>(g_pCtx->dPanel.wide - g_pCtx->iWgXPos);
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->iWgXPos + static_cast<int>(flPerc * flBarMaxWide));

			// Center-text label
			wchar_t wszText[32];
			const int iTextSize = V_swprintf_safe(wszText, wszFormat, *flValue);
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_pCtx->font, wszText, iFontWide, iFontTall);
			surface()->DrawSetTextPos(g_pCtx->dPanel.x + g_pCtx->iWgXPos + (((g_pCtx->dPanel.wide - g_pCtx->iWgXPos) / 2) - (iFontWide / 2)),
									  g_pCtx->dPanel.y + g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
			surface()->DrawPrintText(wszText, iTextSize);

			// TODO: Generate once?
			int iStartBtnXPos, iStartBtnYPos;
			{
				int iPrevNextWide, iPrevNextTall;
				surface()->GetTextSize(g_pCtx->font, L"<", iPrevNextWide, iPrevNextTall);
				iStartBtnXPos = (g_pCtx->iRowTall / 2) - (iPrevNextWide / 2);
				iStartBtnYPos = (g_pCtx->iRowTall / 2) - (iPrevNextTall / 2);
			}

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->iWgXPos + g_pCtx->iRowTall);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + iStartBtnXPos, g_pCtx->iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_pCtx->dPanel.wide - g_pCtx->iRowTall, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->dPanel.wide - g_pCtx->iRowTall + iStartBtnXPos, g_pCtx->iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (bMouseIn && g_pCtx->eCode == MOUSE_LEFT)
			{
				if (g_pCtx->eMousePos == MOUSEPOS_LEFT || g_pCtx->eMousePos == MOUSEPOS_RIGHT)
				{
					*flValue += (g_pCtx->eMousePos == MOUSEPOS_LEFT) ? -flStep : +flStep;
					*flValue = clamp(*flValue, flMin, flMax);
					g_pCtx->bValueEdited = true;
				}
				else if (g_pCtx->eMousePos == MOUSEPOS_CENTER)
				{
					g_pCtx->iFocusDirection = 0;
					g_pCtx->iFocus = g_pCtx->iWidget;
					g_pCtx->iFocusSection = g_pCtx->iSection;
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (bFocused && (g_pCtx->eCode == KEY_LEFT || g_pCtx->eCode == KEY_RIGHT))
			{
				*flValue += (g_pCtx->eCode == KEY_LEFT) ? -flStep : +flStep;
				*flValue = clamp(*flValue, flMin, flMax);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (bMouseIn)
			{
				g_pCtx->iFocus = g_pCtx->iWidget;
			}
			if (bMouseIn && bFocused && g_pCtx->eMousePos == MOUSEPOS_CENTER && input()->IsMouseDown(MOUSE_LEFT))
			{
				const int iBase = g_pCtx->iRowTall + g_pCtx->iWgXPos;
				const float flPerc = static_cast<float>(g_pCtx->iMouseRelX - iBase) / static_cast<float>(g_pCtx->dPanel.wide - g_pCtx->iRowTall - iBase);
				*flValue = flMin + (flPerc * (flMax - flMin));
				*flValue = clamp(*flValue, flMin, flMax);
				g_pCtx->bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

void SliderInt(const wchar_t *wszLeftLabel, int *iValue, const int iMin, const int iMax, const int iStep)
{
	float flValue = *iValue;
	Slider(wszLeftLabel, &flValue, static_cast<float>(iMin), static_cast<float>(iMax), 0, static_cast<float>(iStep));
	*iValue = static_cast<int>(flValue);
}

void TextEdit(const wchar_t *wszLeftLabel, wchar_t *wszText, const int iMaxSize)
{
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();

	if (IN_BETWEEN(0, g_pCtx->iLayoutY, g_pCtx->dPanel.tall))
	{
		const int iBtnWidth = g_pCtx->iHorizontalWidth ? g_pCtx->iHorizontalWidth : g_pCtx->dPanel.wide;
		switch (g_pCtx->eMode)
		{
		case MODE_PAINT:
		{
			InternalLabel(wszLeftLabel, false);
			GCtxDrawFilledRectXtoX(g_pCtx->iWgXPos, g_pCtx->dPanel.wide);
			GCtxDrawSetTextPos(g_pCtx->iWgXPos + g_pCtx->iMarginX, g_pCtx->iLayoutY + g_pCtx->iFontYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
			if (bFocused)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					int iFontWide, iFontTall;
					surface()->GetTextSize(g_pCtx->font, wszText, iFontWide, iFontTall);
					const int iMarkX = g_pCtx->iWgXPos + g_pCtx->iMarginX + iFontWide;
					surface()->DrawSetColor(COLOR_NEOPANELTEXTBRIGHT);
					GCtxDrawFilledRectXtoX(iMarkX, g_pCtx->iFontYOffset,
										   iMarkX + g_pCtx->iMarginX, g_pCtx->iFontYOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEDOUBLEPRESSED:
		{
			g_pCtx->iFocus = g_pCtx->iWidget;
			g_pCtx->iFocusSection = g_pCtx->iSection;
			g_pCtx->iFocusDirection = 0;
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (bFocused && g_pCtx->eCode == KEY_BACKSPACE)
			{
				int iTextSize = V_wcslen(wszText);
				if (iTextSize > 0)
				{
					wszText[--iTextSize] = '\0';
					g_pCtx->bValueEdited = true;
				}
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (bFocused)
			{
				int iTextSize = V_wcslen(wszText);
				if (iTextSize < iMaxSize)
				{
					wszText[iTextSize++] = g_pCtx->unichar;
					wszText[iTextSize] = '\0';
					g_pCtx->bValueEdited = true;
				}
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (bMouseIn)
			{
				g_pCtx->iFocus = g_pCtx->iWidget;
				g_pCtx->iFocusSection = g_pCtx->iSection;
				g_pCtx->iFocusDirection = 0;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

}
