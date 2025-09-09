#include "cbase.h"
#include "neo_player_shared.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#include "tier0/valve_minmax_off.h"
#include <tier3/tier3.h>
#include <vgui/ILocalize.h>
#include <steam/steam_api.h>
#include <system_error>
#include <charconv>
#include "tier0/valve_minmax_on.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#include "c_playerresource.h"
#ifndef CNEO_Player
#define CNEO_Player C_NEO_Player
#endif
#else
#include "neo_player.h"
#endif

#include "convar.h"
#include "neo_weapon_loadout.h"

#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO | FCVAR_ARCHIVE,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);
ConVar neo_aim_hold("neo_aim_hold", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Hold to aim as opposed to toggle aim.", true, 0.0f, true, 1.0f);
#endif

ConVar neo_ghost_bhopping("neo_ghost_bhopping", "0", FCVAR_REPLICATED, "Allow ghost bunnyhopping", true, 0, true, 1);
ConVar sv_neo_dev_loadout("sv_neo_dev_loadout", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_DONTRECORD, "", true, 0.0f, true, 1.0f);

bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep)
{
	if (!pWep || pWep->m_bInReload || pWep->GetRoundBeingChambered())
	{
		return false;
	}

	// These weapons are not allowed to be zoomed in with.
	const auto forbiddenZooms =
		NEO_WEP_DETPACK |
		NEO_WEP_FRAG_GRENADE |
		NEO_WEP_GHOST |
		NEO_WEP_KNIFE |
		NEO_WEP_PROX_MINE |
		NEO_WEP_SMOKE_GRENADE |
		NEO_WEP_BALC;

	return !(pWep->GetNeoWepBits() & forbiddenZooms);
}

CBaseCombatWeapon* GetNeoWepWithBits(const CNEO_Player* player, const NEO_WEP_BITS_UNDERLYING_TYPE& neoWepBits)
{
	int nonNeoWepFoundAtIndex = -1;
	for (int i = 0; i < MAX_WEAPONS; i++)
	{
		auto pWep = player->GetWeapon(i);
		if (!pWep)
		{
			continue;
		}

		auto pNeoWep = dynamic_cast<CNEOBaseCombatWeapon*>(pWep);
		if (!pNeoWep)
		{
			nonNeoWepFoundAtIndex = i;
			continue;
		}

		if (pNeoWep->GetNeoWepBits() & neoWepBits)
		{
			return pWep;
		}
	}

	// NEO HACK (Rain): If wanted a knife, assume it's the non-CNEOBaseCombatWeapon one we found.
	// Should clean this up once all guns inherit from CNEOBaseCombatWeapon.
	if ((neoWepBits & NEO_WEP_KNIFE) && nonNeoWepFoundAtIndex != -1)
	{
		return player->GetWeapon(nonNeoWepFoundAtIndex);
	}

	return NULL;
}

bool ClientWantsAimHold(const CNEO_Player* player)
{
#ifdef CLIENT_DLL
	return neo_aim_hold.GetBool();
#else
	if (!player || player->IsBot())
	{
		return false;
	}

	return 1 == atoi(engine->GetClientConVarValue(engine->IndexOfEdict(player->edict()), "neo_aim_hold"));
#endif
}

