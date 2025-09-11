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

#define USERID2NEOPLAYER(i) ToNEOPlayer( ClientEntityList().GetEnt( engine->GetPlayerForUserID( i ) ) )

#include "neo_predicted_viewmodel.h"
#include "neo_misc.h"
#include "shareddefs.h"
#include "weapon_bits.h"
#include "neo_enums.h"

//////////////////////////////////////////////////////
// NEO MOVEMENT DEFINITIONS

#define NEO_BASE_SPEED 200

// Class Modifiers (No weapons equipped)
#define NEO_RECON_MODIFIER 1
#define NEO_ASSAULT_MODIFIER 0.8
#define NEO_SUPPORT_MODIFIER 0.8

// Sprint Modifiers
#define NEO_RECON_SPRINT_MODIFIER 1.35
#define NEO_ASSAULT_SPRINT_MODIFIER 1.6
#define NEO_SUPPORT_SPRINT_MODIFIER 1.35 // Redundant, but for future usecases

// Aim Modifier
#define NEO_AIM_MODIFIER 0.6
// Crouch Modifier
#define NEO_CROUCH_MODIFIER 0.75
#define NEO_WALK_SPEED 90
#define NEO_CROUCH_WALK_SPEED 60


// Movement Calculations
// Recon
#define NEO_RECON_BASE_SPEED (NEO_BASE_SPEED * NEO_RECON_MODIFIER)
#define NEO_RECON_SPRINT_SPEED (NEO_RECON_BASE_SPEED * NEO_RECON_SPRINT_MODIFIER)
#define NEO_RECON_CROUCH_SPEED (NEO_RECON_BASE_SPEED * NEO_CROUCH_MODIFIER)
#define NEO_RECON_WALK_SPEED NEO_RECON_CROUCH_SPEED
// Assault
#define NEO_ASSAULT_BASE_SPEED (NEO_BASE_SPEED * NEO_ASSAULT_MODIFIER)
#define NEO_ASSAULT_SPRINT_SPEED (NEO_ASSAULT_BASE_SPEED * NEO_ASSAULT_SPRINT_MODIFIER)
#define NEO_ASSAULT_CROUCH_SPEED (NEO_ASSAULT_BASE_SPEED * NEO_CROUCH_MODIFIER)
#define NEO_ASSAULT_WALK_SPEED NEO_ASSAULT_CROUCH_SPEED
// Support
#define NEO_SUPPORT_BASE_SPEED (NEO_BASE_SPEED * NEO_SUPPORT_MODIFIER)
#define NEO_SUPPORT_SPRINT_SPEED (NEO_SUPPORT_BASE_SPEED * NEO_SUPPORT_SPRINT_MODIFIER) // Redundant, but for future usecases
#define NEO_SUPPORT_CROUCH_SPEED (NEO_SUPPORT_BASE_SPEED * NEO_CROUCH_MODIFIER)
#define NEO_SUPPORT_WALK_SPEED NEO_SUPPORT_CROUCH_SPEED
// VIP
#define NEO_VIP_BASE_SPEED NEO_ASSAULT_BASE_SPEED
#define NEO_VIP_SPRINT_SPEED NEO_ASSAULT_SPRINT_SPEED
#define NEO_VIP_CROUCH_SPEED NEO_ASSAULT_CROUCH_SPEED
#define NEO_VIP_WALK_SPEED NEO_VIP_CROUCH_SPEED
// Juggernaut
#define NEO_JUGGERNAUT_BASE_SPEED NEO_ASSAULT_BASE_SPEED
#define NEO_JUGGERNAUT_SPRINT_SPEED NEO_RECON_SPRINT_SPEED
#define NEO_JUGGERNAUT_CROUCH_SPEED NEO_ASSAULT_CROUCH_SPEED
#define NEO_JUGGERNAUT_WALK_SPEED NEO_JUGGERNAUT_CROUCH_SPEED


// Sanity Checks
// Recon
COMPILE_TIME_ASSERT(NEO_RECON_BASE_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_RECON_CROUCH_SPEED > 0);
// Assault
COMPILE_TIME_ASSERT(NEO_ASSAULT_BASE_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_ASSAULT_CROUCH_SPEED > 0);
// Support
COMPILE_TIME_ASSERT(NEO_SUPPORT_BASE_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_SUPPORT_CROUCH_SPEED > 0);
// VIP
COMPILE_TIME_ASSERT(NEO_VIP_BASE_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_VIP_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_VIP_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_VIP_CROUCH_SPEED > 0);
// Juggernaut
COMPILE_TIME_ASSERT(NEO_JUGGERNAUT_BASE_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_JUGGERNAUT_SPRINT_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_JUGGERNAUT_WALK_SPEED > 0);
COMPILE_TIME_ASSERT(NEO_JUGGERNAUT_CROUCH_SPEED > 0);


// Class speeds hierarchy should be: recon > assault > support.
COMPILE_TIME_ASSERT(NEO_RECON_BASE_SPEED > NEO_ASSAULT_BASE_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_BASE_SPEED == NEO_SUPPORT_BASE_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_BASE_SPEED == NEO_VIP_BASE_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_SPRINT_SPEED > NEO_ASSAULT_SPRINT_SPEED);
//COMPILE_TIME_ASSERT(NEO_ASSAULT_SPRINT_SPEED > NEO_SUPPORT_SPRINT_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_SPRINT_SPEED == NEO_VIP_SPRINT_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_WALK_SPEED > NEO_ASSAULT_WALK_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_WALK_SPEED == NEO_SUPPORT_WALK_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_WALK_SPEED == NEO_VIP_WALK_SPEED);

