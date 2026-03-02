#pragma once

#include "bot/neo_bot.h"

class CNEOIgnoredWeaponsCache
{
public:
	void Reset( void ) { m_weapons.Purge(); }
	bool Has( CBaseEntity *pWep ) const { return m_weapons.HasElement( pWep ); }
	void Add( CBaseEntity *pWep ) { m_weapons.AddToTail( pWep ); }

private:
	CUtlVector<CHandle<CBaseEntity>> m_weapons;
};

// Returns the bot's preference rank for a weapon (-1 = not preferred, -2 = empty)
int GetBotWeaponPreferenceRank( CNEOBot *me, NEO_WEP_BITS_UNDERLYING_TYPE wepBit );

bool IsWeaponPreferenceUpgrade( CNEOBot *me, CNEOBaseCombatWeapon *pTargetWep, int myPrefRank, bool bHasReserveAmmo );

bool IsUndroppablePrimary( CBaseCombatWeapon *pPrimary );

// General utility that can be used to see if it makes sense to transition into this behavior
CBaseEntity *FindNearestPrimaryWeapon( CNEOBot *me, bool bAllowDropGhost = false, CNEOIgnoredWeaponsCache *pIgnoredWeapons = nullptr );

//--------------------------------------------------------------------------------------------------------
class CNEOBotSeekWeapon : public Action< CNEOBot >
{
public:
	CNEOBotSeekWeapon( CBaseEntity *pTargetWeapon = nullptr, CNEOIgnoredWeaponsCache *pIgnoredWeapons = nullptr );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot >	OnResume( CNEOBot *me, Action< CNEOBot > *priorAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "SeekWeapon"; }

private:
	PathFollower m_path;
	CHandle<CBaseEntity> m_hTargetWeapon;
	CNEOIgnoredWeaponsCache *m_pIgnoredWeapons;
	CountdownTimer m_repathTimer;
	CountdownTimer m_giveUpTimer;

	CBaseEntity *FindAndPathToWeapon( CNEOBot *me );
};