#ifdef CLIENT_DLL
extern ConVar cl_neo_player_pings;
#endif // CLIENT_DLL
void CheckPingButton(CNEO_Player* player)
{
	if (!player->IsAlive() || !(player->m_afButtonPressed & IN_ATTACK3) || player->m_flNextPingTime > gpGlobals->curtime)
	{
		return;
	}

	if (!player->IsBot())
	{ // players with pings disabled can't ping
#ifdef GAME_DLL
		const bool showPlayerPings = atoi(engine->GetClientConVarValue(engine->IndexOfEdict(player->edict()), "cl_neo_player_pings"));
#else
		const bool showPlayerPings = cl_neo_player_pings.GetBool();
#endif // GAME_DLL
		if (!showPlayerPings)
		{
			return;
		}
	}

	IGameEvent* event = gameeventmanager->CreateEvent("player_ping");
	if (event)
	{
		trace_t tr;
		Vector forward;
		player->EyeVectors(&forward);
		Vector eyePosition = player->EyePosition();
		UTIL_TraceLine(eyePosition, eyePosition + forward * MAX_COORD_RANGE, MASK_VISIBLE_AND_NPCS, player, COLLISION_GROUP_NONE, &tr);

		if (Q_stristr(tr.surface.name, "SKYBOX"))
		{
			return;
		}

		event->SetInt("userid", player->GetUserID());
		event->SetInt("playerteam", player->GetTeamNumber());
		event->SetInt("pingx", tr.endpos.x);
		event->SetInt("pingy", tr.endpos.y);
		event->SetInt("pingz", tr.endpos.z);
		event->SetBool("ghosterping", player->IsCarryingGhost() || player->m_iNeoClass == NEO_CLASS_VIP);
#ifdef GAME_DLL
		gameeventmanager->FireEvent(event);
#else
		gameeventmanager->FireEventClientSide(event);
#endif // GAME_DLL
		constexpr float NEO_PING_DELAY = 2.f;
		if ((gpGlobals->curtime - player->m_flNextPingTime) < NEO_PING_DELAY)
		{ // NEO TODO (Adam) fix for pings placed during the first NEO_PING_DELAY seconds of the server's life?
			player->m_flNextPingTime = gpGlobals->curtime + NEO_PING_DELAY;
		}
		else
		{
			player->m_flNextPingTime = gpGlobals->curtime;
		}
	}
}

void KillerLineStr(char* killByLine, const int killByLineMax,
	CNEO_Player* neoAttacker, const CNEO_Player* neoVictim, const char* killedWith)
{
	const char* dmgerName = neoAttacker->GetNeoPlayerName();
	const char* dmgerClass = GetNeoClassName(neoAttacker->GetClass());
	const int dmgerHP = neoAttacker->GetHealth();
	const float distance = METERS_PER_INCH * neoAttacker->GetAbsOrigin().DistTo(neoVictim->GetAbsOrigin());

	memset(killByLine, 0, killByLineMax);
	Q_snprintf(killByLine, killByLineMax, "Killed by: %s [%s | %d hp] with %s at %.0f m\n",
		dmgerName, dmgerClass, dmgerHP, killedWith, distance);
}

[[nodiscard]] auto StrToInt(std::string_view strView) -> std::optional<int>
{
	int value{};
	if (std::from_chars(strView.data(), strView.data() + strView.size(), value).ec == std::errc{})
	{
		return value;
	}
	return std::nullopt;
}

[[nodiscard]] int NeoAimFOV(const int fovDef, CBaseCombatWeapon *wep)
{
	static constexpr float FOV_AIM_OFFSET_FALLBACK = 30.0f;
	auto *neoWep = dynamic_cast<CNEOBaseCombatWeapon *>(wep);
	return (neoWep) ? neoWep->GetWpnData().iAimFOV : fovDef - FOV_AIM_OFFSET_FALLBACK;
}

#ifdef CLIENT_DLL
void DMClSortedPlayers(PlayerXPInfo (*pPlayersOrder)[MAX_PLAYERS + 1], int *piTotalPlayers)
{
	int iTotalPlayers = 0;

	// First pass: Find all scores of all players
	for (int i = 0; i < (MAX_PLAYERS + 1); i++)
	{
		if (g_PR->IsConnected(i))
		{
			const int playerTeam = g_PR->GetTeam(i);
			if (playerTeam == TEAM_JINRAI || playerTeam == TEAM_NSF)
			{
				(*pPlayersOrder)[iTotalPlayers++] = PlayerXPInfo{
					.idx = i,
					.xp = g_PR->GetXP(i),
					.deaths = g_PR->GetDeaths(i),
				};
			}
		}
	}

	V_qsort_s(*pPlayersOrder, iTotalPlayers, sizeof(PlayerXPInfo),
			  []([[maybe_unused]] void *vpCtx, const void *vpLeft, const void *vpRight) -> int {
		auto *pLeft = static_cast<const PlayerXPInfo *>(vpLeft);
		auto *pRight = static_cast<const PlayerXPInfo *>(vpRight);
		if (pRight->xp == pLeft->xp)
		{
			// More deaths = lower
			return pLeft->deaths - pRight->deaths;
		}
		// More XP = higher
		return pRight->xp - pLeft->xp;
	}, nullptr);

	*piTotalPlayers = iTotalPlayers;
}
#endif

