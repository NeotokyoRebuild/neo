#include "cbase.h"
#include "neo_playeranimstate.h"
#include "datacache/imdlcache.h"
#include "weapon_detpack.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Upper body sequence names, each followed by the weapon's animation suffix
static constexpr const char *AIM_IDLE_NAME = "idle_upper_";
static constexpr const char *AIM_CROUCH_IDLE_NAME = "crouch_idle_upper_";
static constexpr const char *AIM_CROUCH_WALK_NAME = "crouch_walk_upper_";
static constexpr const char *AIM_WALK_NAME = "walk_upper_";
static constexpr const char *AIM_RUN_NAME = "run_upper_";

static constexpr const char *FIRE_IDLE_NAME = "idle_shoot_";
static constexpr const char *FIRE_CROUCH_NAME = "crouch_idle_shoot_";
static constexpr const char *FIRE_CROUCH_WALK_NAME = "crouch_walk_shoot_";
static constexpr const char *FIRE_WALK_NAME = "walk_shoot_";
static constexpr const char *FIRE_RUN_NAME = "run_shoot_";

// The fire, reload and throw sequences play on the layer above the base's aim layers
static constexpr int GESTURE_LAYER = AIMSEQUENCE_LAYER + NUM_AIMSEQUENCE_LAYERS;
static constexpr int NUM_LAYERS = GESTURE_LAYER + 1;

// The legs play at the rate of the player's speed over the speed the walk, run and crouch walk sequences cover,
// which is constant as in classic NEOTOKYO: two of the original models' forward walk and run animations carry no movement
static constexpr float ANIM_TOPSPEED_WALK = 100.0f;
static constexpr float ANIM_TOPSPEED_RUN = 250.0f;
static constexpr float ANIM_TOPSPEED_RUN_CROUCH = 85.0f;

// A sprinting player plays the run legs only above this speed: neo's slowest sprint is 185,
// and bots sprint through turns at 100-180
static constexpr float SPRINT_LEGS_MIN_SPEED = 100.0f;

// The feet face the eyes when they are this close (degrees)
static constexpr float FEET_FACE_EYES_TOLERANCE = 1.0f;

// Don't check if a jumping player is on the ground for this long:
// sometimes the client still has the on-ground flag set right when the jump event comes in
static constexpr float JUMP_MIN_TIME = 0.2f;

// The remote of a thrown detpack fires the remote's sequence only this long after the detpack is ready to trigger
static constexpr float DET_REMOTE_FIRE_DELAY = 0.005f;

#ifdef CLIENT_DLL
// A jump has landed when the drawn body is this close above the ground (the ground check of player movement),
static constexpr float JUMP_LAND_GROUND_DIST = 2.0f;
// or when the ground flag has been set this long and the trace still finds no ground under the drawn body
static constexpr float JUMP_LAND_MAX_FLAG_LEAD = 0.5f;
#endif

CNEOPlayerAnimState *CreateNEOPlayerAnimState( CHL2MP_Player *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();
	return new CNEOPlayerAnimState( pPlayer );
}

CNEOPlayerAnimState::CNEOPlayerAnimState( CHL2MP_Player *pPlayer )
{
	m_pPlayer = pPlayer;
	m_bJumping = false;
	m_bFirstJumpFrame = false;
	m_flJumpStartTime = 0.0f;
#ifdef CLIENT_DLL
	m_flJumpGroundFlagTime = 0.0f;
#endif
	m_iGestureSequence = 0;
	m_flGestureCycle = 0.0f;

	CModAnimConfig config;
	config.m_flMaxBodyYawDegrees = NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES;
	config.m_LegAnimType = LEGANIM_9WAY;
	config.m_bUseAimSequences = true;
	Init( pPlayer, config );
}

void CNEOPlayerAnimState::ClearAnimationState()
{
	m_bJumping = false;
	m_iGestureSequence = 0;
	BaseClass::ClearAnimationState();
}

void CNEOPlayerAnimState::ClearAnimationLayers()
{
	if ( !m_pOuter )
	{
		return;
	}

	m_pOuter->SetNumAnimOverlays( NUM_LAYERS );
	for ( int i = 0; i < m_pOuter->GetNumAnimOverlays(); i++ )
	{
		m_pOuter->GetAnimOverlay( i )->SetOrder( CBaseAnimatingOverlay::MAX_OVERLAYS );
#ifndef CLIENT_DLL
		m_pOuter->GetAnimOverlay( i )->m_fFlags = 0;
#endif
	}
}

void CNEOPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	switch ( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		StartGesture( CalcFireSequence() );
		break;

	case PLAYERANIMEVENT_RELOAD:
		StartGesture( CalcReloadSequence() );

		// a resumed reload starts the gesture where the reload resumes: nData is that cycle x 1000
		if ( nData > 0 && m_iGestureSequence > 0 )
		{
			m_flGestureCycle = nData / 1000.0f;
		}
		break;

	case PLAYERANIMEVENT_RELOAD_END:
		// the reload ended or was cut short: stop its gesture, if the layer still plays it
		if ( m_iGestureSequence > 0 && m_iGestureSequence == CalcReloadSequence() )
		{
			m_iGestureSequence = 0;
		}
		break;

	case PLAYERANIMEVENT_JUMP:
		m_bJumping = true;
		m_bFirstJumpFrame = true;
		m_flJumpStartTime = gpGlobals->curtime;
		break;

	case PLAYERANIMEVENT_SPAWN:
		ClearAnimationState();
		break;

	default:
		break;
	}
}

void CNEOPlayerAnimState::ComputeSequences( CStudioHdr *pStudioHdr )
{
	BaseClass::ComputeSequences( pStudioHdr );
	ComputeGestureSequence( pStudioHdr );
}

void CNEOPlayerAnimState::StartGesture( int iSequence )
{
	if ( iSequence <= 0 )
	{
		return;
	}

	m_iGestureSequence = iSequence;
	m_flGestureCycle = 0.0f;
	m_hGestureWeapon = m_pPlayer->GetActiveWeapon();
}

void CNEOPlayerAnimState::ComputeGestureSequence( CStudioHdr *pStudioHdr )
{
	if ( m_iGestureSequence <= 0 )
	{
		return;
	}

	// a weapon switch ends the gesture of the previous weapon
	if ( m_pPlayer->GetActiveWeapon() != m_hGestureWeapon.Get() )
	{
		m_iGestureSequence = 0;
		return;
	}

	m_flGestureCycle += m_pOuter->GetSequenceCycleRate( pStudioHdr, m_iGestureSequence ) * gpGlobals->frametime;
	if ( m_flGestureCycle >= 1.0f )
	{
		m_iGestureSequence = 0;
		return;
	}

	CAnimationLayer *pLayer = m_pOuter->GetAnimOverlay( GESTURE_LAYER );
	pLayer->m_nSequence = m_iGestureSequence;
	pLayer->m_flCycle = m_flGestureCycle;
	pLayer->m_flPlaybackRate = 1.0f;
	pLayer->m_flWeight = 1.0f;
	pLayer->m_nOrder = GESTURE_LAYER;
#ifndef CLIENT_DLL
	pLayer->m_fFlags |= ANIM_LAYER_ACTIVE;
#endif
}

Activity CNEOPlayerAnimState::CalcMainActivity()
{
	if ( HandleJumping() )
	{
		return ACT_HOP;
	}

	if ( m_pPlayer->GetWaterLevel() >= WL_Waist )
	{
		return ACT_SWIM;
	}

	const float flSpeed = GetOuterXYSpeed();
	if ( m_pPlayer->GetFlags() & FL_DUCKING )
	{
		return ( flSpeed > MOVING_MINIMUM_SPEED ) ? ACT_RUN_CROUCH : ACT_CROUCHIDLE;
	}

	if ( flSpeed <= MOVING_MINIMUM_SPEED )
	{
		return ACT_IDLE;
	}

	return ( m_pPlayer->IsSprinting() && flSpeed > SPRINT_LEGS_MIN_SPEED ) ? ACT_RUN : ACT_WALK;
}

bool CNEOPlayerAnimState::HandleJumping()
{
	if ( !m_bJumping )
	{
		return false;
	}

	if ( m_bFirstJumpFrame )
	{
		m_bFirstJumpFrame = false;
		RestartMainSequence();
#ifdef CLIENT_DLL
		m_flJumpGroundFlagTime = 0.0f;
#endif
	}

	const bool bInWater = m_pPlayer->GetWaterLevel() >= WL_Waist;
	if ( bInWater || ( gpGlobals->curtime - m_flJumpStartTime > JUMP_MIN_TIME && HasLandedFromJump() ) )
	{
		m_bJumping = false;
		RestartMainSequence();
	}

	return m_bJumping;
}

