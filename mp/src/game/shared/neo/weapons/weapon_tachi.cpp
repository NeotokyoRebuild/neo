#include "cbase.h"
#include "weapon_tachi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTachi, DT_WeaponTachi )

BEGIN_NETWORK_TABLE( CWeaponTachi, DT_WeaponTachi )
#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSoonestPrimaryAttack ) ),
	RecvPropTime( RECVINFO( m_flSoonestFiremodeSwitch ) ),
	RecvPropTime( RECVINFO( m_flLastAttackTime ) ),
	RecvPropFloat( RECVINFO( m_flAccuracyPenalty ) ),
	RecvPropInt( RECVINFO( m_nNumShotsFired ) ),
	RecvPropBool( RECVINFO( m_bIsPrimaryFireMode ) ),
#else
	SendPropTime( SENDINFO( m_flSoonestPrimaryAttack ) ),
	SendPropTime( SENDINFO( m_flSoonestFiremodeSwitch ) ),
	SendPropTime( SENDINFO( m_flLastAttackTime ) ),
	SendPropFloat( SENDINFO( m_flAccuracyPenalty ) ),
	SendPropInt( SENDINFO( m_nNumShotsFired ) ),
	SendPropBool( SENDINFO( m_bIsPrimaryFireMode ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponTachi )
	DEFINE_PRED_FIELD( m_flSoonestPrimaryAttack, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flSoonestFiremodeSwitch, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flLastAttackTime, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_flAccuracyPenalty, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nNumShotsFired, FIELD_INTEGER, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsPrimaryFireMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS( weapon_tachi, CWeaponTachi );
PRECACHE_WEAPON_REGISTER( weapon_tachi );

#ifndef CLIENT_DLL
acttable_t CWeaponTachi::m_acttable[] = 
{
	{ ACT_MP_STAND_IDLE, ACT_IDLE, true },
	{ ACT_MP_RUN, ACT_RUN, true },
	{ ACT_MP_WALK, ACT_WALK, true },
	{ ACT_MP_JUMP, ACT_HOP, true },

	{ ACT_MP_AIRWALK, ACT_HOVER, true },
	{ ACT_MP_SWIM, ACT_SWIM, true },

	{ ACT_LEAP, ACT_LEAP, true },
	{ ACT_DIESIMPLE, ACT_DIESIMPLE, true },

	{ ACT_MP_CROUCH_IDLE, ACT_CROUCHIDLE, true },
	{ ACT_MP_CROUCHWALK, ACT_WALK_CROUCH, true },

	{ ACT_MP_RELOAD_STAND, ACT_RELOAD, true },

	{ ACT_CROUCHIDLE, ACT_HL2MP_IDLE_CROUCH_PISTOL, true },
	{ ACT_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_PISTOL, true },
	{ ACT_RANGE_ATTACK1, ACT_RANGE_ATTACK_PISTOL, true }
};
IMPLEMENT_ACTTABLE( CWeaponTachi );
#endif

CWeaponTachi::CWeaponTachi( void )
{
	m_flSoonestPrimaryAttack = gpGlobals->curtime;
	m_flSoonestFiremodeSwitch = gpGlobals->curtime;
	m_flAccuracyPenalty = 0.0f;

	m_fMinRange1		= 24;
	m_fMaxRange1		= 1500;
	m_fMinRange2		= 24;
	m_fMaxRange2		= 200;

	m_bFiresUnderwater	= true;
	m_bIsPrimaryFireMode = true;
}

void CWeaponTachi::Precache( void )
{
	BaseClass::Precache();
}

void CWeaponTachi::DryFire( void )
{
	WeaponSound( EMPTY );
	SendWeaponAnim( ACT_VM_DRYFIRE );

	m_flNextPrimaryAttack		= gpGlobals->curtime + SequenceDuration();
}

void CWeaponTachi::PrimaryAttack( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (pOwner)
	{
		if (!m_iClip1 && !ClientWantsAutoReload(GetOwner()))
		{
			return;
		}

		if (m_bIsPrimaryFireMode)
		{
			// Do nothing if we hold fire whilst semi auto
			if ((pOwner->m_nButtons & IN_ATTACK) &&
			(!(pOwner->m_afButtonPressed & IN_ATTACK)))
			{
				return;
			}
		}
	}

	if ( ( gpGlobals->curtime - m_flLastAttackTime ) > 0.5f )
	{
		m_nNumShotsFired = 0;
	}
	else
	{
		m_nNumShotsFired++;
	}

	m_flLastAttackTime = gpGlobals->curtime;

	if( pOwner )
	{
		// Each time the player fires the pistol, reset the view punch. This prevents
		// the aim from 'drifting off' when the player fires very quickly. This may
		// not be the ideal way to achieve this, but it's cheap and it works, which is
		// great for a feature we're evaluating. (sjb)
		pOwner->ViewPunchReset();
	}

	BaseClass::PrimaryAttack();

	// Add an accuracy penalty which can move past our maximum penalty time if we're really spastic
	m_flAccuracyPenalty += TACHI_ACCURACY_SHOT_PENALTY_TIME;
}

void CWeaponTachi::SwitchFireMode( void )
{
	if (m_flSoonestFiremodeSwitch > gpGlobals->curtime)
	{
		return;
	}

	m_bIsPrimaryFireMode = !m_bIsPrimaryFireMode;

#ifdef CLIENT_DLL
	// NEO TODO (Rain): fire mode indicator
	Msg("Fire mode: %s\n", m_bIsPrimaryFireMode ? "primary" : "alt");
#endif

	WeaponSound( SPECIAL1 );
	SendWeaponAnim( ACT_VM_DRAW_SPECIAL );
}

void CWeaponTachi::ForceSetFireMode( bool bPrimaryMode, bool bPlaySound,
	float flSoonestSwitch )
{
	m_bIsPrimaryFireMode = bPrimaryMode;
	m_flSoonestFiremodeSwitch = flSoonestSwitch;

	if (bPlaySound)
	{
		WeaponSound( SPECIAL1 );
	}
}

void CWeaponTachi::UpdatePenaltyTime( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Check our penalty time decay
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestPrimaryAttack < gpGlobals->curtime ) )
	{
		m_flAccuracyPenalty -= gpGlobals->frametime;
		m_flAccuracyPenalty = clamp( m_flAccuracyPenalty, 0.0f, TACHI_ACCURACY_MAXIMUM_PENALTY_TIME );
	}
}

