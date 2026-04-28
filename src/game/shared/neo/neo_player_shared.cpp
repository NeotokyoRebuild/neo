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
#include "ui/neo_hud_context_hint.h"
#define CNEO_Player C_NEO_Player
#else
#include "neo_player.h"
#endif

#ifdef GAME_DLL
#include "basetypes.h"
#endif

#include "convar.h"
#include "neo_weapon_loadout.h"
#include "weapon_neobasecombatweapon.h"
#include "igameresources.h"
#ifdef GAME_DLL
#include "ai_basenpc.h"
#else
#include "c_ai_basenpc.h"
#endif // GAME_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifdef CLIENT_DLL
ConVar cl_autoreload_when_empty("cl_autoreload_when_empty", "1", FCVAR_USERINFO | FCVAR_ARCHIVE,
	"Automatically start reloading when the active weapon becomes empty.",
	true, 0.0f, true, 1.0f);
#endif

ConVar sv_neo_dev_loadout("sv_neo_dev_loadout", "0", FCVAR_CHEAT | FCVAR_REPLICATED | FCVAR_HIDDEN | FCVAR_DONTRECORD, "", true, 0.0f, true, 1.0f);

// This default value is not a typo. The OGNT ghost beacon distance is 1800 Hammer units/inches, which equals a little over 45 meters.
// Since we can represent this value exactly with floating point, it's not really a problem to store it as meters here.
ConVar sv_neo_ghost_view_distance("sv_neo_ghost_view_distance", "45.72", FCVAR_NOTIFY | FCVAR_REPLICATED, "How far can the ghost user see players in meters.");

ConVar sv_neo_ghost_delay_secs("sv_neo_ghost_delay_secs", "3.3", FCVAR_NOTIFY | FCVAR_REPLICATED,
	"The delay in seconds until the ghost shows up after pick up.", true, 0.0, false, 0.0);

ConVar sv_neo_serverside_beacons("sv_neo_serverside_beacons", "1", FCVAR_NOTIFY | FCVAR_REPLICATED,
	"Whether ghost beacons should be processed server-side.", true, false, true, true);

ConVar sv_neo_spec_replace_player_bot_enable("sv_neo_spec_replace_player_bot_enable", "1", FCVAR_REPLICATED, "Allow spectators to take over bots.", true, 0, true, 1);
ConVar sv_neo_spec_replace_player_afk_enable("sv_neo_spec_replace_player_afk_enable", "0", FCVAR_REPLICATED, "Allow spectators to take over AFK players.", true, 0, true, 1);
ConVar sv_neo_spec_replace_player_min_exp("sv_neo_spec_replace_player_min_exp",
	"0", FCVAR_REPLICATED,
	"Minimum experience allowed to takeover players ",
	true, -999, true, 999);
ConVar sv_neo_spec_replace_player_afk_time_sec( "sv_neo_spec_replace_player_afk_time_sec",
	"180", FCVAR_NONE,
	"Seconds of inactivity before a player is considered AFK for spectator takeover.",
	true, -1, true, 999);

bool IsAllowedToZoom(CNEOBaseCombatWeapon *pWep)
{
	if (!pWep || pWep->m_bInReload || !pWep->CanAim())
	{
		return false;
	}

	if (pWep->GetNeoWepBits() & NEO_WEP_SRS && pWep->GetIdealActivity() == ACT_VM_PULLBACK) {
		return false;
	}

	return true;
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

		UpdatePingCommands(player, tr.endpos);
	}
}

ConVar sv_neo_bot_cmdr_enable("sv_neo_bot_cmdr_enable", "1",
	FCVAR_REPLICATED | FCVAR_ARCHIVE, "Allow bots to follow you after you press use on them", true, 0, true, 1);

#ifdef GAME_DLL
static ConVar sv_neo_bot_cmdr_stop_distance_sq_max("sv_neo_bot_cmdr_stop_distance_sq_max", "50000",
	FCVAR_CHEAT, "Maximum distance bot following gap interval can be set by player pings", true, 5000, true, 500000);
static ConVar sv_neo_bot_cmdr_ping_ignore_delay_min_sec("sv_neo_bot_cmdr_ping_ignore_delay_min_sec", "3",
	FCVAR_CHEAT, "Minimum time bots ignore pings for new waypoint settings", true, 0, true, 1000);
#endif // GAME_DLL

