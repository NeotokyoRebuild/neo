#include "cbase.h"
#include "neo_root.h"
#include "IOverrideInterface.h"

#include "vgui/ILocalize.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "ienginevgui.h"
#include <engine/IEngineSound.h>
#include "filesystem.h"
#include "neo_version_info.h"
#include "cdll_client_int.h"
#include <steam/steam_api.h>
#include <vgui_avatarimage.h>
#include <IGameUIFuncs.h>
#include "inputsystem/iinputsystem.h"
#include "characterset.h"
#include "materialsystem/materialsystem_config.h"
#include "neo_player_shared.h"
#include <ivoicetweak.h>

#include <vgui/IInput.h>
#include <vgui_controls/Controls.h>

extern ConVar neo_name;
extern ConVar cl_onlysteamnick;
extern ConVar sensitivity;
extern ConVar snd_victory_volume;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// TODO: Gamepad
//   Gamepad enable: joystick 0/1
//   Reverse up-down axis: joy_inverty 0/1
//   Swap sticks on dual-stick controllers: joy_movement_stick 0/1
//   Horizontal sens: joy_yawsensitivity: -0.5 to -7.0
//   Vertical sens: joy_pitchsensitivity: 0.5 to 7.0

using namespace vgui;

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule
// the dll and that we don't call Sys_LoadModule over and over again.
static CDllDemandLoader g_GameUIDLL( "GameUI" );

CNeoRoot *g_pNeoRoot = nullptr;

namespace {

int g_iRowsInScreen = 15;
int g_iRowTall = 40;
int g_iMarginX = 10;
int g_iMarginY = 10;
int g_iAvatar = 64;
int g_iRootSubPanelWide = 600;
int g_iGSIX[GSIW__TOTAL] = {};
HFont g_neoFont;
#define COLOR_NEOPANELNORMALBG Color(40, 40, 40, 255)
#define COLOR_NEOPANELSELECTBG Color(40, 10, 10, 255)
#define COLOR_NEOPANELACCENTBG Color(0, 0, 0, 255)
#define COLOR_NEOPANELTEXTNORMAL Color(200, 200, 200, 255)
#define COLOR_NEOPANELTEXTBRIGHT Color(255, 255, 255, 255)
#define COLOR_NEOPANELPOPUPBG Color(0, 0, 0, 170)
#define COLOR_NEOPANELFRAMEBG Color(40, 40, 40, 150)
#define COLOR_NEOTITLE Color(128, 128, 128, 255)
#define COLOR_NEOPANELBAR Color(20, 20, 20, 255)
#define COLOR_NEOPANELMICTEST Color(30, 90, 30, 255)
static constexpr wchar_t WSZ_GAME_TITLE[] = L"neatbkyoc ue";

[[nodiscard]] int LoopAroundMinMax(const int iValue, const int iMin, const int iMax)
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

[[nodiscard]] int LoopAroundInArray(const int iValue, const int iSize)
{
	return LoopAroundMinMax(iValue, 0, iSize - 1);
}

const wchar_t *QUALITY_LABELS[] = {
	L"Low",
	L"Medium",
	L"High",
	L"Very High",
};

enum QualityEnum
{
	QUALITY_LOW = 0,
	QUALITY_MEDIUM,
	QUALITY_HIGH,
	QUALITY_VERYHIGH,
};

const wchar_t *ENABLED_LABELS[] = {
	L"Disabled",
	L"Enabled",
};

}

void OverrideGameUI()
{
	if (!OverrideUI->GetPanel())
	{
		OverrideUI->Create(0U);
	}

	if (g_pNeoRoot->GetGameUI())
	{
		g_pNeoRoot->GetGameUI()->SetMainMenuOverride(g_pNeoRoot->GetVPanel());
	}
}

static NeoUI::Context g_ctx;

// This is only because it'll be an eye-sore and repetitve with those g_ctx.dPanel, iLayout, g_iRowTall...
static void GCtxDrawFilledRectXtoX(const int x1, const int x2)
{
	surface()->DrawFilledRect(g_ctx.dPanel.x + g_ctx.iLayoutX + x1,
							  g_ctx.dPanel.y + g_ctx.iLayoutY,
							  g_ctx.dPanel.x + g_ctx.iLayoutX + x2,
							  g_ctx.dPanel.y + g_ctx.iLayoutY + g_iRowTall);
}
static void GCtxDrawSetTextPos(const int x, const int y)
{
	surface()->DrawSetTextPos(g_ctx.dPanel.x + x, g_ctx.dPanel.y + y);
}

void NeoUI::BeginContext(const NeoUI::Mode eMode)
{
	g_ctx.eMode = eMode;
	g_ctx.iLayoutY = -(g_ctx.iYOffset[0] * g_iRowTall);
	g_ctx.iWidget = 0;
	g_ctx.iFontTall = surface()->GetFontTall(g_neoFont);
	g_ctx.iFontYOffset = (g_iRowTall / 2) - (g_ctx.iFontTall / 2);
	g_ctx.iWgXPos = static_cast<int>(g_ctx.dPanel.wide * 0.4f);
	g_ctx.iSection = 0;
	g_ctx.iHasMouseInPanel = 0;
	g_ctx.iHorizontalWidth = 0;
	g_ctx.bValueEdited = false;

	switch (g_ctx.eMode)
	{
	case MODE_KEYPRESSED:
		if (g_ctx.eCode == KEY_DOWN || g_ctx.eCode == KEY_UP)
		{
			g_ctx.iFocusDirection = (g_ctx.eCode == KEY_UP) ? -1 : +1;
			g_ctx.iFocus = (g_ctx.iFocus == FOCUSOFF_NUM) ? 0 : (g_ctx.iFocus + g_ctx.iFocusDirection);
		}
		break;
	default:
		break;
	}
}

void NeoUI::EndContext()
{
	if (g_ctx.eMode == MODE_MOUSEMOVED && !g_ctx.iHasMouseInPanel)
	{
		g_ctx.eMousePos = MOUSEPOS_NONE;
		g_ctx.iFocusDirection = 0;
		g_ctx.iFocus = FOCUSOFF_NUM;
		g_ctx.iFocusSection = -1;
	}

	if (g_ctx.eMode == MODE_KEYPRESSED && g_ctx.iFocusSection == -1 && (g_ctx.eCode == KEY_DOWN || g_ctx.eCode == KEY_UP))
	{
		g_ctx.iFocusSection = 0;
	}
}

void NeoUI::BeginSection(const bool bDefaultFocus)
{
	g_ctx.iPartitionY = 0;
	g_ctx.iLayoutY = -(g_ctx.iYOffset[g_ctx.iSection] * g_iRowTall);
	g_ctx.iWidget = 0;

	g_ctx.iMouseRelX = g_ctx.iMouseAbsX - g_ctx.dPanel.x;
	g_ctx.iMouseRelY = g_ctx.iMouseAbsY - g_ctx.dPanel.y;
	g_ctx.bMouseInPanel = (0 <= g_ctx.iMouseRelX && g_ctx.iMouseRelX < g_ctx.dPanel.wide &&
						   0 <= g_ctx.iMouseRelY && g_ctx.iMouseRelY < g_ctx.dPanel.tall);

	g_ctx.iHasMouseInPanel += g_ctx.bMouseInPanel;
	if (g_ctx.bMouseInPanel)
	{
		// g_iRowTall used to shape the square buttons
		if (g_ctx.iMouseRelX < g_ctx.iWgXPos)
		{
			g_ctx.eMousePos = NeoUI::MOUSEPOS_NONE;
		}
		else if (g_ctx.iMouseRelX < (g_ctx.iWgXPos + g_iRowTall))
		{
			g_ctx.eMousePos = NeoUI::MOUSEPOS_LEFT;
		}
		else if (g_ctx.iMouseRelX > (g_ctx.dPanel.wide - g_iRowTall))
		{
			g_ctx.eMousePos = NeoUI::MOUSEPOS_RIGHT;
		}
		else
		{
			g_ctx.eMousePos = NeoUI::MOUSEPOS_CENTER;
		}
		g_ctx.iFocusDirection = 0;
		g_ctx.iFocus = g_ctx.iMouseRelY / g_iRowTall;
		g_ctx.iFocusSection = g_ctx.iSection;
	}

	switch (g_ctx.eMode)
	{
	case MODE_PAINT:
		surface()->DrawSetColor(g_ctx.bgColor);
		surface()->DrawFilledRect(g_ctx.dPanel.x,
								  g_ctx.dPanel.y,
								  g_ctx.dPanel.x + g_ctx.dPanel.wide,
								  g_ctx.dPanel.y + g_ctx.dPanel.tall);

		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
		break;
	case MODE_KEYPRESSED:
		if (bDefaultFocus && g_ctx.iFocusSection == -1 && (g_ctx.eCode == KEY_DOWN || g_ctx.eCode == KEY_UP))
		{
			g_ctx.iFocusSection = g_ctx.iSection;
		}
		break;
	}
}

void NeoUI::EndSection()
{
	if (g_ctx.iFocus != FOCUSOFF_NUM && g_ctx.iFocusSection == g_ctx.iSection) g_ctx.iFocus = LoopAroundInArray(g_ctx.iFocus, g_ctx.iWidget);

	if (g_ctx.eMode == MODE_MOUSEWHEELED)
	{
		if (g_ctx.iWidget <= g_iRowsInScreen)
		{
			g_ctx.iYOffset[g_ctx.iSection] = 0;
		}
		else
		{
			g_ctx.iYOffset[g_ctx.iSection] += (g_ctx.eCode == MOUSE_WHEEL_UP) ? -1 : +1;
			g_ctx.iYOffset[g_ctx.iSection] = clamp(g_ctx.iYOffset[g_ctx.iSection], 0, g_ctx.iWidget - g_iRowsInScreen);
		}
	}

	++g_ctx.iSection;
}

void NeoUI::BeginHorizontal(const int iHorizontalWidth)
{
	g_ctx.iHorizontalWidth = iHorizontalWidth;
	g_ctx.iLayoutX = 0;
}

void NeoUI::EndHorizontal()
{
	g_ctx.iHorizontalWidth = 0;
	g_ctx.iLayoutX = 0;
	++g_ctx.iPartitionY;
	g_ctx.iLayoutY += g_iRowTall;
}

static void InternalLabel(const wchar_t *wszText, const bool bCenter)
{
	int iFontTextWidth = 0, iFontTextHeight = 0;
	if (bCenter)
	{
		surface()->GetTextSize(g_neoFont, wszText, iFontTextWidth, iFontTextHeight);
	}
	GCtxDrawSetTextPos(((bCenter) ? ((g_ctx.dPanel.wide / 2) - (iFontTextWidth / 2)) : g_iMarginX),
					   g_ctx.iLayoutY + g_ctx.iFontYOffset);
	surface()->DrawPrintText(wszText, V_wcslen(wszText));
}

struct GetMouseinFocusedRet
{
	bool bMouseIn;
	bool bFocused;
};

static GetMouseinFocusedRet InternalGetMouseinFocused()
{
	bool bMouseIn = (g_ctx.bMouseInPanel && ((g_ctx.iMouseRelY / g_iRowTall) == g_ctx.iPartitionY));
	if (bMouseIn && g_ctx.iHorizontalWidth)
	{
		bMouseIn = IN_BETWEEN(g_ctx.iLayoutX, g_ctx.iMouseRelX, g_ctx.iLayoutX + g_ctx.iHorizontalWidth);
	}
	const bool bFocused = g_ctx.iWidget == g_ctx.iFocus && g_ctx.iSection == g_ctx.iFocusSection;
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
	++g_ctx.iWidget;
	if (bMouseIn || bFocused)
	{
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);
	}
}

static void GCtxOnCursorMoved(int iAbsX, int iAbsY)
{
	g_ctx.iMouseAbsX = iAbsX;
	g_ctx.iMouseAbsY = iAbsY;
}

void NeoUI::Pad()
{
	if (g_ctx.iHorizontalWidth)
	{
		g_ctx.iLayoutX += g_ctx.iHorizontalWidth;
	}
	else
	{
		++g_ctx.iPartitionY;
		g_ctx.iLayoutY += g_iRowTall;
	}
}

void NeoUI::Label(const wchar_t *wszText, const bool bCenter)
{
	if (g_ctx.iWidget == g_ctx.iFocus && g_ctx.iSection == g_ctx.iFocusSection) g_ctx.iFocus += g_ctx.iFocusDirection;
	if (IN_BETWEEN(0, g_ctx.iLayoutY, g_ctx.dPanel.tall))
	{
		InternalLabel(wszText, bCenter);
	}
	InternalUpdatePartitionState(true, true);
}

