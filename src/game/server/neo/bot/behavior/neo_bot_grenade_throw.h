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
	CNavArea *m_vantageArea;
	Vector m_vecTarget; // caches target to aim at during throw action in implementation classes
	Vector m_vecThreatLastKnownPos;
	Vector m_vecThrowLookAt; // far point in m_angThrowSolved's direction, for AimHeadTowards
	QAngle m_angThrowSolved; // eye angles predicted to land the grenade on m_vecTarget; x is FLT_MAX until solved
	CHandle< CNEOBaseCombatWeapon > m_hGrenadeWeapon;
	CHandle< CBaseEntity > m_hThreatGrenadeTarget;
	CountdownTimer m_giveUpTimer;
	CountdownTimer m_scanTimer;
	CountdownTimer m_repathTimer;
	PathFollower m_PathFollower;

	bool m_bFocusedOnThrow; // indicates if grenade prep behavior needs to be cleaned up by EndThrowFocus
	bool m_bPinPulled;
	bool m_bVantagePointBlocked;

	enum ThrowTargetResult
	{
		THROW_TARGET_CANCEL = -1,
		THROW_TARGET_READY = 0,
		THROW_TARGET_WAIT = 1,
	};

	// Matched push/pop pair where BeginThrowFocus pushes the required weapon,
	// which must be balanced by EndThrowFocus.
	void BeginThrowFocus( CNEOBot *me );
	void EndThrowFocus( CNEOBot *me );

	enum ThrowAimResult
	{
		THROW_AIM_TURNING,
		THROW_AIM_READY,
		THROW_AIM_UNREACHABLE,
	};

	ThrowAimResult UpdateThrowAim( CNEOBot *me );
	CNavArea *FindVantageArea( CNEOBot *me );

	virtual ThrowTargetResult UpdateGrenadeTargeting( CNEOBot *me, CNEOBaseCombatWeapon *pWeapon ) = 0;
	virtual void OnThrowReleased( CNEOBot *me ) { }

};

#endif // NEO_BOT_GRENADE_THROW_H
