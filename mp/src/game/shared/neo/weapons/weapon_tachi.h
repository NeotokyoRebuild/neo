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

class CWeaponTachi : public CNEOBaseCombatWeapon
{
#define TACHI_SEMIAUTO_FIRERATE 0.2f
#define TACHI_MAGDUMP_FIRERATE 0.1f

	DECLARE_CLASS(CWeaponTachi, CNEOBaseCombatWeapon);
public:
	DECLARE_NETWORKCLASS(); 
	DECLARE_PREDICTABLE();

#ifdef GAME_DLL
	DECLARE_ACTTABLE();
	DECLARE_DATADESC();
#endif

	CWeaponTachi();

	void	Precache( void );
	void	ItemPostFrame( void );
	void	ItemPreFrame( void );
	void	ItemBusyFrame( void );
	void	PrimaryAttack( void );
	void	AddViewKick( void );
	void	DryFire( void );

	void	UpdatePenaltyTime( void );

	Activity	GetPrimaryAttackActivity( void );

    virtual void SwitchFireMode( void );
    virtual void ForceSetFireMode( bool bPrimaryMode,
        bool bPlaySound = false, float flSoonestSwitch = 0.0f );

	virtual int GetNeoWepBits(void) const { return NEO_WEP_TACHI; }
	virtual int GetNeoWepXPCost(const int neoClass) const { return 0; }

	virtual float GetSpeedScale(void) const { return 1.0; }

	virtual const Vector& GetBulletSpread( void ) OVERRIDE
	{		
		static Vector cone;

		const float ramp = RemapValClamped(m_flAccuracyPenalty, 0.0f,
			GetMaxAccuracyPenalty(), 0.0f, 1.0f); 

		// We lerp from very accurate to inaccurate over time
		VectorLerp( VECTOR_CONE_1DEGREES, VECTOR_CONE_6DEGREES, ramp, cone );

		return cone;
	}
	
	virtual int	GetMinBurst() OVERRIDE { return 1; }
	virtual int	GetMaxBurst() OVERRIDE { return 3; }

	virtual bool IsAutomatic(void) const OVERRIDE
	{
		return (m_bIsPrimaryFireMode == false);
	}

	virtual float GetFireRate(void) OVERRIDE { return (m_bIsPrimaryFireMode ? TACHI_SEMIAUTO_FIRERATE : TACHI_MAGDUMP_FIRERATE); }

protected:
	virtual float GetFastestDryRefireTime() const OVERRIDE { return 0.2f; }
	virtual float GetAccuracyPenalty() const OVERRIDE { return 0.2f; }
	virtual float GetMaxAccuracyPenalty() const OVERRIDE { return 1.5f; }

private:
	CNetworkVar(float, m_flSoonestFiremodeSwitch);
    CNetworkVar(bool, m_bIsPrimaryFireMode);

private:
	CWeaponTachi( const CWeaponTachi &other );
};

#endif // NEO_WEAPON_TACHI_H