NeoUI::RetButton NeoUI::Button(const wchar_t *wszLeftLabel, const wchar_t *wszText)
{
	RetButton ret = {};
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();
	ret.bMouseHover = bMouseIn && (!wszLeftLabel || (wszLeftLabel && g_ctx.eMousePos != MOUSEPOS_NONE));

	if (IN_BETWEEN(0, g_ctx.iLayoutY, g_ctx.dPanel.tall))
	{
		const int iBtnWidth = g_ctx.iHorizontalWidth ? g_ctx.iHorizontalWidth : g_ctx.dPanel.wide;
		switch (g_ctx.eMode)
		{
		case MODE_PAINT:
		{
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_neoFont, wszText, iFontWide, iFontTall);

			if (wszLeftLabel)
			{
				InternalLabel(wszLeftLabel, false);
				GCtxDrawFilledRectXtoX(g_ctx.iWgXPos, g_ctx.dPanel.wide);
				GCtxDrawSetTextPos(g_ctx.iWgXPos + (((g_ctx.dPanel.wide - g_ctx.iWgXPos) / 2) - (iFontWide / 2)),
								   g_ctx.iLayoutY + g_ctx.iFontYOffset);
			}
			else
			{
				// No label, fill the whole partition
				GCtxDrawFilledRectXtoX(0, iBtnWidth);
				GCtxDrawSetTextPos(g_ctx.iLayoutX + ((iBtnWidth / 2) - (iFontWide / 2)), g_ctx.iLayoutY + g_ctx.iFontYOffset);
			}
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			ret.bMousePressed = ret.bPressed = (ret.bMouseHover && g_ctx.eCode == MOUSE_LEFT);
		}
		break;
		case MODE_KEYPRESSED:
		{
			ret.bKeyPressed = ret.bPressed = (bFocused && g_ctx.eCode == KEY_ENTER);
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
	return ret;
}

void NeoUI::RingBoxBool(const wchar_t *wszLeftLabel, bool *bChecked)
{
	int iIndex = static_cast<int>(*bChecked);
	RingBox(wszLeftLabel, ENABLED_LABELS, 2, &iIndex);
	*bChecked = static_cast<bool>(iIndex);
}

void NeoUI::RingBox(const wchar_t *wszLeftLabel, const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();

	if (IN_BETWEEN(0, g_ctx.iLayoutY, g_ctx.dPanel.tall))
	{
		switch (g_ctx.eMode)
		{
		case MODE_PAINT:
		{
			InternalLabel(wszLeftLabel, false);

			// Center-text label
			const wchar_t *wszText = wszLabelsList[*iIndex];
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_neoFont, wszText, iFontWide, iFontTall);
			GCtxDrawSetTextPos(g_ctx.iWgXPos + (((g_ctx.dPanel.wide - g_ctx.iWgXPos) / 2) - (iFontWide / 2)),
							   g_ctx.iLayoutY + g_ctx.iFontYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));

			// TODO: Generate once?
			int iStartBtnXPos, iStartBtnYPos;
			{
				int iPrevNextWide, iPrevNextTall;
				surface()->GetTextSize(g_neoFont, L"<", iPrevNextWide, iPrevNextTall);
				iStartBtnXPos = (g_iRowTall / 2) - (iPrevNextWide / 2);
				iStartBtnYPos = (g_iRowTall / 2) - (iPrevNextTall / 2);
			}

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_ctx.iWgXPos, g_ctx.iWgXPos + g_iRowTall);
			GCtxDrawSetTextPos(g_ctx.iWgXPos + iStartBtnXPos, g_ctx.iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_ctx.dPanel.wide - g_iRowTall, g_ctx.dPanel.wide);
			GCtxDrawSetTextPos(g_ctx.dPanel.wide - g_iRowTall + iStartBtnXPos, g_ctx.iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (bMouseIn && g_ctx.eCode == MOUSE_LEFT && (g_ctx.eMousePos == MOUSEPOS_LEFT || g_ctx.eMousePos == MOUSEPOS_RIGHT))
			{
				*iIndex += (g_ctx.eMousePos == MOUSEPOS_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_ctx.bValueEdited = true;
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (bFocused && (g_ctx.eCode == KEY_LEFT || g_ctx.eCode == KEY_RIGHT))
			{
				*iIndex += (g_ctx.eCode == KEY_LEFT) ? -1 : +1;
				*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
				g_ctx.bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

void NeoUI::Tabs(const wchar_t **wszLabelsList, const int iLabelsSize, int *iIndex)
{
	// This is basically a ringbox but different UI
	if (g_ctx.iWidget == g_ctx.iFocus && g_ctx.iSection == g_ctx.iFocusSection) g_ctx.iFocus += g_ctx.iFocusDirection;
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();
	const int iTabWide = (g_ctx.dPanel.wide / iLabelsSize);

	switch (g_ctx.eMode)
	{
	case MODE_PAINT:
	{
		for (int i = 0, iXPosTab = 0; i < iLabelsSize; ++i, iXPosTab += iTabWide)
		{
			const bool bHoverTab = (bMouseIn && IN_BETWEEN(iXPosTab, g_ctx.iMouseRelX, iXPosTab + iTabWide));
			if (bHoverTab || i == *iIndex)
			{
				surface()->DrawSetColor(bHoverTab ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
				GCtxDrawFilledRectXtoX(iXPosTab, iXPosTab + iTabWide);
			}
			const wchar_t *wszText = wszLabelsList[i];
			GCtxDrawSetTextPos(iXPosTab + g_iMarginX, g_ctx.iLayoutY + g_ctx.iFontYOffset);
			surface()->DrawPrintText(wszText, V_wcslen(wszText));
		}
	}
	break;
	case MODE_MOUSEPRESSED:
	{
		if (bMouseIn && g_ctx.eCode == MOUSE_LEFT)
		{
			const int iNextIndex = (g_ctx.iMouseRelX / iTabWide);
			if (iNextIndex != *iIndex)
			{
				*iIndex = clamp(iNextIndex, 0, iLabelsSize);
				g_ctx.iYOffset[g_ctx.iSection] = 0;
			}
		}
	}
	break;
	case MODE_KEYPRESSED:
	{
		if (g_ctx.eCode == KEY_F1 || g_ctx.eCode == KEY_F3) // Global keybind
		{
			*iIndex += (g_ctx.eCode == KEY_F1) ? -1 : +1;
			*iIndex = LoopAroundInArray(*iIndex, iLabelsSize);
			g_ctx.iYOffset[g_ctx.iSection] = 0;
		}
	}
	break;
	default:
		break;
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

void NeoUI::Slider(const wchar_t *wszLeftLabel, float *flValue, const float flMin, const float flMax,
				   const int iDp, const float flStep)
{
	auto [bMouseIn, bFocused] = InternalGetMouseinFocused();

	if (IN_BETWEEN(0, g_ctx.iLayoutY, g_ctx.dPanel.tall))
	{
		switch (g_ctx.eMode)
		{
		case MODE_PAINT:
		{
			InternalLabel(wszLeftLabel, false);

			wchar_t wszFormat[32];
			V_swprintf_safe(wszFormat, L"%%0.%df", iDp);

			// Background bar
			const float flPerc = (((iDp == 0) ? (int)(*flValue) : *flValue) - flMin) / (flMax - flMin);
			const float flBarMaxWide = static_cast<float>(g_ctx.dPanel.wide - g_ctx.iWgXPos);
			GCtxDrawFilledRectXtoX(g_ctx.iWgXPos, g_ctx.iWgXPos + static_cast<int>(flPerc * flBarMaxWide));

			// Center-text label
			wchar_t wszText[32];
			const int iTextSize = V_swprintf_safe(wszText, wszFormat, *flValue);
			int iFontWide, iFontTall;
			surface()->GetTextSize(g_neoFont, wszText, iFontWide, iFontTall);
			surface()->DrawSetTextPos(g_ctx.dPanel.x + g_ctx.iWgXPos + (((g_ctx.dPanel.wide - g_ctx.iWgXPos) / 2) - (iFontWide / 2)),
									  g_ctx.dPanel.y + g_ctx.iLayoutY + g_ctx.iFontYOffset);
			surface()->DrawPrintText(wszText, iTextSize);

			// TODO: Generate once?
			int iStartBtnXPos, iStartBtnYPos;
			{
				int iPrevNextWide, iPrevNextTall;
				surface()->GetTextSize(g_neoFont, L"<", iPrevNextWide, iPrevNextTall);
				iStartBtnXPos = (g_iRowTall / 2) - (iPrevNextWide / 2);
				iStartBtnYPos = (g_iRowTall / 2) - (iPrevNextTall / 2);
			}

			// Left-side "<" prev button
			GCtxDrawFilledRectXtoX(g_ctx.iWgXPos, g_ctx.iWgXPos + g_iRowTall);
			GCtxDrawSetTextPos(g_ctx.iWgXPos + iStartBtnXPos, g_ctx.iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L"<", 1);

			// Right-side ">" next button
			GCtxDrawFilledRectXtoX(g_ctx.dPanel.wide - g_iRowTall, g_ctx.dPanel.wide);
			GCtxDrawSetTextPos(g_ctx.dPanel.wide - g_iRowTall + iStartBtnXPos, g_ctx.iLayoutY + iStartBtnYPos);
			surface()->DrawPrintText(L">", 1);
		}
		break;
		case MODE_MOUSEPRESSED:
		{
			if (bMouseIn && g_ctx.eCode == MOUSE_LEFT)
			{
				if (g_ctx.eMousePos == MOUSEPOS_LEFT || g_ctx.eMousePos == MOUSEPOS_RIGHT)
				{
					*flValue += (g_ctx.eMousePos == MOUSEPOS_LEFT) ? -flStep : +flStep;
					*flValue = clamp(*flValue, flMin, flMax);
					g_ctx.bValueEdited = true;
				}
				else if (g_ctx.eMousePos == MOUSEPOS_CENTER)
				{
					g_ctx.iFocusDirection = 0;
					g_ctx.iFocus = g_ctx.iWidget;
					g_ctx.iFocusSection = g_ctx.iSection;
				}
			}
		}
		break;
		case MODE_KEYPRESSED:
		{
			if (bFocused && (g_ctx.eCode == KEY_LEFT || g_ctx.eCode == KEY_RIGHT))
			{
				*flValue += (g_ctx.eCode == KEY_LEFT) ? -flStep : +flStep;
				*flValue = clamp(*flValue, flMin, flMax);
				g_ctx.bValueEdited = true;
			}
		}
		break;
		case MODE_MOUSEMOVED:
		{
			if (bMouseIn && bFocused && g_ctx.eMousePos == MOUSEPOS_CENTER && input()->IsMouseDown(MOUSE_LEFT))
			{
				const int iBase = g_iRowTall + g_ctx.iWgXPos;
				const float flPerc = static_cast<float>(g_ctx.iMouseRelX - iBase) / static_cast<float>(g_ctx.dPanel.wide - g_iRowTall - iBase);
				*flValue = flMin + (flPerc * (flMax - flMin));
				*flValue = clamp(*flValue, flMin, flMax);
				g_ctx.bValueEdited = true;
			}
		}
		break;
		default:
			break;
		}
	}

	InternalUpdatePartitionState(bMouseIn, bFocused);
}

CNeoOverlay_KeyCapture::CNeoOverlay_KeyCapture(Panel *parent)
	: EditablePanel(parent, "neo_overlay_keycapture")
{
	// Required to make sure text entries are recognized
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetBgColor(COLOR_NEOPANELNORMALBG);
	SetFgColor(COLOR_NEOPANELNORMALBG);
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_KeyCapture::PerformLayout()
{
	// Like the root panel, we'll want the whole screen
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	SetPos(0, 0);
	SetSize(wide, tall);
	SetFgColor(COLOR_NEOPANELPOPUPBG);
	SetBgColor(COLOR_NEOPANELPOPUPBG);
}

void CNeoOverlay_KeyCapture::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	int yPos = 0;
	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	{
		int textWidth, textHeight;
		surface()->DrawSetTextFont(m_fontMain);
		surface()->GetTextSize(m_fontMain, m_wszBindingText, textWidth, textHeight);
		const int textYPos = tallSplit + (tallSplit / 2) - (textHeight / 2);
		surface()->DrawSetTextPos((wide - textWidth) / 2, textYPos);
		surface()->DrawPrintText(m_wszBindingText, V_wcslen(m_wszBindingText));
		yPos = textYPos + textHeight;
	}
	{
		static constexpr wchar_t SUB_INFO[] = L"Press ESC to cancel or DEL to remove keybind";
		int textWidth, textHeight;
		surface()->DrawSetTextFont(m_fontSub);
		surface()->GetTextSize(m_fontSub, SUB_INFO, textWidth, textHeight);
		surface()->DrawSetTextPos((wide - textWidth) / 2, yPos + (textHeight / 2));
		surface()->DrawPrintText(SUB_INFO, SZWSZ_LEN(SUB_INFO));
	}
}

void CNeoOverlay_KeyCapture::OnThink()
{
	if (ButtonCode_t code; engine->CheckDoneKeyTrapping(code))
	{
		const bool bUpdate = (code != KEY_ESCAPE);
		if (bUpdate)
		{
			m_iButtonCode = (code == KEY_DELETE) ? BUTTON_CODE_NONE : code;
		}
		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("KeybindUpdate", "UpdateKey", bUpdate));
	}
}

CNeoOverlay_Confirm::CNeoOverlay_Confirm(Panel *parent)
	: EditablePanel(parent, "cneooverlay_confirm")
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	SetBgColor(COLOR_NEOPANELNORMALBG);
	SetFgColor(COLOR_NEOPANELNORMALBG);
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_Confirm::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	SetPos(0, 0);
	SetSize(wide, tall);
	SetFgColor(COLOR_NEOPANELPOPUPBG);
	SetBgColor(COLOR_NEOPANELPOPUPBG);
}

void CNeoOverlay_Confirm::Paint()
{
	BaseClass::Paint();

	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	const int btnWide = wide / 6;
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
	surface()->DrawFilledRect(0, tallSplit, wide, tall - tallSplit);

	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	surface()->DrawSetTextFont(m_fontMain);
	{
		static constexpr wchar_t CONFIRM_TEXT[] = L"Settings changed: Do you want to apply the settings?";

		int textWidth, textHeight;
		surface()->GetTextSize(m_fontMain, CONFIRM_TEXT, textWidth, textHeight);
		const int textYPos = tallSplit + (tallSplit / 3) - (textHeight / 2);
		surface()->DrawSetTextPos((wide - textWidth) / 2, textYPos);
		surface()->DrawPrintText(CONFIRM_TEXT, SZWSZ_LEN(CONFIRM_TEXT));
	}
	{
		const int buttonYPos = tallSplit + (tallSplit * 0.67f) - (g_iRowTall / 2);
		surface()->DrawSetTextFont(g_neoFont);

		{
			static constexpr wchar_t APPLY_TEXT[] = L"Apply (Enter)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, APPLY_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2) - btnWide;
			surface()->DrawSetColor((m_buttonHover == BUTTON_APPLY) ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(APPLY_TEXT, SZWSZ_LEN(APPLY_TEXT));
		}
		{
			static constexpr wchar_t DISCARD_TEXT[] = L"Discard (ESC)";
			int textWidth, textHeight;
			surface()->GetTextSize(g_neoFont, DISCARD_TEXT, textWidth, textHeight);

			const int buttonXPos = (wide / 2);
			surface()->DrawSetColor((m_buttonHover == BUTTON_DISCARD) ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(buttonXPos, buttonYPos, buttonXPos + btnWide, buttonYPos + g_iRowTall);
			surface()->DrawSetTextPos(buttonXPos + ((btnWide / 2) - (textWidth / 2)), buttonYPos + ((g_iRowTall / 2) - (textHeight / 2)));
			surface()->DrawPrintText(DISCARD_TEXT, SZWSZ_LEN(DISCARD_TEXT));
		}
	}
}

void CNeoOverlay_Confirm::OnKeyCodePressed(vgui::KeyCode code)
{
	if (code == KEY_ESCAPE || code == KEY_ENTER)
	{
		m_bChoice = (code == KEY_ENTER);

		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("ConfirmDialogUpdate", "Accepted", m_bChoice));
	}
}

void CNeoOverlay_Confirm::OnMousePressed(vgui::MouseCode code)
{
	if (code == MOUSE_LEFT && m_buttonHover != BUTTON_NONE)
	{
		m_bChoice = (m_buttonHover == BUTTON_APPLY);

		SetVisible(false);
		SetEnabled(false);
		PostActionSignal(new KeyValues("ConfirmDialogUpdate", "Accepted", m_bChoice));
	}
}

void CNeoOverlay_Confirm::OnCursorMoved(int x, int y)
{
	int wide, tall;
	GetSize(wide, tall);

	const int tallSplit = tall / 3;
	const int btnWide = wide / 6;
	const int buttonYPos = tallSplit + (tallSplit * 0.67f) - (g_iRowTall / 2);

	m_buttonHover = BUTTON_NONE;
	if (y >= buttonYPos && y < (buttonYPos + g_iRowTall))
	{
		if (x >= ((wide / 2) - btnWide) && x < (wide / 2))
		{
			m_buttonHover = BUTTON_APPLY;
		}
		else if (x >= (wide / 2) && x < ((wide / 2) + btnWide))
		{
			m_buttonHover = BUTTON_DISCARD;
		}
	}
}

void CNeoDataSlider::SetValue(const float value)
{
	iValCur = static_cast<int>(value * flMulti);
	ClampAndUpdate();
}

float CNeoDataSlider::GetValue() const
{
	return static_cast<float>(iValCur) * (1.0f / flMulti);
}

void CNeoDataSlider::ClampAndUpdate()
{
	iValCur = clamp(iValCur, iValMin, iValMax);
	if (flMulti == 1.0f)
	{
		iWszCacheLabelSize = V_swprintf_safe(wszCacheLabel, L"%d", iValCur);
	}
	else
	{
		iWszCacheLabelSize = V_swprintf_safe(wszCacheLabel, L"%.02f", GetValue());
	}
	if (iWszCacheLabelSize < 0) iWszCacheLabelSize = 0;
}

CNeoPanel_Base::CNeoPanel_Base(Panel *parent)
	: EditablePanel(parent, "CNeoPanel_Base")
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
}

void CNeoPanel_Base::UserSettingsRestore()
{
	for (int i = 0; i < TabsListSize(); ++i)
	{
		TabsList()[i]->UserSettingsRestore();
	}
	m_bModified = false;
}

void CNeoPanel_Base::UserSettingsSave()
{
	for (int i = 0; i < TabsListSize(); ++i)
	{
		TabsList()[i]->UserSettingsSave();
	}
	m_bModified = false;
}

void CNeoPanel_Base::PerformLayout()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	BaseClass::PerformLayout();
	SetSize(g_iRootSubPanelWide, g_iRowTall + (tall * 0.8f) + g_iRowTall);
	SetBgColor(COLOR_NEOPANELFRAMEBG);
	SetFgColor(COLOR_NEOPANELFRAMEBG);
}

void CNeoPanel_Base::Paint()
{
	BaseClass::Paint();

	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	surface()->DrawSetTextFont(g_neoFont);
	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;
	int fontWide, fontTall;
	surface()->GetTextSize(g_neoFont, L"<", fontWide, fontTall);
	const int fontStartXPos = (widgetTall / 2) - (fontWide / 2);
	const int fontStartYPos = (widgetTall / 2) - (fontTall / 2);
	const int panelTall = GetTall();
	const bool bEditBlinkShow = (static_cast<int>(gpGlobals->curtime * 1.5f) % 2 == 0);

	const int yOvershoot = m_iScrollOffset % g_iRowTall;
	for (int i = (m_iScrollOffset / g_iRowTall), yPos = TopAreaTall() - yOvershoot;
		 yPos < (panelTall - g_iRowTall) && i < tab->NdvListSize();
		 ++i, yPos += widgetTall)
	{
		CNeoDataVariant *ndv = &tab->NdvList()[i];
		const bool bThisHover = (m_eSectionActive == SECTION_MAIN && i == m_iNdvHover);
		const bool bThisFocus = (i == m_iNdvMainFocus);
		const bool bThisHoverFocus = bThisHover || bThisFocus;

		surface()->DrawSetColor(bThisHoverFocus ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
		surface()->DrawSetTextColor(bThisFocus ? COLOR_NEOPANELTEXTBRIGHT : COLOR_NEOPANELTEXTNORMAL);

		if (ndv->type == CNeoDataVariant::GAMESERVER)
		{
			if (!bThisHoverFocus) surface()->DrawSetColor((i % 2 == 0) ? COLOR_NEOPANELNORMALBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(0, yPos, g_iRootSubPanelWide, yPos + widgetTall);

			// TODO/TEMP (nullsystem): Probably should be more "custom" struct verse gameserveritem_t?
			const gameserveritem_t *gameserver = &ndv->gameServer.info;
			int xPos = 0;

			if (gameserver->m_bPassword)
			{
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(L"P", 1);
			}
			xPos += g_iGSIX[GSIW_LOCKED];

			if (gameserver->m_bSecure)
			{
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(L"S", 1);
			}
			xPos += g_iGSIX[GSIW_VAC];

			{
				wchar_t wszServerName[k_cbMaxGameServerName];
				const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(gameserver->GetName(), wszServerName, sizeof(wszServerName));
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszServerName, iSize);
			}
			xPos += g_iGSIX[GSIW_NAME];

			{
				// In lower resolution, it may overlap from name, so paint a background here
				surface()->DrawFilledRect(xPos, yPos, g_iRootSubPanelWide, yPos + widgetTall);

				wchar_t wszMapName[k_cbMaxGameServerMapName];
				const int iSize = g_pVGuiLocalize->ConvertANSIToUnicode(gameserver->m_szMap, wszMapName, sizeof(wszMapName));
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszMapName, iSize);
			}
			xPos += g_iGSIX[GSIW_MAP];

			{
				// In lower resolution, it may overlap from name, so paint a background here
				surface()->DrawFilledRect(xPos, yPos, g_iRootSubPanelWide, yPos + widgetTall);

				wchar_t wszPlayers[10];
				const int iSize = V_swprintf_safe(wszPlayers, L"%d/%d", gameserver->m_nPlayers, gameserver->m_nMaxPlayers);
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszPlayers, iSize);
			}
			xPos += g_iGSIX[GSIW_PLAYERS];

			{
				wchar_t wszPing[10];
				const int iSize = V_swprintf_safe(wszPing, L"%d", gameserver->m_nPing);
				surface()->DrawSetTextPos(xPos + g_iMarginX, yPos + fontStartYPos);
				surface()->DrawPrintText(wszPing, iSize);
			}

			continue;
		}

		switch (ndv->type)
		{
		case CNeoDataVariant::TEXTENTRY:
		{
			surface()->DrawFilledRect(wgXPos, yPos, wgXPos + widgetWide, yPos + widgetTall);
		}
		break;
		default: break;
		}

		// Draw the left-side label
		if (ndv->labelSize > 0)
		{
			const wchar_t *wszLeftLabel = ndv->label;
			if (ndv->type == CNeoDataVariant::TEXTLABEL)
			{
				wszLeftLabel = ndv->textLabel.wszLabel;

				int fontWide, fontTall;
				surface()->GetTextSize(g_neoFont, ndv->textLabel.wszLabel, fontWide, fontTall);
				surface()->DrawSetTextPos((g_iRootSubPanelWide / 2) - (fontWide / 2), yPos + fontStartYPos);
			}
			else
			{
				surface()->DrawSetTextPos(g_iMarginX, yPos + fontStartYPos);
			}
			surface()->DrawPrintText(wszLeftLabel, ndv->labelSize);
		}

		// Draw the right-side widget - widget specific
		switch (ndv->type)
		{
		case CNeoDataVariant::TEXTENTRY:
		{
			CNeoDataTextEntry *te = &ndv->textEntry;
			surface()->DrawSetTextPos(wgXPos + g_iMarginX, yPos + fontStartYPos);
			surface()->DrawPrintText(te->wszEntry, V_wcslen(te->wszEntry));
			if (bThisFocus && bEditBlinkShow)
			{
				int textWide, textTall;
				surface()->GetTextSize(g_neoFont, te->wszEntry, textWide, textTall);
				surface()->DrawSetTextPos(wgXPos + g_iMarginX + textWide, yPos + fontStartYPos);
				surface()->DrawPrintText(L"_", 1);
			}
		}
		break;
		}
	}

	const int bottomYStart = (panelTall - g_iRowTall);
	{
		const int iBtnWide = g_iRootSubPanelWide / BottomSectionListSize();

		// Draw the bottom part
		surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
		surface()->DrawFilledRect(0, bottomYStart, g_iRootSubPanelWide, panelTall);

		// Draw the buttons on bottom
		for (int i = 0, xPosTab = 0; i < BottomSectionListSize(); ++i, xPosTab += iBtnWide)
		{
			const auto bottomInfo = BottomSectionList()[i];
			const bool bBottomHover = (m_eSectionActive == SECTION_BOTTOM && i == m_iNdvHover);
			if (bottomInfo.text)
			{
				surface()->DrawSetColor(bBottomHover ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELNORMALBG);
				surface()->DrawFilledRect(xPosTab, bottomYStart, xPosTab + iBtnWide, panelTall);
				surface()->DrawSetTextPos(xPosTab + g_iMarginX, bottomYStart + fontStartYPos);
				surface()->DrawPrintText(bottomInfo.text, bottomInfo.size);
			}
		}
	}
}

void CNeoPanel_Base::OnMousePressed(vgui::MouseCode code)
{
	OnExitTextEditMode(true);
	switch (m_eSectionActive)
	{
	break; case SECTION_TOP:
	{
		if (m_iNdvHover >= 0 && m_iNdvHover < TabsListSize())
		{
			m_iNdsCurrent = m_iNdvHover;
			m_iNdvMainFocus = -1;
			m_iScrollOffset = 0;
			InvalidateLayout();
			PerformLayout();
			RequestFocus();
		}
	}
	break; case SECTION_MAIN:
	{
		CNeoDataVariant *ndv = NdvFromIdx((code == MOUSE_LEFT) ? m_iNdvHover : -1);
		if (!ndv) return;

		switch (ndv->type)
		{
		case CNeoDataVariant::RINGBOX:
		{
			if (m_curMouse == WDG_NONE || m_curMouse == WDG_CENTER) return;

			CNeoDataRingBox *rb = &ndv->ringBox;
			rb->iCurIdx += (m_curMouse == WDG_LEFT) ? -1 : +1;
			rb->iCurIdx = LoopAroundInArray(rb->iCurIdx, rb->iItemsSize);

			m_bModified = true;
		}
		break;
		case CNeoDataVariant::SLIDER:
		{
			if (m_curMouse == WDG_NONE) return;
			if (m_curMouse == WDG_CENTER)
			{
				m_iNdvMainFocus = m_iNdvHover;
				return;
			}

			CNeoDataSlider *sl = &ndv->slider;
			sl->iValCur += (m_curMouse == WDG_LEFT) ? -sl->iValStep : +sl->iValStep;
			sl->ClampAndUpdate();

			m_bModified = true;
		}
		break;
		case CNeoDataVariant::BUTTON:
			if (ndv->labelSize == 0 || (ndv->labelSize > 0 && m_curMouse == WDG_CENTER)) OnEnterButton(ndv);
			break;
		case CNeoDataVariant::BINDENTRY:
			if (m_curMouse == WDG_CENTER) OnEnterButton(ndv);
			break;
		case CNeoDataVariant::GAMESERVER:
			m_iNdvMainFocus = m_iNdvHover;
			break;
		case CNeoDataVariant::S_MICTESTER:
		{
			IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
			voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
		}
		break;
		}
	}
	break; case SECTION_BOTTOM:
	{
		OnBottomAction(m_iNdvHover);
	}
	}
}

void CNeoPanel_Base::OnMouseDoublePressed(vgui::MouseCode code)
{
	OnExitTextEditMode();
	if (m_eSectionActive != SECTION_MAIN)
	{
		return;
	}

	CNeoDataVariant *ndv = NdvFromIdx(m_iNdvHover);
	if (!ndv) return;

	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	case CNeoDataVariant::TEXTENTRY:
	{
		if (m_curMouse == WDG_CENTER && code == MOUSE_LEFT)
		{
			m_iNdvMainFocus = m_iNdvHover;
			RequestFocus();
		}
		break;
	}
	case CNeoDataVariant::GAMESERVER:
		OnEnterServer(ndv->gameServer.info);
		break;
	}
}

void CNeoPanel_Base::OnMouseWheeled(int delta)
{
	int iScreenWidth, iScreenHeight;
	engine->GetScreenSize(iScreenWidth, iScreenHeight);

	const float flScrollSpeed = static_cast<float>(iScreenHeight) * 0.05f;

	CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
	const int iWidgetSectionTall = GetTall() - g_iRowTall - TopAreaTall();
	int iMaxScrollOffset = (tab->NdvListSize() * g_iRowTall) - iWidgetSectionTall;
	if (iMaxScrollOffset < 0) iMaxScrollOffset = 0;
	m_iScrollOffset += (int)((float)(-delta) * flScrollSpeed);
	m_iScrollOffset = clamp(m_iScrollOffset, 0, iMaxScrollOffset);
}

void CNeoPanel_Base::OnKeyCodeTyped(vgui::KeyCode code)
{
	// "Universal" binds
	if (const int iBottomBtns = KeyCodeToBottomAction(code);
			iBottomBtns >= 0 && iBottomBtns < BottomSectionListSize())
	{
		OnExitTextEditMode();
		OnBottomAction(iBottomBtns);
		return;
	}
	else if (code == KEY_F1 || code == KEY_F3)
	{
		// Change tab
		OnExitTextEditMode();
		const bool bBackward = (code == KEY_F1);
		const int iIncr = bBackward ? -1 : +1;
		m_iNdsCurrent += iIncr;
		m_iNdsCurrent = LoopAroundInArray(m_iNdsCurrent, TabsListSize());

		m_eSectionActive = SECTION_MAIN;
		m_iNdvHover = 0;
		return;
	}
	else if (code == KEY_TAB)
	{
		// Change section, top may active by key if it has more than just tabs
		const bool bBackward = (input()->IsKeyDown(KEY_LSHIFT) || input()->IsKeyDown(KEY_RSHIFT));
		int iSectionActive = static_cast<int>(m_eSectionActive);
		iSectionActive += (bBackward) ? -1 : +1;
		iSectionActive = LoopAroundInArray(iSectionActive, SECTION__TOTAL);
		if (iSectionActive == SECTION_TOP && TopAreaRows() <= 1) iSectionActive = SECTION_MAIN;
		m_eSectionActive = static_cast<eSectionActive>(iSectionActive);
		// Set to TabsListSize if top-section with extra widgets as that'll be used for first
		// non-tabbed widget there
		m_iNdvHover = (iSectionActive == SECTION_TOP && TopAreaRows() >= 2) ? TabsListSize() : 0;
		return;
	}

	// Section limited binds
	switch (m_eSectionActive)
	{
	break; case SECTION_TOP:
	{
		// no-op, override this function method when needed
	}
	break; case SECTION_MAIN:
	{
		if (code == KEY_DOWN || code == KEY_UP)
		{
			CNeoDataSettings_Base *tab = TabsList()[m_iNdsCurrent];
			const int iNdvListSize = tab->NdvListSize();

			if (iNdvListSize == 0) return;

			OnExitTextEditMode();
			const int iIncr = (code == KEY_UP) ? -1 : +1;

			// Increment or decrement to the next valid NDV
			CNeoDataVariant::Type type;
			int iSeenZero = 0;
			do
			{
				m_iNdvHover += iIncr;
				m_iNdvHover = LoopAroundInArray(m_iNdvHover, iNdvListSize);
				type = tab->NdvList()[m_iNdvHover].type;
				iSeenZero += (m_iNdvHover == 0);
				// If iSeenZero is >= 2, then there isn't really an item we can key to hover at
			}
			while (iSeenZero < 2 && (type == CNeoDataVariant::TEXTLABEL || type == CNeoDataVariant::S_DISPLAYNAME));

			if (iSeenZero >= 2)
			{
				m_iNdvHover = -1;
			}

			if (m_iNdvHover >= 0)
			{
				// Re-adjust scroll offset
				const int iWdgArea = GetTall() - g_iRowTall - TopAreaTall();
				const int iWdgYPos = g_iRowTall * m_iNdvHover;
				if (iWdgYPos < m_iScrollOffset)
				{
					m_iScrollOffset = iWdgYPos;
				}
				else if ((iWdgYPos + g_iRowTall) >= (m_iScrollOffset + iWdgArea))
				{
					m_iScrollOffset = (iWdgYPos - iWdgArea + g_iRowTall);
				}
			}

			m_iNdvMainFocus = (type == CNeoDataVariant::GAMESERVER) ? m_iNdvHover : -1;
			return;
		}

		CNeoDataVariant *ndv = NdvFromIdx(m_iNdvHover);
		if (!ndv) return;

		if (code == KEY_LEFT || code == KEY_RIGHT)
		{
			switch (ndv->type)
			{
			case CNeoDataVariant::RINGBOX:
			{
				CNeoDataRingBox *rb = &ndv->ringBox;
				rb->iCurIdx += (code == KEY_LEFT) ? -1 : +1;
				rb->iCurIdx = LoopAroundInArray(rb->iCurIdx, rb->iItemsSize);
				m_bModified = true;
				OnExitTextEditMode();
			}
			break;
			case CNeoDataVariant::SLIDER:
			{
				CNeoDataSlider *sl = &ndv->slider;
				sl->iValCur += (code == KEY_LEFT) ? -sl->iValStep : +sl->iValStep;
				sl->ClampAndUpdate();
				m_bModified = true;
				OnExitTextEditMode();
			}
			break;
			default: break;
			}
			return;
		}
		else if (code == KEY_ENTER)
		{
			// Change editing mode
			switch (ndv->type)
			{
			case CNeoDataVariant::SLIDER:
			case CNeoDataVariant::TEXTENTRY:
			{
				if (m_iNdvMainFocus != -1 && ndv->type == CNeoDataVariant::SLIDER)
				{
					ndv->slider.ClampAndUpdate();
				}
				m_iNdvMainFocus = (m_iNdvMainFocus == -1) ? m_iNdvHover : -1;
				break;
			}
			case CNeoDataVariant::BUTTON:
			case CNeoDataVariant::BINDENTRY:
				OnEnterButton(ndv);
				break;
			case CNeoDataVariant::S_MICTESTER:
			{
				IVoiceTweak_s *voiceTweak = engine->GetVoiceTweakAPI();
				voiceTweak->IsStillTweaking() ? (void)voiceTweak->EndVoiceTweakMode() : (void)voiceTweak->StartVoiceTweakMode();
				break;
			}
			case CNeoDataVariant::GAMESERVER:
				OnEnterServer(ndv->gameServer.info);
				break;
			default:
				break;
			}
			return;
		}
		else
		{
			CNeoDataVariant *focusNdv = NdvFromIdx(m_iNdvMainFocus);
			if (!focusNdv) return;

			switch (focusNdv->type)
			{
			case CNeoDataVariant::SLIDER:
			{
				CNeoDataSlider *sl = &focusNdv->slider;
				if (code == KEY_BACKSPACE && sl->iWszCacheLabelSize > 0)
				{
					sl->wszCacheLabel[--sl->iWszCacheLabelSize] = '\0';
					m_bModified = true;
				}
			}
			break;
			case CNeoDataVariant::TEXTENTRY:
			{
				CNeoDataTextEntry *te = &focusNdv->textEntry;
				int iWszSize = V_wcslen(te->wszEntry);
				if (code == KEY_BACKSPACE && iWszSize > 0)
				{
					te->wszEntry[--iWszSize] = '\0';
					m_bModified = true;
				}
			}
			break;
			default: break;
			}
		}
	}
	break; case SECTION_BOTTOM:
	{
		if (code == KEY_LEFT || code == KEY_RIGHT)
		{
			do
			{
				m_iNdvHover += (code == KEY_LEFT) ? -1 : +1;
				m_iNdvHover = LoopAroundInArray(m_iNdvHover, BottomSectionListSize());
			}
			while (BottomSectionList()[m_iNdvHover].text == nullptr);
		}
		else if (code == KEY_ENTER)
		{
			OnBottomAction(m_iNdvHover);
		}
	}
	}
}

void CNeoPanel_Base::OnKeyTyped(wchar_t unichar)
{
	CNeoDataVariant *ndv = NdvFromIdx((m_eSectionActive == SECTION_MAIN) ? m_iNdvMainFocus : -1);
	if (!ndv) return;

	switch (ndv->type)
	{
	case CNeoDataVariant::SLIDER:
	{
		CNeoDataSlider *sl = &ndv->slider;
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
		}
	}
	break;
	case CNeoDataVariant::TEXTENTRY:
	{
		CNeoDataTextEntry *te = &ndv->textEntry;
		int iWszSize = V_wcslen(te->wszEntry);
		if (iWszSize < CNeoDataTextEntry::ENTRY_MAX && iswprint(unichar))
		{
			te->wszEntry[iWszSize++] = unichar;
			te->wszEntry[iWszSize] = '\0';
			m_bModified = true;
		}
	}
	break;
	default: break;
	}
}

// Use the y position to get the y-axis partition, then
// use the x position to get the buttons
void CNeoPanel_Base::OnCursorMoved(int x, int y)
{
	m_iPosX = clamp(x, 0, GetWide());
	m_eSectionActive = SECTION_MAIN;
	m_iNdvHover = -1;
	m_curMouse = WDG_NONE;

	if (x < 0 || x >= g_iRootSubPanelWide)
	{
		return;
	}

	if (y < TopAreaTall())
	{
		m_eSectionActive = SECTION_TOP;
		if (y < g_iRowTall)
		{
			const int iTabWide = g_iRootSubPanelWide / TabsListSize();
			m_iNdvHover = x / iTabWide;
		}
		else
		{
			OnCursorMovedTopArea(x, y);
		}
		return;
	}
	else if (y > (GetTall() - g_iRowTall))
	{
		m_eSectionActive = SECTION_BOTTOM;
		const int iTabWide = g_iRootSubPanelWide / BottomSectionListSize();
		m_iNdvHover = x / iTabWide;
		return;
	}

	y -= TopAreaTall() - m_iScrollOffset;
	m_iNdvHover = y / g_iRowTall;

	const int wgXPos = static_cast<int>(g_iRootSubPanelWide * 0.4f);
	const int widgetWide = g_iRootSubPanelWide - wgXPos;
	const int widgetTall = g_iRowTall;

	CNeoDataVariant *ndv = NdvFromIdx(m_iNdvHover);
	if (!ndv) return;

	switch (ndv->type)
	{
	case CNeoDataVariant::RINGBOX:
	case CNeoDataVariant::SLIDER:
		if (x < wgXPos)
		{
			m_curMouse = WDG_NONE;
		}
		else if (x < (wgXPos + widgetTall))
		{
			m_curMouse = WDG_LEFT;
		}
		else if (x > (g_iRootSubPanelWide - widgetTall))
		{
			m_curMouse = WDG_RIGHT;
		}
		else
		{
			m_curMouse = WDG_CENTER;
		}

		if (ndv->type == CNeoDataVariant::SLIDER && m_curMouse == WDG_CENTER &&
				input()->IsMouseDown(MOUSE_LEFT) && m_iNdvMainFocus == m_iNdvHover)
		{
			const int wdgX = x - wgXPos;
			CNeoDataSlider *sl = &ndv->slider;

			const int iRange = sl->iValMax - sl->iValMin;
			const float flSliderWidth = static_cast<float>(widgetWide - (2 * widgetTall));
			const float perc = static_cast<float>(wdgX - widgetTall) / flSliderWidth;
			sl->iValCur = static_cast<int>(perc * static_cast<float>(iRange)) + sl->iValMin;
			sl->ClampAndUpdate();
			m_bModified = true;
		}
		break;
	case CNeoDataVariant::TEXTENTRY:
	case CNeoDataVariant::BUTTON:
	case CNeoDataVariant::BINDENTRY:
	case CNeoDataVariant::S_MICTESTER:
		if (wgXPos <= x && x < g_iRootSubPanelWide)
		{
			m_curMouse = WDG_CENTER;
		}
	default:
		break;
	}
}

CNeoDataVariant *CNeoPanel_Base::NdvFromIdx(const int idx) const
{
	CNeoDataSettings_Base *tab = const_cast<CNeoPanel_Base *>(this)->TabsList()[m_iNdsCurrent];
	return (tab && idx >= 0 && idx < tab->NdvListSize()) ? &tab->NdvList()[idx] : nullptr;
}

void CNeoPanel_Base::OnExitTextEditMode(const bool bKeepGameServerFocus)
{
	CNeoDataVariant *ndv = NdvFromIdx(m_iNdvMainFocus);
	if (ndv && ndv->type == CNeoDataVariant::SLIDER)
	{
		ndv->slider.ClampAndUpdate();
	}

	if (bKeepGameServerFocus && ndv && ndv->type == CNeoDataVariant::GAMESERVER)
	{
		return;
	}

	m_iNdvMainFocus = -1;
}

int CNeoPanel_Base::TopAreaTall() const
{
	return g_iRowTall * TopAreaRows();
}

extern ConVar neo_fov;
extern ConVar neo_viewmodel_fov_offset;
extern ConVar neo_aim_hold;
extern ConVar cl_autoreload_when_empty;
extern ConVar cl_righthand;
extern ConVar cl_showpos;
extern ConVar cl_showfps;
extern ConVar hud_fastswitch;

static const wchar_t *DLFILTER_LABELS[] = {
	L"Allow all custom files from server",
	L"Do not download custom sounds",
	L"Only allow map files",
	L"Do not download any custom files",
};

static const char *DLFILTER_STRMAP[] = {
	"all", "nosounds", "mapsonly", "none"
};

static constexpr int DLFILTER_SIZE = ARRAYSIZE(DLFILTER_LABELS);

static const wchar_t *SHOWFPS_LABELS[] = {
	L"Disabled",
	L"Enabled (FPS)",
	L"Enabled (Smooth FPS)",
};

static constexpr float SLT_VOL = 100.0f;
static const wchar_t *SPEAKER_CFG_LABELS[] = {
	L"Auto",
	L"Headphones",
	L"2 Speakers (Stereo)",
#ifndef LINUX
	L"4 Speakers (Quadraphonic)",
	L"5.1 Surround",
	L"7.1 Surround",
#endif
};

enum MultiThreadIdx
{
	THREAD_SINGLE = 0,
	THREAD_MULTI,
};

enum FilteringEnum
{
	FILTERING_BILINEAR = 0,
	FILTERING_TRILINEAR,
	FILTERING_ANISO2X,
	FILTERING_ANISO4X,
	FILTERING_ANISO8X,
	FILTERING_ANISO16X,

	FILTERING__TOTAL,
};

static const wchar_t *WINDOW_MODE[] = {
	L"Fullscreen",
	L"Windowed",
};

static const wchar_t *QUEUE_MODE[] = {
	L"Single",
	L"Multi",
};

static const wchar_t *QUALITY2_LABELS[] = {
	L"Low",
	L"High",
};

static const wchar_t *WATER_LABELS[] = {
	L"Simple Reflect",
	L"Reflect World",
	L"Reflect All",
};

static const wchar_t *MSAA_LABELS[] = {
	L"None",
	L"2x MSAA",
	L"4x MSAA",
	L"6x MSAA",
	L"8x MSAA",
};

static const wchar_t *FILTERING_LABELS[FILTERING__TOTAL] = {
	L"Bilinear",
	L"Trilinear",
	L"Anisotropic 2X",
	L"Anisotropic 4X",
	L"Anisotropic 8X",
	L"Anisotropic 16X",
};

static const wchar_t *HDR_LABELS[] = {
	L"None",
	L"Bloom",
	L"Full",
};

// Map list
CNeoTab_MapList::CNeoTab_MapList()
{
	FileFindHandle_t findHdl;
	for (const char *pszFilename = filesystem->FindFirst("maps/*.bsp", &findHdl);
		 pszFilename;
		 pszFilename = filesystem->FindNext(findHdl))
	{
		// Sanity check: In-case somehow someone named a directory as *.bsp in here
		if (!filesystem->FindIsDirectory(findHdl))
		{
			CNeoDataVariant ndv = { .type = CNeoDataVariant::BUTTON, .labelSize = 0, };
			ndv.button.iWszBtnLabelSize = g_pVGuiLocalize->ConvertANSIToUnicode(
						pszFilename, ndv.button.wszBtnLabel, sizeof(ndv.button.wszBtnLabel));
			ndv.button.iWszBtnLabelSize -= sizeof(".bsp");
			ndv.button.wszBtnLabel[ndv.button.iWszBtnLabelSize] = '\0';
			m_ndvVec.AddToTail(ndv);
		}
	}
	filesystem->FindClose(findHdl);
}

CNeoOverlay_MapList::CNeoOverlay_MapList(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
{
}

void CNeoOverlay_MapList::OnEnterButton(CNeoDataVariant *ndv)
{
	if (!ndv || ndv->type != CNeoDataVariant::BUTTON) return;
	PostActionSignal(new KeyValues("MapListPicked", "Map", ndv->button.wszBtnLabel));
	SetVisible(false);
	SetEnabled(false);
}

void CNeoOverlay_MapList::OnBottomAction(const int btn)
{
	if (btn == BBTN_BACK)
	{
		SetVisible(false);
		SetEnabled(false);
	}
}

const WLabelWSize *CNeoOverlay_MapList::BottomSectionList() const
{
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWSNULL, LWSNULL, LWSNULL, LWSNULL,
	};
	return BBTN_NAMES;
}

int CNeoOverlay_MapList::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	return (code == KEY_ESCAPE) ? BBTN_BACK : -1;
}

///////////////
// PANEL: NEW GAME
///////////////
CNeoDataNewGame_General::CNeoDataNewGame_General()
	: m_ndvList{
		NDV_INIT_BUTTON(L"Map"),
		NDV_INIT_TEXTENTRY(L"Hostname"),
		NDV_INIT_SLIDER(L"Max players", 1, (MAX_PLAYERS - 1), 1, 1.0f, 2),
		NDV_INIT_TEXTENTRY(L"Password"),
		NDV_INIT_RINGBOX_ONOFF(L"Friendly fire"),
	}
{
	m_ndvList[OPT_NEW_MAPLIST].button.iWszBtnLabelSize = V_swprintf_safe(
				m_ndvList[OPT_NEW_MAPLIST].button.wszBtnLabel, L"nt_oilstain_ctg");
	V_swprintf_safe(m_ndvList[OPT_NEW_HOSTNAME].textEntry.wszEntry, L"NeoTokyo Rebuild");
	m_ndvList[OPT_NEW_MAXPLAYERS].slider.iValCur = 24;
	m_ndvList[OPT_NEW_MAXPLAYERS].slider.ClampAndUpdate();
	V_swprintf_safe(m_ndvList[OPT_NEW_SVPASSWORD].textEntry.wszEntry, L"neo");
	m_ndvList[OPT_NEW_FRIENDLYFIRE].ringBox.iCurIdx = 1;
}

CNeoPanel_NewGame::CNeoPanel_NewGame(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
	, m_mapList(new CNeoOverlay_MapList(this))
{
	m_mapList->AddActionSignalTarget(this);
	m_mapList->SetVisible(false);
	m_mapList->SetEnabled(false);
}

CNeoPanel_NewGame::~CNeoPanel_NewGame()
{
	m_mapList->DeletePanel();
}

void CNeoPanel_NewGame::OnEnterButton(CNeoDataVariant *ndv)
{
	if (ndv->type == CNeoDataVariant::BUTTON)
	{
		int wide, tall;
		surface()->GetScreenSize(wide, tall);
		m_mapList->m_iNdvHover = -1;
		m_mapList->m_curMouse = CNeoPanel_Base::WDG_NONE;
		m_mapList->PerformLayout();
		m_mapList->SetPos((wide / 2) - (g_iRootSubPanelWide / 2),
								(tall / 2) - (m_mapList->GetTall() / 2));
		m_mapList->SetVisible(true);
		m_mapList->SetEnabled(true);
		m_mapList->MoveToFront();
		m_mapList->RequestFocus();
	}
}

void CNeoPanel_NewGame::OnMapListPicked(KeyValues *data)
{
	if (const wchar_t *pWszMapFilename = data->GetWString("Map"))
	{
		CNeoDataButton *btn = &m_ndsGeneral.m_ndvList[CNeoDataNewGame_General::OPT_NEW_MAPLIST].button;
		V_wcscpy_safe(btn->wszBtnLabel, pWszMapFilename);
		btn->iWszBtnLabelSize = V_wcslen(btn->wszBtnLabel);
	}

	MoveToFront();
	RequestFocus();
}

void CNeoPanel_NewGame::OnBottomAction(const int btn)
{
	switch (btn)
	{
	case BBTN_GO:
	{
		if (engine->IsInGame())
		{
			engine->ClientCmd_Unrestricted("disconnect");
		}

		using General = CNeoDataNewGame_General;
		static constexpr int ENTRY_MAX = CNeoDataTextEntry::ENTRY_MAX;

		char szMap[ENTRY_MAX + 1] = {};
		char szHostname[ENTRY_MAX + 1] = {};
		char szPassword[ENTRY_MAX + 1] = {};
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_MAPLIST].button.wszBtnLabel, szMap, sizeof(szMap));
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_HOSTNAME].textEntry.wszEntry, szHostname, sizeof(szHostname));
		g_pVGuiLocalize->ConvertUnicodeToANSI(m_ndsGeneral.m_ndvList[General::OPT_NEW_SVPASSWORD].textEntry.wszEntry, szPassword, sizeof(szPassword));
		const int iPlayers = m_ndsGeneral.m_ndvList[General::OPT_NEW_MAXPLAYERS].slider.iValCur;
		const bool bFriendlyFire = static_cast<bool>(m_ndsGeneral.m_ndvList[General::OPT_NEW_FRIENDLYFIRE].ringBox.iCurIdx);

		ConVarRef cvrHostname("hostname");
		ConVarRef cvrSvPassowrd("sv_password");
		ConVarRef cvrMpFriendlyFire("mp_friendlyfire");

		cvrHostname.SetValue(szHostname);
		cvrSvPassowrd.SetValue(szPassword);
		cvrMpFriendlyFire.SetValue(bFriendlyFire);

		char cmdStr[256];
		V_sprintf_safe(cmdStr, "maxplayers %d; progress_enable; map \"%s\"", iPlayers, szMap);
		engine->ClientCmd_Unrestricted(cmdStr);
	}
		[[fallthrough]]; // Also reset the main menu back to root state
	case BBTN_BACK:
		g_pNeoRoot->m_state = STATE_ROOT;
		g_pNeoRoot->UpdateControls();
		break;
	}
}

