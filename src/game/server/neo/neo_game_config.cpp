#include "neo_game_config.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_game_config, CNEOGameConfig);

BEGIN_DATADESC(CNEOGameConfig)
	DEFINE_KEYFIELD(m_GameType, FIELD_INTEGER, "GameType"),

	DEFINE_INPUTFUNC(FIELD_INTEGER, "FireTeamWin", InputFireTeamWin),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireDMPlayerWin", InputFireDMPlayerWin),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireRoundTie", InputFireRoundTie),

	DEFINE_OUTPUT(m_OnRoundEnd, "OnRoundEnd"),
	DEFINE_OUTPUT(m_OnRoundStart, "OnRoundStart")
END_DATADESC()

CNEOGameConfig* CNEOGameConfig::s_pGameRulesToConfig = nullptr;

void CNEOGameConfig::Spawn()
{
	BaseClass::Spawn();

	s_pGameRulesToConfig = this;
}

// Inputs

void CNEOGameConfig::InputFireTeamWin(inputdata_t& inputData)
{
	int team = inputData.value.Int();
	static_cast<CNEORules*>(g_pGameRules)->SetWinningTeam(team, NEO_VICTORY_MAPIO, false, true, false, false);
}

void CNEOGameConfig::InputFireDMPlayerWin(inputdata_t& inputData)
{
	CBasePlayer* pPlayer = NULL;

	if (inputData.pActivator && inputData.pActivator->IsPlayer())
	{
		pPlayer = (CBasePlayer*)inputData.pActivator;
	}

	if (pPlayer)
    {
        static_cast<CNEORules*>(g_pGameRules)->SetWinningDMPlayer(static_cast<CNEO_Player*>(pPlayer));
    }
}

void CNEOGameConfig::InputFireRoundTie(inputdata_t& inputData)
{
	static_cast<CNEORules*>(g_pGameRules)->SetWinningTeam(1, NEO_VICTORY_STALEMATE, false, true, true, false);
}

// Outputs

void CNEOGameConfig::OutputRoundEnd()
{
	m_OnRoundEnd.FireOutput(NULL, this);
}

void CNEOGameConfig::OutputRoundStart()
{
	m_OnRoundStart.FireOutput(NULL, this);
}
