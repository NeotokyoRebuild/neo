#include "c_neo_killer_damage_infos.h"

#include <cbase.h>
#include "strtools.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"

#include "c_user_message_register.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void NeoUserIDsLocalKilledClear()
{
	g_neoUserIDsLocalKilledSize = 0;
	V_memset(g_neoUserIDsLocalKilled, 0, sizeof(g_neoUserIDsLocalKilled));
}

void NeoDamageReportClear()
{
	g_neoDamageReportSize = 0;
	V_memset(g_neoDamageReport, 0, sizeof(g_neoDamageReport));
}

// Console + activation of damage info
static void __MsgFunc_KillerDamageInfo(bf_read &msg)
{
	const int killerIdx = msg.ReadShort();
	char killedBy[MAX_PLAYER_NAME_LENGTH] = {};
	const bool foundKilledBy = msg.ReadString(killedBy, sizeof(killedBy), false);

	auto *localPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!localPlayer)
	{
		return;
	}

	// Print damage stats into the console
	// Print to console
	const int thisIdx = localPlayer->entindex();

	// Can't rely on Msg as it can print out of order, so do it in chunks
	static char killByLine[512];

	static const char *BORDER = "==========================\n";
	bool setKillByLine = false;

	V_memset(&g_neoKillerInfos, 0, sizeof(CNEOKillerInfos));
	g_neoKillerInfos.bHasDmgInfos = true;

	if (killerIdx > 0 && killerIdx <= gpGlobals->maxClients)
	{
		auto *neoAttacker = assert_cast<C_NEO_Player*>(UTIL_PlayerByIndex(killerIdx));
		if (neoAttacker && neoAttacker->entindex() != thisIdx)
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(neoAttacker->GetNeoPlayerName(),
												  g_neoKillerInfos.wszKillerName,
												  sizeof(g_neoKillerInfos.wszKillerName));
		}
		// If not neoAttacker, already cleared out by memset earlier

		if (neoAttacker)
		{
			g_neoKillerInfos.iEntIndex = killerIdx;
			g_neoKillerInfos.iClass = neoAttacker->GetClass();
			g_neoKillerInfos.iHP = neoAttacker->GetHealth();
			g_neoKillerInfos.flDistance = METERS_PER_INCH * neoAttacker->GetAbsOrigin().DistTo(localPlayer->GetAbsOrigin());
			if (foundKilledBy)
			{
				// Rename very long names
				if (V_strcmp(killedBy, "MURATA SUPA 7") == 0)
				{
					V_strcpy_safe(killedBy, "SUPA 7");
				}
				g_pVGuiLocalize->ConvertANSIToUnicode(killedBy,
						g_neoKillerInfos.wszKilledWith,
						sizeof(g_neoKillerInfos.wszKilledWith));
			}
			else
			{
				g_neoKillerInfos.wszKilledWith[0] = L'\0';
			}

			V_sprintf_safe(killByLine, "Killed by: %s [%s | %d hp] with %s at %.0f m\n",
						   neoAttacker->GetNeoPlayerName(), GetNeoClassName(g_neoKillerInfos.iClass),
						   g_neoKillerInfos.iHP, foundKilledBy ? killedBy : "", g_neoKillerInfos.flDistance);
			setKillByLine = true;
		}
	}
	else if (killerIdx == NEO_ENVIRON_KILLED)
	{
		g_neoKillerInfos.iEntIndex = NEO_ENVIRON_KILLED;
		g_neoKillerInfos.wszKilledWith[0] = L'\0';
		if (foundKilledBy && V_strcmp(killedBy, "neo_npc_targetsystem") == 0)
		{
			const char *pszMap = IGameSystem::MapName();
			if (V_strcmp(pszMap, "ntre_rogue_ctg") == 0)
			{
				V_wcscpy_safe(g_neoKillerInfos.wszKilledWith, L"184-J IFV");
			}
			else
			{
				V_wcscpy_safe(g_neoKillerInfos.wszKilledWith, L"Turret");
			}
		}
	}

	ConMsg("%sDamage infos (Round %d):\n%s\n", BORDER, NEORules()->roundNumber(), setKillByLine ? killByLine : "");

	NeoDamageReportClear();
	AttackersTotals totals = {};

	// Read per-player damage stats from the message (authoritative server data)
	const int iAtkSize = msg.ReadShort();
	for (int i = 0; i < iAtkSize; ++i)
	{
		AttackersTotals *pDmgReport = &g_neoDamageReport[g_neoDamageReportSize];
		pDmgReport->iUserID = msg.ReadShort();
		pDmgReport->dealtDmgs = msg.ReadShort();
		pDmgReport->dealtHits = msg.ReadShort();
		pDmgReport->takenDmgs = msg.ReadShort();
		pDmgReport->takenHits = msg.ReadShort();

		auto *neoAttacker = USERID2NEOPLAYER(pDmgReport->iUserID);
		if (!neoAttacker || neoAttacker->IsHLTV())
		{
			continue;
		}

		const char *dmgerName = neoAttacker->GetNeoPlayerName();
		if (!dmgerName)
		{
			continue;
		}

		const char *dmgerClass = GetNeoClassName(neoAttacker->GetClass());

		char infoLine[128] = {};
		if (pDmgReport->dealtDmgs > 0 && pDmgReport->dealtHits > 0)
		{
			V_sprintf_safe(infoLine, "Damage dealt to %s [%s]: %d in %d hits\n",
					   dmgerName, dmgerClass,
					   pDmgReport->dealtDmgs, pDmgReport->dealtHits);
			ConMsg("%s", infoLine);
			totals.takenDmgs += pDmgReport->dealtDmgs;
			totals.takenHits += pDmgReport->dealtHits;
		}
		if (pDmgReport->takenDmgs > 0 && pDmgReport->takenHits > 0)
		{
			V_sprintf_safe(infoLine, "Damage taken from %s [%s]: %d in %d hits\n",
					   dmgerName, dmgerClass,
					   pDmgReport->takenDmgs, pDmgReport->takenHits);
			ConMsg("%s", infoLine);
			totals.dealtDmgs += pDmgReport->takenDmgs;
			totals.dealtHits += pDmgReport->takenHits;
		}

		++g_neoDamageReportSize;
	}

	ConMsg("Total damage dealt: %d in %d hits\nTotal damage received from players: %d in %d hits\n%s\n",
		totals.dealtDmgs, totals.dealtHits,
		totals.takenDmgs, totals.takenHits,
		BORDER);
}
USER_MESSAGE_REGISTER(KillerDamageInfo);