const WLabelWSize *CNeoPanel_NewGame::BottomSectionList() const
{
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWSNULL, LWSNULL, LWSNULL, LWS(L"Start"),
	};
	return BBTN_NAMES;
}

int CNeoPanel_NewGame::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	switch (code)
	{
	case KEY_ESCAPE: return BBTN_BACK;
	default: break;
	}
	return -1;
}

/////////////////
// SERVER BROWSER
/////////////////

enum AnitCheatMode
{
	ANTICHEAT_ANY,
	ANTICHEAT_ON,
	ANTICHEAT_OFF,

	ANTICHEAT__TOTAL,
};

static const wchar_t *ANTICHEAT_LABELS[ANTICHEAT__TOTAL] = {
	L"<Any>", L"On", L"Off"
};

static constexpr WLabelWSize GS_NAMES[GS__TOTAL] = {
	LWS(L"Internet"), LWS(L"LAN"), LWS(L"Friends"), LWS(L"Fav"), LWS(L"History"), LWS(L"Spec")
};

CNeoDataServerBrowser_General::CNeoDataServerBrowser_General()
{
}

void CNeoDataServerBrowser_General::UpdateFilteredList()
{
	m_ndvFilteredVec.RemoveAll();

	const auto ndvFilters = m_filters->m_ndvList;
	for (const CNeoDataVariant &ndv : m_ndvVec)
	{
		using SBFilter = CNeoDataServerBrowser_Filters;
		if (    (ndvFilters[SBFilter::OPT_FILTER_NOTFULL].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_nPlayers == ndv.gameServer.info.m_nMaxPlayers))
				|| (ndvFilters[SBFilter::OPT_FILTER_HASPLAYERS].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_nPlayers == 0))
				|| (ndvFilters[SBFilter::OPT_FILTER_NOTLOCKED].ringBox.iCurIdx &&
					(ndv.gameServer.info.m_bPassword))
				|| (ndvFilters[SBFilter::OPT_FILTER_VACMODE].ringBox.iCurIdx == ANTICHEAT_OFF &&
					(ndv.gameServer.info.m_bSecure))
				|| (ndvFilters[SBFilter::OPT_FILTER_VACMODE].ringBox.iCurIdx == ANTICHEAT_ON &&
					(!ndv.gameServer.info.m_bSecure))
				)
		{
			continue;
		}

		m_ndvFilteredVec.AddToTail(ndv);
	}

	if (m_ndvFilteredVec.IsEmpty())
	{
		CNeoDataVariant ndv = { .type = CNeoDataVariant::TEXTLABEL, };
		ndv.labelSize = V_swprintf_safe(ndv.textLabel.wszLabel, L"No %ls queries found.", GS_NAMES[m_iType].text);
		m_ndvFilteredVec.AddToTail(ndv);
	}
	else
	{
		// Can't use lamda capture for this, so pass through context
		V_qsort_s(m_ndvFilteredVec.Base(), m_ndvFilteredVec.Size(), sizeof(CNeoDataVariant),
				  [](void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
			const GameServerSortContext gsCtx = *(static_cast<GameServerSortContext *>(vpCtx));
			auto *ndvLeft = static_cast<const CNeoDataVariant *>(vpLeft);
			auto *ndvRight = static_cast<const CNeoDataVariant *>(vpRight);

			// Always set szLeft/szRight to name as fallback
			const char *szLeft = ndvLeft->gameServer.info.GetName();
			const char *szRight = ndvRight->gameServer.info.GetName();
			int iLeft, iRight;
			bool bLeft, bRight;
			switch (gsCtx.col)
			{
			break; case GSIW_LOCKED:
				bLeft = ndvLeft->gameServer.info.m_bPassword;
				bRight = ndvRight->gameServer.info.m_bPassword;
			break; case GSIW_VAC:
				bLeft = ndvLeft->gameServer.info.m_bSecure;
				bRight = ndvRight->gameServer.info.m_bSecure;
			break; case GSIW_MAP:
			{
				const char *szMapLeft = ndvLeft->gameServer.info.m_szMap;
				const char *szMapRight = ndvRight->gameServer.info.m_szMap;
				if (V_strcmp(szMapLeft, szMapRight) != 0)
				{
					szLeft = szMapLeft;
					szRight = szMapRight;
				}
			}
			break; case GSIW_PLAYERS:
				iLeft = ndvLeft->gameServer.info.m_nPlayers;
				iRight = ndvRight->gameServer.info.m_nPlayers;
				if (iLeft == iRight)
				{
					iLeft = ndvLeft->gameServer.info.m_nMaxPlayers;
					iRight = ndvRight->gameServer.info.m_nMaxPlayers;
				}
			break; case GSIW_PING:
				iLeft = ndvLeft->gameServer.info.m_nPing;
				iRight = ndvRight->gameServer.info.m_nPing;
			break; case GSIW_NAME: default: break;
				// no-op, already assigned (default)
			}

			switch (gsCtx.col)
			{
			case GSIW_LOCKED:
			case GSIW_VAC:
				if (bLeft != bRight) return (gsCtx.bDescending) ? bLeft < bRight : bLeft > bRight;
				break;
			case GSIW_PLAYERS:
			case GSIW_PING:
				if (iLeft != iRight) return (gsCtx.bDescending) ? iLeft < iRight : iLeft > iRight;
				break;
			default:
				break;
			}

			return (gsCtx.bDescending) ? V_strcmp(szRight, szLeft) : V_strcmp(szLeft, szRight);
		}, m_pSortCtx);
	}
}

