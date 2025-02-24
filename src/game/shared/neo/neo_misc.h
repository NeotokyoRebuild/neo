#pragma once

#include "vgui/ISurface.h"

/*
 * neo_misc.h - Miscellaneous functions/macros that doesn't belong at a specific place.
 */

#define IN_BETWEEN_AR(min, cmp, max) (((min) <= (cmp)) && ((cmp) < (max)))
#define IN_BETWEEN_EQ(min, cmp, max) (((min) <= (cmp)) && ((cmp) <= (max)))

[[nodiscard]] bool InRect(const vgui::IntRect &rect, const int x, const int y);
[[nodiscard]] int LoopAroundMinMax(const int iValue, const int iMin, const int iMax);
[[nodiscard]] int LoopAroundInArray(const int iValue, const int iSize);

struct SZWSZTexts
{
	const char *szStr;
	const wchar_t *wszStr;
};
#define SZWSZ_INIT(STR) {.szStr = STR, .wszStr = L"" STR}
