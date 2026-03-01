#pragma once

#include "cbase.h"

#include "weapon_neobasecombatweapon.h"

class CNEOBaseProjectile : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CNEOBaseProjectile, CNEOBaseProjectile);
protected:
	void GetThrowPos(const Vector& throwFwd, const Vector& pos, Vector& outPos) const;
};
