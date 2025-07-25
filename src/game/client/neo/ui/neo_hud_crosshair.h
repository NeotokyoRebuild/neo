#pragma once

#include "neo_player_shared.h"

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

extern ConVar cl_neo_crosshair;
extern ConVar cl_neo_crosshair_network;

extern const char **CROSSHAIR_FILES;
extern const wchar_t **CROSSHAIR_LABELS;
extern const wchar_t **CROSSHAIR_SIZETYPE_LABELS;

#define CL_NEO_CROSSHAIR_DEFAULT "2;0;-1;0;6;0.000;2;4;0;0;1;0;0;"
// NEO_XHAIR_SEQMAX defined in neo_player_shared.h instead

enum NeoXHairSegment
{
	NEOXHAIR_SEGMENT_I_VERSION = 0,
	NEOXHAIR_SEGMENT_I_STYLE,
	NEOXHAIR_SEGMENT_I_COLOR,
	NEOXHAIR_SEGMENT_I_SIZETYPE,
	NEOXHAIR_SEGMENT_I_SIZE,
	NEOXHAIR_SEGMENT_FL_SCRSIZE,
	NEOXHAIR_SEGMENT_I_THICK,
	NEOXHAIR_SEGMENT_I_GAP,
	NEOXHAIR_SEGMENT_I_OUTLINE,
	NEOXHAIR_SEGMENT_I_CENTERDOT,
	NEOXHAIR_SEGMENT_B_TOPLINE,
	NEOXHAIR_SEGMENT_I_CIRCLERAD,
	NEOXHAIR_SEGMENT_I_CIRCLESEGMENTS,

	NEOXHAIR_SEGMENT__TOTAL,
	NEOXHAIR_SEGMENT__TOTAL_SERIAL_ALPHA_V17 = NEOXHAIR_SEGMENT_I_CIRCLESEGMENTS + 1,
};

struct CrosshairInfo
{
	int iStyle;
	Color color;
	int iESizeType; // int NeoHudCrosshairSizeType
	int iSize;
	float flScrSize;
	int iThick;
	int iGap;
	int iOutline;
	int iCenterDot;
	bool bTopLine;
	int iCircleRad;
	int iCircleSegments;
};
void PaintCrosshair(const CrosshairInfo &crh, const int x, const int y);

// NEO NOTE (nullsystem): (*&)[NUM] enforces array size
bool ImportCrosshair(CrosshairInfo *crh, const char *pszSequence);
void ExportCrosshair(const CrosshairInfo *crh, char (&szSequence)[NEO_XHAIR_SEQMAX]);
