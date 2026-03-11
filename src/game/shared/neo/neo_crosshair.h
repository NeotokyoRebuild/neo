#pragma once

#include <cstddef>
#include "Color.h"

#ifdef CLIENT_DLL
#include "neo_player_shared.h"

class C_NEOBaseCombatWeapon;
#endif // CLIENT_DLL

// NEO NOTE (nullsystem): Max string length is 
// something like: "2;2;-16711936;1;6;1.000;25;25;5;25;1;50;50;2;1;-16711936;-16711936;-16711936;"
// which is ~77 for v5 serialization | 96 length is enough for now till
// more comes in
static constexpr const size_t NEO_XHAIR_SEQMAX = 96;
static constexpr int CROSSHAIR_MAX_SIZE = 100;
static constexpr int CROSSHAIR_MAX_THICKNESS = 25;
static constexpr int CROSSHAIR_MAX_GAP = 25;
static constexpr int CROSSHAIR_MAX_OUTLINE = 5;
static constexpr int CROSSHAIR_MAX_CENTER_DOT = 25;
static constexpr int CROSSHAIR_MAX_CIRCLE_RAD = 50;
static constexpr int CROSSHAIR_MAX_CIRCLE_SEGMENTS = 50;

enum NeoHudCrosshairStyle
{
	CROSSHAIR_STYLE_DEFAULT = 0,
	CROSSHAIR_STYLE_ALT_B,
	CROSSHAIR_STYLE_CUSTOM,

	CROSSHAIR_STYLE__TOTAL,
};

enum NeoHudCrosshairSizeType
{
	CROSSHAIR_SIZETYPE_ABSOLUTE = 0,
	CROSSHAIR_SIZETYPE_SCREEN,

	CROSSHAIR_SIZETYPE__TOTAL,
};

enum NeoHudCrosshairDynamicType
{
	CROSSHAIR_DYNAMICTYPE_NONE = 0,
	CROSSHAIR_DYNAMICTYPE_GAP,
	CROSSHAIR_DYNAMICTYPE_CIRCLE,
	CROSSHAIR_DYNAMICTYPE_SIZE,

	CROSSHAIR_DYNAMICTYPE__TOTAL,
};

#ifdef CLIENT_DLL
extern ConVar cl_neo_crosshair;
extern ConVar cl_neo_crosshair_network;

extern const char **CROSSHAIR_FILES;
extern const wchar_t **CROSSHAIR_LABELS;
extern const wchar_t **CROSSHAIR_SIZETYPE_LABELS;
extern const wchar_t **CROSSHAIR_DYNAMICTYPE_LABELS;
#endif // CLIENT_DLL

struct CrosshairInfo
{
	int iStyle;
	Color color;
	NeoHudCrosshairSizeType eSizeType;
	int iSize;
	float flScrSize;
	int iThick;
	int iGap;
	int iOutline;
	int iCenterDot;
	bool bTopLine;
	int iCircleRad;
	int iCircleSegments;
	NeoHudCrosshairDynamicType eDynamicType;
	bool bSeparateColorDot;
	Color colorDot;
	Color colorDotOutline;
	Color colorOutline;
};

#ifdef CLIENT_DLL
void PaintCrosshair(const CrosshairInfo &crh, int inaccuracy, const int x, const int y);
int HalfInaccuracyConeInScreenPixels(C_NEOBaseCombatWeapon *pWeapon, int halfScreenWidth);

void InitializeClNeoCrosshair();
#endif // CLIENT_DLL

void ResetCrosshairToDefault(CrosshairInfo *crh);
void DefaultCrosshairSerial(char (&szSequence)[NEO_XHAIR_SEQMAX]);

// NEO NOTE (nullsystem): (*&)[NUM] enforces array size
bool ImportCrosshair(CrosshairInfo *crh, const char *pszSequence);
void ExportCrosshair(CrosshairInfo *crh, char (&szSequence)[NEO_XHAIR_SEQMAX]);

