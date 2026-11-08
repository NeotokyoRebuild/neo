#include "neo_ui_fpsslider.h"

#include <vgui/ILocalize.h>
#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

#include <cwctype>

#include "neo_misc.h"
#include "neo_ui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace NeoUI
{

static const int FPSSLIDER_MIN = 30;
static const int FPSSLIDER_MAX = 400;
static const int FPSSLIDER_UNLIMITED = 0;
static const int FPSSLIDER_UNLIMITED_POS = FPSSLIDER_MAX + 1;
static const int FPSSLIDER_MAX_TYPED = 9999;
static const int FPSSLIDER_MAX_TYPED_DIGITS = 5;
static const wchar_t FPSSLIDER_UNLIMITED_TEXT[] = L"Unlimited";
static const wchar_t FPSSLIDER_INVALID_TEXT[] = L"Invalid Setting";

static inline int g_iFpsMaxBeforeTyping = FPSSLIDER_UNLIMITED;

static int FpsSliderPos(const int iFpsMax)
{
	if (iFpsMax == FPSSLIDER_UNLIMITED || iFpsMax > FPSSLIDER_MAX)
	{
		return FPSSLIDER_UNLIMITED_POS;
	}
	return iFpsMax;
}

static int FpsFromSliderPos(const int iPos)
{
	const int iClampedPos = clamp(iPos, FPSSLIDER_MIN, FPSSLIDER_UNLIMITED_POS);
	return (iClampedPos == FPSSLIDER_UNLIMITED_POS) ? FPSSLIDER_UNLIMITED : iClampedPos;
}

static bool FpsValid(const int iFpsMax)
{
	return iFpsMax == FPSSLIDER_UNLIMITED || IN_BETWEEN_EQ(FPSSLIDER_MIN, iFpsMax, FPSSLIDER_MAX_TYPED);
}

static int FpsFromText(const SliderInfo *pSInfo)
{
	char szText[ARRAYSIZE(pSInfo->wszText)];
	g_pVGuiLocalize->ConvertUnicodeToANSI(pSInfo->wszText, szText, sizeof(szText));
	return atoi(szText);
}

static void FpsApplyText(SliderInfo *pSInfo, int *iFpsMax)
{
	const int iFromText = FpsFromText(pSInfo);
	const bool bValid = (pSInfo->wszText[0] != L'\0' && FpsValid(iFromText));
	const int iNewFpsMax = bValid ? iFromText : g_iFpsMaxBeforeTyping;
	if (iNewFpsMax != *iFpsMax)
	{
		*iFpsMax = iNewFpsMax;
		pSInfo->flCachedValue = *iFpsMax;
		CurrentContext()->bValueEdited = true;
	}
}

void SliderFpsMax(const wchar_t *wszLeftLabel, int *iFpsMax)
{
	BeginMultiWidgetHighlighter(2);
	Label(wszLeftLabel);
	const auto wdgState = BeginWidget(WIDGETFLAG_MOUSE | WIDGETFLAG_MARKACTIVE);

	if (wdgState.bInView)
	{
		Context *pCtx = CurrentContext();
		auto hdl = pCtx->htSliders.Find(wszLeftLabel);
		if (hdl == -1 || pCtx->htSliders.Element(hdl).flCachedValue != *iFpsMax ||
				pCtx->htSliders.Element(hdl).bActive != wdgState.bActive)
		{
			const bool bLeftInvalid = (hdl != -1 && pCtx->htSliders.Element(hdl).bActive && !wdgState.bActive &&
					pCtx->htSliders.Element(hdl).wszText[0] != L'\0' &&
					!FpsValid(FpsFromText(&pCtx->htSliders.Element(hdl))));
			if (hdl == -1)
			{
				SliderInfo sInfoNew = {};
				hdl = pCtx->htSliders.Insert(wszLeftLabel, sInfoNew);
			}
			SliderInfo *pSInfo = &pCtx->htSliders.Element(hdl);

			if (bLeftInvalid || (!wdgState.bActive && !FpsValid(*iFpsMax)))
			{
				V_wcsncpy(pSInfo->wszText, FPSSLIDER_INVALID_TEXT, sizeof(pSInfo->wszText));
			}
			else if (*iFpsMax == FPSSLIDER_UNLIMITED && !wdgState.bActive)
			{
				V_wcsncpy(pSInfo->wszText, FPSSLIDER_UNLIMITED_TEXT, sizeof(pSInfo->wszText));
			}
			else
			{
				V_swprintf_safe(pSInfo->wszText, L"%d", *iFpsMax);
			}
			pSInfo->flCachedValue = *iFpsMax;
			pSInfo->iMaxStrSize = FPSSLIDER_MAX_TYPED_DIGITS;
			pSInfo->bActive = wdgState.bActive;
			if (wdgState.bActive)
			{
				g_iFpsMaxBeforeTyping = *iFpsMax;
			}
		}
		SliderInfo *pSInfo = &pCtx->htSliders.Element(hdl);
		const int iPos = FpsSliderPos(*iFpsMax);

		switch (pCtx->eMode)
		{
		case MODE_PAINT:
		{
			const auto *pFontI = &pCtx->fonts[pCtx->eFont];
			if (wdgState.bHot)
			{
				vgui::surface()->DrawFilledRectArray(&pCtx->rWidgetArea, 1);
			}

			vgui::surface()->DrawFilledRect(pCtx->rWidgetArea.x0, pCtx->rWidgetArea.y0,
											pCtx->rWidgetArea.x0 + pCtx->layout.iRowTall, pCtx->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(pCtx->rWidgetArea.x0 + pFontI->iStartBtnXPos,
											pCtx->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_LEFT, 1);

			vgui::surface()->DrawFilledRect(pCtx->rWidgetArea.x1 - pCtx->layout.iRowTall, pCtx->rWidgetArea.y0,
											pCtx->rWidgetArea.x1, pCtx->rWidgetArea.y1);
			vgui::surface()->DrawSetTextPos(pCtx->rWidgetArea.x1 - pCtx->layout.iRowTall + pFontI->iStartBtnXPos,
											pCtx->rWidgetArea.y0 + pFontI->iStartBtnYPos);
			vgui::surface()->DrawPrintText(WSZ_ARROW_RIGHT, 1);

			int iFontWide, iFontTall;
			vgui::surface()->GetTextSize(pFontI->hdl, pSInfo->wszText, iFontWide, iFontTall);
			const int iFontStartX = ((pCtx->irWidgetWide / 2) - (iFontWide / 2));
			const int iFontEndX = ((pCtx->irWidgetWide / 2) + (iFontWide / 2));
			const int iFontStartPadX = iFontStartX - pCtx->iMarginX;
			const int iFontEndPadX = iFontEndX + pCtx->iMarginX;

			const float flPerc = static_cast<float>(iPos - FPSSLIDER_MIN) / static_cast<float>(FPSSLIDER_UNLIMITED_POS - FPSSLIDER_MIN);
			const float flBarMaxWide = static_cast<float>(pCtx->irWidgetWide - (2 * pCtx->layout.iRowTall));
			const int iBarEndX = pCtx->layout.iRowTall + static_cast<int>(flPerc * flBarMaxWide);
			const int iBarMarginY = static_cast<int>(1.5f * pCtx->iMarginY);
			vgui::surface()->DrawSetColor(wdgState.bActive ? pCtx->colors.sliderActiveBg : wdgState.bHot ? pCtx->colors.sliderHotBg : pCtx->colors.sliderNormalBg);
			if (flPerc > 0.0f)
			{
				vgui::surface()->DrawFilledRect(pCtx->rWidgetArea.x0 + pCtx->layout.iRowTall,
												pCtx->rWidgetArea.y0 + iBarMarginY,
												pCtx->rWidgetArea.x0 + Min(iBarEndX, iFontStartPadX),
												pCtx->rWidgetArea.y1 - iBarMarginY);
			}
			if (iFontEndX < iBarEndX)
			{
				vgui::surface()->DrawFilledRect(pCtx->rWidgetArea.x0 + iFontEndPadX,
												pCtx->rWidgetArea.y0 + iBarMarginY,
												pCtx->rWidgetArea.x0 + iBarEndX,
												pCtx->rWidgetArea.y1 - iBarMarginY);
			}

			vgui::surface()->DrawSetTextPos(pCtx->rWidgetArea.x0 + iFontStartX,
											pCtx->rWidgetArea.y0 + pFontI->iYFontOffset);
			vgui::surface()->DrawPrintText(pSInfo->wszText, V_wcslen(pSInfo->wszText));

			if (wdgState.bActive)
			{
				const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);
				if (bEditBlinkShow)
				{
					const int iMarkX = iFontStartX + iFontWide;
					vgui::surface()->DrawSetColor(pCtx->colors.cursor);
					vgui::surface()->DrawFilledRect(pCtx->rWidgetArea.x0 + iMarkX,
													pCtx->rWidgetArea.y0 + pFontI->iYFontOffset,
													pCtx->rWidgetArea.x0 + iMarkX + pCtx->iMarginX,
													pCtx->rWidgetArea.y0 + pFontI->iYFontOffset + iFontTall);
				}
			}
		}
		break;
		case MODE_MOUSEPRESSED:
		case MODE_MOUSEDOUBLEPRESSED:
		{
			if (wdgState.bHot)
			{
				pCtx->iActive = pCtx->iWidget;
				pCtx->iActiveSection = pCtx->iSection;
				if (pCtx->eCode == MOUSE_LEFT)
				{
					if (IN_BETWEEN_EQ(pCtx->rWidgetArea.x0, pCtx->iMouseAbsX, pCtx->rWidgetArea.x0 + pCtx->layout.iRowTall))
					{
						*iFpsMax = FpsFromSliderPos(iPos - 1);
						pCtx->bValueEdited = true;
					}
					else if (IN_BETWEEN_EQ(pCtx->rWidgetArea.x1 - pCtx->layout.iRowTall, pCtx->iMouseAbsX, pCtx->rWidgetArea.x1))
					{
						*iFpsMax = FpsFromSliderPos(iPos + 1);
						pCtx->bValueEdited = true;
					}
					else
					{
						pCtx->eMousePressedStart = MOUSESTART_SLIDER;
					}
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (wdgState.bActive)
			{
				if (IsKeyLeft() || IsKeyRight())
				{
					*iFpsMax = FpsFromSliderPos(iPos + (IsKeyLeft() ? -1 : +1));
					pCtx->bValueEdited = true;
				}
				else if (IsKeyEnter())
				{
					pCtx->iActive = FOCUSOFF_NUM;
					pCtx->iActiveSection = -1;
				}
				else if (pCtx->eCode == KEY_BACKSPACE)
				{
					const int iTextSize = V_wcslen(pSInfo->wszText);
					if (iTextSize > 0)
					{
						pSInfo->wszText[iTextSize - 1] = L'\0';
						FpsApplyText(pSInfo, iFpsMax);
					}
				}
			}
			else if (wdgState.bHot)
			{
				if (IsKeyEnter())
				{
					pCtx->iActive = pCtx->iWidget;
					pCtx->iActiveSection = pCtx->iSection;
				}
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (wdgState.bActive && vgui::input()->IsMouseDown(MOUSE_LEFT) && pCtx->eMousePressedStart == MOUSESTART_SLIDER)
			{
				const int iMouseRelXWidget = pCtx->iMouseAbsX - pCtx->rWidgetArea.x0;
				const float flPerc = static_cast<float>(iMouseRelXWidget - pCtx->layout.iRowTall) / static_cast<float>(pCtx->irWidgetWide - (2 * pCtx->layout.iRowTall));
				*iFpsMax = FpsFromSliderPos(FPSSLIDER_MIN + RoundFloatToInt(flPerc * (FPSSLIDER_UNLIMITED_POS - FPSSLIDER_MIN)));
				pCtx->bValueEdited = true;
			}
		}
		break;
		case MODE_KEYTYPED:
		{
			if (wdgState.bActive)
			{
				const int iTextSize = V_wcslen(pSInfo->wszText);
				const wchar_t uc = pCtx->unichar;
				if (iTextSize < pSInfo->iMaxStrSize && iswdigit(uc))
				{
					pSInfo->wszText[iTextSize] = uc;
					pSInfo->wszText[iTextSize + 1] = L'\0';
					FpsApplyText(pSInfo, iFpsMax);
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

}
