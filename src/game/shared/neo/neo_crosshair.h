#pragma once

#include <cstddef>
#include "Color.h"
#include "neo_weapon_types.h"

#ifdef UNIT_TEST_DLL
#define COLOR_WHITE Color(255, 255, 255, 255)
#define COLOR_BLACK Color(0, 0, 0, 255)
#endif

#ifdef CLIENT_DLL
#include "neo_player_shared.h"

class C_NEOBaseCombatWeapon;
#endif // CLIENT_DLL

// NEO NOTE (nullsystem): Check in test/test_neo_crosshair.cpp TestSerial_LongestLength unit test
// on longest length. Currently it sits under 400.
static constexpr const size_t NEO_XHAIR_SEQMAX = 400;

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

enum ENeoCrosshairWep
{
	CROSSHAIR_WEP_NONE = -1,
	CROSSHAIR_WEP_DEFAULT = 0,
	CROSSHAIR_WEP_SECONDARY,
	CROSSHAIR_WEP_SHOTGUN,
	CROSSHAIR_WEP_DEFAULT_HIPFIRE,
	CROSSHAIR_WEP_SECONDARY_HIPFIRE,
	CROSSHAIR_WEP_SHOTGUN_HIPFIRE,

	CROSSHAIR_WEP__TOTAL,
};

extern const ENeoCrosshairWep MAP_WEAPON_TYPE_TO_XHAIR[WEP_TYPE__TOTAL];

enum NeoCrosshairWepFlag_
{
	CROSSHAIR_WEP_FLAG_DEFAULT = 0,					// Default
	CROSSHAIR_WEP_FLAG_SECONDARY = 1 << 0,			// Pistols (Milso, Tachi, Kyla)
	CROSSHAIR_WEP_FLAG_SHOTGUN = 1 << 1,			// Shotguns (Supa, AA13)
	CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE = 1 << 2,	// Default hipfire
	CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE = 1 << 3,	// Pistols hipfire
	CROSSHAIR_WEP_FLAG_SHOTGUN_HIPFIRE = 1 << 4,	// Shotguns hipfire
	
	CROSSHAIR_WEP_FLAG__FINALENUM,
	CROSSHAIR_WEP_FLAG__HIGHESTFLAG = 2 * (CROSSHAIR_WEP_FLAG__FINALENUM - 1),
};
typedef int NeoCrosshairWepFlags;

// EX: 	CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE = 0, CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT = 0
// 			Default hipfire disabled
// 		CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE = 1, CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT = 0
// 			Default hipfire enabled + use default, fallback to non-hipfire default
// 		CROSSHAIR_WEP_FLAG_DEFAULT_HIPFIRE = 1, CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT = 1
// 			Default hipfire enabled + fully custom
// 		CROSSHAIR_WEP_FLAG_SECONDARY_HIPFIRE = 1, CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY = 0
// 			Secondary hipfire enabled + use default, fallback to hipfire default
enum NeoCrosshairHipfireCustomFlag_
{
	CROSSHAIR_HIPFIRECUSTOM_FLAG_NIL = 0,
	CROSSHAIR_HIPFIRECUSTOM_FLAG_DEFAULT = 1 << 0,		// If not set: Fallback to non-hipfire default
	CROSSHAIR_HIPFIRECUSTOM_FLAG_SECONDARY = 1 << 1,	// If not set: Fallback to hipfire default
	CROSSHAIR_HIPFIRECUSTOM_FLAG_SHOTGUN = 1 << 2,		// If not set: Fallback to hipfire default
	
	CROSSHAIR_HIPFIRECUSTOM_FLAG__FINALENUM,
	CROSSHAIR_HIPFIRECUSTOM_FLAG__HIGHESTFLAG = 2 * (CROSSHAIR_HIPFIRECUSTOM_FLAG__FINALENUM - 1),
};
typedef int NeoCrosshairHipfireCustomFlags;

enum NeoCrosshairFlag_
{
	CROSSHAIR_FLAG_DEFAULT = 0,
	CROSSHAIR_FLAG_NOTOPLINE = 1 << 0,			// Do not draw top line
	CROSSHAIR_FLAG_SEPERATEDOTCOLOR = 1 << 1,	// Draw dot as separate color

	CROSSHAIR_FLAG__FINALENUM,
	CROSSHAIR_FLAG__HIGHESTFLAG = 2 * (CROSSHAIR_FLAG__FINALENUM - 1),
};
typedef int NeoCrosshairFlags;