//-----------------------------------------------------------------------------
// Purpose: Whether a jumping player has landed. The client draws other players a few tenths of a second behind
//			their networked ground flag, so there the jump lasts until the drawn body reaches the ground.
//-----------------------------------------------------------------------------
bool CNEOPlayerAnimState::HasLandedFromJump()
{
	if ( !( m_pPlayer->GetFlags() & FL_ONGROUND ) )
	{
#ifdef CLIENT_DLL
		m_flJumpGroundFlagTime = 0.0f;
#endif
		return false;
	}

#ifdef CLIENT_DLL
	if ( m_flJumpGroundFlagTime <= 0.0f )
	{
		m_flJumpGroundFlagTime = gpGlobals->curtime;
	}

	if ( gpGlobals->curtime - m_flJumpGroundFlagTime > JUMP_LAND_MAX_FLAG_LEAD )
	{
		return true;
	}

	const Vector &vecOrigin = m_pPlayer->GetAbsOrigin();
	trace_t tr;
	UTIL_TraceHull( vecOrigin, vecOrigin - Vector( 0, 0, JUMP_LAND_GROUND_DIST ),
		m_pPlayer->GetPlayerMins(), m_pPlayer->GetPlayerMaxs(),
		MASK_PLAYERSOLID, m_pPlayer, COLLISION_GROUP_PLAYER_MOVEMENT, &tr );

	return tr.fraction < 1.0f || tr.startsolid;
#else
	return true;
#endif // CLIENT_DLL
}

float CNEOPlayerAnimState::GetCurrentMaxGroundSpeed()
{
	switch ( m_pOuter->GetSequenceActivity( m_pOuter->GetSequence() ) )
	{
	case ACT_IDLE:
	case ACT_WALK:
		return ANIM_TOPSPEED_WALK;
	case ACT_RUN:
		return ANIM_TOPSPEED_RUN;
	case ACT_RUN_CROUCH:
		return ANIM_TOPSPEED_RUN_CROUCH;
	default:
		return 0.0f;
	}
}

void CNEOPlayerAnimState::ComputePoseParam_BodyYaw()
{
	// a leaning player's feet turn to the eyes, as the base turns a new player's (no turn yet),
	const bool bLeaning = IsLeaning();
	if ( bLeaning )
	{
		m_flLastTurnTime = 0.0f;
	}

	BaseClass::ComputePoseParam_BodyYaw();

	// and the face-front time counts from the lean or the last frame the feet faced the eyes:
	// the base counts from the feet's last turn, so after standing still it squared the feet up on the next turn at once
	if ( bLeaning || fabsf( AngleDiff( m_flEyeYaw, m_flCurrentFeetYaw ) ) < FEET_FACE_EYES_TOLERANCE )
	{
		m_flLastTurnTime = gpGlobals->curtime;
	}
}

bool CNEOPlayerAnimState::IsLeaning() const
{
	constexpr int LEAN_BONE_CONTROLLER = 0;
	// the client reads the lean controller encoded 0-1 (0.5 is upright), the server reads it in degrees
#ifdef CLIENT_DLL
	constexpr float BONE_CONTROLLER_VALUE_NOT_LEANING = 0.5f;
#else
	constexpr float BONE_CONTROLLER_VALUE_NOT_LEANING = 0.0f;
#endif // CLIENT_DLL
	return m_pPlayer->GetBoneController( LEAN_BONE_CONTROLLER ) != BONE_CONTROLLER_VALUE_NOT_LEANING;
}

//-----------------------------------------------------------------------------
// Purpose: Whether a remote detpack shows its remote: triggered, or thrown and ready to trigger
//-----------------------------------------------------------------------------
static bool IsDetRemoteOut( CBaseCombatWeapon *pWeapon, float flReadyDelay )
{
	auto pDetpack = static_cast<CWeaponDetpack *>( pWeapon );
	return pDetpack->m_bRemoteHasBeenTriggered ||
		( pDetpack->m_bThisDetpackHasBeenThrown && gpGlobals->curtime > pDetpack->m_flNextPrimaryAttack + flReadyDelay );
}

static bool IsRemoteDet( CBaseCombatWeapon *pWeapon )
{
	return !Q_strcmp( pWeapon->GetClassname(), "weapon_remotedet" );
}

