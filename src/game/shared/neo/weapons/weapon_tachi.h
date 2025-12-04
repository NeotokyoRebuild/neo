#ifndef NEO_WEAPON_TACHI_H
#define NEO_WEAPON_TACHI_H
#ifdef _WIN32
#pragma once
#endif

#include "cbase.h"
#include "npcevent.h"
#include "in_buttons.h"

#ifdef CLIENT_DLL
	#include "c_neo_player.h"
#else
	#include "neo_player.h"
#endif

#include "weapon_neobasecombatweapon.h"

#ifdef CLIENT_DLL
#define CWeaponTachi C_WeaponTachi
#endif

namespace Tachi {
	enum Firemode {
		Auto = false,
		Single = true
	};
};

class CWeaponTachi : public CNEOBaseCombatWeapon
{
	DECLARE_CLASS(CWeaponTachi, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponTachi();

	void	ItemPostFrame( void ) override;

    virtual void SwitchFireMode( void );
    virtual void ForceSetFireMode( Tachi::Firemode primaryMode,
        bool bPlaySound = false, float flSoonestSwitch = 0.0f );

	virtual NEO_WEP_BITS_UNDERLYING_TYPE GetNeoWepBits(void) const override { return NEO_WEP_TACHI | NEO_WEP_FIREARM; }
	virtual int GetNeoWepXPCost(const int neoClass) const override { return 0; }

	virtual float GetSpeedScale(void) const OVERRIDE { return 0.85f; }

	virtual int	GetMinBurst() OVERRIDE { return 1; }
	virtual int	GetMaxBurst() OVERRIDE { return 3; }

	inline virtual bool IsAutomatic(void) const override final
	{
		return (m_bIsPrimaryFireMode == Tachi::Firemode::Auto);
	}

	bool CanBePickedUpByClass(int classId) OVERRIDE;

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }

private:
	CNetworkVar(float, m_flSoonestFiremodeSwitch);
    CNetworkVar(bool, m_bIsPrimaryFireMode);

private:
	CWeaponTachi( const CWeaponTachi &other );
};

#endif // NEO_WEAPON_TACHI_H
