#include <cbase.h>
#include "neo_hud_killer_damage_info.h"

#include "iclientmode.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "IGameUIFuncs.h"
#include "neo_misc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info)

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(KillerDamageInfo, 0.1)

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
		// NEO TODO (nullsystem): Maybe m_bShowInfo be put back on on every show events?
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

static constexpr int PAGE_MAX_SHOW = 10;

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
		"NeoUINormal", "NHudOCR", "NHudOCRSmallNoAdditive", "ClientTitleFont", "ClientTitleFontSmall",
		"NeoUILarge"
	};
	for (int i = 0; i < NeoUI::FONT__TOTAL; ++i)
	{
		m_uiCtx.fonts[i].hdl = pScheme->GetFont(FONT_NAMES[i], true);
	}

	m_uiCtx.layout.iDefRowTall = tall / 27;
	m_uiCtx.iMarginX = wide / 192;
	m_uiCtx.iMarginY = tall / 108;
}

void CNEOHud_KillerDamageInfo::resetHUDState()
{
	V_memset(&g_neoKDmgInfos, 0, sizeof(CNEOKillerDamageInfos));
	m_iCurPage = 0;
}

// NEO TODO (nullsystem): This really only needed to update
// the info once.
void CNEOHud_KillerDamageInfo::UpdateStateForNeoHudElementDraw()
{
	const C_NEO_Player *localPlayer = C_NEO_Player::GetLocalNEOPlayer();

	// If to show or not
	m_bPlayerShownHud = localPlayer
			&& ((const_cast<C_NEO_Player *>(localPlayer)->IsPlayerDead() && g_neoKDmgInfos.bHasDmgInfos) ||
				(NEORules()->GetRoundStatus() == NeoRoundStatus::PostRound && NEORules()->roundNumber() > 0))
			&& (localPlayer->GetTeamNumber() == TEAM_JINRAI ||
				localPlayer->GetTeamNumber() == TEAM_NSF);

	// Text infos
	V_swprintf_safe(m_wszKillerTitle, L"Killed by %ls", g_neoKDmgInfos.wszKillerName);

	// UI sizing
	int iScrWide, iScrTall;
	vgui::surface()->GetScreenSize(iScrWide, iScrTall);
	const int iWide = (iScrWide / 3);
	const int iSidePad = iWide / 8;
	m_uiCtx.dPanel.x = iScrWide - iWide - iSidePad;
	m_uiCtx.dPanel.y = (iScrTall / 4);
	m_uiCtx.dPanel.wide = iScrWide - iSidePad - m_uiCtx.dPanel.x;
	m_uiCtx.dPanel.tall = iScrTall - (iScrTall / 4) - m_uiCtx.dPanel.y;

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

		V_swprintf_safe(m_wszBindBtnMsg, L"Use [%ls] to toggle | Previous: [%ls] | Next: [%ls]",
				wszBindToggle, wszBindPrev, wszBindNext);
	}

	m_iAttackersTotalsSize = 0;
	for (int pIdx = 1; pIdx <= gpGlobals->maxClients && m_iAttackersTotalsSize < MAX_PLAYERS; ++pIdx)
	{
		if (pIdx == localPlayer->entindex())
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
#define DEBUG_SHOW_ALL (0)
#if DEBUG_SHOW_ALL
		if (dmgerName)
#else
		if (dmgerName && (attackerInfo.dealtDmgs > 0 || attackerInfo.takenDmgs > 0))
#endif
		{
			m_attackersTotals[m_iAttackersTotalsSize] = attackerInfo;
			g_pVGuiLocalize->ConvertANSIToUnicode(dmgerName,
					m_wszDmgerNamesList[m_iAttackersTotalsSize],
					sizeof(m_wszDmgerNamesList[m_iAttackersTotalsSize]));
			m_iClassIdxsList[m_iAttackersTotalsSize] = neoAttacker->GetClass();

			++m_iAttackersTotalsSize;
		}
	}
	m_iTotalPages = 1 + (static_cast<float>(m_iAttackersTotalsSize - 1) / PAGE_MAX_SHOW);
}

void CNEOHud_KillerDamageInfo::DrawNeoHudElement()
{
	if (!ShouldDraw() || !m_bPlayerShownHud || !m_bShowInfo)
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
		NeoUI::Label(m_wszBindBtnMsg);

		if (g_neoKDmgInfos.killerInfo.iEntIndex > 0)
		{
			NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
			// Killer title
			{
				NeoUI::SetPerRowLayout(1);
				if (g_neoKDmgInfos.killerInfo.iEntIndex == localPlayer->entindex())
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
			// Killer info
			{
				const int iTmpMarginX = m_uiCtx.iMarginX;
				m_uiCtx.iMarginX = m_uiCtx.iMarginX / 3;

				NeoUI::SetPerRowLayout(6);
				static wchar_t wszLabel[16] = {};

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_RIGHT;
				NeoUI::Label(L"HP:");
				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				V_swprintf_safe(wszLabel, L"%d", g_neoKDmgInfos.killerInfo.iHP);
				NeoUI::Label(wszLabel);

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_RIGHT;
				NeoUI::Label(L"Distance:");
				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				V_swprintf_safe(wszLabel, L"%.2f", g_neoKDmgInfos.killerInfo.flDistance);
				NeoUI::Label(wszLabel);

				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_RIGHT;
				NeoUI::Label(L"Class:");
				m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_LEFT;
				V_swprintf_safe(wszLabel, L"%ls", GetNeoClassNameW(g_neoKDmgInfos.killerInfo.iClass));
				NeoUI::Label(wszLabel);

				m_uiCtx.iMarginX = iTmpMarginX;
			}
		}

		// General infos - Inflicted damage, name, received damage
		{
			// Display pagination info
			{
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
			const int iCurPageEnd = min(m_iAttackersTotalsSize, iCurPageStart + PAGE_MAX_SHOW);

			NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
			NeoUI::Label(L"Dealt");
			NeoUI::Label(L"Player");
			NeoUI::Label(L"Taken");

			for (int i = iCurPageStart; i < iCurPageEnd; ++i)
			{
				const auto &attackerInfo = m_attackersTotals[i];
				static wchar_t wszLabel[32] = {};

				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				V_swprintf_safe(wszLabel, L"%d (%d)", attackerInfo.dealtDmgs, attackerInfo.dealtHits);
				NeoUI::Label(wszLabel);

				NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
				NeoUI::SetPerCellVertLayout(2);
				{
					// NEO TODO (nullsystem): Add in team name?
					NeoUI::Label(m_wszDmgerNamesList[i]);
					NeoUI::Label(GetNeoClassNameW(m_iClassIdxsList[i]));
				}
				NeoUI::SetPerCellVertLayout(0);

				NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
				V_swprintf_safe(wszLabel, L"%d (%d)", attackerInfo.takenDmgs, attackerInfo.takenHits);
				NeoUI::Label(wszLabel);
			}
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

