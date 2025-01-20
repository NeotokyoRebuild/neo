#pragma once

#include "cbase.h"
#include "baseentity.h"
#include "neo_gamerules.h"

class CNEOGameConfig : public CLogicalEntity
{
	DECLARE_CLASS(CNEOGameConfig, CBaseEntity);
	DECLARE_DATADESC();

public:
	void Spawn() override;
	static CNEOGameConfig* s_pGameRulesToConfig;

	int m_GameType = NEO_GAME_TYPE_TDM;

	// Inputs
	void InputFireTeamWin(inputdata_t& inputData);
	void InputFireDMPlayerWin(inputdata_t& inputData);
	void InputFireRoundTie(inputdata_t& inputData);

	// Outputs
	void OutputRoundEnd();
	void OutputRoundStart();

	COutputEvent m_OnRoundEnd;
	COutputEvent m_OnRoundStart;
};
