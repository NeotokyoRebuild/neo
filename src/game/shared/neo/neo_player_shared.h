#ifndef NEO_PLAYER_SHARED_H
#define NEO_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/valve_minmax_off.h"
#include <string_view>
#include <optional>
#ifndef min
	#define min(a,b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
	#define max(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#include "neo_predicted_viewmodel.h"

#ifdef INCLUDE_WEP_PBK
// Type to use if we need to ensure more than 32 bits in the mask.
#define NEO_WEP_BITS_UNDERLYING_TYPE long long int
#else
// Using plain int if we don't need to ensure >32 bits in the mask.
#define NEO_WEP_BITS_UNDERLYING_TYPE int
#endif

// All of these should be able to stack create even slower speeds (at least in original NT)
#define NEO_SPRINT_MODIFIER 1.6
#define NEO_SLOW_MODIFIER 0.75

#define NEO_BASE_NORM_SPEED 136
#define NEO_BASE_SPRINT_SPEED (NEO_BASE_NORM_SPEED * NEO_SPRINT_MODIFIER)
#define NEO_BASE_WALK_SPEED (NEO_BASE_NORM_SPEED * NEO_SLOW_MODIFIER)
#define NEO_BASE_CROUCH_SPEED (NEO_BASE_NORM_SPEED * NEO_SLOW_MODIFIER)

#define NEO_RECON_SPEED_MODIFIER 1.25
#define NEO_ASSAULT_SPEED_MODIFIER 1.0
#define NEO_SUPPORT_SPEED_MODIFIER 1.0
#define NEO_VIP_SPEED_MODIFIER 1.0

#define NEO_RECON_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_RECON_SPEED_MODIFIER)
#define NEO_RECON_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_RECON_SPEED_MODIFIER)

#define NEO_ASSAULT_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_ASSAULT_SPEED_MODIFIER)
#define NEO_ASSAULT_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_ASSAULT_SPEED_MODIFIER)

#define NEO_SUPPORT_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_SUPPORT_SPEED_MODIFIER)
#define NEO_SUPPORT_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_SUPPORT_SPEED_MODIFIER)

#define NEO_VIP_NORM_SPEED (NEO_BASE_NORM_SPEED * NEO_VIP_SPEED_MODIFIER)
#define NEO_VIP_SPRINT_SPEED (NEO_BASE_SPRINT_SPEED * NEO_VIP_SPEED_MODIFIER)
#define NEO_VIP_WALK_SPEED (NEO_BASE_WALK_SPEED * NEO_VIP_SPEED_MODIFIER)
#define NEO_VIP_CROUCH_SPEED (NEO_BASE_CROUCH_SPEED * NEO_VIP_SPEED_MODIFIER)

// Sanity checks for class speeds.
// These values are divided with in some contexts, so should never equal zero.
COMPILE_TIME_ASSERT(NEO_RECON_NORM_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_CROUCH_SPEED > 0);

COMPILE_TIME_ASSERT(NEO_ASSAULT_NORM_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_CROUCH_SPEED > 0);

COMPILE_TIME_ASSERT(NEO_SUPPORT_NORM_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_CROUCH_SPEED > 0);

// Class speeds hierarchy should be: recon > assault > support.
COMPILE_TIME_ASSERT(NEO_RECON_NORM_SPEED > NEO_ASSAULT_NORM_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_NORM_SPEED == NEO_SUPPORT_NORM_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_SPRINT_SPEED > NEO_ASSAULT_SPRINT_SPEED);
//COMPILE_TIME_ASSERT(NEO_ASSAULT_SPRINT_SPEED > NEO_SUPPORT_SPRINT_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_WALK_SPEED > NEO_ASSAULT_WALK_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_WALK_SPEED == NEO_SUPPORT_WALK_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_CROUCH_SPEED > NEO_ASSAULT_CROUCH_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_CROUCH_SPEED == NEO_SUPPORT_CROUCH_SPEED);

#define SUPER_JMP_COST 45.0f
#define CLOAK_AUX_COST 1.0f
#define MIN_CLOAK_AUX 0.1f
#define SPRINT_START_MIN (2.0f)

// Original NT allows chaining superjumps up ramps,
// so leaving this zeroed for enabling movement tricks.
#define SUPER_JMP_DELAY_BETWEEN_JUMPS 0