void CWeaponTachi::ItemPreFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemPreFrame();
}

void CWeaponTachi::ItemBusyFrame( void )
{
	UpdatePenaltyTime();

	BaseClass::ItemBusyFrame();
}

float CWeaponTachi::GetFireRate( void )
{
	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if (!pOwner)
	{
		return TACHI_FASTEST_MANUAL_REFIRE_TIME;
	}

	// Semi auto
	if (m_bIsPrimaryFireMode)
	{
		return TACHI_FASTEST_MANUAL_REFIRE_TIME;
	}
	// Full auto
	else
	{
		// We are tap firing, get the semi auto fire rate
		if ((pOwner->m_afButtonPressed & IN_ATTACK) &&
			(!(pOwner->m_afButtonLast & IN_ATTACK)))
		{
			if ((gpGlobals->curtime - m_flLastAttackTime) <
				TACHI_FASTEST_AUTO_REFIRE_TIME)
			{
				return TACHI_FASTEST_MANUAL_REFIRE_TIME;
			}
		}

		return TACHI_FASTEST_AUTO_REFIRE_TIME;
	}
}

void CWeaponTachi::ItemPostFrame( void )
{
	BaseClass::ItemPostFrame();

	if ( m_bInReload )
	{
		return;
	}

	CBasePlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
	{
		return;
	}	

	if ( (pOwner->m_nButtons & IN_ATTACK2) && (!(pOwner->m_afButtonLast & IN_ATTACK2)) )
	{
		if (m_flSoonestFiremodeSwitch < gpGlobals->curtime)
		{
			SwitchFireMode();

			m_flSoonestFiremodeSwitch = gpGlobals->curtime + TACHI_FASTEST_FIREMODE_SWITCH_TIME;
			m_flSoonestPrimaryAttack = gpGlobals->curtime + TACHI_FASTEST_FIREMODE_SWITCH_TIME;
		}
	}

	if (m_iClip1 <= 0)
	{
		return;
	}

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestPrimaryAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestPrimaryAttack = gpGlobals->curtime + TACHI_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				// Semi-auto mode
				if (m_bIsPrimaryFireMode)
				{
					m_flSoonestPrimaryAttack = gpGlobals->curtime + TACHI_FASTEST_MANUAL_REFIRE_TIME;
				}
				// Full-auto mode
				else
				{
					m_flSoonestPrimaryAttack = gpGlobals->curtime + TACHI_FASTEST_AUTO_REFIRE_TIME;
				}
			}
		}
	}
	else if (pOwner->m_afButtonReleased & IN_ATTACK)
	{
		// We let go of fire whilst full auto, set semi auto refire time
		// to prevent tap firing at full auto speeds.
		if (!m_bIsPrimaryFireMode)
		{
			m_flSoonestPrimaryAttack = gpGlobals->curtime + TACHI_FASTEST_MANUAL_REFIRE_TIME;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
Activity CWeaponTachi::GetPrimaryAttackActivity( void )
{
	if ( m_nNumShotsFired < 1 )
		return ACT_VM_PRIMARYATTACK;

	if ( m_nNumShotsFired < 2 )
		return ACT_VM_RECOIL1;

	if ( m_nNumShotsFired < 3 )
		return ACT_VM_RECOIL2;

	return ACT_VM_RECOIL3;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CWeaponTachi::Reload( void )
{
	bool fRet = BaseClass::Reload();

	if ( fRet )
	{
		WeaponSound( RELOAD );
		m_flAccuracyPenalty = 0.0f;
	}
	return fRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTachi::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "pistolpax", 0.25f, 0.5f );
	viewPunch.y = SharedRandomFloat( "pistolpay", -0.6f, 0.6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}