CNeoDataVariant *CNeoDataServerBrowser_General::NdvList()
{
	return m_ndvFilteredVec.Base();
}

WLabelWSize CNeoDataServerBrowser_General::Title()
{
	return GS_NAMES[m_iType];
}

void CNeoDataServerBrowser_General::RequestList(MatchMakingKeyValuePair_t **filters, const uint32 iFiltersSize)
{
	static constexpr HServerListRequest (ISteamMatchmakingServers::*pFnReq[GS__TOTAL])(
				AppId_t, MatchMakingKeyValuePair_t **, uint32, ISteamMatchmakingServerListResponse *) = {
		&ISteamMatchmakingServers::RequestInternetServerList,
		nullptr,
		&ISteamMatchmakingServers::RequestFriendsServerList,
		&ISteamMatchmakingServers::RequestFavoritesServerList,
		&ISteamMatchmakingServers::RequestHistoryServerList,
		&ISteamMatchmakingServers::RequestSpectatorServerList,
	};

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	m_hdlRequest = (m_iType == GS_LAN) ?
				steamMM->RequestLANServerList(engine->GetAppID(), this) :
				(steamMM->*pFnReq[m_iType])(engine->GetAppID(), filters, iFiltersSize, this);
}

// Server has responded ok with updated data
void CNeoDataServerBrowser_General::ServerResponded(HServerListRequest hRequest, int iServer)
{
	if (hRequest != m_hdlRequest) return;

	ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
	gameserveritem_t *pServerDetails = steamMM->GetServerDetails(hRequest, iServer);
	if (pServerDetails)
	{
		CNeoDataVariant ndv = { .type = CNeoDataVariant::GAMESERVER, };
		ndv.gameServer.info = *pServerDetails;
		m_ndvVec.AddToTail(ndv);
		m_bModified = true;
	}
}

