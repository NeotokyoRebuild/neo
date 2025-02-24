#ifndef NEO_TRACEFILTER_COLLISIONGROUPDELTA_H
#define NEO_TRACEFILTER_COLLISIONGROUPDELTA_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"

// This will hit only things that are in newCollisionGroup, but NOT in collisionGroupAlreadyChecked
class CNEOTraceFilterCollisionGroupDelta : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE(CNEOTraceFilterCollisionGroupDelta);

	CNEOTraceFilterCollisionGroupDelta(const IHandleEntity* passentity, int collisionGroupAlreadyChecked, int newCollisionGroup)
		: m_pPassEnt(passentity), m_collisionGroupAlreadyChecked(collisionGroupAlreadyChecked), m_newCollisionGroup(newCollisionGroup)
	{
	}

	virtual bool ShouldHitEntity(IHandleEntity* pHandleEntity, int contentsMask)
	{
		if (!PassServerEntityFilter(pHandleEntity, m_pPassEnt))
		{
			return false;
		}

		CBaseEntity* pEntity = EntityFromEntityHandle(pHandleEntity);

		if (pEntity)
		{
			if (g_pGameRules->ShouldCollide(m_collisionGroupAlreadyChecked, pEntity->GetCollisionGroup()))
			{
				return false;
			}
			if (g_pGameRules->ShouldCollide(m_newCollisionGroup, pEntity->GetCollisionGroup()))
			{
				return true;
			}
		}

		return false;
	}

protected:
	const IHandleEntity* m_pPassEnt;
	int		m_collisionGroupAlreadyChecked;
	int		m_newCollisionGroup;
};

#endif // NEO_TRACEFILTER_COLLISIONGROUPDELTA_H