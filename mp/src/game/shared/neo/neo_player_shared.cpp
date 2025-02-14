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

#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO | FCVAR_ARCHIVE,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);
ConVar neo_aim_hold("neo_aim_hold", "0", FCVAR_USERINFO | FCVAR_ARCHIVE, "Hold to aim as opposed to toggle aim.", true, 0.0f, true, 1.0f);
#endif
ConVar neo_recon_superjump_intensity("neo_recon_superjump_intensity", "250", FCVAR_REPLICATED | FCVAR_CHEAT,
	"Recon superjump intensity multiplier.", true, 1.0, false, 0);

ConVar neo_ghost_bhopping("neo_ghost_bhopping", "0", FCVAR_REPLICATED, "Allow ghost bunnyhopping", true, 0, true, 1);

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
		NEO_WEP_SMOKE_GRENADE;

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
	if (!player)
	{
		return false;
	}
	else if (player->GetFlags() & FL_FAKECLIENT)
	{
		return true;
	}

	return 1 == atoi(engine->GetClientConVarValue(engine->IndexOfEdict(player->edict()), "neo_aim_hold"));
#endif
}

int DmgLineStr(char* infoLine, const int infoLineMax,
	const char* dmgerName, const char* dmgerClass,
	const AttackersTotals &totals)
{
	memset(infoLine, 0, infoLineMax);
	if (totals.dealtDmgs > 0 && totals.takenDmgs > 0)
	{
		Q_snprintf(infoLine, infoLineMax, "%s [%s]: Dealt: %d in %d hits | Taken: %d in %d hits\n",
				   dmgerName, dmgerClass,
				   totals.dealtDmgs, totals.dealtHits, totals.takenDmgs, totals.takenHits);
	}
	else if (totals.dealtDmgs > 0)
	{
		Q_snprintf(infoLine, infoLineMax, "%s [%s]: Dealt: %d in %d hits\n",
				   dmgerName, dmgerClass,
				   totals.dealtDmgs, totals.dealtHits);
	}
	else if (totals.takenDmgs > 0)
	{
		Q_snprintf(infoLine, infoLineMax, "%s [%s]: Taken: %d in %d hits\n",
				   dmgerName, dmgerClass,
				   totals.takenDmgs, totals.takenHits);
	}
	return Q_strlen(infoLine);
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

void GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH + 1],
						 const wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH + 1],
						 const bool bOnlySteamNick)
{
	const bool bShowSteamNick = bOnlySteamNick || wszNeoName[0] == '\0';
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
}

void GetClNeoDisplayName(wchar_t (&pWszDisplayName)[NEO_MAX_DISPLAYNAME],
						 const char *pSzNeoName,
						 const char *pSzNeoClantag,
						 const bool bOnlySteamNick)
{
	wchar_t wszNeoName[MAX_PLAYER_NAME_LENGTH + 1];
	wchar_t wszNeoClantag[NEO_MAX_CLANTAG_LENGTH + 1];
	g_pVGuiLocalize->ConvertANSIToUnicode(pSzNeoName, wszNeoName, sizeof(wszNeoName));
	g_pVGuiLocalize->ConvertANSIToUnicode(pSzNeoClantag, wszNeoClantag, sizeof(wszNeoClantag));
	GetClNeoDisplayName(pWszDisplayName, wszNeoName, wszNeoClantag, bOnlySteamNick);
}

