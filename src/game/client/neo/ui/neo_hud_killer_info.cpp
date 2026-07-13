#include <cbase.h>
#include "neo_hud_killer_info.h"

#include "iclientmode.h"

#include "c_neo_killer_infos.h"
#include "c_neo_player.h"
#include "neo_gamerules.h"
#include "vgui/ILocalize.h"
#include "inputsystem/iinputsystem.h"
#include "IGameUIFuncs.h"
#include "igamesystem.h"
#include "cdll_client_int.h"
#include "neo_scoreboard.h"
#include <vgui_controls/ImageList.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

DECLARE_NAMED_HUDELEMENT(CNEOHud_KillerInfo, neo_killer_damage_info)

NEO_HUD_ELEMENT_DECLARE_FREQ_CVAR(KillerInfo, 0.25)

CNEOHud_KillerInfo::CNEOHud_KillerInfo(const char *pszName, vgui::Panel *parent)
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
	SetVisible(false);
	CommonSetupHUD();
}

void CNEOHud_KillerInfo::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	vgui::Panel::ApplySchemeSettings(pScheme);
	m_hFontNormal = pScheme->GetFont("HudSelectionText", true);
	m_hFontTitle = pScheme->GetFont("NeoUINormal", true);
	CommonSetupHUD();
}

void CNEOHud_KillerInfo::CommonSetupHUD()
{
	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);
	SetBounds(0, 0, wide, tall);

	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
}

void CNEOHud_KillerInfo::resetHUDState()
{
	V_memset(&g_neoKillerInfos, 0, sizeof(CNEOKillerInfos));
}

void CNEOHud_KillerInfo::UpdateStateForNeoHudElementDraw()
{
	const bool bIsPreRoundFreeze = (NEORules()->GetRoundStatus() == NeoRoundStatus::PreRoundFreeze);
	if (bIsPreRoundFreeze && false == m_preRoundFreezeCleared)
	{
		resetHUDState();
	}
	m_preRoundFreezeCleared = bIsPreRoundFreeze;

	// If to show or not
	const C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	m_bPlayerShownHud = pLocalPlayer
			&& (const_cast<C_NEO_Player *>(pLocalPlayer)->IsPlayerDead() && g_neoKillerInfos.bHasDmgInfos)
			&& (pLocalPlayer->GetTeamNumber() == TEAM_JINRAI
					|| pLocalPlayer->GetTeamNumber() == TEAM_NSF)
			&& (gpGlobals->curtime < (const_cast<C_NEO_Player *>(pLocalPlayer)->GetDeathTime() + DEATH_ANIMATION_TIME));

	const CSteamID steamID = GetSteamIDForPlayerIndex(g_neoKillerInfos.iEntIndex);
	if (steamID.IsValid())
	{
		m_avatar.SetAvatarSteamID(steamID, k_EAvatarSize184x184);
	}
	else
	{
		m_avatar.ClearAvatarSteamID();
	}
}

