#include <cbase.h>
#include "neo_hud_killer_damage_info.h"

#include "iclientmode.h"

#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "IGameUIFuncs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_KillerDamageInfo, neo_killer_damage_info)

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(KillerDamageInfo, 0.1)

CON_COMMAND_F(togglekdinfo, "Toggle killer damage info", FCVAR_USERINFO)
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
}

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
	const int iWide = (iScrWide / 4);
	const int iSidePad = iWide / 8;
	m_uiCtx.dPanel.x = iScrWide - iWide - iSidePad;
	m_uiCtx.dPanel.y = (iScrTall / 4);
	m_uiCtx.dPanel.wide = iScrWide - iSidePad - m_uiCtx.dPanel.x;
	m_uiCtx.dPanel.tall = iScrTall - (iScrTall / 4) - m_uiCtx.dPanel.y;

	wchar_t wszBindBtnToggleName[64] = {};
	const char *szBindBtnName = g_pInputSystem->ButtonCodeToString(gameuifuncs->GetButtonCodeForBind("togglekdinfo"));
	g_pVGuiLocalize->ConvertANSIToUnicode(szBindBtnName, wszBindBtnToggleName, sizeof(wszBindBtnToggleName));
	V_swprintf_safe(m_wszBindBtnToggleMessage, L"Use [%ls] to toggle", wszBindBtnToggleName);
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
		NeoUI::Label(m_wszBindBtnToggleMessage);

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
				NeoUI::SetPerRowLayout(3);
				static wchar_t wszLabel[32] = {};

				V_swprintf_safe(wszLabel, L"HP: %d", g_neoKDmgInfos.killerInfo.iHP);
				NeoUI::Label(wszLabel);

				V_swprintf_safe(wszLabel, L"Distance: %.2f", g_neoKDmgInfos.killerInfo.flDistance);
				NeoUI::Label(wszLabel);

				V_swprintf_safe(wszLabel, L"Class: %ls", GetNeoClassNameW(g_neoKDmgInfos.killerInfo.iClass));
				NeoUI::Label(wszLabel);
			}
		}

		// General infos - Inflicted damage, name, received damage
		{
			static const int ROWLAYOUT_INFO[] = {
				25, 50, -1
			};
			NeoUI::SetPerRowLayout(ARRAYSIZE(ROWLAYOUT_INFO), ROWLAYOUT_INFO);
			m_uiCtx.eLabelTextStyle = NeoUI::TEXTSTYLE_CENTER;

			for (int pIdx = 1; pIdx <= gpGlobals->maxClients; ++pIdx)
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
					static wchar_t wszDmgerName[MAX_PLAYER_NAME_LENGTH + 1];
					g_pVGuiLocalize->ConvertANSIToUnicode(dmgerName, wszDmgerName, sizeof(wszDmgerName));

					static wchar_t wszLabel[32] = {};

					NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
					V_swprintf_safe(wszLabel, L"%d (%d)", attackerInfo.dealtDmgs, attackerInfo.dealtHits);
					NeoUI::Label(wszLabel);

					NeoUI::SwapFont(NeoUI::FONT_NTHORIZSIDES);
					V_swprintf_safe(wszLabel, L"%ls (%ls)", wszDmgerName, GetNeoClassNameW(neoAttacker->GetClass()));
					NeoUI::Label(wszLabel);

					NeoUI::SwapFont(NeoUI::FONT_NTNORMAL);
					V_swprintf_safe(wszLabel, L"%d (%d)", attackerInfo.takenDmgs, attackerInfo.takenHits);
					NeoUI::Label(wszLabel);
				}
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

