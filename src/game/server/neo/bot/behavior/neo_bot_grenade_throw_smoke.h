#ifndef NEO_BOT_GRENADE_THROW_SMOKE_H
#define NEO_BOT_GRENADE_THROW_SMOKE_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_grenade_throw.h"

class CNEOBotGrenadeThrowSmoke : public CNEOBotGrenadeThrow
{
public:
	CNEOBotGrenadeThrowSmoke( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat );
	virtual ~CNEOBotGrenadeThrowSmoke() override { }

	virtual const char *GetName( void ) const override final { return "GrenadeThrowSmoke"; }

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;

protected:
	virtual ThrowTargetResult UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon ) override;
};

#endif // NEO_BOT_GRENADE_THROW_SMOKE_H
