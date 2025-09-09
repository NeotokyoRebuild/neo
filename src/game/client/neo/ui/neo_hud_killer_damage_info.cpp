#include <cbase.h>
#include "neo_hud_killer_damage_info.h"

#include "iclientmode.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "IGameUIFuncs.h"
#include "neo_misc.h"
#include "c_user_message_register.h"
#include "igamesystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_neo_kdinfo_toggletype("cl_neo_kdinfo_toggletype", "0", FCVAR_ARCHIVE,
		"Killer damage information HUD toggle behavior, 0 = always auto show, 1 = auto show based on toggle per match, 2 = never auto show",
		true, 0.0f, true, static_cast<float>(KDMGINFO_TOGGLETYPE__TOTAL - 1));
ConVar cl_neo_kdinfo_debug_show_local("cl_neo_kdinfo_debug_show_local", "0", FCVAR_CHEAT | FCVAR_HIDDEN | FCVAR_DONTRECORD, "Killer damage information HUD debug: Show local player in the list");

DECLARE_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info)

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(KillerDamageInfo, 0.1)

static const constexpr float UI_SCALE = 0.66f;

CON_COMMAND_F(kdinfo_toggle, "Toggle killer damage info", FCVAR_USERINFO)
{
	CNEOHud_KillerDamageInfo *pKillerDmgInfo = GET_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info);
	Assert(pKillerDmgInfo);
	if (!pKillerDmgInfo)
	{
		return;
	}

	// Only allow toggle when it's at a state available to show the stats
	// to the player
	if (pKillerDmgInfo->m_bPlayerShownHud)
	{
		pKillerDmgInfo->m_bShowInfo = !pKillerDmgInfo->m_bShowInfo;
	}
}

CON_COMMAND_F(kdinfo_page_prev, "Killer damage info previous page", FCVAR_USERINFO)
{
	CNEOHud_KillerDamageInfo *pKillerDmgInfo = GET_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info);
	Assert(pKillerDmgInfo);
	if (!pKillerDmgInfo)
	{
		return;
	}

	pKillerDmgInfo->m_iCurPage = LoopAroundInArray(pKillerDmgInfo->m_iCurPage - 1, pKillerDmgInfo->m_iTotalPages);
}

CON_COMMAND_F(kdinfo_page_next, "Killer damage info next page", FCVAR_USERINFO)
{
	CNEOHud_KillerDamageInfo *pKillerDmgInfo = GET_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info);
	Assert(pKillerDmgInfo);
	if (!pKillerDmgInfo)
	{
		return;
	}

	pKillerDmgInfo->m_iCurPage = LoopAroundInArray(pKillerDmgInfo->m_iCurPage + 1, pKillerDmgInfo->m_iTotalPages);
}

static constexpr int PAGE_MAX_SHOW = 6;

