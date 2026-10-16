#pragma once

#include "util_shared.h"   // CountdownTimer

class CNEOBot;
class CNEOBaseCombatWeapon;
class CKnownEntity;
class CTakeDamageInfo;

// Fires at where an obscured threat is believed to be
// Driven from CNEOBotMainAction::FireWeaponAtEnemy, which owns the threats the bot can see.
class CNEOBotSuppressiveFire
{
public:
	void Reset();
	void OnInjured( CNEOBot *me, const CTakeDamageInfo &info );
	void OnHeardGunfire( CNEOBot *me, CBaseEntity *shooter );
	bool Update( CNEOBot *me, const CKnownEntity *threat );

private:
	static Vector RotateBearing( const Vector &from, const Vector &to, float degrees, float rangeScale = 1.0f );
	static bool IsSmokeOrEnemyOnLine( CNEOBot *me, const Vector &from, const Vector &to );
	static bool IsLineWorthFiring( CNEOBot *me, const Vector &from, const Vector &to );
	static bool IsBarrelOnWorthwhileLine( CNEOBot *me, const Vector &aimSpot, const Vector &believedSpot, float sweepHalfAngle );
	static CNEOBaseCombatWeapon *GetLoadedFirearm( CNEOBot *me );

	void OpenFireWindow( CNEOBot *me, CBaseEntity *target, const Vector &pos, bool bSeen );
	Vector GetSweptAimSpot( const Vector &from, const Vector &center, float *halfAngle ) const;

	EHANDLE m_hTarget;
	Vector m_vecBelievedPos;
	CountdownTimer m_fireWindowTimer;
	CountdownTimer m_holdAimTimer;
	float m_sweepPhase;
	float m_bearingErrorDeg;
	float m_rangeScale;
};