// NEO Activities
#define ACT_NEO_ATTACK ACT_RANGE_ATTACK1
#define ACT_NEO_RELOAD ACT_RELOAD
#define ACT_NEO_IDLE_STAND ACT_IDLE
#define ACT_NEO_IDLE_CROUCH ACT_CROUCHIDLE
#define ACT_NEO_MOVE_RUN ACT_RUN
#define ACT_NEO_MOVE_WALK ACT_WALK
#define ACT_NEO_MOVE_CROUCH ACT_RUN_CROUCH
#define ACT_NEO_DIE ACT_DIESIMPLE
#define ACT_NEO_HOVER ACT_HOVER
#define ACT_NEO_JUMP ACT_HOP
#define ACT_NEO_SWIM ACT_SWIM

#ifdef GAME_DLL
#define NEO_ACT_TABLE_ENTRY_REQUIRED false
#define NEO_IMPLEMENT_ACTTABLE(CNEOWepClass) acttable_t CNEOWepClass::m_acttable[] = {\
{ ACT_NEO_ATTACK, ACT_NEO_ATTACK, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_RELOAD, ACT_NEO_RELOAD, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_IDLE_STAND, ACT_NEO_IDLE_STAND, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_IDLE_STAND, ACT_NEO_IDLE_STAND, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_IDLE_CROUCH, ACT_NEO_IDLE_CROUCH, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_MOVE_RUN, ACT_NEO_MOVE_RUN, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_MOVE_WALK, ACT_NEO_MOVE_WALK, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_MOVE_CROUCH, ACT_NEO_MOVE_CROUCH, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_DIE, ACT_NEO_DIE, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_HOVER, ACT_NEO_HOVER, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_JUMP, ACT_NEO_JUMP, NEO_ACT_TABLE_ENTRY_REQUIRED },\
{ ACT_NEO_SWIM, ACT_NEO_SWIM, NEO_ACT_TABLE_ENTRY_REQUIRED },\
};IMPLEMENT_ACTTABLE(CNEOWepClass);
#else
#define NEO_IMPLEMENT_ACTTABLE(CNEOWepClass)
#endif

#define NEO_ASSAULT_PLAYERMODEL_HEIGHT 67.0
#define NEO_ASSAULT_PLAYERMODEL_DUCK_HEIGHT 50.0

// These look like magic but are actually taken straight from the og binaries.
#define NEO_RECON_DAMAGE_MODIFIER 1.485f
#define NEO_ASSAULT_DAMAGE_MODIFIER 1.2375f
#define NEO_SUPPORT_DAMAGE_MODIFIER 0.66f

#define NEO_ANIMSTATE_LEGANIM_TYPE LegAnimType_t::LEGANIM_9WAY
#define NEO_ANIMSTATE_USES_AIMSEQUENCES true
#define NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES 90.0f

static constexpr float NEO_ZOOM_SPEED = 0.115f;
static_assert(NEO_ZOOM_SPEED != 0.0f, "Divide by zero");

enum NeoSkin {
	NEO_SKIN_FIRST = 0,
	NEO_SKIN_SECOND,
	NEO_SKIN_THIRD,

	NEO_SKIN__ENUM_COUNT
};
static constexpr int NEO_SKIN_ENUM_COUNT = NEO_SKIN__ENUM_COUNT;

enum NeoClass {
	NEO_CLASS_RECON = 0,
	NEO_CLASS_ASSAULT,
	NEO_CLASS_SUPPORT,

	// NOTENOTE: VIP *must* be last, because we are
	// using array offsets for recon/assault/support
	NEO_CLASS_VIP,

	NEO_CLASS__ENUM_COUNT
};
static constexpr int NEO_CLASS_ENUM_COUNT = NEO_CLASS__ENUM_COUNT;

enum NeoStar {
	STAR_NONE = 0,
	STAR_ALPHA,
	STAR_BRAVO,
	STAR_CHARLIE,
	STAR_DELTA,
	STAR_ECHO,
	STAR_FOXTROT,

	STAR__TOTAL
};
#define NEO_DEFAULT_STAR STAR_ALPHA

// Implemented by CNEOPlayer::m_fNeoFlags.
// Rolling our own because Source FL_ flags already reserve all 32 bits,
// and extending the type would require a larger refactor.
#define NEO_FL_FREEZETIME (1 << 1) // Freeze player movement, but allow looking around.

#if defined(CLIENT_DLL) && !defined(CNEOBaseCombatWeapon)
#define CNEOBaseCombatWeapon C_NEOBaseCombatWeapon
#endif