// Server has failed to respond
void CNeoDataServerBrowser_General::ServerFailedToRespond(HServerListRequest hRequest, [[maybe_unused]] int iServer)
{
	if (hRequest != m_hdlRequest) return;
}

// A list refresh you had initiated is now 100% completed
void CNeoDataServerBrowser_General::RefreshComplete(HServerListRequest hRequest, EMatchMakingServerResponse response)
{
	if (hRequest != m_hdlRequest) return;

	if (response == eNoServersListedOnMasterServer && m_ndvVec.IsEmpty())
	{
		m_bModified = true;
	}
}

CNeoDataServerBrowser_Filters::CNeoDataServerBrowser_Filters()
	: m_ndvList{
		NDV_INIT_RINGBOX_ONOFF(L"Server not full"),
		NDV_INIT_RINGBOX_ONOFF(L"Has users playing"),
		NDV_INIT_RINGBOX_ONOFF(L"Is not password protected"),
		NDV_INIT_RINGBOX(L"Anti-cheat", ANTICHEAT_LABELS, ARRAYSIZE(ANTICHEAT_LABELS)),
	}
{
}

CNeoPanel_ServerBrowser::CNeoPanel_ServerBrowser(vgui::Panel *parent)
	: CNeoPanel_Base(parent)
{
	for (int i = 0; i < GS__TOTAL; ++i)
	{
		m_ndsGeneral[i].m_iType = static_cast<GameServerType>(i);
		m_ndsGeneral[i].m_filters = &m_ndsFilters;
		m_ndsGeneral[i].m_pSortCtx = &m_sortCtx;
	}
	ivgui()->AddTickSignal(GetVPanel(), 200);
}

void CNeoPanel_ServerBrowser::OnBottomAction(const int btn)
{
	switch (btn)
	{
	case BBTN_LEGACY:
		g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenServerBrowser");
		break;
	case BBTN_REFRESH:
	{
		if (m_iNdsCurrent < 0 || m_iNdsCurrent >= GS__TOTAL)
		{
			return;
		}

		ISteamMatchmakingServers *steamMM = steamapicontext->SteamMatchmakingServers();
		m_ndsGeneral[m_iNdsCurrent].m_ndvVec.RemoveAll();
		m_ndsGeneral[m_iNdsCurrent].m_ndvFilteredVec.RemoveAll();
		{
			CNeoDataVariant ndv = { .type = CNeoDataVariant::TEXTLABEL, };
			ndv.labelSize = V_swprintf_safe(ndv.textLabel.wszLabel, L"Searching %ls queries...", GS_NAMES[m_iNdsCurrent].text);
			m_ndsGeneral[m_iNdsCurrent].m_ndvFilteredVec.AddToTail(ndv);
		}

		if (m_ndsGeneral[m_iNdsCurrent].m_hdlRequest)
		{
			steamMM->CancelQuery(m_ndsGeneral[m_iNdsCurrent].m_hdlRequest);
			steamMM->ReleaseRequest(m_ndsGeneral[m_iNdsCurrent].m_hdlRequest);
			m_ndsGeneral[m_iNdsCurrent].m_hdlRequest = nullptr;
		}
		static MatchMakingKeyValuePair_t mmFilters[] = {
			{"gamedir", "neo"},
		};
		MatchMakingKeyValuePair_t *pMMFilters = mmFilters;
		m_ndsGeneral[m_iNdsCurrent].RequestList(&pMMFilters, 1);
		break;
	}
	case BBTN_GO:
	{
		if (CNeoDataVariant *ndv = NdvFromIdx(m_iNdvMainFocus))
		{
			OnEnterServer(ndv->gameServer.info);
		}
	}
		[[fallthrough]]; // Also reset the main menu back to root state
	case BBTN_BACK:
		g_pNeoRoot->m_state = STATE_ROOT;
		g_pNeoRoot->UpdateControls();
		break;
	}
}

void CNeoPanel_ServerBrowser::OnEnterServer(const gameserveritem_t gameserver)
{
	if (engine->IsInGame())
	{
		engine->ClientCmd_Unrestricted("disconnect");
	}

	// NEO NOTE (nullsystem): Deal with password protected server
	if (gameserver.m_bPassword)
	{
		// TODO
	}
	else
	{
		char connectCmd[256];
		const char *szAddress = gameserver.m_NetAdr.GetConnectionAddressString();
		V_sprintf_safe(connectCmd, "progress_enable; wait; connect %s", szAddress);
		engine->ClientCmd_Unrestricted(connectCmd);

		g_pNeoRoot->m_state = STATE_ROOT;
		g_pNeoRoot->UpdateControls();
	}
}

void CNeoPanel_ServerBrowser::OnKeyCodeTyped(vgui::KeyCode code)
{
	BaseClass::OnKeyCodeTyped(code);
	if (m_eSectionActive == SECTION_TOP && m_iNdsCurrent != TAB_FILTERS)
	{
		if (code == KEY_LEFT || code == KEY_RIGHT)
		{
			int iIncr = (code == KEY_LEFT) ? -1 : +1;
			if (m_iNdvHover < TabsListSize() ||
					m_iNdvHover >= (TabsListSize() + GSIW__TOTAL))
			{
				// Reset to server-browser headers
				m_iNdvHover = TabsListSize();
				iIncr = 0;
			}

			m_iNdvHover += iIncr;
			m_iNdvHover = LoopAroundMinMax(m_iNdvHover,  TabsListSize(), TabsListSize() + GSIW__TOTAL - 1);
		}
		else if (code == KEY_ENTER)
		{
			OnSetSortCol();
		}
	}
}

const WLabelWSize *CNeoPanel_ServerBrowser::BottomSectionList() const
{
	CNeoDataSettings_Base *tab = const_cast<CNeoPanel_ServerBrowser *>(this)->TabsList()[m_iNdsCurrent];
	if (m_iNdvMainFocus >= 0 && m_iNdvMainFocus < tab->NdvListSize())
	{
		static constexpr WLabelWSize BBTN_WITHFOCUS[BBTN__TOTAL] = {
			LWS(L"Back (ESC)"), LWS(L"Legacy"), LWS(L"Details"), LWS(L"Refresh (Ctrl+R)"), LWS(L"Enter"),
		};
		return BBTN_WITHFOCUS;
	}
	static constexpr WLabelWSize BBTN_NAMES[BBTN__TOTAL] = {
		LWS(L"Back (ESC)"), LWS(L"Legacy"), LWSNULL, LWS(L"Refresh (Ctrl+R)"), LWSNULL,
	};
	return BBTN_NAMES;
}

int CNeoPanel_ServerBrowser::KeyCodeToBottomAction(vgui::KeyCode code) const
{
	if (code == KEY_ESCAPE)
	{
		return BBTN_BACK;
	}
	else if (code == KEY_R && (input()->IsKeyDown(KEY_LCONTROL) || input()->IsKeyDown(KEY_RCONTROL)))
	{
		return BBTN_REFRESH;
	}
	return -1;
}

void CNeoPanel_ServerBrowser::TopAreaPaint()
{
	if (m_iNdsCurrent == TAB_FILTERS)
	{
		return;
	}

	const int iFontTall = surface()->GetFontTall(g_neoFont);
	const int iFontStartYPos = (g_iRowTall / 2) - (iFontTall / 2);

	static constexpr WLabelWSize SBLABEL_NAMES[GSIW__TOTAL] = {
		LWS(L"Lock"), LWS(L"VAC"), LWS(L"Name"), LWS(L"Map"), LWS(L"Players"), LWS(L"Ping"),
	};
	GameServerInfoW eColHover = GSIW__TOTAL;
	if (m_eSectionActive == SECTION_TOP && m_iNdvHover >= TabsListSize())
	{
		const int iColHover = m_iNdvHover - TabsListSize();
		if (iColHover >= 0 && iColHover < GSIW__TOTAL)
		{
			eColHover = static_cast<GameServerInfoW>(iColHover);
		}
	}

	for (int i = 0, xPos = 0; i < GSIW__TOTAL; ++i)
	{
		if (m_sortCtx.col == i || eColHover == i)
		{
			const int xPosEnd = xPos + g_iGSIX[i];
			const int yPosEnd = g_iRowTall + g_iRowTall;
			int iHintTall = g_iMarginY / 3;
			if (iHintTall <= 0) iHintTall = 1;

			// Background color
			surface()->DrawSetColor((eColHover == i) ? COLOR_NEOPANELSELECTBG : COLOR_NEOPANELACCENTBG);
			surface()->DrawFilledRect(xPos, g_iRowTall, xPosEnd, yPosEnd);

			if (m_sortCtx.col == i)
			{
				// Ascending/descending hint
				surface()->DrawSetColor(COLOR_NEOPANELTEXTNORMAL);
				if (!m_sortCtx.bDescending)	surface()->DrawFilledRect(xPos, g_iRowTall, xPosEnd, g_iRowTall + iHintTall);
				else						surface()->DrawFilledRect(xPos, yPosEnd - iHintTall, xPosEnd, yPosEnd);
			}
		}
		surface()->DrawSetTextPos(xPos + g_iMarginX, g_iRowTall + iFontStartYPos);
		surface()->DrawPrintText(SBLABEL_NAMES[i].text, SBLABEL_NAMES[i].size);
		xPos += g_iGSIX[i];
	}
	surface()->DrawSetColor(COLOR_NEOPANELNORMALBG);
}

void CNeoPanel_ServerBrowser::OnSetSortCol()
{
	const int iSortCol = (m_iNdvHover - TabsListSize());
	if (iSortCol >= 0 && iSortCol < GSIW__TOTAL)
	{
		if (m_sortCtx.col == iSortCol)
		{
			m_sortCtx.bDescending = !m_sortCtx.bDescending;
		}
		m_sortCtx.col = static_cast<GameServerInfoW>(iSortCol);
		m_bModified = true;
	}
}

