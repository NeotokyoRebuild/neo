#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesJGR C_NEORulesJGR
	#define CNEOGameRulesJGRProxy C_NEOGameRulesJGRProxy
#endif
//
//ConVar sv_neo_jgr_score_limit("sv_neo_jgr_score_limit", "1", FCVAR_REPLICATED, "JGR score limit", true, 0.0f, true, 99.0f);
//ConVar sv_neo_jgr_round_limit("sv_neo_jgr_round_limit", "0", FCVAR_REPLICATED, "JGR max amount of rounds, 0 for no limit.", true, 0.0f, false, 0.0f);
//ConVar sv_neo_jgr_round_timelimit("neo_jgr_round_timelimit", "10.25", FCVAR_REPLICATED, "JGR round timelimit, in minutes.",	true, 0.0f, false, 600.0f);

class CNEOGameRulesJGRProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesJGRProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesJGR : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesJGR, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesJGR();
	//virtual ~CNEORulesJGR();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	virtual int GetGameType() override final { return NEO_GAME_TYPE_JGR; }
	virtual const char* GetGameTypeName() override { return "JGR"; }
	const char* GetGameDescription() override { return "Juggernaut"; }
	virtual bool GetTeamPlayEnabled() const override { return true; }
	virtual bool GetCompEnabled() const override final { return true; }
	virtual bool GetCapPreventEnabled() const override final { return false; }
	virtual bool CanChangeTeamClassLoadoutWhenAlive() const override final { return false; }
	virtual bool CanRespawnAnyTime() const override final { return true; }

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;
	
	virtual void EnemyPlayerKilled(CNEO_Player* pVictim, CNEO_Player* pAttacker, const CTakeDamageInfo& info) override final;

	virtual void SetGameRelatedVars() override final;
	virtual const int GetScoreLimit() const override final;
	virtual const int GetRoundLimit() const override final;
	virtual void RoundTimeout() override final;
#endif // GAME_DLL
	virtual void Think() override final;
#ifdef GAME_DLL
	virtual void PlayerRespawnThink() override final;
#endif // GAME_DLL
};

inline CNEORulesJGR *NEORulesJGR()
{
	return static_cast<CNEORulesJGR*>(g_pGameRules);
}