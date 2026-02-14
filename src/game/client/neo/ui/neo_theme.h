#pragma once

// NEO NOTE (nullsystem): These are colors specific
// to only NT;RE main menu theme

#define COLOR_ALMOST_WHITE Color(240, 240, 240, 255)
#define COLOR_ALMOST_BLACK Color(15, 15, 15, 255)
#define COLOR_BLACK_TRANSPARENT Color(0, 0, 0, 190)
#define COLOR_DARK_RED Color(180, 32, 32, 255)
#define COLOR_NEOPANELMICTEST Color(30, 90, 30, 255)
#define COLOR_NEOPANELACCENTBG Color(40, 0, 0, 255)

namespace NeoUI
{
struct Context;
}

void SetupNTRETheme(NeoUI::Context *pNeoUICtx);

