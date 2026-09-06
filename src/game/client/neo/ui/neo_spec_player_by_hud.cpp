#include "cbase.h"

#include "neo_hud_spectator_overlay.h"
#include "neo_hud_round_state.h"
#include "hud_element_helper.h"
#include "hltvcamera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static void SetPlayerEntityTarget(const int iEntIndex)
{
	if (iEntIndex > 0)
	{
		if (engine->IsHLTV())
		{
			HLTVCamera()->SetPrimaryTarget(iEntIndex);
		}
		else
		{
			engine->ClientCmd(VarArgs("spec_player_entity_number %d", iEntIndex));
		}
	}
}

struct HUDCommonRet
{
	CNEOHud_SpectatorOverlay *pOverlay = nullptr;
	CNEOHud_RoundState *pRoundState = nullptr;
	C_NEO_Player *pNeoPlayer = nullptr;
};

static HUDCommonRet CheckCommonSpecOverlayErrors(const char *pszFuncName)
{
	HUDCommonRet ret = {};

	const bool bUseOverlay = cl_neo_hud_spectator_overlay_enabled.GetBool();

	auto *pOverlay = g_pNeoHudSpecOverlay;
	auto *pRoundState = g_pNeoHudRoundState;
	if ((bUseOverlay && !pOverlay) || (!bUseOverlay && !pRoundState))
	{
		return ret;
	}

	if (engine->IsHLTV() && HLTVCamera()->IsPVSLocked())
	{
		ConMsg("%s: HLTV Camera is PVS locked\n", pszFuncName);
		return ret;
	}

	C_NEO_Player *pNeoPlayer = C_NEO_Player::GetLocalNEOPlayer();
	if (!pNeoPlayer || !pNeoPlayer->IsObserver())
	{
		return ret;
	}

	if (bUseOverlay)
	{
		ret.pOverlay = pOverlay;
	}
	else
	{
		ret.pRoundState = pRoundState;
	}
	ret.pNeoPlayer = pNeoPlayer;
	return ret;
}

CON_COMMAND_F(spec_player_by_hud_position, "Spectate player by position in the hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	static constexpr const char HELP_MSG_OL[] =
			"Usage: spec_player_by_hud_position { player position in overlay hud, from first of Jinrai to last of NSF, 0 indexed }\n";
	static constexpr const char HELP_MSG_RS[] =
			"Usage: spec_player_by_hud_position { player position in top hud, 0 indexed }\n";

	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (args.ArgC() != 2)
	{
		ConMsg(pOverlay ? HELP_MSG_OL : HELP_MSG_RS);
		return;
	}

	if (pOverlay)
	{
		const int iSpecTargetIdx = atoi(args[1]);
		if (iSpecTargetIdx < 0 || iSpecTargetIdx >= pOverlay->m_iCardsSize)
		{
			ConMsg(HELP_MSG_OL);
			return;
		}
		SetPlayerEntityTarget(pOverlay->m_cards[iSpecTargetIdx].iEntIndex);
	}
	else if (pRoundState)
	{
		const int positionInHud = atoi(args[1]);
		if (positionInHud < 0 || positionInHud > MAX_PLAYERS - 1)
		{
			ConMsg(HELP_MSG_RS);
			return;
		}
		SetPlayerEntityTarget(g_pNeoHudRoundState->GetEntityIndexAtPositionInHud(positionInHud, true));
	}
}

CON_COMMAND_F(spec_next_entity_in_hud, "Spectate the next valid player in the hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (pOverlay)
	{
		pOverlay->m_iEntIndexSelect = 0;
		SetPlayerEntityTarget(pOverlay->SpecTargetNextEntIdx(
					CNEOHud_SpectatorOverlay::SPECTYPE_DIRECT,
					CNEOHud_SpectatorOverlay::DIRECTION_NEXT));
	}
	else if (pRoundState)
	{
		C_BaseEntity *pSpectateTarget = pNeoPlayer->GetObserverTarget();
		const int iSpecTargetMinusIdxPos = (pSpectateTarget)
				? g_pNeoHudRoundState->GetMinusIndexedPositionOfPlayerInHud(pSpectateTarget->entindex())
				: 0;
		SetPlayerEntityTarget(
				g_pNeoHudRoundState->GetEntityIndexAtPositionInHud(
					g_pNeoHudRoundState->GetNextAlivePlayerInHud(
						iSpecTargetMinusIdxPos, false)));
	}
}

CON_COMMAND_F(spec_previous_entity_in_hud, "Spectate the previous valid player in the hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (pOverlay)
	{
		pOverlay->m_iEntIndexSelect = 0;
		SetPlayerEntityTarget(pOverlay->SpecTargetNextEntIdx(
					CNEOHud_SpectatorOverlay::SPECTYPE_DIRECT,
					CNEOHud_SpectatorOverlay::DIRECTION_PREV));
	}
	else if (pRoundState)
	{
		C_BaseEntity *pSpectateTarget = pNeoPlayer->GetObserverTarget();
		const int iSpecTargetMinusIdxPos = (pSpectateTarget)
				? pRoundState->GetMinusIndexedPositionOfPlayerInHud(pSpectateTarget->entindex())
				: 0;
		SetPlayerEntityTarget(
				pRoundState->GetEntityIndexAtPositionInHud(
					pRoundState->GetNextAlivePlayerInHud(
						iSpecTargetMinusIdxPos, true)));
	}
}

CON_COMMAND_F(select_next_alive_player_in_hud, "Select the next alive player in the hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (pOverlay)
	{
		pOverlay->m_iEntIndexSelect = pOverlay->SpecTargetNextEntIdx(
				CNEOHud_SpectatorOverlay::SPECTYPE_SELECT,
				CNEOHud_SpectatorOverlay::DIRECTION_NEXT);
	}
	else if (pRoundState)
	{
		pRoundState->SelectNextAlivePlayerInHud();
	}
}

CON_COMMAND_F(select_previous_alive_player_in_hud, "Select the previous alive player in the hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (pOverlay)
	{
		pOverlay->m_iEntIndexSelect = pOverlay->SpecTargetNextEntIdx(
				CNEOHud_SpectatorOverlay::SPECTYPE_SELECT,
				CNEOHud_SpectatorOverlay::DIRECTION_PREV);
	}
	else if (pRoundState)
	{
		pRoundState->SelectPreviousAlivePlayerInHud();
	}
}

CON_COMMAND_F(spectate_player_selected_in_hud, "Spectate entity selected in the top hud", FCVAR_CLIENTCMD_CAN_EXECUTE)
{
	auto [pOverlay, pRoundState, pNeoPlayer] = CheckCommonSpecOverlayErrors(__FUNCTION__);

	if (pOverlay)
	{
		SetPlayerEntityTarget(pOverlay->m_iEntIndexSelect);
		pOverlay->m_iEntIndexSelect = 0;
	}
	else if (pRoundState)
	{
		SetPlayerEntityTarget(g_pNeoHudRoundState->GetSelectedPlayerInHud());
	}
}

