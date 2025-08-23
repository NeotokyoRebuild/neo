//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Player for HL2.
//
//=============================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "c_hl2mp_player.h"
#include "view.h"
#include "takedamageinfo.h"
#include "hl2mp_gamerules.h"
#include "in_buttons.h"
#include "iviewrender_beams.h"			// flashlight beam
#include "r_efx.h"
#include "dlight.h"
#include "c_basetempentity.h"
#include "prediction.h"
#include "bone_setup.h"

#ifdef NEO
#include "c_neo_player.h"
#include "neo_player_shared.h"
#endif

// Don't alias here
#if defined( CHL2MP_Player )
#undef CHL2MP_Player	
#endif

// misyl: Can be set to Msg if you want some info for debugging prediction
#define MsgPredTest(...)
#define MsgPredTest2(...)

ConVar sv_infinite_aux_power( "sv_infinite_aux_power", "0", FCVAR_CHEAT | FCVAR_REPLICATED );

#ifndef NEO
LINK_ENTITY_TO_CLASS( player, C_HL2MP_Player );
#endif

// specific to the local player
BEGIN_RECV_TABLE_NOBASE( C_HL2MP_Player, DT_HL2MPLocalPlayerExclusive )
#ifdef NEO
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
#else
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),
#endif

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
#ifndef NEO
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
#endif
END_RECV_TABLE()