#define COLOR_JINRAI COLOR_NEO_GREEN
#define COLOR_NSF COLOR_NEO_BLUE
#define COLOR_SPEC COLOR_NEO_ORANGE

#define COLOR_NEO_BLUE Color(154, 205, 255, 255)
#define COLOR_NEO_GREEN Color(154, 255, 154, 255)
#define COLOR_NEO_ORANGE Color(243, 190, 52, 255)
#define COLOR_NEO_WHITE Color(218, 217, 213, 255)

class CNEO_Player;
class CNEOBaseCombatWeapon;
class C_BaseCombatWeapon;

extern bool IsThereRoomForLeanSlide(CNEO_Player *player,
	const Vector &targetViewOffset, bool &outStartInSolid);

// Is the player allowed to aim zoom with a weapon of this type?
bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep);

extern ConVar neo_recon_superjump_intensity;

//ConVar sv_neo_resupply_anywhere("sv_neo_resupply_anywhere", "0", FCVAR_CHEAT | FCVAR_REPLICATED);

inline const char* GetNeoClassName(int neoClassIdx)
{
	switch (neoClassIdx)
	{
	case NEO_CLASS_RECON: return "Recon";
	case NEO_CLASS_ASSAULT: return "Assault";
	case NEO_CLASS_SUPPORT: return "Support";
	case NEO_CLASS_VIP: return "VIP";
	default: return "";
	}
}

inline const char *GetRankName(int xp, bool shortened = false)
{
	if (xp < 0)
	{
		return shortened ? "Dog" : "Rankless Dog";
	}
	else if (xp < 4)
	{
		return shortened ? "Pvt" : "Private";
	}
	else if (xp < 10)
	{
		return shortened ? "Cpl" : "Corporal";
	}
	else if (xp < 20)
	{
		return shortened ? "Sgt" : "Sergeant";
	}
	else
	{
		return shortened ? "Lt" : "Lieutenant";
	}
}

CBaseCombatWeapon* GetNeoWepWithBits(const CNEO_Player* player, const NEO_WEP_BITS_UNDERLYING_TYPE& neoWepBits);

enum NeoLeanDirectionE {
	NEO_LEAN_NONE = 0,
	NEO_LEAN_LEFT,
	NEO_LEAN_RIGHT,
};

enum NeoWeponAimToggleE {
	NEO_TOGGLE_DEFAULT = 0,
	NEO_TOGGLE_FORCE_AIM,
	NEO_TOGGLE_FORCE_UN_AIM,
};

bool ClientWantsAimHold(const CNEO_Player* player);

struct AttackersTotals
{
	int dealtDmgs;
	int dealtHits;
	int takenDmgs;
	int takenHits;

	void operator+=(const AttackersTotals &other)
	{
		dealtDmgs += other.dealtDmgs;
		dealtHits += other.dealtHits;
		takenDmgs += other.takenDmgs;
		takenHits += other.takenHits;
	}
};

int DmgLineStr(char* infoLine, const int infoLineMax,
	const char* dmgerName, const char* dmgerClass,
	const AttackersTotals &totals);

void KillerLineStr(char* killByLine, const int killByLineMax,
	CNEO_Player* neoAttacker, const CNEO_Player* neoVictim, const char* killedWith = "");

[[nodiscard]] auto StrToInt(std::string_view strView) -> std::optional<int>;
[[nodiscard]] int NeoAimFOV(const int fovDef, CBaseCombatWeapon *wep);

struct IntPos
{
	int x;
	int y;
};

struct IntDim
{
	int w;
	int h;
};

#ifdef CLIENT_DLL
struct PlayerXPInfo
{
	int idx;
	int xp;
	int deaths;
};
void DMClSortedPlayers(PlayerXPInfo (*pPlayersOrder)[MAX_PLAYERS + 1], int *piTotalPlayers);
#endif

inline char gStreamerModeNames[MAX_PLAYERS + 1][MAX_PLAYER_NAME_LENGTH + 1];

static constexpr int NEO_MAX_CLANTAG_LENGTH = 12;
static constexpr int NEO_MAX_DISPLAYNAME = MAX_PLAYER_NAME_LENGTH + 1 + NEO_MAX_CLANTAG_LENGTH + 1;
void GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH + 1],
						 const wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH + 1],
						 const bool bOnlySteamNick);

void GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const char *pSzNeoName,
						 const char *pSzNeoClantag,
						 const bool bOnlySteamNick);

#endif // NEO_PLAYER_SHARED_H