void CNEOHud_KillerInfo::DrawNeoHudElement()
{
	const C_NEO_Player *pLocalPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!ShouldDraw() || !m_bPlayerShownHud || !pLocalPlayer)
	{
		return;
	}

	const int iKillerEntIndex = g_neoKillerInfos.iEntIndex;
	const bool bHasKiller = iKillerEntIndex > 0 || iKillerEntIndex == NEO_ENVIRON_KILLED;
	if (!bHasKiller)
	{
		return;
	}

	int wide, tall;
	vgui::surface()->GetScreenSize(wide, tall);

	const bool bSuicideKill = (iKillerEntIndex == pLocalPlayer->entindex()) ||
			(iKillerEntIndex == NEO_ENVIRON_KILLED);

	CAvatarImage *pAvatarImg = (CNEOScoreBoard::ShowAvatars() && m_avatar.IsValid())
			? &m_avatar : nullptr;

	// NEO NOTE (nullsystem): Similar sizing to how the scoreboard popup card is
	const int iMargin = wide / 384;
	const int iAvatarWT = pAvatarImg ? (((tall / 35) * 3) - (iMargin * 2)) : 0;
	const int iTextOffsetX = pAvatarImg ? (iAvatarWT + (2 * iMargin)) : iMargin;

	wchar_t wszText[256] = {};

	// Killer title
	if (bSuicideKill)
	{
		// Show a suicide/self kill if it matches the local player's entity index
		V_wcscpy_safe(wszText, L"YOU KILLED YOURSELF");
	}
	else
	{
		V_swprintf_safe(wszText, L"Killed by %ls", g_neoKillerInfos.wszKillerName);
	}

	// Make sure box at least fits the texts
	int iTitleWide = 0;
	int iTitleTall = 0;
	[[maybe_unused]] int iNormalWide = 0;
	int iNormalTall = 0;
	vgui::surface()->GetTextSize(m_hFontTitle, wszText, iTitleWide, iTitleTall);
	vgui::surface()->GetTextSize(m_hFontNormal, L"A", iNormalWide, iNormalTall);

	const int iBoxWide = Max(wide / 4, iTextOffsetX + iTitleWide + iMargin);
	const int iBoxHeight = (pAvatarImg)
			? (2 * iMargin) + iAvatarWT
			: iMargin + iTitleTall + iNormalTall + iMargin;

	const vgui::IntRect rect = {
		.x0 = (wide / 2) - (iBoxWide / 2),
		.y0 = static_cast<int>(tall * 0.66f) - (iBoxHeight / 2),
		.x1 = (wide / 2) + (iBoxWide / 2),
		.y1 = static_cast<int>(tall * 0.66f) + (iBoxHeight / 2),
	};
	DrawNeoHudRoundedBox(rect.x0, rect.y0, rect.x1, rect.y1, COLOR_BLACK_TRANSPARENT);

	if (pAvatarImg)
	{
		pAvatarImg->SetPos(rect.x0 + iMargin, rect.y0 + iMargin);
		pAvatarImg->SetSize(iAvatarWT, iAvatarWT);
		pAvatarImg->Paint();
	}

	vgui::surface()->DrawSetTextFont(m_hFontTitle);
	vgui::surface()->DrawSetTextColor(COLOR_NEO_WHITE);
	vgui::surface()->DrawSetTextPos(
			rect.x0 + iTextOffsetX,
			rect.y0 + iMargin);
	vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));

	// Killer info
	if (bSuicideKill)
	{
		// Just state the weapon, the rest of the infos aren't useful
		// If it's an empty weapon, it's generally a suicide command kill
		if (g_neoKillerInfos.wszKilledWith[0] == L'\0')
		{
			V_wcscpy_safe(wszText, (iKillerEntIndex == NEO_ENVIRON_KILLED)
					? L"Killed by map"
					: L"Killed by map or used suicide command");
		}
		else
		{
			V_swprintf_safe(wszText, L"Killed by %ls", g_neoKillerInfos.wszKilledWith);
		}
	}
	else
	{
		V_swprintf_safe(wszText, L"%dhp %ls at %.2fm with %ls"
				, g_neoKillerInfos.iHP
				, GetNeoClassNameW(g_neoKillerInfos.iClass)
				, g_neoKillerInfos.flDistance
				, g_neoKillerInfos.wszKilledWith);
	}

	vgui::surface()->DrawSetTextFont(m_hFontNormal);
	vgui::surface()->DrawSetTextPos(
			rect.x0 + iTextOffsetX,
			rect.y0 + iMargin + iTitleTall);
	vgui::surface()->DrawPrintText(wszText, V_wcslen(wszText));
}

void CNEOHud_KillerInfo::Paint()
{
	SetFgColor(COLOR_TRANSPARENT);
	SetBgColor(COLOR_TRANSPARENT);
	vgui::Panel::Paint();
	PaintNeoElement();
}