CNEOHud_KillerDamageInfo::CNEOHud_KillerDamageInfo(const char *pszName, vgui::Panel *parent)
	: CHudElement(pszName)
	, vgui::Panel(parent, pszName)
{
	SetAutoDelete(true);

	if (parent)
	{
		SetParent(parent);
	}
	else
	{
		SetParent(g_pClientMode->GetViewport());
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	SetVisible(false);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

CNEOHud_KillerDamageInfo::~CNEOHud_KillerDamageInfo()
{
	NeoUI::FreeContext(&m_uiCtx);
}

void CNEOHud_KillerDamageInfo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	vgui::Panel::ApplySchemeSettings(pScheme);

	m_hFont = pScheme->GetFont("NHudOCRSmall", true);

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);

	static constexpr const char *FONT_NAMES[NeoUI::FONT__TOTAL] = {
		"NeoUISmall", "NHudOCR", "NHudOCRSmallerNoAdditive", "ClientTitleFont", "ClientTitleFontSmall",
		"NeoUINormal"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		m_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	// NEO TODO (nullsystem): #1253 - Unify layout sizing with root + loading
	m_uiCtx.layout.iDefRowTall = (tall / 27) * UI_SCALE;
	m_uiCtx.iMarginX = (wide / 192) * UI_SCALE;
	m_uiCtx.iMarginY = (tall / 108) * UI_SCALE;
}

void CNEOHud_KillerDamageInfo::resetHUDState()
{
	V_memset(&g_neoKDmgInfos, 0, sizeof(CNEOKillerDamageInfos));
	for (int pIdx = 1; pIdx <= gpGlobals->maxClients; ++pIdx)
	{
		auto *neoAttacker = dynamic_cast<C_NEO_Player *>(UTIL_PlayerByIndex(pIdx));
		if (neoAttacker)
		{
			for (int i = 0; i < MAX_PLAYERS; ++i)
			{
				neoAttacker->m_rfAttackersScores.GetForModify(i) = 0;
				neoAttacker->m_rfAttackersAccumlator.GetForModify(i) = 0.0f;
				neoAttacker->m_rfAttackersHits.GetForModify(i) = 0;
			}
		}
	}
	ResetDisplayInfos();
}

void CNEOHud_KillerDamageInfo::ResetDisplayInfos()
{
	m_iCurPage = 0;
	m_iTotalPages = 0;
	m_iAttackersTotalsSize = 0;
	V_memset(m_attackersTotals, 0, sizeof(m_attackersTotals));
	V_memset(m_wszDmgerNamesList, 0, sizeof(m_wszDmgerNamesList));
}

void CNEOHud_KillerDamageInfo::UpdateStateForNeoHudElementDraw()
{
	const C_NEO_Player *localPlayer = C_NEO_Player::GetLocalNEOPlayer();

	if (m_iTotalPages > 0 && NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze)
	{
		resetHUDState();
	}

	// If to show or not
	const bool bNextPlayerShownHud = localPlayer
			&& ((const_cast<C_NEO_Player *>(localPlayer)->IsPlayerDead() && g_neoKDmgInfos.bHasDmgInfos) ||
				(NEORules()->GetRoundStatus() == NeoRoundStatus::PostRound && NEORules()->roundNumber() > 0))
			&& (localPlayer->GetTeamNumber() == TEAM_JINRAI ||
				localPlayer->GetTeamNumber() == TEAM_NSF);
	const bool bToShownStateChanged = (bNextPlayerShownHud && bNextPlayerShownHud != m_bPlayerShownHud);
	m_bPlayerShownHud = bNextPlayerShownHud;
	if (!bToShownStateChanged)
	{
		// Only bother processing when changing towards showing state
		return;
	}

	const int iTT = cl_neo_kdinfo_toggletype.GetInt();
	m_bShowInfo =
		(iTT == KDMGINFO_TOGGLETYPE_ROUND || iTT == KDMGINFO_TOGGLETYPE_NEVER) ?
			(iTT == KDMGINFO_TOGGLETYPE_ROUND) :
			m_bShowInfo;

	// Text infos
	V_swprintf_safe(m_wszKillerTitle, L"Killed by %ls", g_neoKDmgInfos.wszKillerName);

	// UI sizing
	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);
	// This is the baseline width, text width will determine full width
	const int iBaselineWide = (iScrWide / 3) * UI_SCALE;
	m_uiCtx.dPanel.tall = (iScrTall / 2) * UI_SCALE;
	m_uiCtx.dPanel.x = m_uiCtx.iMarginX * 10;
	m_uiCtx.dPanel.y = (iScrTall / 2) - (m_uiCtx.dPanel.tall / 2);

	// Set keybind message
	{
		wchar_t wszBindToggle[32] = {};
		wchar_t wszBindPrev[32] = {};
		wchar_t wszBindNext[32] = {};

		const char *szBindToggle = g_pInputSystem->ButtonCodeToString(gameuifuncs->GetButtonCodeForBind("kdinfo_toggle"));
		const char *szBindPrev = g_pInputSystem->ButtonCodeToString(gameuifuncs->GetButtonCodeForBind("kdinfo_page_prev"));
		const char *szBindNext = g_pInputSystem->ButtonCodeToString(gameuifuncs->GetButtonCodeForBind("kdinfo_page_next"));

		g_pVGuiLocalize->ConvertANSIToUnicode(szBindToggle, wszBindToggle, sizeof(wszBindToggle));
		g_pVGuiLocalize->ConvertANSIToUnicode(szBindPrev, wszBindPrev, sizeof(wszBindPrev));
		g_pVGuiLocalize->ConvertANSIToUnicode(szBindNext, wszBindNext, sizeof(wszBindNext));

		V_swprintf_safe(m_wszBindBtnMsg, L"Toggle: [%ls] | Previous: [%ls] | Next: [%ls]",
				wszBindToggle, wszBindPrev, wszBindNext);
	}

	ResetDisplayInfos();

	int iMaxNameTextWidth = 0;
	for (int pIdx = 1; pIdx <= gpGlobals->maxClients && m_iAttackersTotalsSize < MAX_PLAYERS; ++pIdx)
	{
		if (!cl_neo_kdinfo_debug_show_local.GetBool() && pIdx == localPlayer->entindex())
		{
			continue;
		}

		auto *neoAttacker = dynamic_cast<C_NEO_Player *>(UTIL_PlayerByIndex(pIdx));
		if (!neoAttacker || neoAttacker->IsHLTV())
		{
			continue;
		}

		const AttackersTotals attackerInfo = {
			.dealtDmgs = neoAttacker->GetAttackersScores(localPlayer->entindex()),
			.dealtHits = neoAttacker->GetAttackerHits(localPlayer->entindex()),
			.takenDmgs = localPlayer->GetAttackersScores(pIdx),
			.takenHits = localPlayer->GetAttackerHits(pIdx),
		};
		const char *dmgerName = neoAttacker->GetNeoPlayerName();
		const int iDmgerTeam = neoAttacker->GetTeamNumber();
#define DEBUG_SHOW_ALL (0)
#if DEBUG_SHOW_ALL
		if (dmgerName)
#else
		if (dmgerName && (attackerInfo.dealtDmgs > 0 || attackerInfo.takenDmgs > 0) &&
				(iDmgerTeam == TEAM_JINRAI || iDmgerTeam == TEAM_NSF))
#endif
		{
			m_attackersTotals[m_iAttackersTotalsSize] = attackerInfo;
			g_pVGuiLocalize->ConvertANSIToUnicode(dmgerName,
					m_wszDmgerNamesList[m_iAttackersTotalsSize],
					sizeof(m_wszDmgerNamesList[m_iAttackersTotalsSize]));
			m_iClassIdxsList[m_iAttackersTotalsSize] = neoAttacker->GetClass();
			m_iTeamIdxsList[m_iAttackersTotalsSize] = iDmgerTeam;
			m_iEntIdxsList[m_iAttackersTotalsSize] = pIdx;

			int iFontTextWidth = 0, iFontTextHeight = 0;
			vgui::surface()->GetTextSize(m_uiCtx.fonts[NeoUI::FONT_NTNORMAL].hdl,
					m_wszDmgerNamesList[m_iAttackersTotalsSize],
					iFontTextWidth, iFontTextHeight);
			iMaxNameTextWidth = Max(iMaxNameTextWidth, iFontTextWidth);

			++m_iAttackersTotalsSize;
		}
	}
	m_iTotalPages = (m_iAttackersTotalsSize > 0) ?
			(1 + (static_cast<float>(m_iAttackersTotalsSize - 1) / PAGE_MAX_SHOW)) :
			0;
	m_uiCtx.dPanel.wide = Max(iBaselineWide, static_cast<int>(((iScrWide / 6) * UI_SCALE) + iMaxNameTextWidth + (2 * m_uiCtx.iMarginX)));
}