enum EHipfireOpt
{
	HIPFIREOPT_DISABLED = 0,
	HIPFIREOPT_USEDEFAULT,
	HIPFIREOPT_ENABLED,

	HIPFIREOPT__TOTAL,
};

#ifdef CLIENT_DLL
extern ConVar cl_neo_crosshair;
extern ConVar cl_neo_crosshair_network;

extern const char **CROSSHAIR_FILES;
extern const wchar_t **CROSSHAIR_LABELS;
extern const wchar_t **CROSSHAIR_SIZETYPE_LABELS;
extern const wchar_t **CROSSHAIR_DYNAMICTYPE_LABELS;
#endif // CLIENT_DLL

struct CrosshairWepInfo
{
	int iStyle;
	Color color;
	NeoCrosshairFlags flags;
	NeoHudCrosshairSizeType eSizeType;
	int iSize;
	float flScrSize;
	int iThick;
	int iGap;
	int iOutline;
	int iCenterDot;
	int iCircleRad;
	int iCircleSegments;
	NeoHudCrosshairDynamicType eDynamicType;
	Color colorDot;
	Color colorDotOutline;
	Color colorOutline;
};

struct CrosshairInfo
{
	NeoCrosshairWepFlags wepFlags;
	NeoCrosshairHipfireCustomFlags hipfireFlags;
	CrosshairWepInfo wep[CROSSHAIR_WEP__TOTAL];
};

// NEO WARNING (nullsystem): When adding new/removing items to serialize, only append enum one after another
// before the NEOXHAIR_SERIAL__LATESTPLUSONE section!
// When working on this put in the current in-dev/unreleased version for the enum name.
// Bump NEO_XHAIR_SEQMAX if it's going to go over the string length
enum NeoXHairSerial
{
	NEOXHAIR_SERIAL_PREALPHA_V8_2 = 1,
	NEOXHAIR_SERIAL_ALPHA_V17,
	NEOXHAIR_SERIAL_ALPHA_V19,
	NEOXHAIR_SERIAL_ALPHA_V22,
	NEOXHAIR_SERIAL_ALPHA_V28,
	NEOXHAIR_SERIAL_ALPHA_V29,

	NEOXHAIR_SERIAL__LATESTPLUSONE,
	NEOXHAIR_SERIAL_CURRENT = NEOXHAIR_SERIAL__LATESTPLUSONE - 1,
};

#ifdef CLIENT_DLL
void PaintCrosshair(const CrosshairWepInfo *crh, const int inaccuracy, const int x, const int y);
int HalfInaccuracyConeInScreenPixels(C_NEOBaseCombatWeapon *pWeapon, int halfScreenWidth);

void InitializeClNeoCrosshair();

class IConVar;
void NeoConVarCrosshairChangeCallback(IConVar *cvar, const char *pOldVal, float flOldVal);

#endif // CLIENT_DLL

void ResetCrosshairToDefault(CrosshairInfo *xhairInfo,
		EHipfireOpt (*paeHipfireOpts)[CROSSHAIR_WEP__TOTAL] = nullptr);
void DefaultCrosshairSerial(char (&szSequence)[NEO_XHAIR_SEQMAX]);

int UseCrosshairIndexFor(const CrosshairInfo *xhairInfo, const int iXHairWep, bool *pbHide = nullptr);

bool ValidateCrosshairSerial(const char *pszSequence);

// NEO NOTE (nullsystem): (*&)[NUM] enforces array size
// paeHipfireOpts - Maps NeoUI RingBox int <-> NeoCrosshairWepFlags + NeoCrosshairHipfireCustomFlags
// Only for import as export must have directly already set by flags and managed within UI
bool ImportCrosshair(CrosshairInfo *xhairInfo, const char *pszSequence,
		EHipfireOpt (*paeHipfireOpts)[CROSSHAIR_WEP__TOTAL] = nullptr);

// iExportSerialVersion is only used for unit testing purpose, for usage in-game
// it should always be exporting to NEOXHAIR_SERIAL_CURRENT
void ExportCrosshair(CrosshairInfo *xhairInfo, char (&szSequence)[NEO_XHAIR_SEQMAX]
#ifdef UNIT_TEST_DLL
		, const int iExportSerialVersion = NEOXHAIR_SERIAL_CURRENT
#endif
		);