COMPILE_TIME_ASSERT(NEO_RECON_CROUCH_SPEED > NEO_ASSAULT_CROUCH_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_CROUCH_SPEED == NEO_SUPPORT_CROUCH_SPEED);
COMPILE_TIME_ASSERT(NEO_ASSAULT_CROUCH_SPEED == NEO_VIP_CROUCH_SPEED);

#define NEO_RECON_CROUCH_JUMP_HEIGHT 65.f
#define NEO_CROUCH_JUMP_HEIGHT 56.f

// END OF NEO MOVEMENT DEFINITIONS
//////////////////////////////////////////////////////

#define SUPER_JMP_COST 45.0f
#define CLOAK_AUX_COST 1.0f
#define MIN_CLOAK_AUX 0.1f
#define SPRINT_START_MIN (2.0f)
#define THERMALS_OBJECT_COOL_TIME 5.f

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
#define NEO_JUGGERNAUT_DAMAGE_MODIFIER 0.15f

#define NEO_ANIMSTATE_LEGANIM_TYPE LegAnimType_t::LEGANIM_9WAY
#define NEO_ANIMSTATE_USES_AIMSEQUENCES true
#define NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES 90.0f

static constexpr float NEO_ZOOM_SPEED = 0.115f;
static_assert(NEO_ZOOM_SPEED != 0.0f, "Divide by zero");

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

//ConVar sv_neo_resupply_anywhere("sv_neo_resupply_anywhere", "0", FCVAR_CHEAT | FCVAR_REPLICATED);

static constexpr const SZWSZTexts SZWSZ_NEO_CLASS_STRS[NEO_CLASS__ENUM_COUNT] = {
	SZWSZ_INIT("Recon"),
	SZWSZ_INIT("Assault"),
	SZWSZ_INIT("Support"),
	SZWSZ_INIT("VIP"),
	SZWSZ_INIT("Juggernaut"),
};

inline const char *GetNeoClassName(const int neoClassIdx)
{
	return (IN_BETWEEN_AR(0, neoClassIdx, NEO_CLASS__ENUM_COUNT)) ? SZWSZ_NEO_CLASS_STRS[neoClassIdx].szStr : "";
}

inline const wchar_t *GetNeoClassNameW(const int neoClassIdx)
{
	return (IN_BETWEEN_AR(0, neoClassIdx, NEO_CLASS__ENUM_COUNT)) ? SZWSZ_NEO_CLASS_STRS[neoClassIdx].wszStr : L"";
}

int GetRank(const int xp);
const char *GetRankName(const int xp, const bool shortened = false);

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

void CheckPingButton(CNEO_Player* player);

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

[[deprecated]] void KillerLineStr(char* killByLine, const int killByLineMax,
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

enum EClNeoDisplayNameFlag_
{
	CL_NEODISPLAYNAME_FLAG_NONE = 0,
	CL_NEODISPLAYNAME_FLAG_ONLYSTEAMNICK = 1 << 0,
	CL_NEODISPLAYNAME_FLAG_CHECK = 1 << 1,
};
typedef int EClNeoDisplayNameFlag;

static constexpr int NEO_MAX_CLANTAG_LENGTH = 11 + 1; // Includes null character
static constexpr int NEO_MAX_DISPLAYNAME = MAX_PLAYER_NAME_LENGTH + 1 + NEO_MAX_CLANTAG_LENGTH + 2;
bool GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const wchar_t (&wszNeoName)[MAX_PLAYER_NAME_LENGTH],
						 const wchar_t (&wszNeoClantag)[NEO_MAX_CLANTAG_LENGTH],
						 const EClNeoDisplayNameFlag flags = CL_NEODISPLAYNAME_FLAG_NONE);

bool GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const char *pSzNeoName,
						 const char *pSzNeoClantag,
						 const EClNeoDisplayNameFlag flags = CL_NEODISPLAYNAME_FLAG_NONE);

// NEO NOTE (nullsystem): Max string length is 
// something like: "2;2;-16711936;1;6;1.000;25;25;5;25;1;50;50;"
// which is ~43 for v2 serialization | 64 length is enough for now till
// more comes in
static constexpr const int NEO_XHAIR_SEQMAX = 64;
#define NEO_CROSSHAIR_DEFAULT "2;0;-1;0;6;0.000;2;4;0;0;1;0;0;"

#define TUTORIAL_MAP_CLASSES "ntre_class_tut"
#define TUTORIAL_MAP_SHOOTING "ntre_shooting_tut"

enum
{
	TEAM_JINRAI = LAST_SHARED_TEAM + 1,
	TEAM_NSF,

	TEAM__TOTAL, // Always last enum in here
};

#define TEAM_STR_JINRAI "Jinrai"
#define TEAM_STR_NSF "NSF"
#define TEAM_STR_SPEC "Spectator"

static constexpr const SZWSZTexts SZWSZ_NEO_TEAM_STRS[TEAM__TOTAL] = {
	SZWSZ_INIT("Unassigned"), // TEAM_UNASSIGNED
	SZWSZ_INIT("Spectator"), // TEAM_SPECTATOR
	X_SZWSZ_INIT(TEAM_STR_JINRAI), // TEAM_JINRAI
	X_SZWSZ_INIT(TEAM_STR_NSF), // TEAM_NSF
};

#define NEO_GAME_NAME "Neotokyo; Rebuild"

#endif // NEO_PLAYER_SHARED_H