void CNEOHud_KillerDamageInfo::DrawNeoHudElement()
{
	if (!ShouldDraw() || !m_bPlayerShownHud || !m_bShowInfo)
	{
		return;
	}

	const int iKillerEntIndex = g_neoKDmgInfos.killerInfo.iEntIndex;

	const bool bHasKiller = iKillerEntIndex > 0 || iKillerEntIndex == NEO_ENVIRON_KILLED;
	const bool bHasPages = m_iTotalPages > 0;

	const bool bHasInfo = bHasPages || bHasKiller;
	if (!bHasInfo)
	{
		return;
	}

	const C_NEO_Player *localPlayer = C_NEO_Player::GetLocalNEOPlayer();

	// Show info box while it just died spectating itself
	m_uiCtx.bgColor = COLOR_FADED_DARK;
	NeoUI::BeginContext(&m_uiCtx, NeoUI::MODE_PAINT, nullptr, "NeoHudKillerDmgInfo");
	NeoUI::BeginSection();
	{
		NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
		m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
		NeoUI::Label(m_wszBindBtnMsg);
		m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;

		if (bHasKiller)
		{
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			const bool bSuicideKill = (iKillerEntIndex == localPlayer->entindex()) ||
					(iKillerEntIndex == NEO_ENVIRON_KILLED);
			// Killer title
			{
				NeoUI::SetPerRowLayout(1);
				if (bSuicideKill)
				{
					// Show a suicide/self kill if it matches the local player's entity index
					NeoUI::HeadingLabel(L"YOU KILLED YOURSELF");
				}
				else
				{
					NeoUI::HeadingLabel(m_wszKillerTitle);
				}
			}

			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			static wchar_t wszLabel[32] = {};

			// Killer info
			if (bSuicideKill)
			{
				// Just state the weapon, the rest of the infos aren't useful
				NeoUI::SetPerRowLayout(1);
				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

				// If it's an empty weapon, it's generally a suicide command kill
				if (iKillerEntIndex == NEO_ENVIRON_KILLED)
				{
					if (g_neoKDmgInfos.killerInfo.wszKilledWith[0] != L'\0')
					{
						V_swprintf_safe(wszLabel, L"Killed by %ls", g_neoKDmgInfos.killerInfo.wszKilledWith);
						NeoUI::Label(wszLabel);
					}
					else
					{
						NeoUI::Label(L"Killed by map");
					}
				}
				else if (g_neoKDmgInfos.killerInfo.wszKilledWith[0] == L'\0')
				{
					NeoUI::Label(L"Killed by map or used suicide command");
				}
				else
				{
					V_swprintf_safe(wszLabel, L"Weapon: %ls", g_neoKDmgInfos.killerInfo.wszKilledWith);
					NeoUI::Label(wszLabel);
				}
			}
			else
			{
				const int iTmpMarginX = m_uiCtx.iMarginX;
				m_uiCtx.iMarginX = m_uiCtx.iMarginX / 3;

				NeoUI::SetPerRowLayout(4);
				NeoUI::SetPerCellVertLayout(2);

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_RIGHT;
				NeoUI::Label(L"Health:");
				NeoUI::Label(L"Distance:");

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				V_swprintf_safe(wszLabel, L"%d", g_neoKDmgInfos.killerInfo.iHP);
				NeoUI::Label(wszLabel);
				V_swprintf_safe(wszLabel, L"%.2fm", g_neoKDmgInfos.killerInfo.flDistance);
				NeoUI::Label(wszLabel);

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_RIGHT;
				NeoUI::Label(L"Class:");
				NeoUI::Label(L"Weapon:");

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				V_swprintf_safe(wszLabel, L"%ls", GetNeoClassNameW(g_neoKDmgInfos.killerInfo.iClass));
				NeoUI::Label(wszLabel);
				V_swprintf_safe(wszLabel, L"%ls", g_neoKDmgInfos.killerInfo.wszKilledWith);
				NeoUI::Label(wszLabel);

				NeoUI::SetPerCellVertLayout(0);

				m_uiCtx.iMarginX = iTmpMarginX;
			}
		}

		// General infos - Inflicted damage, name, received damage
		if (bHasPages)
		{
			// Display pagination info if more than 1 page
			if (m_iTotalPages > 1)
			{
				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;
				NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
				NeoUI::SetPerRowLayout(1);

				static wchar_t wszPageLabel[32] = {};
				V_swprintf_safe(wszPageLabel, L"(Page %d/%d)", m_iCurPage + 1, m_iTotalPages);
				NeoUI::Label(wszPageLabel);
			}

			// General attacker/inflictor infos
			static const int ROWLAYOUT_INFO[] = {
				25, 50, -1
			};
			NeoUI::SetPerRowLayout(ARRAYSIZE(ROWLAYOUT_INFO), ROWLAYOUT_INFO);
			m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

			const int iCurPageStart = m_iCurPage * PAGE_MAX_SHOW;
			const int iCurPageEnd = Min(m_iAttackersTotalsSize, iCurPageStart + PAGE_MAX_SHOW);

			const int iYOffsetting = (m_uiCtx.layout.iRowTall / 4);
			m_uiCtx.iLayoutY += iYOffsetting;

			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::Label(L"Dealt");
			NeoUI::Label(L"Player");
			NeoUI::Label(L"Taken");

			m_uiCtx.iLayoutY -= iYOffsetting;

			NeoUI::SetPerCellVertLayout(2);

			for (int i = iCurPageStart; i < iCurPageEnd; ++i)
			{
				const auto &attackerInfo = m_attackersTotals[i];
				if (attackerInfo.dealtDmgs <= 0 && attackerInfo.takenDmgs <= 0)
				{
					continue;
				}

				static wchar_t wszLabel[64] = {};

				const int iAttackerEntIdx = m_iEntIdxsList[i];
				const bool bKilledYou = (iAttackerEntIdx == iKillerEntIndex);
				const bool bYouKilled = localPlayer->m_rfNeoPlayerIdxsKilledByLocal[iAttackerEntIdx];

				// Left side - Dealt damages + hits to attacker
				if (attackerInfo.dealtDmgs > 0)
				{
					NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
					if (bYouKilled)
					{
						vgui::surface()->DrawSetTextColor(COLOR_RED);
					}
					V_swprintf_safe(wszLabel, L"%d", attackerInfo.dealtDmgs);
					NeoUI::Label(wszLabel);
					vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);

					NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
					V_swprintf_safe(wszLabel, L"%d hit%ls", attackerInfo.dealtHits, (attackerInfo.dealtHits <= 1) ? L"" : L"s");
					NeoUI::Label(wszLabel);
				}
				else
				{
					NeoUI::Pad();
					NeoUI::Pad();
				}

				// Middle - Attacker's name and class
				{
					const int iDmgerTeam = m_iTeamIdxsList[i];
					Assert(iDmgerTeam == TEAM_JINRAI || iDmgerTeam == TEAM_NSF);

					NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
					vgui::surface()->DrawSetTextColor((iDmgerTeam == TEAM_JINRAI) ? COLOR_JINRAI : COLOR_NSF);
					NeoUI::Label(m_wszDmgerNamesList[i]);
					vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);

					NeoUI::Label(GetNeoClassNameW(m_iClassIdxsList[i]));
				}

				// Right side - Taken damages + hits from attacker
				if (attackerInfo.takenDmgs > 0)
				{
					NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
					if (bKilledYou)
					{
						vgui::surface()->DrawSetTextColor(COLOR_RED);
					}
					V_swprintf_safe(wszLabel, L"%d", attackerInfo.takenDmgs);
					NeoUI::Label(wszLabel);
					vgui::surface()->DrawSetTextColor(COLOR_NEOPANELTEXTNORMAL);

					NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
					V_swprintf_safe(wszLabel, L"%d hit%ls", attackerInfo.takenHits, (attackerInfo.takenHits <= 1) ? L"" : L"s");
					NeoUI::Label(wszLabel);
				}
				else
				{
					NeoUI::Pad();
					NeoUI::Pad();
				}

				m_uiCtx.iLayoutY += iYOffsetting;
			}

			NeoUI::SetPerCellVertLayout(0);
		}
	}
	NeoUI::EndSection();
	NeoUI::EndContext();
}