void UpdatePingCommands(CNEO_Player* player, const Vector& pingPos)
{
#ifdef GAME_DLL
	if (sv_neo_bot_cmdr_enable.GetBool())
	{
		player->m_vLastPingByStar.GetForModify(player->GetStar()) = pingPos;
		float distSqrToPing = player->GetAbsOrigin().DistToSqr(pingPos);
		if (distSqrToPing < sv_neo_bot_cmdr_stop_distance_sq_max.GetFloat())
		{
			// If pinging close to self, calibrate follow distance of commanded bots based on distance to ping
			float minFollowDistanceSq = 0.0f;
			sv_neo_bot_cmdr_stop_distance_sq_max.GetMin(minFollowDistanceSq);
			player->m_flBotDynamicFollowDistanceSq = Clamp(distSqrToPing, minFollowDistanceSq, sv_neo_bot_cmdr_stop_distance_sq_max.GetFloat());
		}
		else
		{
			player->m_flBotDynamicFollowDistanceSq = 0.0f;
		}

		player->m_tBotPlayerPingCooldown.Start(sv_neo_bot_cmdr_ping_ignore_delay_min_sec.GetFloat());
	}
#endif
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
	return (neoWep) ? neoWep->GetNEOWpnData().iAimFOV : fovDef - FOV_AIM_OFFSET_FALLBACK;
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

void CNEO_Player::CheckAimButtons()
{
	if (auto *pNeoWep = static_cast<CNEOBaseCombatWeapon *>(GetActiveWeapon()))
	{
		if (IsSprinting() || !IsAllowedToZoom(pNeoWep))
		{
			Weapon_SetZoom(false);
		}
		else if (m_nButtons & IN_ZOOM)
		{
			Weapon_SetZoom(true);
		}
		else if (m_afButtonReleased & IN_ZOOM)
		{
			Weapon_SetZoom(false);
		}
		else if (m_afButtonPressed & IN_AIM)
		{
			Weapon_SetZoom(!IsInAim());
		}
	}
	else
	{
	    Weapon_SetZoom(false);
	}
}

bool CNEO_Player::IsAFK() const
{
#ifdef GAME_DLL
	return gpGlobals->curtime - m_flLastInputTime > sv_neo_spec_replace_player_afk_time_sec.GetInt();
#else
	return GameResources()->IsAfk(entindex());
#endif // GAME_DLL
}

bool CNEO_Player::IsFakePlayer() const
{
#ifdef GAME_DLL
	return IsBot();
#else
	return GameResources()->IsFakePlayer(entindex());
#endif // GAME_DLL
}

bool CNEO_Player::ValidTakeoverTargetFor(CNEO_Player *pPlayerTakingOver)
{
	return pPlayerTakingOver && pPlayerTakingOver->IsObserver() && !pPlayerTakingOver->IsAlive()
		&& NEORules()->GetRoundStatus() != PostRound
		&& pPlayerTakingOver->m_iXP >= sv_neo_spec_replace_player_min_exp.GetInt()
		&& IsAlive()
		&& InSameTeam(pPlayerTakingOver) && NEORules()->IsTeamplay()
		&& (sv_neo_spec_replace_player_bot_enable.GetBool() && IsFakePlayer() ||
			sv_neo_spec_replace_player_afk_enable.GetBool() && IsAFK());
}

#ifdef CLIENT_DLL
ConVar cl_neo_hud_context_hint_show_adjacent_interactable_objects("cl_neo_hud_context_hint_show_adjacent_interactable_objects", "1", FCVAR_ARCHIVE, "Show adjacent interactable objects", true, 0.f, true, 1.f);
static bool gAddUseItemsToUseItemsList = false;
bool SetAddUseEntitysToUseEntityList(bool addUseItemsToUseItemsList)
{
	if (cl_neo_hud_context_hint_show_adjacent_interactable_objects.GetBool())
		gAddUseItemsToUseItemsList = addUseItemsToUseItemsList;
	return true;
};
#endif // CLIENT_DLL

extern ConVar sv_debug_player_use;
CBaseEntity *CNEO_Player::FindUseEntity()
{
	Vector forward;
	EyeVectors( &forward, nullptr, nullptr );

	trace_t tr;
	// Search for objects in a sphere (tests for entities that are not solid, yet still useable)
	Vector searchCenter = EyePosition();

	// NOTE: Some debris objects are useable too, so hit those as well
	// A button, etc. can be made out of clip brushes, make sure it's +useable via a traceline, too.
	int useableContents = MASK_SOLID | CONTENTS_DEBRIS | CONTENTS_PLAYERCLIP;

#ifndef CLIENT_DLL
	CBaseEntity *pFoundByTrace = nullptr;
#endif

	// UNDONE: Might be faster to just fold this range into the sphere query
	CBaseEntity *pObject = nullptr;

	float nearestDot = -1;

	// try the hit entity if there is one, or the ground entity if there isn't.
	CBaseEntity *pNearest = nullptr;

	{
		UTIL_TraceLine( searchCenter, searchCenter + forward * 1024, useableContents, this, COLLISION_GROUP_NONE, &tr );
		pObject = tr.m_pEnt;

#ifndef CLIENT_DLL
		pFoundByTrace = pObject;
#endif
		bool bUsable = IsUseableEntity(pObject, 0);
		while ( pObject && !bUsable && pObject->GetMoveParent() )
		{
			pObject = pObject->GetMoveParent();
			bUsable = IsUseableEntity(pObject, 0);
		}

		if ( bUsable )
		{
			Vector delta = tr.endpos - tr.startpos;
			float centerZ = CollisionProp()->WorldSpaceCenter().z;
			delta.z = IntervalDistance( tr.endpos.z, centerZ + CollisionProp()->OBBMins().z, centerZ + CollisionProp()->OBBMaxs().z );
			float dist = delta.Length();
			if ( dist < PLAYER_USE_RADIUS )
			{
#ifndef CLIENT_DLL
				if ( sv_debug_player_use.GetBool() )
				{
					NDebugOverlay::Line( searchCenter, tr.endpos, 0, 255, 0, true, 30 );
					NDebugOverlay::Cross3D( tr.endpos, 16, 0, 255, 0, true, 30 );
				}

				if ( pObject->MyNPCPointer() && pObject->MyNPCPointer()->IsPlayerAlly( this ) )
				{
					// If about to select an NPC, do a more thorough check to ensure
					// that we're selecting the right one from a group.
					pObject = DoubleCheckUseNPC( pObject, searchCenter, forward );
				}
#endif
				if ( sv_debug_player_use.GetBool() )
				{
					Msg( "Trace using: %s\n", pObject ? pObject->GetDebugName() : "no usable entity found" );
				}

				pNearest = pObject;
				
				// if there is an entity directly under the cursor just return it now
				// NEO NOTE (Adam) weapon axis aligned collision bounds are usually far removed from where the weapon is visually. If a weapon can be interacted with in a radius, use that instead
				if (pObject && !((pObject->ObjectCaps() & FCAP_USE_IN_RADIUS) && pObject->IsBaseCombatWeapon()))
				{
#ifdef CLIENT_DLL
					// Client side do the sphere query anyway to show adjacent items
					if (gAddUseItemsToUseItemsList)
						nearestDot = 0.f;
					else
#endif // CLIENT_DLL
					return pObject;
				}
			}
		}
	}

	// check ground entity first
	// if you've got a useable ground entity, then shrink the cone of this search to 45 degrees
	// otherwise, search out in a 90 degree cone (hemisphere)
	if ( GetGroundEntity() && IsUseableEntity(GetGroundEntity(), FCAP_USE_ONGROUND) )
	{
		pNearest = GetGroundEntity();
	}
	if ( pNearest )
	{
		// estimate nearest object by distance from the view vector
		nearestDot = DotProduct((pNearest->CollisionProp()->WorldSpaceCenter() - searchCenter).Normalized(), forward);
		if ( sv_debug_player_use.GetBool() )
		{
			Vector point;
			pNearest->CollisionProp()->CalcNearestPoint( searchCenter, &point );
			Msg("Trace found %s, dist %.2f\n", pNearest->GetClassname(), CalcDistanceToLine( point, searchCenter, forward ) );
		}
	}

#ifdef CLIENT_DLL
	int useEntityListIndex = 0;
	ClearUseEntityListEntry();
#endif // CLIENT_DLL
	for ( CEntitySphereQuery sphere( searchCenter, PLAYER_USE_RADIUS ); ( pObject = sphere.GetCurrentEntity() ) != nullptr; sphere.NextEntity() )
	{
		if ( !pObject )
			continue;

#ifdef CLIENT_DLL
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );
		float dot = -1;
		dot = DotProduct((pObject->CollisionProp()->WorldSpaceCenter() - searchCenter).Normalized(), forward);

		if (gAddUseItemsToUseItemsList && IsUseableEntity(pObject, 0) && dot >= 0.8)
		{
			pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );
			trace_t trCheckOccluded;
			const int useableOccluder = MASK_SOLID | CONTENTS_PLAYERCLIP;
			UTIL_TraceLine( searchCenter, point, useableOccluder, this, COLLISION_GROUP_PLAYER, &trCheckOccluded );

			if (trCheckOccluded.fraction == 1.0 || trCheckOccluded.m_pEnt == pObject)
			{
				SetUseEntityListEntry(useEntityListIndex, pObject);
				useEntityListIndex ++;
			}
		}
#endif // CLIENT_DLL

		if ( !IsUseableEntity( pObject, FCAP_USE_IN_RADIUS ) )
			continue;

#ifdef GAME_DLL
		float dot = -1;
		dot = DotProduct((pObject->CollisionProp()->WorldSpaceCenter() - searchCenter).Normalized(), forward);
#endif // GAME_DLL

		// Need to be looking at the object more or less
		if ( dot < 0.8 )
			continue;

#ifdef GAME_DLL
		Vector point;
		pObject->CollisionProp()->CalcNearestPoint( searchCenter, &point );
#endif // GAME_DLL
		
		if ( sv_debug_player_use.GetBool() )
		{
			// NEO NOTE (Adam) looks like CEntitySphereQuery is using surrounding bounds instead of collision bounds. This distance could be significantly greater than PLAYER_USE_RADIUS
			Msg("Radius found %s, dist %.2f\n", pObject->GetClassname(), CalcDistanceToLine( point, searchCenter, forward ) );
		}

		if ( dot > nearestDot )
		{
			// Since this has purely been a radius search to this point, we now
			// make sure the object isn't behind glass or a grate.
			trace_t trCheckOccluded;
			const int useableOccluder = MASK_SOLID | CONTENTS_PLAYERCLIP;
			UTIL_TraceLine( searchCenter, point, useableOccluder, this, COLLISION_GROUP_PLAYER, &trCheckOccluded );

			if ( trCheckOccluded.fraction == 1.0 || trCheckOccluded.m_pEnt == pObject )
			{
				pNearest = pObject;
				nearestDot = dot;
			}
		}
	}

