#pragma once

#include "neo_gamerules.h"
#include "GameEventListener.h"

#ifdef CLIENT_DLL
	#define CNEORulesTDM C_NEORulesTDM
	#define CNEOGameRulesTDMProxy C_NEOGameRulesTDMProxy
#endif

class CNEOGameRulesTDMProxy : public CNEOGameRulesProxy
{
public:
	DECLARE_CLASS( CNEOGameRulesTDMProxy, CNEOGameRulesProxy );
	DECLARE_NETWORKCLASS();
};

class CNEORulesTDM : public CNEORules, public CGameEventListener
{
public:
	DECLARE_CLASS(CNEORulesTDM, CNEORules);
	DECLARE_NETWORKCLASS_NOBASE();
	
	//CNEORulesTDM();
	//virtual ~CNEORulesTDM();
	
	// IGameEventListener interface:
	virtual void FireGameEvent(IGameEvent *event) override;
	
	const char* GetGameDescription() override { return "Team Deathmatch"; }
	virtual bool GetTeamPlayEnabled() const override { return true; };

	virtual float GetRoundRemainingTime() const override final;
#ifdef GAME_DLL
	virtual bool FPlayerCanRespawn(CBasePlayer* pPlayer) override final;

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

inline CNEORulesTDM *NEORulesTDM()
{
	return static_cast<CNEORulesTDM*>(g_pGameRules);
}