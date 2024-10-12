#pragma once

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

extern ConVar neo_cl_crosshair_style;
extern ConVar neo_cl_crosshair_color_r;
extern ConVar neo_cl_crosshair_color_g;
extern ConVar neo_cl_crosshair_color_b;
extern ConVar neo_cl_crosshair_color_a;
extern ConVar neo_cl_crosshair_size;
extern ConVar neo_cl_crosshair_size_screen;
extern ConVar neo_cl_crosshair_size_type;
extern ConVar neo_cl_crosshair_thickness;
extern ConVar neo_cl_crosshair_gap;
extern ConVar neo_cl_crosshair_outline;
extern ConVar neo_cl_crosshair_center_dot;
extern ConVar neo_cl_crosshair_top_line;
extern ConVar neo_cl_crosshair_circle_radius;
extern ConVar neo_cl_crosshair_circle_segments;

extern const char **CROSSHAIR_FILES;
extern const wchar_t **CROSSHAIR_LABELS;
extern const wchar_t **CROSSHAIR_SIZETYPE_LABELS;

#define NEO_XHAIR_EXT "neoxhr"

struct CrosshairInfo
{
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
void ImportCrosshair(CrosshairInfo *crh, const char *szFullpath);
void ExportCrosshair(CrosshairInfo *crh, const char *szFullpath);