bool GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const wchar_t (&wszNeoName)[MAX_PLAYER_NAME_LENGTH],
						 const wchar_t (&wszNeoClantag)[NEO_MAX_CLANTAG_LENGTH],
						 const EClNeoDisplayNameFlag flags)
{
	const bool bShowSteamNick = (flags & CL_NEODISPLAYNAME_FLAG_ONLYSTEAMNICK) || wszNeoName[0] == '\0';
	wchar_t wszDisplayName[MAX_PLAYER_NAME_LENGTH + 1] = {};
	if (bShowSteamNick)
	{
		g_pVGuiLocalize->ConvertANSIToUnicode(steamapicontext->SteamFriends()->GetPersonaName(),
											  wszDisplayName, sizeof(wszDisplayName));
	}
	else
	{
		V_wcscpy_safe(wszDisplayName, wszNeoName);
	}

	if (wszNeoClantag[0] != L'\0')
	{
		V_swprintf_safe(pWszDisplayName, L"[%ls] %ls", wszNeoClantag, wszDisplayName);
	}
	else
	{
		V_wcscpy_safe(pWszDisplayName, wszDisplayName);
	}

	if (flags & CL_NEODISPLAYNAME_FLAG_CHECK)
	{
		// Double it so we can check for overflow
		char szNeoName[2 * MAX_PLAYER_NAME_LENGTH];
		const int iSzNeoNameSize = g_pVGuiLocalize->ConvertUnicodeToANSI(wszNeoName, szNeoName, sizeof(szNeoName));
		if (iSzNeoNameSize > MAX_PLAYER_NAME_LENGTH)
		{
			return false;
		}

		char szNeoClantag[2 * NEO_MAX_CLANTAG_LENGTH];
		const int iSzNeoClantagSize = g_pVGuiLocalize->ConvertUnicodeToANSI(wszNeoClantag, szNeoClantag, sizeof(szNeoClantag));
		if (iSzNeoClantagSize > NEO_MAX_CLANTAG_LENGTH)
		{
			return false;
		}
	}

	return true;
}

bool GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const char *pSzNeoName,
						 const char *pSzNeoClantag,
						 const EClNeoDisplayNameFlag flags)
{
	wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH];
	wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH];
	g_pVGuiLocalize->ConvertANSIToUnicode(pSzNeoName, wszNeoName, sizeof(wszNeoName));
	g_pVGuiLocalize->ConvertANSIToUnicode(pSzNeoClantag, wszNeoClantag, sizeof(wszNeoClantag));
	return GetClNeoDisplayName(pWszDisplayName, wszNeoName, wszNeoClantag, flags);
}

int GetRank(const int xp)
{
	int iRank = NEO_RANK_RANKLESS_DOG;
	if (xp < XP_PRIVATE)
	{
		iRank = NEO_RANK_RANKLESS_DOG;
	}
	else if (xp < XP_CORPORAL)
	{
		iRank = NEO_RANK_PRIVATE;
	}
	else if (xp < XP_SERGEANT)
	{
		iRank = NEO_RANK_CORPORAL;
	}
	else if (xp < XP_LIEUTENANT)
	{
		iRank = NEO_RANK_SERGEANT;
	}
	else
	{
		iRank = NEO_RANK_LIEUTENANT;
	}
	return iRank + 1;
}

const char *GetRankName(const int xp, const bool shortened)
{
	static constexpr const char *RANK_NAME_LONG[] = {
		"Rankless Dog", "Private", "Corporal", "Sergeant", "Lieutenant"
	};
	static constexpr const char *RANK_NAME_SHORT[] = {
		"Dog", "Pvt", "Cpl", "Sgt", "Lt"
	};
	static_assert(ARRAYSIZE(RANK_NAME_LONG) == ARRAYSIZE(RANK_NAME_SHORT));

	const int iRank = GetRank(xp);
	if (IN_BETWEEN_AR(0, iRank, ARRAYSIZE(RANK_NAME_LONG)))
	{
		return (shortened ? RANK_NAME_SHORT : RANK_NAME_LONG)[iRank];
	}
	return "";
}