void CNeoPanel_ServerBrowser::OnMousePressed(vgui::MouseCode code)
{
	if (m_iNdsCurrent != TAB_FILTERS && m_eSectionActive == SECTION_TOP)
	{
		OnSetSortCol();
	}
	BaseClass::OnMousePressed(code);
}

void CNeoPanel_ServerBrowser::OnCursorMovedTopArea(int x, int y)
{
	if (m_iNdsCurrent == TAB_FILTERS || m_eSectionActive != SECTION_TOP)
	{
		return;
	}

	for (int i = 0, xPos = 0; i < GSIW__TOTAL; ++i)
	{
		if (x >= xPos && x < (xPos + g_iGSIX[i]))
		{
			m_iNdvHover = i + TabsListSize();
			return;
		}
		xPos += g_iGSIX[i];
	}
}

void CNeoPanel_ServerBrowser::OnTick()
{
	if (m_bModified)
	{
		// Pass modified over to the tabs so it doesn't trigger
		// the filter refresh immeditely
		for (int i = 0; i < GS__TOTAL; ++i)
		{
			m_ndsGeneral[i].m_bModified = true;
		}
		m_bModified = false;
	}

	if (m_ndsGeneral[m_iNdsCurrent].m_bModified)
	{
		m_ndsGeneral[m_iNdsCurrent].UpdateFilteredList();
		m_ndsGeneral[m_iNdsCurrent].m_bModified = false;
	}
}

void NeoSettingsInit(NeoSettings *ns)
{
	int iNativeWidth, iNativeHeight;
	gameuifuncs->GetDesktopResolution(iNativeWidth, iNativeHeight);
	static constexpr int DISP_SIZE = sizeof("(#######x#######) (Native)");
	NeoSettings::Video *pVideo = &ns->video;
	gameuifuncs->GetVideoModes(&pVideo->vmList, &pVideo->iVMListSize);
	pVideo->p2WszVmDispList = (wchar_t **)calloc(sizeof(wchar_t *), pVideo->iVMListSize);
	pVideo->p2WszVmDispList[0] = (wchar_t *)calloc(sizeof(wchar_t) * DISP_SIZE, pVideo->iVMListSize);
	for (int i = 0, offset = 0; i < pVideo->iVMListSize; ++i, offset += DISP_SIZE)
	{
		vmode_t *vm = &pVideo->vmList[i];
		swprintf(pVideo->p2WszVmDispList[0] + offset, DISP_SIZE - 1, L"%d x %d%ls",
				 vm->width, vm->height,
				 (iNativeWidth == vm->width && iNativeHeight == vm->height) ? L" (Native)" : L"");
		pVideo->p2WszVmDispList[i] = pVideo->p2WszVmDispList[0] + offset;
	}

	// TODO: Alt/secondary keybind, different way on finding keybind
	CUtlBuffer buf(0, 0, CUtlBuffer::TEXT_BUFFER | CUtlBuffer::READ_ONLY);
	if (filesystem->ReadFile("scripts/kb_act.lst", nullptr, buf))
	{
		characterset_t breakSet = {};
		breakSet.set[0] = '"';
		NeoSettings::Keys *keys = &ns->keys;
		keys->iBindsSize = 0;

		while (buf.IsValid() && keys->iBindsSize < ARRAYSIZE(keys->vBinds))
		{
			char szFirstCol[64];
			char szRawDispText[64];

			bool bIsOk = false;
			bIsOk = buf.ParseToken(&breakSet, szFirstCol, sizeof(szFirstCol));
			if (!bIsOk) break;
			bIsOk = buf.ParseToken(&breakSet, szRawDispText, sizeof(szRawDispText));
			if (!bIsOk) break;

			if (szFirstCol[0] == '\0') continue;

			wchar_t wszDispText[64];
			if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(szRawDispText))
			{
				V_wcscpy_safe(wszDispText, localizedWszStr);
			}
			else
			{
				g_pVGuiLocalize->ConvertANSIToUnicode(szRawDispText, wszDispText, sizeof(wszDispText));
			}

			const bool bIsBlank = V_strcmp(szFirstCol, "blank") == 0;
			if (bIsBlank && szRawDispText[0] != '=')
			{
				// This is category label
				auto *bind = &keys->vBinds[keys->iBindsSize++];
				bind->szBindingCmd[0] = '\0';
				V_wcscpy_safe(bind->wszDisplayText, wszDispText);
			}
			else if (!bIsBlank)
			{
				// This is a keybind
				auto *bind = &keys->vBinds[keys->iBindsSize++];
				V_strcpy_safe(bind->szBindingCmd, szFirstCol);
				V_wcscpy_safe(bind->wszDisplayText, wszDispText);
			}
		}
	}
}

void NeoSettingsDeinit(NeoSettings *ns)
{
	free(ns->video.p2WszVmDispList[0]);
	free(ns->video.p2WszVmDispList);
}

void NeoSettingsRestore(NeoSettings *ns)
{
	ns->bModified = false;
	NeoSettings::CVR *cvr = &ns->cvr;
	{
		NeoSettings::General *pGeneral = &ns->general;
		g_pVGuiLocalize->ConvertANSIToUnicode(neo_name.GetString(), pGeneral->wszNeoName, sizeof(pGeneral->wszNeoName));
		pGeneral->bOnlySteamNick = cl_onlysteamnick.GetBool();
		pGeneral->flFov = neo_fov.GetInt();
		pGeneral->flViewmodelFov = neo_viewmodel_fov_offset.GetInt();
		pGeneral->bAimHold = neo_aim_hold.GetBool();
		pGeneral->bReloadEmpty = cl_autoreload_when_empty.GetBool();
		pGeneral->bViewmodelRighthand = cl_righthand.GetBool();
		pGeneral->bShowPlayerSprays = !(cvr->cl_player_spray_disable.GetBool()); // Inverse
		pGeneral->bShowPos = cl_showpos.GetBool();
		pGeneral->iShowFps = cl_showfps.GetInt();
		{
			const char *szDlFilter = cvr->cl_download_filter.GetString();
			pGeneral->iDlFilter = 0;
			for (int i = 0; i < DLFILTER_SIZE; ++i)
			{
				if (V_strcmp(szDlFilter, DLFILTER_STRMAP[i]) == 0)
				{
					pGeneral->iDlFilter = i;
					break;
				}
			}
		}
	}
	{
		NeoSettings::Keys *pKeys = &ns->keys;
		pKeys->bWeaponFastSwitch = hud_fastswitch.GetBool();
		pKeys->bDeveloperConsole = (gameuifuncs->GetButtonCodeForBind("toggleconsole") > KEY_NONE);
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			auto *bind = &pKeys->vBinds[i];
			bind->bcNext = bind->bcCurrent = gameuifuncs->GetButtonCodeForBind(bind->szBindingCmd);
		}
	}
	{
		NeoSettings::Mouse *pMouse = &ns->mouse;
		pMouse->flSensitivity = sensitivity.GetFloat();
		pMouse->bRawInput = cvr->m_raw_input.GetBool();
		pMouse->bFilter = cvr->m_filter.GetBool();
		pMouse->bReverse = (cvr->pitch.GetFloat() < 0.0f);
		pMouse->bCustomAccel = (cvr->m_customaccel.GetInt() == 3);
		pMouse->flExponent = cvr->m_customaccel_exponent.GetFloat();
	}
	{
		NeoSettings::Audio *pAudio = &ns->audio;

		// Output
		pAudio->flVolMain = cvr->volume.GetFloat();
		pAudio->flVolMusic = cvr->snd_musicvolume.GetFloat();
		pAudio->flVolVictory = snd_victory_volume.GetFloat();
		pAudio->iSoundSetup = 0;
		switch (cvr->snd_surround_speakers.GetInt())
		{
		break; case 0: pAudio->iSoundSetup = 1;
		break; case 2: pAudio->iSoundSetup = 2;
	#ifndef LINUX
		break; case 4: pAudio->iSoundSetup = 3;
		break; case 5: pAudio->iSoundSetup = 4;
		break; case 7: pAudio->iSoundSetup = 5;
	#endif
		break; default: break;
		}
		pAudio->bMuteAudioUnFocus = cvr->snd_mute_losefocus.GetBool();

		// Sound quality:  snd_pitchquality   dsp_slow_cpu
		// High                   1                0
		// Medium                 0                0
		// Low                    0                1
		pAudio->iSoundQuality =	(cvr->snd_pitchquality.GetBool()) 	? QUALITY_HIGH :
								(cvr->dsp_slow_cpu.GetBool()) 		? QUALITY_LOW :
																	  QUALITY_MEDIUM;

		// Input
		pAudio->bVoiceEnabled = cvr->voice_enable.GetBool();
		pAudio->flVolVoiceRecv = cvr->voice_scale.GetFloat();
		pAudio->bMicBoost = (engine->GetVoiceTweakAPI()->GetControlFloat(MicBoost) != 0.0f);
	}
	{
		NeoSettings::Video *pVideo = &ns->video;

		int iScreenWidth, iScreenHeight;
		engine->GetScreenSize(iScreenWidth, iScreenHeight); // Or: g_pMaterialSystem->GetDisplayMode?
		pVideo->iResolution = pVideo->iVMListSize - 1;
		for (int i = 0; i < pVideo->iVMListSize; ++i)
		{
			vmode_t *vm = &pVideo->vmList[i];
			if (vm->width == iScreenWidth && vm->height == iScreenHeight)
			{
				pVideo->iResolution = i;
				break;
			}
		}

		pVideo->iWindow = static_cast<int>(g_pMaterialSystem->GetCurrentConfigForVideoCard().Windowed());
		const int queueMode = cvr->mat_queue_mode.GetInt();
		pVideo->iCoreRendering = (queueMode == -1 || queueMode == 2) ? THREAD_MULTI : THREAD_SINGLE;
		pVideo->iModelDetail = 2 - cvr->r_rootlod.GetInt(); // Inverse, highest = 0, lowest = 2
		pVideo->iTextureDetail = 3 - (cvr->mat_picmip.GetInt() + 1); // Inverse+1, highest = -1, lowest = 2
		pVideo->iShaderDetail = 1 - cvr->mat_reducefillrate.GetInt(); // Inverse, 1 = low, 0 = high
		// Water detail
		//                r_waterforceexpensive        r_waterforcereflectentities
		// Simple:                  0                              0
		// Reflect World:           1                              0
		// Reflect all:             1                              1
		pVideo->iWaterDetail = 	(cvr->r_waterforcereflectentities.GetBool()) 	? QUALITY_HIGH :
								(cvr->r_waterforceexpensive.GetBool()) 			? QUALITY_MEDIUM :
																				  QUALITY_LOW;

		// Shadow detail
		//         r_flashlightdepthtexture     r_shadowrendertotexture
		// Low:              0                            0
		// Medium:           0                            1
		// High:             1                            1
		pVideo->iShadowDetail =	(cvr->r_flashlightdepthtexture.GetBool()) 		? QUALITY_HIGH :
								(cvr->r_shadowrendertotexture.GetBool()) 		? QUALITY_MEDIUM :
																				  QUALITY_LOW;

		pVideo->bColorCorrection = cvr->mat_colorcorrection.GetBool();
		pVideo->iAntiAliasing = (cvr->mat_antialias.GetInt() / 2); // MSAA: Times by 2

		// Filtering mode
		// mat_trilinear: 0 = bilinear, 1 = trilinear (both: mat_forceaniso 1)
		// mat_forceaniso: Antisotropic 2x/4x/8x/16x (all aniso: mat_trilinear 0)
		int filterIdx = 0;
		if (cvr->mat_forceaniso.GetInt() < 2)
		{
			filterIdx = cvr->mat_trilinear.GetBool() ? FILTERING_TRILINEAR : FILTERING_BILINEAR;
		}
		else
		{
			switch (cvr->mat_forceaniso.GetInt())
			{
			break; case 2: filterIdx = FILTERING_ANISO2X;
			break; case 4: filterIdx = FILTERING_ANISO4X;
			break; case 8: filterIdx = FILTERING_ANISO8X;
			break; case 16: filterIdx = FILTERING_ANISO16X;
			break; default: filterIdx = FILTERING_ANISO4X; // Some invalid number, just set to 4X (idx 3)
			}
		}
		pVideo->iFilteringMode = filterIdx;
		pVideo->bVSync = cvr->mat_vsync.GetBool();
		pVideo->bMotionBlur = cvr->mat_motion_blur_enabled.GetBool();
		pVideo->iHDR = cvr->mat_hdr_level.GetInt();
		pVideo->flGamma = cvr->mat_monitorgamma.GetFloat();
	}
}

void NeoSettingsSave(const NeoSettings *ns)
{
	const_cast<NeoSettings *>(ns)->bModified = false;
	auto *cvr = const_cast<NeoSettings::CVR *>(&ns->cvr);
	{
		const NeoSettings::General *pGeneral = &ns->general;
		char neoNameText[sizeof(pGeneral->wszNeoName) / sizeof(wchar_t)];
		g_pVGuiLocalize->ConvertUnicodeToANSI(pGeneral->wszNeoName, neoNameText, sizeof(neoNameText));
		neo_name.SetValue(neoNameText);
		cl_onlysteamnick.SetValue(pGeneral->bOnlySteamNick);
		neo_fov.SetValue(static_cast<int>(pGeneral->flFov));
		neo_viewmodel_fov_offset.SetValue(static_cast<int>(pGeneral->flViewmodelFov));
		neo_aim_hold.SetValue(pGeneral->bAimHold);
		cl_autoreload_when_empty.SetValue(pGeneral->bReloadEmpty);
		cl_righthand.SetValue(pGeneral->bViewmodelRighthand);
		cvr->cl_player_spray_disable.SetValue(!pGeneral->bShowPlayerSprays); // Inverse
		cl_showpos.SetValue(pGeneral->bShowPos);
		cl_showfps.SetValue(pGeneral->iShowFps);
		cvr->cl_download_filter.SetValue(DLFILTER_STRMAP[pGeneral->iDlFilter]);
	}
	{
		const NeoSettings::Keys *pKeys = &ns->keys;
		hud_fastswitch.SetValue(pKeys->bWeaponFastSwitch);
		{
			char cmdStr[128];
			V_sprintf_safe(cmdStr, "unbind \"`\"\n");
			engine->ClientCmd_Unrestricted(cmdStr);

			if (pKeys->bDeveloperConsole)
			{
				V_sprintf_safe(cmdStr, "bind \"`\" \"toggleconsole\"\n");
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			const auto *bind = &pKeys->vBinds[i];
			if (bind->szBindingCmd[0] != '\0')
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcCurrent);
				V_sprintf_safe(cmdStr, "unbind \"%s\"\n", bindBtnName);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
		for (int i = 0; i < pKeys->iBindsSize; ++i)
		{
			const auto *bind = &pKeys->vBinds[i];
			if (bind->szBindingCmd[0] != '\0')
			{
				char cmdStr[128];
				const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind->bcNext);
				V_sprintf_safe(cmdStr, "bind \"%s\" \"%s\"\n", bindBtnName, bind->szBindingCmd);
				engine->ClientCmd_Unrestricted(cmdStr);
			}
		}
	}
	{
		const NeoSettings::Mouse *pMouse = &ns->mouse;
		sensitivity.SetValue(pMouse->flSensitivity);
		cvr->m_raw_input.SetValue(pMouse->bRawInput);
		cvr->m_filter.SetValue(pMouse->bFilter);
		const float absPitch = abs(cvr->pitch.GetFloat());
		cvr->pitch.SetValue(pMouse->bReverse ? -absPitch : absPitch);
		cvr->m_customaccel.SetValue(pMouse->bCustomAccel ? 3 : 0);
		cvr->m_customaccel_exponent.SetValue(pMouse->flExponent);
	}
	{
		const NeoSettings::Audio *pAudio = &ns->audio;

		static constexpr int SURROUND_RE_MAP[] = {
			-1, 0, 2,
#ifndef LINUX
			4, 5, 7
#endif
		};

		// Output
		cvr->volume.SetValue(pAudio->flVolMain);
		cvr->snd_musicvolume.SetValue(pAudio->flVolMusic);
		snd_victory_volume.SetValue(pAudio->flVolVictory);
		cvr->snd_surround_speakers.SetValue(SURROUND_RE_MAP[pAudio->iSoundSetup]);
		cvr->snd_mute_losefocus.SetValue(pAudio->bMuteAudioUnFocus);
		cvr->snd_pitchquality.SetValue(pAudio->iSoundQuality == QUALITY_HIGH);
		cvr->dsp_slow_cpu.SetValue(pAudio->iSoundQuality == QUALITY_LOW);

		// Input
		cvr->voice_enable.SetValue(pAudio->bVoiceEnabled);
		cvr->voice_scale.SetValue(pAudio->flVolVoiceRecv);
		engine->GetVoiceTweakAPI()->SetControlFloat(MicBoost, static_cast<float>(pAudio->bMicBoost));
	}
	{
		const NeoSettings::Video *pVideo = &ns->video;

		const int resIdx = pVideo->iResolution;
		if (resIdx >= 0 && resIdx < pVideo->iVMListSize)
		{
			// mat_setvideomode [width] [height] [mode] | mode: 0 = fullscreen, 1 = windowed
			vmode_t *vm = &pVideo->vmList[resIdx];
			char cmdStr[128];
			V_sprintf_safe(cmdStr, "mat_setvideomode %d %d %d", vm->width, vm->height, pVideo->iWindow);
			engine->ClientCmd_Unrestricted(cmdStr);
		}
		cvr->mat_queue_mode.SetValue((pVideo->iCoreRendering == THREAD_MULTI) ? 2 : 0);
		cvr->r_rootlod.SetValue(2 - pVideo->iModelDetail);
		cvr->mat_picmip.SetValue(2 - pVideo->iTextureDetail);
		cvr->mat_reducefillrate.SetValue(1 - pVideo->iShaderDetail);
		cvr->r_waterforceexpensive.SetValue(pVideo->iWaterDetail >= QUALITY_MEDIUM);
		cvr->r_waterforcereflectentities.SetValue(pVideo->iWaterDetail == QUALITY_HIGH);
		cvr->r_shadowrendertotexture.SetValue(pVideo->iShadowDetail >= QUALITY_MEDIUM);
		cvr->r_flashlightdepthtexture.SetValue(pVideo->iShadowDetail == QUALITY_HIGH);
		cvr->mat_colorcorrection.SetValue(pVideo->bColorCorrection);
		cvr->mat_antialias.SetValue(pVideo->iAntiAliasing * 2);
		cvr->mat_trilinear.SetValue(pVideo->iFilteringMode == FILTERING_TRILINEAR);
		static constexpr int ANISO_MAP[FILTERING__TOTAL] = {
			1, 1, 2, 4, 8, 16
		};
		cvr->mat_forceaniso.SetValue(ANISO_MAP[pVideo->iFilteringMode]);
		cvr->mat_vsync.SetValue(pVideo->bVSync);
		cvr->mat_motion_blur_enabled.SetValue(pVideo->bMotionBlur);
		cvr->mat_hdr_level.SetValue(pVideo->iHDR);
		cvr->mat_monitorgamma.SetValue(pVideo->flGamma);
	}
}

