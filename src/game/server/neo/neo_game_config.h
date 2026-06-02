#pragma once

#include "cbase.h"
#include "neo_gamerules.h"

class CNEOGameConfig : public CLogicalEntity
{
	DECLARE_CLASS(CNEOGameConfig, CLogicalEntity);
	DECLARE_DATADESC();

public:
	CNEOGameConfig();
	~CNEOGameConfig();
	virtual void Spawn() override;

	int m_GameType = NEO_GAME_TYPE_TDM;
	int m_HiddenHudElements = 0;
	int m_ForcedTeam = -1;
	int m_ForcedClass = -1;
	int m_ForcedSkin = -1;
	int m_ForcedWeapon = -1;
	bool m_Cyberspace = false;

	// Inputs
	void InputFireTeamWin(inputdata_t& inputData);
	void InputFireDMPlayerWin(inputdata_t& inputData);
	void InputFireRoundTie(inputdata_t& inputData);

	COutputInt m_OnRoundEnd;
	COutputEvent m_OnDMRoundEnd;
	COutputEvent m_OnRoundStart;
	COutputEvent m_OnCompetitive;
};

extern CNEOGameConfig *g_pNEOGameConfig;
inline CNEOGameConfig *NEOGameConfig()
{
	return g_pNEOGameConfig;
}