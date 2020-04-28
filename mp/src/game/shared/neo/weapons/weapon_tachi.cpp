#include "cbase.h"
#include "weapon_tachi.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED( WeaponTachi, DT_WeaponTachi )

BEGIN_NETWORK_TABLE( CWeaponTachi, DT_WeaponTachi )
	DEFINE_NEO_BASE_WEP_NETWORK_TABLE

#ifdef CLIENT_DLL
	RecvPropTime( RECVINFO( m_flSoonestFiremodeSwitch ) ),
	RecvPropBool( RECVINFO( m_bIsPrimaryFireMode ) ),
#else
	SendPropTime( SENDINFO( m_flSoonestFiremodeSwitch ) ),
	SendPropBool( SENDINFO( m_bIsPrimaryFireMode ) ),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA( CWeaponTachi )
	DEFINE_NEO_BASE_WEP_PREDICTION

	DEFINE_PRED_FIELD( m_flSoonestFiremodeSwitch, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_bIsPrimaryFireMode, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()
#endif

NEO_IMPLEMENT_ACTTABLE(CWeaponTachi)

LINK_ENTITY_TO_CLASS( weapon_tachi, CWeaponTachi );

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponTachi)
	DEFINE_FIELD(m_flSoonestFiremodeSwitch, FIELD_TIME),
	DEFINE_FIELD(m_bIsPrimaryFireMode, FIELD_BOOLEAN),
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER( weapon_tachi );

CWeaponTachi::CWeaponTachi()
{
	m_flSoonestAttack = gpGlobals->curtime;
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
	if (m_iClip1 == 0)
	{
		if (!m_bFireOnEmpty)
		{
			CheckReload();
		}
		else
		{
			WeaponSound(EMPTY);
			m_flNextPrimaryAttack = 0.2;
		}

		return;
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

	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
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
	if ( ( ( pOwner->m_nButtons & IN_ATTACK ) == false ) && ( m_flSoonestAttack < gpGlobals->curtime ) )
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
			m_flSoonestAttack = gpGlobals->curtime + TACHI_FASTEST_FIREMODE_SWITCH_TIME;
		}
	}

	if (pOwner->m_nButtons & IN_ATTACK)
	{
		if (m_flSoonestAttack < gpGlobals->curtime)
		{
			if (m_iClip1 <= 0)
			{
				DryFire();

				m_flSoonestAttack = gpGlobals->curtime + TACHI_FASTEST_DRY_REFIRE_TIME;
			}
			else
			{
				// Semi-auto mode
				if (m_bIsPrimaryFireMode)
				{
					m_flSoonestAttack = gpGlobals->curtime + TACHI_FASTEST_MANUAL_REFIRE_TIME;
				}
				// Full-auto mode
				else
				{
					m_flSoonestAttack = gpGlobals->curtime + TACHI_FASTEST_AUTO_REFIRE_TIME;
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
			m_flSoonestAttack = gpGlobals->curtime + TACHI_FASTEST_MANUAL_REFIRE_TIME;
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
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponTachi::AddViewKick( void )
{
	CBasePlayer *pPlayer  = ToBasePlayer( GetOwner() );
	
	if ( pPlayer == NULL )
		return;

	QAngle	viewPunch;

	viewPunch.x = SharedRandomFloat( "tachipax", 0.25f, 0.5f );
	viewPunch.y = SharedRandomFloat( "tachipay", -0.6f, 0.6f );
	viewPunch.z = 0.0f;

	//Add it to the view punch
	pPlayer->ViewPunch( viewPunch );
}