void NeoSettingsMainLoop(NeoSettings *ns, const NeoUI::Mode eMode)
{
	static constexpr void (*P_FN[])(NeoSettings *) = {
		NeoSettings_General,
		NeoSettings_Keys,
		NeoSettings_Mouse,
		NeoSettings_Audio,
		NeoSettings_Video,
	};
	static const wchar_t *WSZ_TABS_LABELS[ARRAYSIZE(P_FN)] = {
		L"Multiplayer", L"Keybinds", L"Mouse", L"Audio", L"Video"
	};

	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	const int iTallTotal = g_iRowTall * (g_iRowsInScreen + 2);

	g_ctx.dPanel.wide = g_iRootSubPanelWide;
	g_ctx.dPanel.x = (wide / 2) - (g_iRootSubPanelWide / 2);
	g_ctx.dPanel.y = (tall / 2) - (iTallTotal / 2);
	g_ctx.dPanel.tall = g_iRowTall;
	g_ctx.bgColor = COLOR_NEOPANELFRAMEBG;
	NeoUI::BeginContext(eMode);
	{
		NeoUI::BeginSection();
		{
			NeoUI::Tabs(WSZ_TABS_LABELS, ARRAYSIZE(WSZ_TABS_LABELS), &ns->iCurTab);
		}
		NeoUI::EndSection();
		g_ctx.dPanel.y += g_ctx.dPanel.tall;
		g_ctx.dPanel.tall = g_iRowTall * g_iRowsInScreen;
		NeoUI::BeginSection(true);
		{
			P_FN[ns->iCurTab](ns);
		}
		NeoUI::EndSection();
		g_ctx.dPanel.y += g_ctx.dPanel.tall;
		g_ctx.dPanel.tall = g_iRowTall;
		NeoUI::BeginSection();
		{
			NeoUI::BeginHorizontal(g_ctx.dPanel.wide / 5);
			{
				if (NeoUI::Button(nullptr, L"Back (ESC)").bPressed)
				{
					ns->bBack = true;
				}
				if (NeoUI::Button(nullptr, L"Legacy").bPressed)
				{
					g_pNeoRoot->GetGameUI()->SendMainMenuCommand("OpenOptionsDialog");
				}
				NeoUI::Pad();
				if (ns->bModified)
				{
					if (NeoUI::Button(nullptr, L"Restore").bPressed)
					{
						NeoSettingsRestore(ns);
					}
					if (NeoUI::Button(nullptr, L"Accept").bPressed)
					{
						NeoSettingsSave(ns);
					}
				}
			}
			NeoUI::EndHorizontal();
		}
		NeoUI::EndSection();
	}
	NeoUI::EndContext();
	if (!ns->bModified && g_ctx.bValueEdited)
	{
		ns->bModified = true;
	}
}

void NeoSettings_General(NeoSettings *ns)
{
	NeoSettings::General *pGeneral = &ns->general;
	NeoUI::Label(L"TODO: neo_name");
	NeoUI::RingBoxBool(L"Show only steam name", &pGeneral->bOnlySteamNick);
	wchar_t wszDisplayName[128];
	const bool bShowSteamNick = pGeneral->bOnlySteamNick || pGeneral->wszNeoName[0] == '\0';
	(bShowSteamNick) ? V_swprintf_safe(wszDisplayName, L"Display name: %s", steamapicontext->SteamFriends()->GetPersonaName())
					 : V_swprintf_safe(wszDisplayName, L"Display name: %ls", pGeneral->wszNeoName);
	NeoUI::Label(wszDisplayName);
	NeoUI::Slider(L"FOV", &pGeneral->flFov, 75.0f, 110.0f, 0);
	NeoUI::Slider(L"Viewmodel FOV Offset", &pGeneral->flViewmodelFov, -20.0f, 40.0f, 0);
	NeoUI::RingBoxBool(L"Aim hold", &pGeneral->bAimHold);
	NeoUI::RingBoxBool(L"Reload empty", &pGeneral->bReloadEmpty);
	NeoUI::RingBoxBool(L"Right hand viewmodel", &pGeneral->bViewmodelRighthand);
	NeoUI::RingBoxBool(L"Show player spray", &pGeneral->bShowPlayerSprays);
	NeoUI::RingBoxBool(L"Show position", &pGeneral->bShowPos);
	NeoUI::RingBox(L"Show FPS", SHOWFPS_LABELS, ARRAYSIZE(SHOWFPS_LABELS), &pGeneral->iShowFps);
	NeoUI::RingBox(L"Download filter", DLFILTER_LABELS, DLFILTER_SIZE, &pGeneral->iDlFilter);
}

void NeoSettings_Keys(NeoSettings *ns)
{
	NeoSettings::Keys *pKeys = &ns->keys;
	NeoUI::RingBoxBool(L"Weapon fastswitch", &pKeys->bWeaponFastSwitch);
	NeoUI::RingBoxBool(L"Developer console", &pKeys->bDeveloperConsole);
	for (int i = 0; i < pKeys->iBindsSize; ++i)
	{
		const auto &bind = pKeys->vBinds[i];
		if (bind.szBindingCmd[0] == '\0')
		{
			NeoUI::Label(bind.wszDisplayText, true);
		}
		else
		{
			wchar_t wszBindBtnName[64];
			const char *bindBtnName = g_pInputSystem->ButtonCodeToString(bind.bcNext);
			g_pVGuiLocalize->ConvertANSIToUnicode(bindBtnName, wszBindBtnName, sizeof(wszBindBtnName));
			if (NeoUI::Button(bind.wszDisplayText, wszBindBtnName).bPressed)
			{
				// TODO
			}
		}
	}
}

void NeoSettings_Mouse(NeoSettings *ns)
{
	NeoSettings::Mouse *pMouse = &ns->mouse;
	NeoUI::Slider(L"Sensitivity", &pMouse->flSensitivity, 0.1f, 10.0f, 2, 0.25f);
	NeoUI::RingBoxBool(L"Raw input", &pMouse->bRawInput);
	NeoUI::RingBoxBool(L"Mouse Filter", &pMouse->bFilter);
	NeoUI::RingBoxBool(L"Mouse Reverse", &pMouse->bReverse);
	NeoUI::RingBoxBool(L"Custom Acceleration", &pMouse->bCustomAccel);
	NeoUI::Slider(L"Exponent", &pMouse->flExponent, 1.0f, 1.4f, 2, 0.1f);
}

void NeoSettings_Audio(NeoSettings *ns)
{
	NeoSettings::Audio *pAudio = &ns->audio;
	NeoUI::Slider(L"Main Volume", &pAudio->flVolMain, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Music Volume", &pAudio->flVolMusic, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::Slider(L"Victory Volume", &pAudio->flVolVictory, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBox(L"Sound Setup", SPEAKER_CFG_LABELS, ARRAYSIZE(SPEAKER_CFG_LABELS), &pAudio->iSoundSetup);
	NeoUI::RingBox(L"Sound Quality", QUALITY_LABELS, 3, &pAudio->iSoundQuality);
	NeoUI::RingBoxBool(L"Mute Audio on un-focus", &pAudio->bMuteAudioUnFocus);
	NeoUI::RingBoxBool(L"Voice Enabled", &pAudio->bVoiceEnabled);
	NeoUI::Slider(L"Voice Receive", &pAudio->flVolVoiceRecv, 0.0f, 1.0f, 2, 0.1f);
	NeoUI::RingBoxBool(L"Microphone Boost", &pAudio->bMicBoost);
	IVoiceTweak_s *pVoiceTweak = engine->GetVoiceTweakAPI();
	const bool bTweaking = pVoiceTweak->IsStillTweaking();
	if (NeoUI::Button(L"Microphone Tester",
					  bTweaking ? L"Stop testing" : L"Start testing").bPressed)
	{
		bTweaking ? pVoiceTweak->EndVoiceTweakMode() : (void)pVoiceTweak->StartVoiceTweakMode();
	}
	if (bTweaking && g_ctx.eMode == NeoUI::MODE_PAINT)
	{
		const float flSpeakingVol = pVoiceTweak->GetControlFloat(SpeakingVolume);
		surface()->DrawSetColor(COLOR_NEOPANELMICTEST);
		GCtxDrawFilledRectXtoX(0, flSpeakingVol * g_ctx.dPanel.wide);
		g_ctx.iLayoutY += g_iRowTall;
		surface()->DrawSetColor(COLOR_NEOPANELACCENTBG);
	}
}

void NeoSettings_Video(NeoSettings *ns)
{
	NeoSettings::Video *pVideo = &ns->video;
	NeoUI::RingBox(L"Resolution", const_cast<const wchar_t **>(pVideo->p2WszVmDispList), pVideo->iVMListSize, &pVideo->iResolution);
	NeoUI::RingBox(L"Window", WINDOW_MODE, ARRAYSIZE(WINDOW_MODE), &pVideo->iWindow);
	NeoUI::RingBox(L"Core Rendering", QUEUE_MODE, ARRAYSIZE(QUEUE_MODE), &pVideo->iCoreRendering);
	NeoUI::RingBox(L"Model detail", QUALITY_LABELS, 3, &pVideo->iModelDetail);
	NeoUI::RingBox(L"Texture detail", QUALITY_LABELS, 4, &pVideo->iTextureDetail);
	NeoUI::RingBox(L"Shader detail", QUALITY2_LABELS, 2, &pVideo->iShaderDetail);
	NeoUI::RingBox(L"Water detail", WATER_LABELS, ARRAYSIZE(WATER_LABELS), &pVideo->iWaterDetail);
	NeoUI::RingBox(L"Shadow detail", QUALITY_LABELS, 3, &pVideo->iShadowDetail);
	NeoUI::RingBoxBool(L"Color correction", &pVideo->bColorCorrection);
	NeoUI::RingBox(L"Anti-aliasing", MSAA_LABELS, ARRAYSIZE(MSAA_LABELS), &pVideo->iAntiAliasing);
	NeoUI::RingBox(L"Filtering mode", FILTERING_LABELS, FILTERING__TOTAL, &pVideo->iFilteringMode);
	NeoUI::RingBoxBool(L"V-Sync", &pVideo->bVSync);
	NeoUI::RingBoxBool(L"Motion blur", &pVideo->bMotionBlur);
	NeoUI::RingBox(L"HDR", HDR_LABELS, ARRAYSIZE(HDR_LABELS), &pVideo->iHDR);
	NeoUI::Slider(L"Gamma", &pVideo->flGamma, 1.6, 2.6, 2, 0.1f);
}

///////
// MAIN ROOT PANEL
///////

CNeoRootInput::CNeoRootInput(CNeoRoot *rootPanel)
	: Panel(rootPanel, "NeoRootPanelInput")
	, m_pNeoRoot(rootPanel)
{
	MakePopup(true);
	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(false);
	SetVisible(true);
	SetEnabled(true);
	PerformLayout();
}

void CNeoRootInput::PerformLayout()
{
	SetPos(0, 0);
	SetSize(1, 1);
	SetBgColor(COLOR_TRANSPARENT);
	SetFgColor(COLOR_TRANSPARENT);
}

void CNeoRootInput::OnKeyCodeTyped(vgui::KeyCode code)
{
	m_pNeoRoot->OnRelayedKeyCodeTyped(code);
}

CNeoRoot::CNeoRoot(VPANEL parent)
	: EditablePanel(nullptr, "NeoRootPanel")
	, m_panelCaptureInput(new CNeoRootInput(this))
{
	SetParent(parent);
	g_pNeoRoot = this;
	LoadGameUI();
	SetVisible(true);
	SetProportional(false);

	vgui::HScheme neoscheme = vgui::scheme()->LoadSchemeFromFileEx(
		enginevgui->GetPanel(PANEL_CLIENTDLL), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(neoscheme);

	for (int i = 0; i < BTNS_TOTAL; ++i)
	{
		const char *label = BTNS_INFO[i].label;
		if (wchar_t *localizedWszStr = g_pVGuiLocalize->Find(label))
		{
			V_wcsncpy(m_wszDispBtnTexts[i], localizedWszStr, sizeof(m_wszDispBtnTexts[i]));
		}
		else
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(label, m_wszDispBtnTexts[i], sizeof(m_wszDispBtnTexts[i]));
		}
		m_iWszDispBtnTextsSizes[i] = V_wcslen(m_wszDispBtnTexts[i]);
	}

	NeoSettingsInit(&m_ns);

	SetKeyBoardInputEnabled(true);
	SetMouseInputEnabled(true);
	UpdateControls();

	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme(neoscheme);
	ApplySchemeSettings(pScheme);
}

CNeoRoot::~CNeoRoot()
{
	m_panelCaptureInput->DeletePanel();
	if (m_avImage) delete m_avImage;
	NeoSettingsDeinit(&m_ns);

	m_gameui = nullptr;
	g_GameUIDLL.Unload();
}

IGameUI *CNeoRoot::GetGameUI()
{
	if (!m_gameui && !LoadGameUI()) return nullptr;
	return m_gameui;
}

void CNeoRoot::UpdateControls()
{
	int wide, tall;
	GetSize(wide, tall);
	if (m_state == STATE_ROOT)
	{
		const int yTopPos = tall / 2 - ((g_iRowTall * BTNS_TOTAL) / 2);

		int iTitleWidth, iTitleHeight;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_LOGO]);
		surface()->GetTextSize(m_hTextFonts[FONT_LOGO], WSZ_GAME_TITLE, iTitleWidth, iTitleHeight);

		g_ctx.dPanel.wide = iTitleWidth + (2 * g_iMarginX);
		g_ctx.dPanel.tall = tall;
		g_ctx.dPanel.x = (wide / 4) - (g_ctx.dPanel.wide / 2);
		g_ctx.dPanel.y = yTopPos;
		g_ctx.bgColor = COLOR_TRANSPARENT;
	}
	g_ctx.iFocusDirection = 0;
	g_ctx.iFocus = NeoUI::FOCUSOFF_NUM;
	g_ctx.iFocusSection = -1;
	m_ns.bBack = false;
	RequestFocus();
	m_panelCaptureInput->RequestFocus();
	InvalidateLayout();
}