void CNEOHud_KillerDamageInfo::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	vgui::Panel::Paint();
	PaintNeoElement();
}

// Console + activation of damage info
static void __MsgFunc_DamageInfo(bf_read& msg)
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
	AttackersTotals totals = {};

	const int thisIdx = localPlayer->entindex();

	// Can't rely on Msg as it can print out of order, so do it in chunks
	static char killByLine[512];

	static const char* BORDER = "==========================\n";
	bool setKillByLine = false;

	V_memset(&g_neoKDmgInfos, 0, sizeof(CNEOKillerDamageInfos));
	g_neoKDmgInfos.bHasDmgInfos = true;

	if (killerIdx > 0 && killerIdx <= gpGlobals->maxClients)
	{
		auto *neoAttacker = assert_cast<C_NEO_Player*>(UTIL_PlayerByIndex(killerIdx));
		if (neoAttacker && neoAttacker->entindex() != thisIdx)
		{
			g_pVGuiLocalize->ConvertANSIToUnicode(neoAttacker->GetNeoPlayerName(),
												  g_neoKDmgInfos.wszKillerName,
												  sizeof(g_neoKDmgInfos.wszKillerName));
		}
		// If not neoAttacker, already cleared out by memset earlier

		if (neoAttacker)
		{
			CNEOKillerInfo killerInfo = {
				.iEntIndex = killerIdx,
				.iClass = neoAttacker->GetClass(),
				.iHP = neoAttacker->GetHealth(),
				.flDistance = METERS_PER_INCH * neoAttacker->GetAbsOrigin().DistTo(localPlayer->GetAbsOrigin()),
			};
			g_neoKDmgInfos.killerInfo = killerInfo;
			if (foundKilledBy)
			{
				// Rename very long names
				if (V_strcmp(killedBy, "MURATA SUPA 7") == 0)
				{
					V_strcpy_safe(killedBy, "SUPA 7");
				}
				g_pVGuiLocalize->ConvertANSIToUnicode(killedBy,
						g_neoKDmgInfos.killerInfo.wszKilledWith,
						sizeof(g_neoKDmgInfos.killerInfo.wszKilledWith));
			}
			else
			{
				g_neoKDmgInfos.killerInfo.wszKilledWith[0] = L'\0';
			}

			V_sprintf_safe(killByLine, "Killed by: %s [%s | %d hp] with %s at %.0f m\n",
						   neoAttacker->GetNeoPlayerName(), GetNeoClassName(killerInfo.iClass),
						   killerInfo.iHP, foundKilledBy ? killedBy : "", killerInfo.flDistance);
			setKillByLine = true;
		}
	}
	else if (killerIdx == NEO_ENVIRON_KILLED)
	{
		g_neoKDmgInfos.killerInfo.iEntIndex = NEO_ENVIRON_KILLED;
		g_neoKDmgInfos.killerInfo.wszKilledWith[0] = L'\0';
		if (foundKilledBy)
		{
			const char *pszMap = IGameSystem::MapName();
			if (V_strcmp(killedBy, "tank_targetsystem") == 0 && V_strcmp(pszMap, "ntre_rogue_ctg") == 0)
			{
				V_wcscpy_safe(g_neoKDmgInfos.killerInfo.wszKilledWith, L"Jeff");
			}
		}
	}

	ConMsg("%sDamage infos (Round %d):\n%s\n", BORDER, NEORules()->roundNumber(), setKillByLine ? killByLine : "");
	
	for (int pIdx = 1; pIdx <= gpGlobals->maxClients; ++pIdx)
	{
		if (pIdx == thisIdx)
		{
			continue;
		}

		auto* neoAttacker = assert_cast<C_NEO_Player*>(UTIL_PlayerByIndex(pIdx));
		if (!neoAttacker || neoAttacker->IsHLTV())
		{
			continue;
		}

		const char *dmgerName = neoAttacker->GetNeoPlayerName();
		if (!dmgerName)
		{
			continue;
		}

		const AttackersTotals attackerInfo = {
			.dealtDmgs = neoAttacker->GetAttackersScores(thisIdx),
			.dealtHits = neoAttacker->GetAttackerHits(thisIdx),
			.takenDmgs = localPlayer->GetAttackersScores(pIdx),
			.takenHits = localPlayer->GetAttackerHits(pIdx),
		};
		if (attackerInfo.dealtDmgs > 0 || attackerInfo.takenDmgs > 0)
		{
			const char *dmgerClass = GetNeoClassName(neoAttacker->GetClass());

			char infoLine[128] = {};
			if (attackerInfo.dealtDmgs > 0 && attackerInfo.takenDmgs > 0)
			{
				V_sprintf_safe(infoLine, "%s [%s]: Dealt: %d in %d hits | Taken: %d in %d hits\n",
						   dmgerName, dmgerClass,
						   attackerInfo.dealtDmgs, attackerInfo.dealtHits, attackerInfo.takenDmgs, attackerInfo.takenHits);
			}
			else if (attackerInfo.dealtDmgs > 0)
			{
				V_sprintf_safe(infoLine, "%s [%s]: Dealt: %d in %d hits\n",
						   dmgerName, dmgerClass,
						   attackerInfo.dealtDmgs, attackerInfo.dealtHits);
			}
			else if (attackerInfo.takenDmgs > 0)
			{
				V_sprintf_safe(infoLine, "%s [%s]: Taken: %d in %d hits\n",
						   dmgerName, dmgerClass,
						   attackerInfo.takenDmgs, attackerInfo.takenHits);
			}
			else
			{
				// Should never happen since the if statement one-level
				// above prevents this anyway
				Assert(false);
			}

			ConMsg("%s", infoLine);

			totals += attackerInfo;
		}
	}

	ConMsg("\nTOTAL: Dealt: %d in %d hits | Taken: %d in %d hits\n%s\n",
		totals.dealtDmgs, totals.dealtHits,
		totals.takenDmgs, totals.takenHits,
		BORDER);
}
USER_MESSAGE_REGISTER(DamageInfo);