//-----------------------------------------------------------------------------
// Purpose: The weapon's animation name suffix, with the substitutions for the classes that lack the weapon's animations
//-----------------------------------------------------------------------------
const char *CNEOPlayerAnimState::GetAimSuffix() const
{
	auto pWeapon = m_pPlayer->GetActiveWeapon();
	if ( !pWeapon )
	{
		return nullptr;
	}

	const int iClass = ToNEOPlayer( m_pPlayer )->m_iNeoClass;
	if ( IsRemoteDet( pWeapon ) )
	{
		// only recon has the detpack animations, the other classes hold it like a grenade
		if ( iClass != NEO_CLASS_RECON )
		{
			return "Grenade";
		}

		return IsDetRemoteOut( pWeapon, 0.0f ) ? "detremote" : "detpack";
	}

	const char *pSuffix = pWeapon->GetWpnData().szAnimationPrefix;
	if ( iClass != NEO_CLASS_VIP && !Q_strcmp( pSuffix, "smac" ) )
	{
		return "srm";
	}

	if ( iClass == NEO_CLASS_VIP && !Q_strcmp( pSuffix, "ZR68C" ) )
	{
		return "m4";
	}

	return pSuffix;
}

int CNEOPlayerAnimState::CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle )
{
	const char *pSuffix = GetAimSuffix();
	if ( !pSuffix )
	{
		return 0;
	}

	const Activity act = GetCurrentMainSequenceActivity();
	if ( bForceIdle )
	{
		return CalcSequenceIndex( "%s%s", ( act == ACT_CROUCHIDLE ) ? AIM_CROUCH_IDLE_NAME : AIM_IDLE_NAME, pSuffix );
	}

	switch ( act )
	{
	case ACT_RUN:
		return CalcSequenceIndex( "%s%s", AIM_RUN_NAME, pSuffix );
	case ACT_WALK:
		return CalcSequenceIndex( "%s%s", AIM_WALK_NAME, pSuffix );
	case ACT_CROUCHIDLE:
		return CalcSequenceIndex( "%s%s", AIM_CROUCH_IDLE_NAME, pSuffix );
	case ACT_RUN_CROUCH:
		return CalcSequenceIndex( "%s%s", AIM_CROUCH_WALK_NAME, pSuffix );
	default:
		return CalcSequenceIndex( "%s%s", AIM_IDLE_NAME, pSuffix );
	}
}

int CNEOPlayerAnimState::CalcFireSequence()
{
	const char *pSuffix = GetAimSuffix();
	if ( !pSuffix )
	{
		return 0;
	}

	auto pWeapon = m_pPlayer->GetActiveWeapon();
	if ( IsRemoteDet( pWeapon ) )
	{
		// a class without the detpack animations throws it like a grenade and does not swing the remote
		const bool bRemote = IsDetRemoteOut( pWeapon, DET_REMOTE_FIRE_DELAY );
		if ( ToNEOPlayer( m_pPlayer )->m_iNeoClass == NEO_CLASS_RECON )
		{
			pSuffix = bRemote ? "detremote" : "detpack";
		}
		else
		{
			pSuffix = bRemote ? "SRM" : "Gren1";
		}
	}
	else if ( !Q_strcmp( pSuffix, "Grenade" ) )
	{
		pSuffix = "Gren1";
	}

	switch ( GetCurrentMainSequenceActivity() )
	{
	case ACT_RUN:
		return CalcSequenceIndex( "%s%s", FIRE_RUN_NAME, pSuffix );
	case ACT_WALK:
		return CalcSequenceIndex( "%s%s", FIRE_WALK_NAME, pSuffix );
	case ACT_CROUCHIDLE:
		return CalcSequenceIndex( "%s%s", FIRE_CROUCH_NAME, pSuffix );
	case ACT_RUN_CROUCH:
		return CalcSequenceIndex( "%s%s", FIRE_CROUCH_WALK_NAME, pSuffix );
	default:
		return CalcSequenceIndex( "%s%s", FIRE_IDLE_NAME, pSuffix );
	}
}

int CNEOPlayerAnimState::CalcReloadSequence()
{
	auto pWeapon = m_pPlayer->GetActiveWeapon();
	if ( !pWeapon )
	{
		return 0;
	}

	return CalcSequenceIndex( "Reload_%s", pWeapon->GetWpnData().szAnimationPrefix );
}
