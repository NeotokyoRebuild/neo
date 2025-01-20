#include "neo_game_config.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_game_config, CNEOGameConfig);

BEGIN_DATADESC(CNEOGameConfig)
	DEFINE_KEYFIELD(m_GameType, FIELD_INTEGER, "GameType"),

	DEFINE_INPUTFUNC(FIELD_INTEGER, "FireTeamWin", InputFireTeamWin),
	DEFINE_INPUTFUNC(FIELD_VOID, "FireDMPlayerWin", InputFireDMPlayerWin),

	DEFINE_OUTPUT(m_OnRoundEnd, "OnRoundEnd"),
	DEFINE_OUTPUT(m_OnRoundStart, "OnRoundStart")
END_DATADESC()

void CNEOGameConfig::Spawn()
{
	BaseClass::Spawn();

	s_pGameRulesToConfig = this;
}

// Inputs

void CNEOGameConfig::InputFireTeamWin(inputdata_t& inputData)
{
	int team = inputData.value.Int();
	static_cast<CNEORules*>(g_pGameRules)->SetWinningTeam(team, 8, false, true, false, false);
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

// Outputs

void CNEOGameConfig::OutputRoundEnd()
{
	m_OnRoundEnd.FireOutput(NULL, this);
}

void CNEOGameConfig::OutputRoundStart()
{
	m_OnRoundStart.FireOutput(NULL, this);
}