// all players except the local player
BEGIN_RECV_TABLE_NOBASE( C_HL2MP_Player, DT_HL2MPNonLocalPlayerExclusive )
#ifdef NEO
	RecvPropVector( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
#else
	RecvPropVectorXY( RECVINFO_NAME( m_vecNetworkOrigin, m_vecOrigin ) ),
	RecvPropFloat( RECVINFO_NAME( m_vecNetworkOrigin[2], m_vecOrigin[2] ) ),
#endif

	RecvPropFloat( RECVINFO( m_angEyeAngles[0] ) ),
	RecvPropFloat( RECVINFO( m_angEyeAngles[1] ) ),
#ifdef NEO
	RecvPropFloat( RECVINFO( m_angEyeAngles[2] ) ),
#endif
END_RECV_TABLE()

IMPLEMENT_CLIENTCLASS_DT(C_HL2MP_Player, DT_HL2MP_Player, CHL2MP_Player)
	RecvPropDataTable( "hl2mplocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPLocalPlayerExclusive ) ),
	RecvPropDataTable( "hl2mpnonlocaldata", 0, 0, &REFERENCE_RECV_TABLE( DT_HL2MPNonLocalPlayerExclusive ) ),

	RecvPropEHandle( RECVINFO( m_hRagdoll ) ),
	RecvPropInt( RECVINFO( m_iSpawnInterpCounter ) ),
	RecvPropInt( RECVINFO( m_iPlayerSoundType) ),

	RecvPropBool( RECVINFO( m_fIsWalking ) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_HL2MP_Player )
#ifdef NEO
	DEFINE_PRED_FIELD( m_flCycle, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),
	DEFINE_PRED_FIELD( m_nSequence, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_FIELD( m_flPlaybackRate, FIELD_FLOAT, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
	DEFINE_PRED_ARRAY_TOL( m_flEncodedController, FIELD_FLOAT, MAXSTUDIOBONECTRLS, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE, 0.02f ),
	DEFINE_PRED_FIELD( m_nNewSequenceParity, FIELD_INTEGER, FTYPEDESC_OVERRIDE | FTYPEDESC_PRIVATE | FTYPEDESC_NOERRORCHECK ),
#else
	DEFINE_PRED_FIELD( m_fIsWalking, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE ),

	// misyl: Ammo is server side entities in HL2MP. Not catastrophic to error about.
	// Just let the server stomp all over us.
	//
	// There is 1 instance in which is can be a runaway pred error, and that is if you have eg. ar2
	// with just altfire ammo, and get new ammo and we force reload. But the additional pred error sorts that out itself
	// without this for every pickup which is 1000% more common.
	DEFINE_PRED_ARRAY( m_iAmmo, FIELD_INTEGER, MAX_AMMO_TYPES, FTYPEDESC_INSENDTABLE | FTYPEDESC_OVERRIDE | FTYPEDESC_NOERRORCHECK ),
#endif
END_PREDICTION_DATA()

ConVar hl2_walkspeed( "hl2_walkspeed", "150", FCVAR_REPLICATED );
ConVar hl2_normspeed( "hl2_normspeed", "190", FCVAR_REPLICATED );
ConVar hl2_sprintspeed( "hl2_sprintspeed", "320", FCVAR_REPLICATED );

#ifdef NEO
#define	HL2_WALK_SPEED NEO_ASSAULT_WALK_SPEED
#define	HL2_NORM_SPEED NEO_ASSAULT_NORM_SPEED
#define	HL2_SPRINT_SPEED NEO_ASSAULT_SPRINT_SPEED
#else
#define	HL2_WALK_SPEED hl2_walkspeed.GetFloat()
#define	HL2_NORM_SPEED hl2_normspeed.GetFloat()
#define	HL2_SPRINT_SPEED hl2_sprintspeed.GetFloat()
#endif

static ConVar cl_playermodel( "cl_playermodel", "none", FCVAR_USERINFO | FCVAR_ARCHIVE | FCVAR_SERVER_CAN_EXECUTE, "Default Player Model");
static ConVar cl_defaultweapon( "cl_defaultweapon", "weapon_physcannon", FCVAR_USERINFO | FCVAR_ARCHIVE, "Default Spawn Weapon");

void SpawnBlood (Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

//
// SUIT POWER DEVICES
//
#ifdef NEO
#define SUITPOWER_CHARGE_RATE GetAuxChargeRate(this)
static inline float GetAuxChargeRate(C_BaseCombatCharacter *player)
{
	auto neoPlayer = static_cast<C_NEO_Player*>(player);

	switch (neoPlayer->GetClass())
	{
	case NEO_CLASS_RECON:
		if (neoPlayer->IsSprinting())
		{
			return 2.5f;	// 100 units in 40 seconds
		}
		return 5.0f;	// 100 units in 20 seconds
	case NEO_CLASS_ASSAULT:
		if (neoPlayer->IsSprinting())
		{
			return 0.0f;
		}
		return 2.5f;	// 100 units in 40 seconds
	case NEO_CLASS_VIP:
		return 2.5f;	// 100 units in 40 seconds
	case NEO_CLASS_JUGGERNAUT:
		return 10.0f;	// 100 units in 10 seconds
	default:
		break;
	}
	return 0.0f;
}

#else
#define SUITPOWER_CHARGE_RATE	12.5											// 100 units in 8 seconds
#endif

#if defined(NEO)
	CSuitPowerDevice SuitDeviceSprint( bits_SUIT_DEVICE_SPRINT, 5.0f );					// 100 units in 20 seconds
#elif defined(HL2MP)
	CSuitPowerDevice SuitDeviceSprint( bits_SUIT_DEVICE_SPRINT, 25.0f );				// 100 units in 4 seconds
#else
	CSuitPowerDevice SuitDeviceSprint( bits_SUIT_DEVICE_SPRINT, 12.5f );				// 100 units in 8 seconds
#endif

#ifdef HL2_EPISODIC
	CSuitPowerDevice SuitDeviceFlashlight( bits_SUIT_DEVICE_FLASHLIGHT, 1.111 );	// 100 units in 90 second
#else
	CSuitPowerDevice SuitDeviceFlashlight( bits_SUIT_DEVICE_FLASHLIGHT, 2.222 );	// 100 units in 45 second
#endif
CSuitPowerDevice SuitDeviceBreather( bits_SUIT_DEVICE_BREATHER, 6.7f );		// 100 units in 15 seconds (plus three padded seconds)

#ifdef NEO
C_HL2MP_Player::C_HL2MP_Player() : m_iv_angEyeAngles( "C_HL2MP_Player::m_iv_angEyeAngles" )
#else
C_HL2MP_Player::C_HL2MP_Player() : m_PlayerAnimState( this ), m_iv_angEyeAngles( "C_HL2MP_Player::m_iv_angEyeAngles" )
#endif
{
	m_iIDEntIndex = 0;
	m_iSpawnInterpCounterCache = 0;

	AddVar( &m_angEyeAngles, &m_iv_angEyeAngles, LATCH_SIMULATION_VAR );

#ifndef NEO
	m_EntClientFlags |= ENTCLIENTFLAG_DONTUSEIK;
#endif
#ifdef NEO
	m_PlayerAnimState = CreateHL2MPPlayerAnimState( this );
#endif

	m_blinkTimer.Invalidate();

	m_pFlashlightBeam = NULL;

	SuitPower_Initialize();
}

C_HL2MP_Player::~C_HL2MP_Player( void )
{
	ReleaseFlashlight();
#ifdef NEO
	m_PlayerAnimState->Release();
#endif
}

int C_HL2MP_Player::GetIDTarget() const
{
	return m_iIDEntIndex;
}

//-----------------------------------------------------------------------------
// Purpose: Update this client's target entity
//-----------------------------------------------------------------------------
void C_HL2MP_Player::UpdateIDTarget()
{
	if ( !IsLocalPlayer() )
		return;

	// Clear old target and find a new one
	m_iIDEntIndex = 0;

	// don't show IDs in chase spec mode
	if ( GetObserverMode() == OBS_MODE_CHASE || 
		 GetObserverMode() == OBS_MODE_DEATHCAM )
		 return;

	trace_t tr;
	Vector vecStart, vecEnd;
	VectorMA( MainViewOrigin(), 1500, MainViewForward(), vecEnd );
	VectorMA( MainViewOrigin(), 10,   MainViewForward(), vecStart );
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, this, COLLISION_GROUP_NONE, &tr );

	if ( !tr.startsolid && tr.DidHitNonWorldEntity() )
	{
		C_BaseEntity *pEntity = tr.m_pEnt;

		if ( pEntity && (pEntity != this) )
		{
			m_iIDEntIndex = pEntity->entindex();
		}
	}
}

void C_HL2MP_Player::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	Vector vecOrigin = ptr->endpos - vecDir * 4;

	float flDistance = 0.0f;
	
	if ( info.GetAttacker() )
	{
		flDistance = (ptr->endpos - info.GetAttacker()->GetAbsOrigin()).Length();
	}

	if ( m_takedamage )
	{
		AddMultiDamage( info, this );

		int blood = BloodColor();
		
		CBaseEntity *pAttacker = info.GetAttacker();

		if ( pAttacker )
		{
			if ( HL2MPRules()->IsTeamplay() && pAttacker->InSameTeam( this ) == true )
				return;
		}

		if ( blood != DONT_BLEED )
		{
			SpawnBlood( vecOrigin, vecDir, blood, flDistance );// a little surface blood.
			TraceBleed( flDistance, vecDir, ptr, info.GetDamageType() );
		}
	}
}


C_HL2MP_Player* C_HL2MP_Player::GetLocalHL2MPPlayer()
{
	return (C_HL2MP_Player*)C_BasePlayer::GetLocalPlayer();
}

void C_HL2MP_Player::Initialize( void )
{
	m_headYawPoseParam = LookupPoseParameter( "head_yaw" );
	GetPoseParameterRange( m_headYawPoseParam, m_headYawMin, m_headYawMax );

	m_headPitchPoseParam = LookupPoseParameter( "head_pitch" );
	GetPoseParameterRange( m_headPitchPoseParam, m_headPitchMin, m_headPitchMax );

	CStudioHdr *hdr = GetModelPtr();
	for ( int i = 0; i < hdr->GetNumPoseParameters() ; i++ )
	{
		SetPoseParameter( hdr, i, 0.0 );
	}
}

CStudioHdr *C_HL2MP_Player::OnNewModel( void )
{
	CStudioHdr *hdr = BaseClass::OnNewModel();
	
	Initialize( );

#ifdef NEO
	// Reset the players animation states, gestures
	if ( m_PlayerAnimState )
	{
		m_PlayerAnimState->OnNewModel();
	}
#endif

	return hdr;
}

//-----------------------------------------------------------------------------
/**
 * Orient head and eyes towards m_lookAt.
 */
void C_HL2MP_Player::UpdateLookAt( void )
{
	// head yaw
	if (m_headYawPoseParam < 0 || m_headPitchPoseParam < 0)
		return;

	// orient eyes
	m_viewtarget = m_vLookAtTarget;

	// blinking
	if (m_blinkTimer.IsElapsed())
	{
		m_blinktoggle = !m_blinktoggle;
		m_blinkTimer.Start( RandomFloat( 1.5f, 4.0f ) );
	}

	// Figure out where we want to look in world space.
	QAngle desiredAngles;
	Vector to = m_vLookAtTarget - EyePosition();
	VectorAngles( to, desiredAngles );

	// Figure out where our body is facing in world space.
	QAngle bodyAngles( 0, 0, 0 );
	bodyAngles[YAW] = GetLocalAngles()[YAW];


	float flBodyYawDiff = bodyAngles[YAW] - m_flLastBodyYaw;
	m_flLastBodyYaw = bodyAngles[YAW];
	

	// Set the head's yaw.
	float desired = AngleNormalize( desiredAngles[YAW] - bodyAngles[YAW] );
	desired = clamp( desired, m_headYawMin, m_headYawMax );
	m_flCurrentHeadYaw = ApproachAngle( desired, m_flCurrentHeadYaw, 130 * gpGlobals->frametime );

	// Counterrotate the head from the body rotation so it doesn't rotate past its target.
	m_flCurrentHeadYaw = AngleNormalize( m_flCurrentHeadYaw - flBodyYawDiff );
	desired = clamp( desired, m_headYawMin, m_headYawMax );
	
	SetPoseParameter( m_headYawPoseParam, m_flCurrentHeadYaw );

	
	// Set the head's yaw.
	desired = AngleNormalize( desiredAngles[PITCH] );
	desired = clamp( desired, m_headPitchMin, m_headPitchMax );
	
	m_flCurrentHeadPitch = ApproachAngle( desired, m_flCurrentHeadPitch, 130 * gpGlobals->frametime );
	m_flCurrentHeadPitch = AngleNormalize( m_flCurrentHeadPitch );
	SetPoseParameter( m_headPitchPoseParam, m_flCurrentHeadPitch );
}

void C_HL2MP_Player::ClientThink( void )
{
	bool bFoundViewTarget = false;
	
	Vector vForward;
	AngleVectors( GetLocalAngles(), &vForward );

	for( int iClient = 1; iClient <= gpGlobals->maxClients; ++iClient )
	{
		CBaseEntity *pEnt = UTIL_PlayerByIndex( iClient );
		if(!pEnt || !pEnt->IsPlayer())
			continue;

		if ( pEnt->entindex() == entindex() )
			continue;

		Vector vTargetOrigin = pEnt->GetAbsOrigin();
		Vector vMyOrigin =  GetAbsOrigin();

		Vector vDir = vTargetOrigin - vMyOrigin;
		
		if ( vDir.Length() > 128 ) 
			continue;

		VectorNormalize( vDir );

		if ( DotProduct( vForward, vDir ) < 0.0f )
			 continue;

		m_vLookAtTarget = pEnt->EyePosition();
		bFoundViewTarget = true;
		break;
	}

	if ( bFoundViewTarget == false )
	{
		m_vLookAtTarget = GetAbsOrigin() + vForward * 512;
	}

	UpdateIDTarget();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int C_HL2MP_Player::DrawModel( int flags )
{
	if ( !m_bReadyToDraw )
		return 0;

    return BaseClass::DrawModel(flags);
}

//-----------------------------------------------------------------------------
// Should this object receive shadows?
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::ShouldReceiveProjectedTextures( int flags )
{
	Assert( flags & SHADOW_FLAGS_PROJECTED_TEXTURE_TYPE_MASK );

	if ( IsEffectActive( EF_NODRAW ) )
		 return false;

	if( flags & SHADOW_FLAGS_FLASHLIGHT )
	{
		return true;
	}

	return BaseClass::ShouldReceiveProjectedTextures( flags );
}

void C_HL2MP_Player::DoImpactEffect( trace_t &tr, int nDamageType )
{
	if ( GetActiveWeapon() )
	{
		GetActiveWeapon()->DoImpactEffect( tr, nDamageType );
		return;
	}

	BaseClass::DoImpactEffect( tr, nDamageType );
}

void C_HL2MP_Player::PreThink( void )
{
	BaseClass::PreThink();
}

const QAngle &C_HL2MP_Player::EyeAngles()
{
	if( IsLocalPlayer() )
	{
		return BaseClass::EyeAngles();
	}
	else
	{
		return m_angEyeAngles;
	}
}

//-----------------------------------------------------------------------------
// Charge battery fully, turn off all devices.
//-----------------------------------------------------------------------------
void C_HL2MP_Player::SuitPower_Initialize( void )
{
	m_HL2Local.m_bitsActiveDevices = 0x00000000;
	m_HL2Local.m_flSuitPower = 100.0;
	m_HL2Local.m_flSuitPowerLoad = 0.0;
}


//-----------------------------------------------------------------------------
// Purpose: Interface to drain power from the suit's power supply.
// Input:	Amount of charge to remove (expressed as percentage of full charge)
// Output:	Returns TRUE if successful, FALSE if not enough power available.
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::SuitPower_Drain( float flPower )
{
	// Suitpower cheat on?
	if ( sv_infinite_aux_power.GetBool() )
		return true;

	m_HL2Local.m_flSuitPower -= flPower;

	if ( m_HL2Local.m_flSuitPower < 0.01 )
	{
		// Power is depleted!
		// Clamp and fail
		m_HL2Local.m_flSuitPower = 0.0;
		return false;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Interface to add power to the suit's power supply
// Input:	Amount of charge to add
//-----------------------------------------------------------------------------
void C_HL2MP_Player::SuitPower_Charge( float flPower )
{
	m_HL2Local.m_flSuitPower += flPower;

	if( m_HL2Local.m_flSuitPower > 100.0 )
	{
		// Full charge, clamp.
		m_HL2Local.m_flSuitPower = 100.0;
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::SuitPower_IsDeviceActive( const CSuitPowerDevice &device )
{
	return (m_HL2Local.m_bitsActiveDevices & device.GetDeviceID()) != 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::SuitPower_AddDevice( const CSuitPowerDevice &device )
{
	// Make sure this device is NOT active!!
	if( m_HL2Local.m_bitsActiveDevices & device.GetDeviceID() )
		return false;

	if( !IsSuitEquipped() )
		return false;

	m_HL2Local.m_bitsActiveDevices |= device.GetDeviceID();
	m_HL2Local.m_flSuitPowerLoad += device.GetDeviceDrainRate();
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::SuitPower_RemoveDevice( const CSuitPowerDevice &device )
{
	// Make sure this device is active!!
	if( ! (m_HL2Local.m_bitsActiveDevices & device.GetDeviceID()) )
		return false;

	if( !IsSuitEquipped() )
		return false;

#ifdef NEO
	// Fix for https://github.com/NeotokyoRebuild/neo/issues/1165
	// related to the engine's anti-exploit code block below.
	const bool isRecon = static_cast<CNEO_Player*>(this)->GetClass() == NEO_CLASS_RECON;
	if (isRecon) goto skipDrain;
#endif

	// Take a little bit of suit power when you disable a device. If the device is shutting off
	// because the battery is drained, no harm done, the battery charge cannot go below 0. 
	// This code in combination with the delay before the suit can start recharging are a defense
	// against exploits where the player could rapidly tap sprint and never run out of power.
	MsgPredTest2( "[Client %d] [A REMOVE] m_HL2Local.m_flSuitPower: %f\n", gpGlobals->tickcount, m_HL2Local.m_flSuitPower );
	SuitPower_Drain( device.GetDeviceDrainRate() * 0.1f );
	MsgPredTest2( "[Client %d] [B REMOVE] m_HL2Local.m_flSuitPower: %f\n", gpGlobals->tickcount, m_HL2Local.m_flSuitPower );

#ifdef NEO
	skipDrain:
#endif

	m_HL2Local.m_bitsActiveDevices &= ~device.GetDeviceID();
	m_HL2Local.m_flSuitPowerLoad -= device.GetDeviceDrainRate();

#ifdef NEO
	if (static_cast<CNEO_Player*>(this)->GetClass() == NEO_CLASS_RECON)
		return true;
#endif

	if( m_HL2Local.m_bitsActiveDevices == 0x00000000 )
	{
		// With this device turned off, we can set this timer which tells us when the
		// suit power system entered a no-load state.
		m_HL2Local.m_flTimeAllSuitDevicesOff = gpGlobals->curtime;
	}

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define SUITPOWER_BEGIN_RECHARGE_DELAY	0.5f
bool C_HL2MP_Player::SuitPower_ShouldRecharge( void )
{
	// Make sure all devices are off.
	if( m_HL2Local.m_bitsActiveDevices != 0x00000000 )
	{
#ifdef NEO
		// Is the system fully charged?
		if (m_HL2Local.m_flSuitPower >= 100.0f)
		{
			return false;
		}

		// Has the system been in a no-load state for long enough
		// to begin recharging?
		if (gpGlobals->curtime < m_HL2Local.m_flTimeAllSuitDevicesOff + SUITPOWER_BEGIN_RECHARGE_DELAY)
		{
			return false;
		}

		auto neoPlayer = static_cast<C_NEO_Player *>(this);
		switch (neoPlayer->GetClass())
		{
		case NEO_CLASS_RECON:
		{
			// Recons are allowed to recharge AUX whilst sprinting
			return ((m_HL2Local.m_bitsActiveDevices & ~bits_SUIT_DEVICE_SPRINT) == 0);
		} break;
		case NEO_CLASS_ASSAULT:
		{
			return !neoPlayer->IsSprinting();
		} break;
		default:
			break;
		}
#endif
		return false;
	}

	// Is the system fully charged?
	if( m_HL2Local.m_flSuitPower >= 100.0f )
		return false; 

	// Has the system been in a no-load state for long enough
	// to begin recharging?
	if( gpGlobals->curtime < m_HL2Local.m_flTimeAllSuitDevicesOff + SUITPOWER_BEGIN_RECHARGE_DELAY )
		return false;

	return true;
}

void C_HL2MP_Player::SuitPower_Update( void )
{
	if( SuitPower_ShouldRecharge() )
	{
		SuitPower_Charge( SUITPOWER_CHARGE_RATE * gpGlobals->frametime );
	}
	else if( m_HL2Local.m_bitsActiveDevices )
	{
#ifdef NEO
		float flPowerLoad = SuitDeviceSprint.GetDeviceDrainRate();
#else
		float flPowerLoad = m_HL2Local.m_flSuitPowerLoad;
#endif

		//Since stickysprint quickly shuts off sprint if it isn't being used, this isn't an issue.
		// misyl: no sticky sprint for hl2mp.
		//if ( !sv_stickysprint.GetBool() )
		{
			if( SuitPower_IsDeviceActive(SuitDeviceSprint) )
			{
#ifdef NEO
				if( !fabs(GetAbsVelocity().x) && !fabs(GetAbsVelocity().y) )
				{
					// If player's not moving, don't drain sprint juice.
					flPowerLoad -= SuitDeviceSprint.GetDeviceDrainRate();
				}
				else
				{
					if (static_cast<C_NEO_Player*>(this)->GetClass() == NEO_CLASS_RECON)
					{
						flPowerLoad -= SuitDeviceSprint.GetDeviceDrainRate();
					}
				}
#else
				if( CloseEnough(fabs(GetAbsVelocity().x), 0.0f) && CloseEnough(fabs(GetAbsVelocity().y), 0.0f) )
				{
					if ( CloseEnough( m_HL2Local.m_flSuitPowerLoad, SuitDeviceSprint.GetDeviceDrainRate() ) )
					{
						flPowerLoad = 0.0f;
					}
					else
					{
						// If player's not moving, don't drain sprint juice.
						flPowerLoad -= SuitDeviceSprint.GetDeviceDrainRate();
					}
				}
#endif
			}
		}

		if( SuitPower_IsDeviceActive(SuitDeviceFlashlight) )
		{
			//float factor;

			//factor = 1.0f / m_flFlashlightPowerDrainScale;

			float factor = 1.0f;

			flPowerLoad -= ( SuitDeviceFlashlight.GetDeviceDrainRate() * (1.0f - factor) );
		}

		SuitPower_Drain( flPowerLoad * gpGlobals->frametime );

	}
	MsgPredTest2( "[Client %d] m_HL2Local.m_flSuitPower: %f m_fIsSprinting: %d\n", gpGlobals->tickcount, m_HL2Local.m_flSuitPower, m_fIsSprinting ? 1 : 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HL2MP_Player::AddEntity( void )
{
	BaseClass::AddEntity();

	// Zero out model pitch, blending takes care of all of it.
	SetLocalAnglesDim( X_INDEX, 0 );

	if( this != C_BasePlayer::GetLocalPlayer() )
	{
		if ( IsEffectActive( EF_DIMLIGHT ) )
		{
			int iAttachment = LookupAttachment( "anim_attachment_RH" );

			if ( iAttachment < 0 )
				return;

			Vector vecOrigin;
			QAngle eyeAngles = m_angEyeAngles;
	
			GetAttachment( iAttachment, vecOrigin, eyeAngles );

			Vector vForward;
			AngleVectors( eyeAngles, &vForward );
				
			trace_t tr;
			UTIL_TraceLine( vecOrigin, vecOrigin + (vForward * 200), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			if( !m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_nType = TE_BEAMPOINTS;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_pszModelName = "sprites/glow01.vmt";
				beamInfo.m_pszHaloName = "sprites/glow01.vmt";
				beamInfo.m_flHaloScale = 3.0;
				beamInfo.m_flWidth = 8.0f;
				beamInfo.m_flEndWidth = 35.0f;
				beamInfo.m_flFadeLength = 300.0f;
				beamInfo.m_flAmplitude = 0;
				beamInfo.m_flBrightness = 60.0;
				beamInfo.m_flSpeed = 0.0f;
				beamInfo.m_nStartFrame = 0.0;
				beamInfo.m_flFrameRate = 0.0;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;
				beamInfo.m_nSegments = 8;
				beamInfo.m_bRenderable = true;
				beamInfo.m_flLife = 0.5;
				beamInfo.m_nFlags = FBEAM_FOREVER | FBEAM_ONLYNOISEONCE | FBEAM_NOTILE | FBEAM_HALOBEAM;
				
				m_pFlashlightBeam = beams->CreateBeamPoints( beamInfo );
			}

			if( m_pFlashlightBeam )
			{
				BeamInfo_t beamInfo;
				beamInfo.m_vecStart = tr.startpos;
				beamInfo.m_vecEnd = tr.endpos;
				beamInfo.m_flRed = 255.0;
				beamInfo.m_flGreen = 255.0;
				beamInfo.m_flBlue = 255.0;

				beams->UpdateBeamInfo( m_pFlashlightBeam, beamInfo );

				dlight_t *el = effects->CL_AllocDlight( 0 );
				el->origin = tr.endpos;
				el->radius = 50; 
				el->color.r = 200;
				el->color.g = 200;
				el->color.b = 200;
				el->die = gpGlobals->curtime + 0.1;
			}
		}
		else if ( m_pFlashlightBeam )
		{
			ReleaseFlashlight();
		}
	}
}

ShadowType_t C_HL2MP_Player::ShadowCastType( void ) 
{
	if ( !IsVisible() )
		 return SHADOWS_NONE;

	return SHADOWS_RENDER_TO_TEXTURE_DYNAMIC;
}


const QAngle& C_HL2MP_Player::GetRenderAngles()
{
	if ( IsRagdoll() )
	{
		return vec3_angle;
	}
	else
	{
#ifdef NEO
		return m_PlayerAnimState->GetRenderAngles();
#else
		return m_PlayerAnimState.GetRenderAngles();
#endif
	}
}

bool C_HL2MP_Player::ShouldDraw( void )
{
	// If we're dead, our ragdoll will be drawn for us instead.
	if ( !IsAlive() )
		return false;

//	if( GetTeamNumber() == TEAM_SPECTATOR )
//		return false;

	if( IsLocalPlayer() && IsRagdoll() )
		return true;
	
	if ( IsRagdoll() )
		return false;

	return BaseClass::ShouldDraw();
}

void C_HL2MP_Player::NotifyShouldTransmit( ShouldTransmitState_t state )
{
	if ( state == SHOULDTRANSMIT_END )
	{
		if( m_pFlashlightBeam != NULL )
		{
			ReleaseFlashlight();
		}
	}

	BaseClass::NotifyShouldTransmit( state );
}

void C_HL2MP_Player::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );
	}

	UpdateVisibility();
}

void C_HL2MP_Player::PostDataUpdate( DataUpdateType_t updateType )
{
	if ( m_iSpawnInterpCounter != m_iSpawnInterpCounterCache )
	{
		MoveToLastReceivedPosition( true );
		ResetLatched();
		m_iSpawnInterpCounterCache = m_iSpawnInterpCounter;
	}

	BaseClass::PostDataUpdate( updateType );
}

void C_HL2MP_Player::ReleaseFlashlight( void )
{
	if( m_pFlashlightBeam )
	{
		m_pFlashlightBeam->flags = 0;
		m_pFlashlightBeam->die = gpGlobals->curtime - 1;

		m_pFlashlightBeam = NULL;
	}
}

float C_HL2MP_Player::GetFOV( void )
{
	//Find our FOV with offset zoom value
	float flFOVOffset = C_BasePlayer::GetFOV() + GetZoom();

	// Clamp FOV in MP
	int min_fov = GetMinFOV();
	
	// Don't let it go too low
	flFOVOffset = MAX( min_fov, flFOVOffset );

	return flFOVOffset;
}

//=========================================================
// Autoaim
// set crosshair position to point to enemey
//=========================================================
Vector C_HL2MP_Player::GetAutoaimVector( float flDelta )
{
	// Never autoaim a predicted weapon (for now)
	Vector	forward;
	AngleVectors( EyeAngles() + m_Local.m_vecPunchAngle, &forward );
	return	forward;
}

//-----------------------------------------------------------------------------
// Purpose: Returns whether or not we are allowed to sprint now.
//-----------------------------------------------------------------------------
bool C_HL2MP_Player::CanSprint( void )
{
	return (
		(!m_Local.m_bDucked && !m_Local.m_bDucking) &&
		(GetWaterLevel() != 3) );
}

extern ConVar sv_maxspeed;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StartSprinting( void )
{
#ifdef NEO
	if (m_HL2Local.m_flSuitPower < SPRINT_START_MIN)
#else
	if( m_HL2Local.m_flSuitPower < 10 )
#endif
	{
#ifdef NEO
#if(0)
		CPASAttenuationFilter filter( this );
		filter.UsePredictionRules();
		EmitSound( filter, entindex(), "HL2Player.SprintNoPower" );
#endif
#endif

		// Don't sprint unless there's a reasonable
		// amount of suit power.
		return;
	}

#ifdef NEO
#if(0)
	CPASAttenuationFilter filter( this );
	filter.UsePredictionRules();
	EmitSound( filter, entindex(), "HL2Player.SprintStart" );
#endif
#endif

#ifndef NEO
	SetMaxSpeed( HL2_SPRINT_SPEED );
#endif

	m_fIsSprinting = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StopSprinting( void )
{
#ifndef NEO
	SetMaxSpeed( HL2_NORM_SPEED );
#endif

	m_fIsSprinting = false;
}

void C_HL2MP_Player::HandleSpeedChanges( CMoveData *mv )
{
#ifndef NEO
	int nChangedButtons = mv->m_nButtons ^ mv->m_nOldButtons;

	bool bJustPressedSpeed = !!( nChangedButtons & IN_SPEED );

#ifdef NEO
	constexpr float MOVING_SPEED_MINIMUM = 0.5f; // NEOTODO (Adam) This is the same value as defined in cbaseanimating, should we be using the same value? Should we import it here?
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) && GetLocalVelocity().IsLengthGreaterThan(MOVING_SPEED_MINIMUM));
#else
	const bool bWantSprint = ( CanSprint() && IsSuitEquipped() && ( mv->m_nButtons & IN_SPEED ) );
#endif

#ifdef NEO
	const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && ((mv->m_nButtons & IN_SPEED) || (( nChangedButtons & IN_SPEED ) != 0));
#else
	const bool bWantsToChangeSprinting = ( m_HL2Local.m_bNewSprinting != bWantSprint ) && ( nChangedButtons & IN_SPEED ) != 0;
#endif

	bool bSprinting = m_HL2Local.m_bNewSprinting;
	if ( bWantsToChangeSprinting )
	{
		if ( bWantSprint )
		{
#ifdef NEO
			if ( m_HL2Local.m_flSuitPower < SPRINT_START_MIN )
#else
			if ( m_HL2Local.m_flSuitPower < 10.0f )
#endif
			{
#ifndef NEO
				if ( bJustPressedSpeed )
				{
					CPASAttenuationFilter filter( this );
					filter.UsePredictionRules();
					EmitSound( filter, entindex(), "HL2Player.SprintNoPower" );
				}
#endif
			}
			else
			{
				bSprinting = true;
			}
		}
		else
		{
			bSprinting = false;
		}
	}

	if ( m_HL2Local.m_flSuitPower < 0.01 )
	{
		bSprinting = false;
	}

	bool bWantWalking;

	if ( IsSuitEquipped() )
	{
		bWantWalking = ( mv->m_nButtons & IN_WALK ) && !bSprinting && !( mv->m_nButtons & IN_DUCK );
	}
	else
	{
		bWantWalking = true;
	}

	if ( bWantWalking )
	{
		bSprinting = false;
	}

	m_HL2Local.m_bNewSprinting = bSprinting;

	// NEO TODO (nullsystem): Really we should start using this kind of max speed setting
#ifdef NEO
	mv->m_flClientMaxSpeed = MaxSpeed();
#else
	if ( bSprinting )
	{
		if ( bJustPressedSpeed )
		{
			CPASAttenuationFilter filter( this );
			filter.UsePredictionRules();
			EmitSound( filter, entindex(), "HL2Player.SprintStart" );
		}
		mv->m_flClientMaxSpeed = HL2_SPRINT_SPEED;
	}
	else if ( bWantWalking )
	{
		mv->m_flClientMaxSpeed = HL2_WALK_SPEED;
	}
	else
	{
		mv->m_flClientMaxSpeed = HL2_NORM_SPEED;
	}
#endif

	mv->m_flMaxSpeed = sv_maxspeed.GetFloat();
#endif // NEO
}

void C_HL2MP_Player::ReduceTimers( CMoveData* mv )
{
#ifdef NEO
	bool bSprinting = IsSprinting();
#else
	bool bSprinting = mv->m_flClientMaxSpeed == HL2_SPRINT_SPEED;
#endif

	if ( bSprinting )
	{
		SuitPower_AddDevice( SuitDeviceSprint );
	}
	else
	{
		SuitPower_RemoveDevice( SuitDeviceSprint );
	}

	SuitPower_Update();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StartWalking( void )
{
#ifndef NEO
	SetMaxSpeed( HL2_WALK_SPEED );
#endif

	m_fIsWalking = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void C_HL2MP_Player::StopWalking( void )
{
#ifndef NEO
	SetMaxSpeed( HL2_NORM_SPEED );
#endif

	m_fIsWalking = false;
}

void C_HL2MP_Player::ItemPreFrame( void )
{
#ifdef NEO
	if (static_cast<C_NEO_Player*>(this)->GetNeoFlags() & FL_FROZEN)
	{
		return;
	}
#endif

	if ( GetFlags() & FL_FROZEN )
		 return;

	// Disallow shooting while zooming
	if ( m_nButtons & IN_ZOOM )
	{
		//FIXME: Held weapons like the grenade get sad when this happens
		m_nButtons &= ~(IN_ATTACK|IN_ATTACK2);
	}

	BaseClass::ItemPreFrame();

}
	
void C_HL2MP_Player::ItemPostFrame( void )
{
#ifdef NEO
	if (static_cast<C_NEO_Player*>(this)->GetNeoFlags() & FL_FROZEN)
	{
		return;
	}
#endif

	if ( GetFlags() & FL_FROZEN )
		 return;

	BaseClass::ItemPostFrame();
}

C_BaseAnimating *C_HL2MP_Player::BecomeRagdollOnClient()
{
	// Let the C_CSRagdoll entity do this.
	// m_builtRagdoll = true;
	return NULL;
}

void C_HL2MP_Player::CalcView( Vector &eyeOrigin, QAngle &eyeAngles, float &zNear, float &zFar, float &fov )
{
	if ( m_lifeState != LIFE_ALIVE && !IsObserver() )
	{
		Vector origin = EyePosition();			

		IRagdoll *pRagdoll = GetRepresentativeRagdoll();

		if ( pRagdoll )
		{
			origin = pRagdoll->GetRagdollOrigin();
			origin.z += VEC_DEAD_VIEWHEIGHT_SCALED( this ).z; // look over ragdoll, not through
		}

		BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );

		eyeOrigin = origin;
		
		Vector vForward; 
		AngleVectors( eyeAngles, &vForward );

		VectorNormalize( vForward );
		VectorMA( origin, -CHASE_CAM_DISTANCE_MAX, vForward, eyeOrigin );

		Vector WALL_MIN( -WALL_OFFSET, -WALL_OFFSET, -WALL_OFFSET );
		Vector WALL_MAX( WALL_OFFSET, WALL_OFFSET, WALL_OFFSET );

		trace_t trace; // clip against world
		C_BaseEntity::PushEnableAbsRecomputations( false ); // HACK don't recompute positions while doing RayTrace
		UTIL_TraceHull( origin, eyeOrigin, WALL_MIN, WALL_MAX, MASK_SOLID_BRUSHONLY, this, COLLISION_GROUP_NONE, &trace );
		C_BaseEntity::PopEnableAbsRecomputations();

		if (trace.fraction < 1.0)
		{
			eyeOrigin = trace.endpos;
		}
		
		return;
	}

	BaseClass::CalcView( eyeOrigin, eyeAngles, zNear, zFar, fov );
}

IRagdoll* C_HL2MP_Player::GetRepresentativeRagdoll() const
{
	if ( m_hRagdoll.Get() )
	{
		C_HL2MPRagdoll *pRagdoll = (C_HL2MPRagdoll*)m_hRagdoll.Get();

		return pRagdoll->GetIRagdoll();
	}
	else
	{
		return NULL;
	}
}

//HL2MPRAGDOLL


IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_HL2MPRagdoll, DT_HL2MPRagdoll, CHL2MPRagdoll )
	RecvPropVector( RECVINFO(m_vecRagdollOrigin) ),
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_nModelIndex ) ),
	RecvPropInt( RECVINFO(m_nForceBone) ),
	RecvPropVector( RECVINFO(m_vecForce) ),
	RecvPropVector( RECVINFO( m_vecRagdollVelocity ) )
END_RECV_TABLE()



C_HL2MPRagdoll::C_HL2MPRagdoll()
{
#ifdef NEO
	m_flNeoCreateTime = gpGlobals->curtime;
#endif // NEO
}

C_HL2MPRagdoll::~C_HL2MPRagdoll()
{
	PhysCleanupFrictionSounds( this );

	if ( m_hPlayer )
	{
		m_hPlayer->CreateModelInstance();
	}
}

void C_HL2MPRagdoll::Interp_Copy( C_BaseAnimatingOverlay *pSourceEntity )
{
	if ( !pSourceEntity )
		return;
	
	VarMapping_t *pSrc = pSourceEntity->GetVarMapping();
	VarMapping_t *pDest = GetVarMapping();
    
	// Find all the VarMapEntry_t's that represent the same variable.
	for ( int i = 0; i < pDest->m_Entries.Count(); i++ )
	{
		VarMapEntry_t *pDestEntry = &pDest->m_Entries[i];
		const char *pszName = pDestEntry->watcher->GetDebugName();
		for ( int j=0; j < pSrc->m_Entries.Count(); j++ )
		{
			VarMapEntry_t *pSrcEntry = &pSrc->m_Entries[j];
			if ( !Q_strcmp( pSrcEntry->watcher->GetDebugName(), pszName ) )
			{
				pDestEntry->watcher->Copy( pSrcEntry->watcher );
				break;
			}
		}
	}
}

void C_HL2MPRagdoll::ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName )
{
	IPhysicsObject *pPhysicsObject = VPhysicsGetObject();

	if( !pPhysicsObject )
		return;

	Vector dir = pTrace->endpos - pTrace->startpos;

	if ( iDamageType == DMG_BLAST )
	{
		dir *= 4000;  // adjust impact strenght
				
		// apply force at object mass center
		pPhysicsObject->ApplyForceCenter( dir );
	}
	else
	{
		Vector hitpos;  
	
		VectorMA( pTrace->startpos, pTrace->fraction, dir, hitpos );
		VectorNormalize( dir );

		dir *= 4000;  // adjust impact strenght

		// apply force where we hit it
		pPhysicsObject->ApplyForceOffset( dir, hitpos );	

		// Blood spray!
//		FX_CS_BloodSpray( hitpos, dir, 10 );
	}

	m_pRagdoll->ResetRagdollSleepAfterTime();
}


void C_HL2MPRagdoll::CreateHL2MPRagdoll( void )
{
	// First, initialize all our data. If we have the player's entity on our client,
	// then we can make ourselves start out exactly where the player is.
	C_HL2MP_Player *pPlayer = dynamic_cast< C_HL2MP_Player* >( m_hPlayer.Get() );
	
	if ( pPlayer && !pPlayer->IsDormant() )
	{
		// move my current model instance to the ragdoll's so decals are preserved.
		pPlayer->SnatchModelInstance( this );

		VarMapping_t *varMap = GetVarMapping();

		// Copy all the interpolated vars from the player entity.
		// The entity uses the interpolated history to get bone velocity.
		bool bRemotePlayer = (pPlayer != C_BasePlayer::GetLocalPlayer());			
		if ( bRemotePlayer )
		{
			Interp_Copy( pPlayer );

			SetAbsAngles( pPlayer->GetRenderAngles() );
			GetRotationInterpolator().Reset();

			m_flAnimTime = pPlayer->m_flAnimTime;
			SetSequence( pPlayer->GetSequence() );
			m_flPlaybackRate = pPlayer->GetPlaybackRate();
		}
		else
		{
			// This is the local player, so set them in a default
			// pose and slam their velocity, angles and origin
			SetAbsOrigin( m_vecRagdollOrigin );
			
			SetAbsAngles( pPlayer->GetRenderAngles() );

			SetAbsVelocity( m_vecRagdollVelocity );

			int iSeq = pPlayer->GetSequence();
			if ( iSeq == -1 )
			{
				Assert( false );	// missing walk_lower?
				iSeq = 0;
			}
			
			SetSequence( iSeq );	// walk_lower, basic pose
			SetCycle( 0.0 );

			Interp_Reset( varMap );
		}		
	}
	else
	{
		// overwrite network origin so later interpolation will
		// use this position
		SetNetworkOrigin( m_vecRagdollOrigin );

		SetAbsOrigin( m_vecRagdollOrigin );
		SetAbsVelocity( m_vecRagdollVelocity );

		Interp_Reset( GetVarMapping() );
		
	}

	SetModelIndex( m_nModelIndex );

	// Make us a ragdoll..
	m_nRenderFX = kRenderFxRagdoll;

	matrix3x4_t boneDelta0[MAXSTUDIOBONES];
	matrix3x4_t boneDelta1[MAXSTUDIOBONES];
	matrix3x4_t currentBones[MAXSTUDIOBONES];
	const float boneDt = 0.05f;

#ifdef NEO
	// NEO HACK DG: Crazy mismatch between the JGR model and the old
	// playermodel sends the ragdoll flying (when turning into the JGR)
	// Probably because of the time based stuff in GetRagdollInitBoneArrays()
	C_NEO_Player* pNEOPlayer = ToNEOPlayer(pPlayer);
	if (pNEOPlayer && !pNEOPlayer->IsDormant() && (pNEOPlayer->GetClass() != NEO_CLASS_JUGGERNAUT))
#else
	if ( pPlayer && !pPlayer->IsDormant() )
#endif
	{
		pPlayer->GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}
	else
	{
		GetRagdollInitBoneArrays( boneDelta0, boneDelta1, currentBones, boneDt );
	}

	InitAsClientRagdoll( boneDelta0, boneDelta1, currentBones, boneDt );
}


void C_HL2MPRagdoll::OnDataChanged( DataUpdateType_t type )
{
	BaseClass::OnDataChanged( type );

	if ( type == DATA_UPDATE_CREATED )
	{
		CreateHL2MPRagdoll();
	}
}

IRagdoll* C_HL2MPRagdoll::GetIRagdoll() const
{
	return m_pRagdoll;
}

void C_HL2MPRagdoll::UpdateOnRemove( void )
{
	VPhysicsSetObject( NULL );

	BaseClass::UpdateOnRemove();
}

//-----------------------------------------------------------------------------
// Purpose: clear out any face/eye values stored in the material system
//-----------------------------------------------------------------------------
void C_HL2MPRagdoll::SetupWeights( const matrix3x4_t *pBoneToWorld, int nFlexWeightCount, float *pFlexWeights, float *pFlexDelayedWeights )
{
	BaseClass::SetupWeights( pBoneToWorld, nFlexWeightCount, pFlexWeights, pFlexDelayedWeights );

	static float destweight[128];
	static bool bIsInited = false;

	CStudioHdr *hdr = GetModelPtr();
	if ( !hdr )
		return;

	int nFlexDescCount = hdr->numflexdesc();
	if ( nFlexDescCount )
	{
		Assert( !pFlexDelayedWeights );
		memset( pFlexWeights, 0, nFlexWeightCount * sizeof(float) );
	}

	if ( m_iEyeAttachment > 0 )
	{
		matrix3x4_t attToWorld;
		if (GetAttachment( m_iEyeAttachment, attToWorld ))
		{
			Vector local, tmp;
			local.Init( 1000.0f, 0.0f, 0.0f );
			VectorTransform( local, attToWorld, tmp );
			modelrender->SetViewTarget( GetModelPtr(), GetBody(), tmp );
		}
	}
}
#ifdef NEO
#ifdef GLOWS_ENABLE
extern ConVar glow_outline_effect_enable;
#endif // GLOWS_ENABLE
int C_HL2MPRagdoll::DrawModel(int flags)
{
#ifdef GLOWS_ENABLE
	auto pTargetPlayer = glow_outline_effect_enable.GetBool() ? C_NEO_Player::GetLocalNEOPlayer() : C_NEO_Player::GetVisionTargetNEOPlayer();
#else
	auto pTargetPlayer = C_NEO_Player::GetTargetNEOPlayer();
#endif // GLOWS_ENABLE
	if (!pTargetPlayer)
	{
		Assert(false);
		return BaseClass::DrawModel(flags);
	}

	bool inThermalVision = pTargetPlayer ? (pTargetPlayer->IsInVision() && pTargetPlayer->GetClass() == NEO_CLASS_SUPPORT) : false;
	if (inThermalVision)
	{
		IMaterial* pass = materials->FindMaterial("dev/thermal_ragdoll_model", TEXTURE_GROUP_MODEL);
		modelrender->ForcedMaterialOverride(pass);
		int ret = BaseClass::DrawModel(flags);
		modelrender->ForcedMaterialOverride(nullptr);
		return ret;
	}

	return BaseClass::DrawModel(flags);
}
#endif // NEO

void C_HL2MP_Player::UpdateClientSideAnimation()
{
#ifdef NEO // TODO (nullsystem): 2025-02-18 SOURCE SDK 2013 CHECK
	m_PlayerAnimState->Update( EyeAngles()[YAW], EyeAngles()[PITCH] );
#else
	m_PlayerAnimState.Update();
#endif

	BaseClass::UpdateClientSideAnimation();
}

// TODO (nullsystem): 2025-02-18 SOURCE SDK 2013 CHECK

// -------------------------------------------------------------------------------- //
// Player animation event. Sent to the client when a player fires, jumps, reloads, etc..
// -------------------------------------------------------------------------------- //

class C_TEPlayerAnimEvent : public C_BaseTempEntity
{
public:
	DECLARE_CLASS( C_TEPlayerAnimEvent, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	virtual void PostDataUpdate( DataUpdateType_t updateType )
	{
		// Create the effect.
		C_HL2MP_Player *pPlayer = dynamic_cast< C_HL2MP_Player* >( m_hPlayer.Get() );
		if ( pPlayer && !pPlayer->IsDormant() )
		{
			pPlayer->DoAnimationEvent( (PlayerAnimEvent_t)m_iEvent.Get(), m_nData );
		}	
	}

public:
	CNetworkHandle( CBasePlayer, m_hPlayer );
	CNetworkVar( int, m_iEvent );
	CNetworkVar( int, m_nData );
};

IMPLEMENT_CLIENTCLASS_EVENT( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent, CTEPlayerAnimEvent );

BEGIN_RECV_TABLE_NOBASE( C_TEPlayerAnimEvent, DT_TEPlayerAnimEvent )
	RecvPropEHandle( RECVINFO( m_hPlayer ) ),
	RecvPropInt( RECVINFO( m_iEvent ) ),
	RecvPropInt( RECVINFO( m_nData ) )
END_RECV_TABLE()

void C_HL2MP_Player::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	if ( IsLocalPlayer() )
	{
		if ( ( prediction->InPrediction() && !prediction->IsFirstTimePredicted() ) )
			return;
	}

	MDLCACHE_CRITICAL_SECTION();
#ifdef NEO
	m_PlayerAnimState->DoAnimationEvent( event, nData );
#else
	m_PlayerAnimState.DoAnimationEvent( event, nData );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_HL2MP_Player::CalculateIKLocks( float currentTime )
{
	if (!m_pIk) 
		return;

	int targetCount = m_pIk->m_target.Count();
	if ( targetCount == 0 )
		return;

	// In TF, we might be attaching a player's view to a walking model that's using IK. If we are, it can
	// get in here during the view setup code, and it's not normally supposed to be able to access the spatial
	// partition that early in the rendering loop. So we allow access right here for that special case.
	SpatialPartitionListMask_t curSuppressed = partition->GetSuppressedLists();
	partition->SuppressLists( PARTITION_ALL_CLIENT_EDICTS, false );
	CBaseEntity::PushEnableAbsRecomputations( false );

	for (int i = 0; i < targetCount; i++)
	{
		trace_t trace;
		CIKTarget *pTarget = &m_pIk->m_target[i];

		if (!pTarget->IsActive())
			continue;

		switch( pTarget->type)
		{
		case IK_GROUND:
			{
				pTarget->SetPos( Vector( pTarget->est.pos.x, pTarget->est.pos.y, GetRenderOrigin().z ));
				pTarget->SetAngles( GetRenderAngles() );
			}
			break;

		case IK_ATTACHMENT:
			{
				C_BaseEntity *pEntity = NULL;
				float flDist = pTarget->est.radius;

				// FIXME: make entity finding sticky!
				// FIXME: what should the radius check be?
				for ( CEntitySphereQuery sphere( pTarget->est.pos, 64 ); ( pEntity = sphere.GetCurrentEntity() ) != NULL; sphere.NextEntity() )
				{
					C_BaseAnimating *pAnim = pEntity->GetBaseAnimating( );
					if (!pAnim)
						continue;

					int iAttachment = pAnim->LookupAttachment( pTarget->offset.pAttachmentName );
					if (iAttachment <= 0)
						continue;

					Vector origin;
					QAngle angles;
					pAnim->GetAttachment( iAttachment, origin, angles );

					// debugoverlay->AddBoxOverlay( origin, Vector( -1, -1, -1 ), Vector( 1, 1, 1 ), QAngle( 0, 0, 0 ), 255, 0, 0, 0, 0 );

					float d = (pTarget->est.pos - origin).Length();

					if ( d >= flDist)
						continue;

					flDist = d;
					pTarget->SetPos( origin );
					pTarget->SetAngles( angles );
					// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 255, 0, 0, 0 );
				}

				if (flDist >= pTarget->est.radius)
				{
					// debugoverlay->AddBoxOverlay( pTarget->est.pos, Vector( -pTarget->est.radius, -pTarget->est.radius, -pTarget->est.radius ), Vector( pTarget->est.radius, pTarget->est.radius, pTarget->est.radius), QAngle( 0, 0, 0 ), 0, 0, 255, 0, 0 );
					// no solution, disable ik rule
					pTarget->IKFailed( );
				}
			}
			break;
		}
	}

	CBaseEntity::PopEnableAbsRecomputations();
	partition->SuppressLists( curSuppressed, true );
}

void C_HL2MP_Player::PostThink( void )
{
	BaseClass::PostThink();

	// Store the eye angles pitch so the client can compute its animation state correctly.
	m_angEyeAngles = EyeAngles();

	if ( GetFlags() & FL_DUCKING )
	{
#ifdef NEO
		CNEO_Player* neoPlayer = static_cast<CNEO_Player*>(this);
		SetCollisionBounds(VEC_DUCK_HULL_MIN_NEOSCALED(neoPlayer), VEC_DUCK_HULL_MAX_NEOSCALED(neoPlayer));
#else
		SetCollisionBounds( VEC_CROUCH_TRACE_MIN, VEC_CROUCH_TRACE_MAX );
#endif
	}
}

