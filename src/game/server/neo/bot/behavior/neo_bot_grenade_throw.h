#ifndef NEO_BOT_GRENADE_THROW_H
#define NEO_BOT_GRENADE_THROW_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_behavior.h"
#include "NextBot/Path/NextBotPathFollow.h"

class CNEOBaseCombatWeapon;
class CKnownEntity;

class CNEOBotGrenadeThrow : public Action< CNEOBot >
{
public:
	CNEOBotGrenadeThrow( CNEOBaseCombatWeapon *pWeapon, const CKnownEntity *threat );
	virtual ~CNEOBotGrenadeThrow() override { }

	virtual ActionResult< CNEOBot >	OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual void					OnEnd( CNEOBot *me, Action< CNEOBot > *nextAction ) override;
	virtual ActionResult< CNEOBot >	OnSuspend( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;
	virtual QueryResultType			ShouldRetreat( const INextBot *me ) const override;

protected:
	Vector m_vecTarget; // caches target to aim at during throw action in implementation classes
	Vector m_vecThreatLastKnownPos;
	CHandle< CNEOBaseCombatWeapon > m_hGrenadeWeapon;
	CHandle< CBaseEntity > m_hThreatGrenadeTarget;
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_scanTimer;
	CountdownTimer m_repathTimer;
	PathFollower m_PathFollower;

	bool m_bPinPulled;

	enum ThrowTargetResult
	{
		THROW_TARGET_CANCEL = -1,
		THROW_TARGET_READY = 0,
		THROW_TARGET_WAIT = 1,
	};

	static const Vector& FindEmergencePointAlongPath( const CNEOBot *me, const Vector &familiarPos, const Vector &obscuredPos );
	
	virtual ThrowTargetResult UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon ) = 0;

};

#endif // NEO_BOT_GRENADE_THROW_H