#ifndef CLIENT_DLL
	if ( !pNearest )
	{
		// Haven't found anything near the player to use, nor any NPC's at distance.
		// Check to see if the player is trying to select an NPC through a rail, fence, or other 'see-though' volume.
		trace_t trAllies;
		UTIL_TraceLine( searchCenter, searchCenter + forward * PLAYER_USE_RADIUS, MASK_OPAQUE_AND_NPCS, this, COLLISION_GROUP_NONE, &trAllies );

		if ( trAllies.m_pEnt && IsUseableEntity( trAllies.m_pEnt, 0 ) && trAllies.m_pEnt->MyNPCPointer() && trAllies.m_pEnt->MyNPCPointer()->IsPlayerAlly( this ) )
		{
			// This is an NPC, take it!
			pNearest = trAllies.m_pEnt;
		}
	}

	if ( pNearest && pNearest->MyNPCPointer() && pNearest->MyNPCPointer()->IsPlayerAlly( this ) )
	{
		pNearest = DoubleCheckUseNPC( pNearest, searchCenter, forward );
	}

	if ( sv_debug_player_use.GetBool() )
	{
		if ( !pNearest )
		{
			NDebugOverlay::Line( searchCenter, tr.endpos, 255, 0, 0, true, 30 );
			NDebugOverlay::Cross3D( tr.endpos, 16, 255, 0, 0, true, 30 );
		}
		else if ( pNearest == pFoundByTrace )
		{
			NDebugOverlay::Line( searchCenter, tr.endpos, 0, 255, 0, true, 30 );
			NDebugOverlay::Cross3D( tr.endpos, 16, 0, 255, 0, true, 30 );
		}
		else
		{
			NDebugOverlay::Box( pNearest->WorldSpaceCenter(), Vector(-8, -8, -8), Vector(8, 8, 8), 0, 255, 0, true, 30 );
		}
	}
#endif

	if ( sv_debug_player_use.GetBool() )
	{
		Msg( "Radial using: %s\n", pNearest ? pNearest->GetDebugName() : "no usable entity found" );
	}

	return pNearest;
}
