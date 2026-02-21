#include "neo_theme.h"

#include "neo_ui.h"
#include "shareddefs.h"

void SetupNTRETheme(NeoUI::Context *pNeoUICtx)
{
	NeoUI::Colors *pColors = &pNeoUICtx->colors;
	pColors->normalFg = COLOR_WHITE;
	pColors->normalBg = COLOR_TRANSPARENT;
	pColors->hotFg = COLOR_WHITE;
	pColors->hotBg = COLOR_TRANSPARENT;
	pColors->hotBorder = COLOR_WHITE;
	pColors->activeFg = COLOR_BLACK;
	pColors->activeBg = COLOR_WHITE;
	pColors->overrideFg = COLOR_RED;
	pColors->titleFg = COLOR_WHITE;
	pColors->cursor = COLOR_BLACK;
	pColors->textSelectionBg = COLOR_DARK_RED;
	pColors->divider = Color(220, 220, 220, 200);
	pColors->popupBg = COLOR_BLACK;
	pColors->sectionBg = COLOR_BLACK_TRANSPARENT;
	pColors->scrollbarBg = COLOR_TRANSPARENT;
	pColors->scrollbarHandleNormalBg = COLOR_FADED_WHITE;
	pColors->scrollbarHandleActiveBg = COLOR_WHITE;
	pColors->sliderNormalBg = COLOR_GREY;
	pColors->sliderHotBg = COLOR_GREY;
	pColors->sliderActiveBg = COLOR_BLACK;
	pColors->tabHintsFg = COLOR_WHITE;
}

