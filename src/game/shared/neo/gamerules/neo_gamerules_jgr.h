#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesJGR C_NEORulesJGR
	#define CNEOGameRulesJGRProxy C_NEOGameRulesJGRProxy
#endif

class CNEOGameRulesJGRProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesJGRProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesJGR : public CNEORules
{
public:
	DECLARE_CLASS(CNEORulesJGR, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_JGR; }
	virtual const char* GetGameTypeName() override final { return "JGR"; }
	const char* GetGameDescription() override final { return "Juggernaut"; }
	virtual bool GetTeamPlayEnabled() const override final { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool RespawnsEnabled() const override final { return true; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;
	virtual bool PlayerCanChangeLoadout(CNEO_Player* pPlayer) override final;
	
	virtual void EnemyPlayerKilled(CNEO_Player* pVictim, CNEO_Player* pAttacker, const CTakeDamageInfo& info) override final;

	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
	bool JuggernautUnlockCheckWinCondition();
#endif // GAME_DLL
};

inline CNEORulesJGR *NEORulesJGR()
{
	return static_cast<CNEORulesJGR*>(g_pGameRules);
}
