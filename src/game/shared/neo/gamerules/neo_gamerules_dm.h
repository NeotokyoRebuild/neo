#pragma once

#include "neo_gamerules.h"

#ifdef CLIENT_DLL
	#define CNEORulesDM C_NEORulesDM
	#define CNEOGameRulesDMProxy C_NEOGameRulesDMProxy
#endif

class CNEOGameRulesDMProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesDMProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesDM : public CNEORules
{
public:
	DECLARE_CLASS(CNEORulesDM, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_DM; }
	virtual const char* GetGameTypeName() override final { return "DM"; }
	const char* GetGameDescription() override final { return "Deathmatch"; }
	virtual bool GetTeamPlayEnabled() const override final { return false; }
	virtual bool GetCompEnabled() const override final { return false; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return true; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;
	virtual bool PlayerCanChangeLoadout(CNEO_Player* pPlayer) override final;

	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
	
	void GetDMHighestScorers(
#ifdef GAME_DLL
			CNEO_Player *(*pHighestPlayers)[MAX_PLAYERS + 1],
#endif
			int *iHighestPlayersTotal, int *iHighestXP) const;
#ifdef GAME_DLL
	void SetWinningDMPlayer(CNEO_Player *pWinner);
#endif // GAME_DLL
};

inline CNEORulesDM *NEORulesDM()
{
	return static_cast<CNEORulesDM*>(g_pGameRules);
}
