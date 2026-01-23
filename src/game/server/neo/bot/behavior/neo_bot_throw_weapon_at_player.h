#ifndef NEO_BOT_THROW_WEAPON_AT_PLAYER_H
#define NEO_BOT_THROW_WEAPON_AT_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

#include "NextBotBehavior.h"

class CNEO_Player;

class CNEOBotThrowWeaponAtPlayer : public Action< CNEOBot >
{
public:
	CNEOBotThrowWeaponAtPlayer( CNEO_Player *pTargetPlayer );

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;

	virtual const char *GetName( void ) const override { return "ThrowWeaponAtPlayer"; };

private:
	CHandle<CNEO_Player> m_hTargetPlayer;
};

#endif // NEO_BOT_THROW_WEAPON_AT_PLAYER_H