bool CNeoRoot::LoadGameUI()
{
	if (!m_gameui)
	{
		CreateInterfaceFn gameUIFactory = g_GameUIDLL.GetFactory();
		if (!gameUIFactory) return false;

		m_gameui = reinterpret_cast<IGameUI *>(gameUIFactory(GAMEUI_INTERFACE_VERSION, nullptr));
		if (!m_gameui) return false;
	}
	return true;
}

void CNeoRoot::ApplySchemeSettings(IScheme *pScheme)
{
	BaseClass::ApplySchemeSettings(pScheme);
	// NEO TODO (nullsystem): If we're to provide color scheme controls:
	// LoadControlSettings("resource/NeoRootScheme.res");

	// Resize the panel to the screen size
	int wide, tall;
	surface()->GetScreenSize(wide, tall);
	SetSize(wide, tall);
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[FONT__TOTAL] = {
		"NHudOCR", "NHudOCRSmallNoAdditive", "ClientTitleFont"
	};
	for (int i = 0; i < FONT__TOTAL; ++i)
	{
		m_hTextFonts[i] = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// In 1080p, g_iRowTall == 40, g_iMarginX = 10, g_iAvatar = 64,
	// other resolution scales up/down from it
	g_iRowTall = tall / 27;
	g_iRowsInScreen = (tall * 0.85f) / g_iRowTall;
	g_iMarginX = wide / 192;
	g_iMarginY = tall / 108;
	g_iAvatar = wide / 30;
	const float flWide = static_cast<float>(wide);
	float flWideAs43 = static_cast<float>(tall) * (4.0f / 3.0f);
	if (flWideAs43 > flWide) flWideAs43 = flWide;
	g_iRootSubPanelWide = static_cast<int>(flWideAs43 * 0.9f);

	g_neoFont = m_hTextFonts[FONT_NTSMALL];

	constexpr int PARTITION = GSIW__TOTAL * 4;
	const int iSubDiv = g_iRootSubPanelWide / PARTITION;
	g_iGSIX[GSIW_LOCKED] = iSubDiv * 2;
	g_iGSIX[GSIW_VAC] = iSubDiv * 2;
	g_iGSIX[GSIW_NAME] = iSubDiv * 10;
	g_iGSIX[GSIW_MAP] = iSubDiv * 5;
	g_iGSIX[GSIW_PLAYERS] = iSubDiv * 3;
	g_iGSIX[GSIW_PING] = iSubDiv * 2;

	UpdateControls();
}

void CNeoRoot::Paint()
{
	OnMainLoop(NeoUI::MODE_PAINT);
}

void CNeoRoot::PaintRootMainSection()
{
	int wide, tall;
	GetSize(wide, tall);

	const int iBtnPlaceXMid = (wide / 4);

	const int yTopPos = tall / 2 - ((g_iRowTall * BTNS_TOTAL) / 2);
	const int iRightXPos = iBtnPlaceXMid + (m_iBtnWide / 2) + g_iMarginX;
	int iRightSideYStart = yTopPos;

	// Draw title
	int iTitleWidth, iTitleHeight;
	surface()->DrawSetTextFont(m_hTextFonts[FONT_LOGO]);
	surface()->GetTextSize(m_hTextFonts[FONT_LOGO], WSZ_GAME_TITLE, iTitleWidth, iTitleHeight);
	m_iBtnWide = iTitleWidth + (2 * g_iMarginX);

	surface()->DrawSetTextColor(COLOR_NEOTITLE);
	surface()->DrawSetTextPos(iBtnPlaceXMid - (iTitleWidth / 2), yTopPos - iTitleHeight);
	surface()->DrawPrintText(WSZ_GAME_TITLE, SZWSZ_LEN(WSZ_GAME_TITLE));

	surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
	ISteamUser *steamUser = steamapicontext->SteamUser();
	ISteamFriends *steamFriends = steamapicontext->SteamFriends();
	if (steamUser && steamFriends)
	{
		const int iSteamPlaceXStart = iRightXPos;

		// Draw player info (top left corner)
		const CSteamID steamID = steamUser->GetSteamID();
		if (!m_avImage)
		{
			m_avImage = new CAvatarImage;
			m_avImage->SetAvatarSteamID(steamID, k_EAvatarSize64x64);
		}
		m_avImage->SetPos(iSteamPlaceXStart + g_iMarginX, iRightSideYStart + g_iMarginY);
		m_avImage->SetSize(g_iAvatar, g_iAvatar);
		m_avImage->Paint();

		char szTextBuf[512] = {};
		const char *szSteamName = steamFriends->GetPersonaName();
		const char *szNeoName = neo_name.GetString();
		const bool bUseNeoName = (szNeoName && szNeoName[0] != '\0' && !cl_onlysteamnick.GetBool());
		V_sprintf_safe(szTextBuf, "%s", (bUseNeoName) ? szNeoName : szSteamName);

		wchar_t wszTextBuf[512] = {};
		g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

		int iMainTextWidth, iMainTextHeight;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
		surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], wszTextBuf, iMainTextWidth, iMainTextHeight);

		const int iMainTextStartPosX = g_iMarginX + g_iAvatar + g_iMarginX;
		surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX, iRightSideYStart + g_iMarginY);
		surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));

		if (bUseNeoName)
		{
			V_sprintf_safe(szTextBuf, "(Steam name: %s)", szSteamName);
			g_pVGuiLocalize->ConvertANSIToUnicode(szTextBuf, wszTextBuf, sizeof(wszTextBuf));

			int iSteamSubTextWidth, iSteamSubTextHeight;
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
			surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszTextBuf, iSteamSubTextWidth, iSteamSubTextHeight);

			const int iRightOfNicknameXPos = iSteamPlaceXStart + iMainTextStartPosX + iMainTextWidth + g_iMarginX;
			// If we have space on the right, set it, otherwise on top of nickname
			if ((iRightOfNicknameXPos + iSteamSubTextWidth) < wide)
			{
				surface()->DrawSetTextPos(iRightOfNicknameXPos, iRightSideYStart + g_iMarginY);
			}
			else
			{
				surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
										  iRightSideYStart - iSteamSubTextHeight);
			}
			surface()->DrawPrintText(wszTextBuf, V_strlen(szTextBuf));
		}

		static constexpr const wchar_t *WSZ_PERSONA_STATES[k_EPersonaStateMax] = {
			L"Offline", L"Online", L"Busy", L"Away", L"Snooze", L"Trading", L"Looking to play"
		};
		const auto eCurStatus = steamFriends->GetPersonaState();
		int iStatusTall = 0;
		if (eCurStatus != k_EPersonaStateMax)
		{
			const wchar_t *wszState = WSZ_PERSONA_STATES[static_cast<int>(eCurStatus)];
			const int iStatusTextStartPosY = g_iMarginY + iMainTextHeight + g_iMarginY;

			[[maybe_unused]] int iStatusWide;
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
			surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszState, iStatusWide, iStatusTall);
			surface()->DrawSetTextPos(iSteamPlaceXStart + iMainTextStartPosX,
									  iRightSideYStart + iStatusTextStartPosY);
			surface()->DrawPrintText(wszState, V_wcslen(wszState));
		}

		// Put the news title in either from avatar or text end Y position
		const int iTextTotalTall = iMainTextHeight + iStatusTall;
		iRightSideYStart += (g_iMarginX * 2) + ((iTextTotalTall > g_iAvatar) ? iTextTotalTall : g_iAvatar);
	}

	{
		// Show the news
		static constexpr wchar_t WSZ_NEWS_TITLE[] = L"News";

		int iMainTextWidth, iMainTextHeight;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
		surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], WSZ_NEWS_TITLE, iMainTextWidth, iMainTextHeight);
		surface()->DrawSetTextPos(iRightXPos, iRightSideYStart + g_iMarginY);
		surface()->DrawPrintText(WSZ_NEWS_TITLE, SZWSZ_LEN(WSZ_NEWS_TITLE));

		// Write some headlines
		static constexpr const wchar_t *WSZ_NEWS_HEADLINES[] = {
			L"2024-08-03: NT;RE v7.1 Released",
		};

		int iHlYPos = iRightSideYStart + (2 * g_iMarginY) + iMainTextHeight;
		for (const wchar_t *wszHeadline : WSZ_NEWS_HEADLINES)
		{
			int iHlTextWidth, iHlTextHeight;
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
			surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], wszHeadline, iHlTextWidth, iHlTextHeight);
			surface()->DrawSetTextPos(iRightXPos, iHlYPos);
			surface()->DrawPrintText(wszHeadline, V_wcslen(wszHeadline));

			iHlYPos += g_iMarginY + iHlTextHeight;
		}
	}
}

void CNeoRoot::OnMousePressed(vgui::MouseCode code)
{
	g_ctx.eCode = code;
	OnMainLoop(NeoUI::MODE_MOUSEPRESSED);
}

void CNeoRoot::OnMouseWheeled(int delta)
{
	g_ctx.eCode = (delta > 0) ? MOUSE_WHEEL_UP : MOUSE_WHEEL_DOWN;
	OnMainLoop(NeoUI::MODE_MOUSEWHEELED);
}

void CNeoRoot::OnCursorMoved(int x, int y)
{
	GCtxOnCursorMoved(x, y);
	OnMainLoop(NeoUI::MODE_MOUSEMOVED);
}

void CNeoRoot::OnRelayedKeyCodeTyped(vgui::KeyCode code)
{
	g_ctx.eCode = code;
	OnMainLoop(NeoUI::MODE_KEYPRESSED);
}

void CNeoRoot::OnMainLoop(const NeoUI::Mode eMode)
{
	int wide, tall;
	GetSize(wide, tall);

	if (eMode == NeoUI::MODE_PAINT)
	{
		// Draw version info (bottom left corner) - Always
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);
		int textWidth, textHeight;
		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
		surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], BUILD_DISPLAY, textWidth, textHeight);

		surface()->DrawSetTextPos(g_iMarginX, tall - textHeight - g_iMarginY);
		surface()->DrawPrintText(BUILD_DISPLAY, *BUILD_DISPLAY_SIZE);

#ifdef DEBUG
		surface()->DrawSetTextColor(Color(180, 180, 180, 120));
		wchar_t wszDebugInfo[512];
		const int iDebugInfoSize = V_swprintf_safe(wszDebugInfo, L"ABS: %d,%d | REL: %d,%d | IN PANEL: %s | PANELY: %d",
												   g_ctx.iMouseAbsX, g_ctx.iMouseAbsY,
												   g_ctx.iMouseRelX, g_ctx.iMouseRelY,
												   g_ctx.bMouseInPanel ? "TRUE" : "FALSE",
												   (g_ctx.iMouseRelY / g_iRowTall));

		surface()->DrawSetTextPos(g_iMarginX, g_iMarginY);
		surface()->DrawPrintText(wszDebugInfo, iDebugInfoSize);
#endif
	}
	else if (eMode == NeoUI::MODE_KEYPRESSED && m_state != STATE_ROOT && g_ctx.eCode == KEY_ESCAPE)
	{
		m_state = STATE_ROOT;
		UpdateControls();
	}

	if (m_state != STATE_ROOT)
	{
		// Print the title
		static constexpr int STATE_TO_BTN_MAP[STATE__TOTAL] = {
			0, 5, 2, 1, // TODO: REPLACE WITH SOMETHING ELSE?
		};
		const int iBtnIdx = STATE_TO_BTN_MAP[m_state];

		surface()->DrawSetTextFont(m_hTextFonts[FONT_NTNORMAL]);
		surface()->DrawSetTextColor(COLOR_NEOPANELTEXTBRIGHT);

		const int iPanelTall = g_iRowTall + (tall * 0.8f) + g_iRowTall;
		const int xPanelPos = (wide / 2) - (g_iRootSubPanelWide / 2);
		const int yTopPos = (tall - iPanelTall) / 2;
		int iTitleWidth, iTitleHeight;
		surface()->GetTextSize(m_hTextFonts[FONT_NTNORMAL], m_wszDispBtnTexts[iBtnIdx], iTitleWidth, iTitleHeight);
		surface()->DrawSetTextPos(xPanelPos, (yTopPos / 2) - (iTitleHeight / 2));
		surface()->DrawPrintText(m_wszDispBtnTexts[iBtnIdx], m_iWszDispBtnTextsSizes[iBtnIdx]);

		// Print F1 - F3 tab keybinds
		if (m_state != STATE_NEWGAME)
		{
			surface()->DrawSetTextFont(m_hTextFonts[FONT_NTSMALL]);
			surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);

			// NEO NOTE (nullsystem): F# as 1 is thinner than 3/not monospaced font
			int iFontWidth, iFontHeight;
			surface()->GetTextSize(m_hTextFonts[FONT_NTSMALL], L"F##", iFontWidth, iFontHeight);
			const int iHintYPos = yTopPos + (iFontHeight / 2);

			surface()->DrawSetTextPos(xPanelPos - g_iMarginX - iFontWidth, iHintYPos);
			surface()->DrawPrintText(L"F 1", 3);
			surface()->DrawSetTextPos(xPanelPos + g_iRootSubPanelWide + g_iMarginX, iHintYPos);
			surface()->DrawPrintText(L"F 3", 3);
		}
	}

	switch (m_state)
	{
	case STATE_ROOT:
	{
		NeoUI::BeginContext(eMode);
		NeoUI::BeginSection(true);
		const int iFlagToMatch = engine->IsInGame() ? FLAG_SHOWINGAME : FLAG_SHOWINMAIN;
		for (int i = 0; i < BTNS_TOTAL; ++i)
		{
			const auto btnInfo = BTNS_INFO[i];
			if (btnInfo.flags & iFlagToMatch)
			{
				const auto retBtn = NeoUI::Button(nullptr, m_wszDispBtnTexts[i]);
				if (retBtn.bPressed)
				{
					surface()->PlaySound("ui/buttonclickrelease.wav");
					if (btnInfo.gamemenucommand)
					{
						m_state = STATE_ROOT;
						UpdateControls();
						GetGameUI()->SendMainMenuCommand(btnInfo.gamemenucommand);
					}
					else if (btnInfo.nextState < STATE__TOTAL)
					{
						m_state = btnInfo.nextState;
						if (m_state == STATE_SETTINGS)
						{
							NeoSettingsRestore(&m_ns);
						}
						UpdateControls();
					}
				}
				if (retBtn.bMouseHover && i != m_iHoverBtn)
				{
					// Sound rollover feedback
					surface()->PlaySound("ui/buttonrollover.wav");
					m_iHoverBtn = i;
				}
			}
		}
		NeoUI::EndSection();
		NeoUI::EndContext();

		if (eMode == NeoUI::MODE_PAINT) PaintRootMainSection();
	}
	break;
	case STATE_SETTINGS:
	{
		NeoSettingsMainLoop(&m_ns, eMode);
		if (m_ns.bBack)
		{
			m_ns.bBack = false;
			m_state = STATE_ROOT;
			UpdateControls();
		}
	}
	break;
	case STATE_NEWGAME:
	{
	}
	break;
	case STATE_SERVERBROWSER:
	{
	}
	break;
	}
}

bool NeoRootCaptureESC()
{
	return (g_pNeoRoot && g_pNeoRoot->IsVisible());
}
