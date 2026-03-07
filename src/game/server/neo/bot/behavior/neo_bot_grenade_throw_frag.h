#ifndef NEO_BOT_GRENADE_THROW_FRAG_H
#define NEO_BOT_GRENADE_THROW_FRAG_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_grenade_throw.h"

class CNEOBotGrenadeThrowFrag : public CNEOBotGrenadeThrow
{
public:
	CNEOBotGrenadeThrowFrag( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat );
	virtual ~CNEOBotGrenadeThrowFrag() override { }

	virtual const char *GetName( void ) const override final { return "GrenadeThrowFrag"; }

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;

	static float GetFragSafetyRadius();
	static bool IsFragSafe( const CNEOBot *me, const Vector &vecTarget );

protected:
	virtual ThrowTargetResult UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon ) override;
};

#endif // NEO_BOT_GRENADE_THROW_FRAG_H
