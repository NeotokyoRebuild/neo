//========= Copyright Valve Corporation, All rights reserved. ============//
#include "cbase.h"
#include "tier0/vprof.h"
#include "animation.h"
#include "studio.h"
#include "apparent_velocity_helper.h"
#include "utldict.h"

#include "hl2mp_playeranimstate.h"
#include "base_playeranimstate.h"
#include "datacache/imdlcache.h"

#ifdef CLIENT_DLL
#include "c_hl2mp_player.h"
#ifdef NEO
#include "c_neo_player.h"
#endif
#else
#include "hl2mp_player.h"
#ifdef NEO
#include "neo_player.h"
#endif
#endif

#ifdef NEO
#include "shareddefs.h"

#define DEFAULT_IDLE_NAME "Idle_Upper_"
#define DEFAULT_CROUCH_IDLE_NAME "Crouch_Idle_Upper_"
#define DEFAULT_CROUCH_WALK_NAME "Crouch_Walk_Upper_"
#define DEFAULT_WALK_NAME "Walk_Upper_"
#define DEFAULT_RUN_NAME "Run_Upper_"

#define DEFAULT_FIRE_IDLE_NAME "Idle_Shoot_"
#define DEFAULT_FIRE_CROUCH_NAME "Crouch_Idle_Shoot_"
#define DEFAULT_FIRE_CROUCH_WALK_NAME "Crouch_Walk_Shoot_"
#define DEFAULT_FIRE_WALK_NAME "Walk_Shoot_"
#define DEFAULT_FIRE_RUN_NAME "Run_Shoot_"
#endif

#include "filesystem.h"
#include "engine/ivdebugoverlay.h"

#define HL2MP_RUN_SPEED				320.0f
#define HL2MP_WALK_SPEED			75.0f
#define HL2MP_CROUCHWALK_SPEED		110.0f

extern ConVar sv_showanimstate;
extern ConVar showanimstate_log;

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
// Output : CMultiPlayerAnimState*
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState* CreateHL2MPPlayerAnimState( CHL2MP_Player *pPlayer )
{
	MDLCACHE_CRITICAL_SECTION();

	// Setup the movement data.
	MultiPlayerMovementData_t movementData;
	movementData.m_flBodyYawRate = 720.0f;
	movementData.m_flRunSpeed = HL2MP_RUN_SPEED;
	movementData.m_flWalkSpeed = HL2MP_WALK_SPEED;
	movementData.m_flSprintSpeed = -1.0f;

	// Create animation state for this player.
	CHL2MPPlayerAnimState *pRet = new CHL2MPPlayerAnimState( pPlayer, movementData );

	// Specific HL2MP player initialization.
	pRet->InitHL2MPAnimState( pPlayer );

	return pRet;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState::CHL2MPPlayerAnimState()
{
	m_pHL2MPPlayer = NULL;

	// Don't initialize HL2MP specific variables here. Init them in InitHL2MPAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pPlayer - 
//			&movementData - 
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState::CHL2MPPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData )
: CMultiPlayerAnimState( pPlayer, movementData )
{
	m_pHL2MPPlayer = NULL;

	// Don't initialize HL2MP specific variables here. Init them in InitHL2MPAnimState()
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  :  - 
//-----------------------------------------------------------------------------
CHL2MPPlayerAnimState::~CHL2MPPlayerAnimState()
{
}

//-----------------------------------------------------------------------------
// Purpose: Initialize HL2MP specific animation state.
// Input  : *pPlayer - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::InitHL2MPAnimState( CHL2MP_Player *pPlayer )
{
	m_pHL2MPPlayer = pPlayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ClearAnimationState( void )
{
	BaseClass::ClearAnimationState();
}

int CHL2MPPlayerAnimState::CalcSequenceIndex(const char* pBaseName, ...)
{
	char szFullName[512];
	va_list marker;
	va_start(marker, pBaseName);
	Q_vsnprintf(szFullName, sizeof(szFullName), pBaseName, marker);
	va_end(marker);
	CBaseAnimatingOverlay* pPlayer = GetBasePlayer();
	int iSequence = pPlayer->LookupSequence(szFullName);

	// Show warnings if we can't find anything here.
	if (iSequence == -1)
	{
		static CUtlDict<int, int> dict;
		if (dict.Find(szFullName) == -1)
		{
			dict.Insert(szFullName, 0);
			Warning("CalcSequenceIndex: can't find '%s'.\n", szFullName);
		}

		iSequence = 0;
	}

	//DevMsg("CalcSeqIdx: \"%s\": %d\n", szFullName, iSequence);

	return iSequence;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : actDesired - 
// Output : Activity
//-----------------------------------------------------------------------------
Activity CHL2MPPlayerAnimState::TranslateActivity( Activity actDesired )
{
	// Hook into baseclass when / if hl2mp player models get swim animations.
	Activity translateActivity = actDesired; //BaseClass::TranslateActivity( actDesired );
	if ( GetHL2MPPlayer()->GetActiveWeapon() )
	{
		bool required = false;
		translateActivity = GetHL2MPPlayer()->GetActiveWeapon()->ActivityOverride( translateActivity, &required);
	}
	return translateActivity;
}

#ifdef NEO
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pStudioHdr - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputeSequences(CStudioHdr* pStudioHdr)
{
	VPROF("CBasePlayerAnimState::ComputeSequences");

	// Lower body (walk/run/idle).
	ComputeMainSequence();

	// The groundspeed interpolator uses the main sequence info.
	UpdateInterpolators();
	ComputeGestureSequence(pStudioHdr);
}

#endif //NEO
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::Update( float eyeYaw, float eyePitch )
{
	// Profile the animation update.
	VPROF( "CHL2MPPlayerAnimState::Update" );

	// Get the HL2MP player.
	CHL2MP_Player *pHL2MPPlayer = GetHL2MPPlayer();
	if ( !pHL2MPPlayer )
		return;

	// Get the studio header for the player.
	CStudioHdr *pStudioHdr = pHL2MPPlayer->GetModelPtr();
	if ( !pStudioHdr )
		return;

	// Check to see if we should be updating the animation state - dead, ragdolled?
	if ( !ShouldUpdateAnimState() )
	{
		ClearAnimationState();
		return;
	}

	// Store the eye angles.
	m_flEyeYaw = AngleNormalize( eyeYaw );
	m_flEyePitch = AngleNormalize( eyePitch );

	// Compute the player sequences.
	ComputeSequences( pStudioHdr );

	if ( SetupPoseParameters( pStudioHdr ) )
	{
		// Pose parameter - what direction are the player's legs running in.
		ComputePoseParam_MoveYaw( pStudioHdr );

		// Pose parameter - Torso aiming (up/down).
		ComputePoseParam_AimPitch( pStudioHdr );

		// Pose parameter - Torso aiming (rotation).
		ComputePoseParam_AimYaw( pStudioHdr );
	}

#ifdef CLIENT_DLL 
	if ( C_BasePlayer::ShouldDrawLocalPlayer() )
	{
		m_pHL2MPPlayer->SetPlaybackRate( 1.0f );
	}
#endif

#ifdef CLIENT_DLL
	if (cl_showanimstate.GetInt() == m_pHL2MPPlayer->entindex())
	{
		DebugShowAnimStateFull(5);
	}
	else if (cl_showanimstate.GetInt() == -2)
	{
		C_BasePlayer* targetPlayer = C_BasePlayer::GetLocalPlayer();

		if (targetPlayer && (targetPlayer->GetObserverMode() == OBS_MODE_IN_EYE || targetPlayer->GetObserverMode() == OBS_MODE_CHASE))
		{
			C_BaseEntity* target = targetPlayer->GetObserverTarget();

			if (target && target->IsPlayer())
			{
				targetPlayer = ToBasePlayer(target);
			}
		}

		if (m_pHL2MPPlayer == targetPlayer)
		{
			DebugShowAnimStateFull(6);
		}
	}
#else
	if (sv_showanimstate.GetInt() == m_pHL2MPPlayer->entindex())
	{
		DebugShowAnimState(20);
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : event - 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::DoAnimationEvent( PlayerAnimEvent_t event, int nData )
{
	Activity iGestureActivity = ACT_INVALID;

	switch( event )
	{
	case PLAYERANIMEVENT_ATTACK_PRIMARY:
		{
#ifdef NEO
		AddToGestureSlot(GESTURE_SLOT_ATTACK_AND_RELOAD, CalcFireSequence(), true);
#else
			// Weapon primary fire.
			if ( m_pHL2MPPlayer->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_PRIMARYFIRE );

			iGestureActivity = ACT_VM_PRIMARYATTACK;
#endif
			break;
		}

	case PLAYERANIMEVENT_VOICE_COMMAND_GESTURE:
		{
			if ( !IsGestureSlotActive( GESTURE_SLOT_ATTACK_AND_RELOAD ) )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, (Activity)nData );

			break;
		}
	case PLAYERANIMEVENT_ATTACK_SECONDARY:
		{
#ifdef NEO
		AddToGestureSlot(GESTURE_SLOT_ATTACK_AND_RELOAD, CalcFireSequence(), true);
#else
			// Weapon secondary fire.
			if ( m_pHL2MPPlayer->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_SECONDARYFIRE );

			iGestureActivity = ACT_VM_PRIMARYATTACK;
#endif
			break;
		}
	case PLAYERANIMEVENT_ATTACK_PRE:
		{
			if ( m_pHL2MPPlayer->GetFlags() & FL_DUCKING ) 
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc in crouch.
				iGestureActivity = ACT_MP_ATTACK_CROUCH_PREFIRE;
			}
			else
			{
				// Weapon pre-fire. Used for minigun windup, sniper aiming start, etc.
				iGestureActivity = ACT_MP_ATTACK_STAND_PREFIRE;
			}

			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, iGestureActivity, false );

			break;
		}
	case PLAYERANIMEVENT_ATTACK_POST:
		{
			RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_ATTACK_STAND_POSTFIRE );
			break;
		}

	case PLAYERANIMEVENT_RELOAD:
		{
#ifdef NEO
		AddToGestureSlot(GESTURE_SLOT_ATTACK_AND_RELOAD, CalcReloadSequence(), true);
#else
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND );
#endif
			break;
		}
	case PLAYERANIMEVENT_RELOAD_LOOP:
		{
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_LOOP );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_LOOP );
			break;
		}
	case PLAYERANIMEVENT_RELOAD_END:
		{
			// Weapon reload.
			if ( GetBasePlayer()->GetFlags() & FL_DUCKING )
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_CROUCH_END );
			else
				RestartGesture( GESTURE_SLOT_ATTACK_AND_RELOAD, ACT_MP_RELOAD_STAND_END );
			break;
		}
	default:
		{
			BaseClass::DoAnimationEvent( event, nData );
			break;
		}
	}

#ifdef CLIENT_DLL
	// Make the weapon play the animation as well
	if ( iGestureActivity != ACT_INVALID )
	{
		CBaseCombatWeapon *pWeapon = GetHL2MPPlayer()->GetActiveWeapon();
		if ( pWeapon )
		{
//			pWeapon->EnsureCorrectRenderingModel();
			pWeapon->SendWeaponAnim( iGestureActivity );
//			// Force animation events!
//			pWeapon->ResetEventsParity();		// reset event parity so the animation events will occur on the weapon. 
			pWeapon->DoAnimationEvents( pWeapon->GetModelPtr() );
		}
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleSwimming( Activity &idealActivity )
{
	bool bInWater = BaseClass::HandleSwimming( idealActivity );

	return bInWater;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleMoving( Activity &idealActivity )
{
#ifdef NEO
	if (m_pHL2MPPlayer->IsSprinting())
	{
		idealActivity = ACT_RUN;
	}
	else
	{
		Vector vecVelocity;
		GetOuterAbsVelocity(vecVelocity);
		if (vecVelocity.Length() > 0.5)
		{
			idealActivity = ACT_WALK;
		}
	}

	return true;
#else
	return BaseClass::HandleMoving( idealActivity );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *idealActivity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHL2MPPlayerAnimState::HandleDucking( Activity &idealActivity )
{
	if ( m_pHL2MPPlayer->GetFlags() & FL_DUCKING )
	{
		if ( GetOuterXYSpeed() < MOVING_MINIMUM_SPEED )
		{
#ifdef NEO
			idealActivity = ACT_CROUCHIDLE;
#else
			idealActivity = ACT_MP_CROUCH_IDLE;
#endif
		}
		else
		{
#ifdef NEO
			idealActivity = ACT_RUN_CROUCH;
#else
			idealActivity = ACT_MP_CROUCHWALK;
#endif
		}

		return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose: 
bool CHL2MPPlayerAnimState::HandleJumping( Activity &idealActivity )
{
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	if ( m_bJumping )
	{
		static bool bNewJump = false; //Tony; hl2mp players only have a 'hop'

		if ( m_bFirstJumpFrame )
		{
			m_bFirstJumpFrame = false;
			RestartMainSequence();	// Reset the animation.
		}

		// Reset if we hit water and start swimming.
		if ( m_pHL2MPPlayer->GetWaterLevel() >= WL_Waist )
		{
			m_bJumping = false;
			RestartMainSequence();
		}
		// Don't check if he's on the ground for a sec.. sometimes the client still has the
		// on-ground flag set right when the message comes in.
		else if ( gpGlobals->curtime - m_flJumpStartTime > 0.2f )
		{
			if ( m_pHL2MPPlayer->GetFlags() & FL_ONGROUND )
			{
				m_bJumping = false;
				RestartMainSequence();

				if ( bNewJump )
				{
#ifdef NEO
					RestartGesture(GESTURE_SLOT_JUMP, ACT_HOP);
#else
					RestartGesture( GESTURE_SLOT_JUMP, ACT_MP_JUMP_LAND );					
#endif
				}
			}
		}

		// if we're still jumping
		if ( m_bJumping )
		{
#ifdef NEO
			idealActivity = ACT_HOP;
#else
			if ( bNewJump )
			{
				if ( gpGlobals->curtime - m_flJumpStartTime > 0.5 )
				{
					idealActivity = ACT_MP_JUMP_FLOAT;
				}
				else
				{
					idealActivity = ACT_MP_JUMP_START;
				}
			}
			else
			{
				idealActivity = ACT_MP_JUMP;
			}
#endif
		}
	}	

	if ( m_bJumping )
		return true;

	return false;
}

bool CHL2MPPlayerAnimState::SetupPoseParameters( CStudioHdr *pStudioHdr )
{
#ifdef NEO
	BaseClass::SetupPoseParameters(pStudioHdr);
#endif
	// Check to see if this has already been done.
	if ( m_bPoseParameterInit )
		return true;

	// Save off the pose parameter indices.
	if ( !pStudioHdr )
		return false;

	// Tony; just set them both to the same for now.
	m_PoseParameterData.m_iMoveX = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_yaw" );
	m_PoseParameterData.m_iMoveY = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "move_yaw" );
	if ( ( m_PoseParameterData.m_iMoveX < 0 ) || ( m_PoseParameterData.m_iMoveY < 0 ) )
		return false;

	// Look for the aim pitch blender.
	m_PoseParameterData.m_iAimPitch = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_pitch" );
	if ( m_PoseParameterData.m_iAimPitch < 0 )
		return false;

	// Look for aim yaw blender.
	m_PoseParameterData.m_iAimYaw = GetBasePlayer()->LookupPoseParameter( pStudioHdr, "body_yaw" );
	if ( m_PoseParameterData.m_iAimYaw < 0 )
		return false;

	m_bPoseParameterInit = true;

	return true;
}
float SnapYawTo( float flValue );
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::EstimateYaw( void )
{
	// Get the frame time.
	float flDeltaTime = gpGlobals->frametime;
	if ( flDeltaTime == 0.0f )
		return;

#if 0 // 9way
	// Get the player's velocity and angles.
	Vector vecEstVelocity;
	GetOuterAbsVelocity( vecEstVelocity );
	QAngle angles = GetBasePlayer()->GetLocalAngles();

	// If we are not moving, sync up the feet and eyes slowly.
	if ( vecEstVelocity.x == 0.0f && vecEstVelocity.y == 0.0f )
	{
		float flYawDelta = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
		flYawDelta = AngleNormalize( flYawDelta );

		if ( flDeltaTime < 0.25f )
		{
			flYawDelta *= ( flDeltaTime * 4.0f );
		}
		else
		{
			flYawDelta *= flDeltaTime;
		}

		m_PoseParameterData.m_flEstimateYaw += flYawDelta;
		AngleNormalize( m_PoseParameterData.m_flEstimateYaw );
	}
	else
	{
		m_PoseParameterData.m_flEstimateYaw = ( atan2( vecEstVelocity.y, vecEstVelocity.x ) * 180.0f / M_PI );
		m_PoseParameterData.m_flEstimateYaw = clamp( m_PoseParameterData.m_flEstimateYaw, -180.0f, 180.0f );
	}
#else
	float dt = gpGlobals->frametime;

	// Get the player's velocity and angles.
	Vector vecEstVelocity;
	GetOuterAbsVelocity( vecEstVelocity );
	QAngle angles = GetBasePlayer()->GetLocalAngles();

	if ( vecEstVelocity.y == 0 && vecEstVelocity.x == 0 )
	{
		float flYawDiff = angles[YAW] - m_PoseParameterData.m_flEstimateYaw;
		flYawDiff = flYawDiff - (int)(flYawDiff / 360) * 360;
		if (flYawDiff > 180)
			flYawDiff -= 360;
		if (flYawDiff < -180)
			flYawDiff += 360;

		if (dt < 0.25)
			flYawDiff *= dt * 4;
		else
			flYawDiff *= dt;

		m_PoseParameterData.m_flEstimateYaw += flYawDiff;
		m_PoseParameterData.m_flEstimateYaw = m_PoseParameterData.m_flEstimateYaw - (int)(m_PoseParameterData.m_flEstimateYaw / 360) * 360;
	}
	else
	{
		m_PoseParameterData.m_flEstimateYaw = (atan2(vecEstVelocity.y, vecEstVelocity.x) * 180 / M_PI);

		if (m_PoseParameterData.m_flEstimateYaw > 180)
			m_PoseParameterData.m_flEstimateYaw = 180;
		else if (m_PoseParameterData.m_flEstimateYaw < -180)
			m_PoseParameterData.m_flEstimateYaw = -180;
	}
#endif
}
//-----------------------------------------------------------------------------
// Purpose: Override for backpeddling
// Input  : dt - 
//-----------------------------------------------------------------------------
float SnapYawTo( float flValue );
void CHL2MPPlayerAnimState::ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr )
{
#ifdef NEO
	BaseClass::ComputePoseParam_MoveYaw(pStudioHdr);
#endif
	// Get the estimated movement yaw.
	EstimateYaw();

#if 1 // 9way
	ConVarRef mp_slammoveyaw("mp_slammoveyaw");

	// Get the view yaw.
	float flAngle = AngleNormalize( m_flEyeYaw );

	// Calc side to side turning - the view vs. movement yaw.
	float flYaw = flAngle - m_PoseParameterData.m_flEstimateYaw;
	flYaw = AngleNormalize( -flYaw );

	// Get the current speed the character is running.
	bool bIsMoving;
	float flPlaybackRate = 	CalcMovementPlaybackRate( &bIsMoving );

	// Setup the 9-way blend parameters based on our speed and direction.
	Vector2D vecCurrentMoveYaw( 0.0f, 0.0f );
	if ( bIsMoving )
	{
		if ( mp_slammoveyaw.GetBool() )
			flYaw = SnapYawTo( flYaw );

		vecCurrentMoveYaw.x = cos( DEG2RAD( flYaw ) ) * flPlaybackRate;
		vecCurrentMoveYaw.y = -sin( DEG2RAD( flYaw ) ) * flPlaybackRate;
	}

	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, vecCurrentMoveYaw.x );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, vecCurrentMoveYaw.y );

	m_DebugAnimData.m_vecMoveYaw = vecCurrentMoveYaw;
#else
	// view direction relative to movement
	float flYaw;	 

	QAngle	angles = GetBasePlayer()->GetLocalAngles();
	float ang = angles[ YAW ];
	if ( ang > 180.0f )
	{
		ang -= 360.0f;
	}
	else if ( ang < -180.0f )
	{
		ang += 360.0f;
	}

	// calc side to side turning
	flYaw = ang - m_PoseParameterData.m_flEstimateYaw;
	// Invert for mapping into 8way blend
	flYaw = -flYaw;
	flYaw = flYaw - (int)(flYaw / 360) * 360;

	if (flYaw < -180)
	{
		flYaw = flYaw + 360;
	}
	else if (flYaw > 180)
	{
		flYaw = flYaw - 360;
	}

	//Tony; oops, i inverted this previously above.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, flYaw );

#endif
	
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr )
{
	// Get the view pitch.
	float flAimPitch = m_flEyePitch;

	// Set the aim pitch pose parameter and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimPitch, flAimPitch );
	m_DebugAnimData.m_flAimPitch = flAimPitch;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr )
{
	// Get the movement velocity.
	Vector vecVelocity;
	GetOuterAbsVelocity( vecVelocity );

	// Check to see if we are moving.
	bool bMoving = ( vecVelocity.Length() > 1.0f ) ? true : false;

	// If we are moving or are prone and undeployed.
	if ( bMoving || m_bForceAimYaw )
	{
		// The feet match the eye direction when moving - the move yaw takes care of the rest.
		m_flGoalFeetYaw = m_flEyeYaw;
	}
	// Else if we are not moving.
	else
	{
		// Initialize the feet.
		if ( m_PoseParameterData.m_flLastAimTurnTime <= 0.0f )
		{
			m_flGoalFeetYaw	= m_flEyeYaw;
			m_flCurrentFeetYaw = m_flEyeYaw;
			m_PoseParameterData.m_flLastAimTurnTime = gpGlobals->curtime;
		}
		// Make sure the feet yaw isn't too far out of sync with the eye yaw.
		// TODO: Do something better here!
		else
		{
			float flYawDelta = AngleNormalize(  m_flGoalFeetYaw - m_flEyeYaw );

#ifdef NEO
			if (fabs(flYawDelta) > 90.f /*NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES*/)
#else
			if ( fabs( flYawDelta ) > 45.0f )
#endif
			{
				float flSide = ( flYawDelta > 0.0f ) ? -1.0f : 1.0f;

#ifdef NEO
				m_flGoalFeetYaw += (90.f/*NEO_ANIMSTATE_MAX_BODY_YAW_DEGREES*/ * flSide);
#else
				m_flGoalFeetYaw += ( 45.0f * flSide );
#endif
			}
		}
	}

	// Fix up the feet yaw.
	m_flGoalFeetYaw = AngleNormalize( m_flGoalFeetYaw );
	if ( m_flGoalFeetYaw != m_flCurrentFeetYaw )
	{
		if ( m_bForceAimYaw )
		{
			m_flCurrentFeetYaw = m_flGoalFeetYaw;
		}
		else
		{
			ConvergeYawAngles( m_flGoalFeetYaw, 720.0f, gpGlobals->frametime, m_flCurrentFeetYaw );
			m_flLastAimTurnTime = gpGlobals->curtime;
		}
	}

	// Rotate the body into position.
	m_angRender[YAW] = m_flCurrentFeetYaw;

	// Find the aim(torso) yaw base on the eye and feet yaws.
	float flAimYaw = m_flEyeYaw - m_flCurrentFeetYaw;
	flAimYaw = AngleNormalize( flAimYaw );

	// Set the aim yaw and save.
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iAimYaw, flAimYaw );
	m_DebugAnimData.m_flAimYaw	= flAimYaw;

	// Turn off a force aim yaw - either we have already updated or we don't need to.
	m_bForceAimYaw = false;

#ifndef CLIENT_DLL
	QAngle angle = GetBasePlayer()->GetAbsAngles();
	angle[YAW] = m_flCurrentFeetYaw;

	GetBasePlayer()->SetAbsAngles( angle );
#endif
}

//-----------------------------------------------------------------------------
// Purpose: Override the default, because hl2mp models don't use moveX
// Input  :  - 
// Output : float
//-----------------------------------------------------------------------------
float CHL2MPPlayerAnimState::GetCurrentMaxGroundSpeed()
{
#ifdef NEO
	return BaseClass::GetCurrentMaxGroundSpeed();
#endif
	CStudioHdr *pStudioHdr = GetBasePlayer()->GetModelPtr();

	if ( pStudioHdr == NULL )
		return 1.0f;

//	float prevX = GetBasePlayer()->GetPoseParameter( m_PoseParameterData.m_iMoveX );
	float prevY = GetBasePlayer()->GetPoseParameter( m_PoseParameterData.m_iMoveY );

	float d = sqrt( /*prevX * prevX + */prevY * prevY );
	float newY;//, newX;
	if ( d == 0.0 )
	{ 
//		newX = 1.0;
		newY = 0.0;
	}
	else
	{
//		newX = prevX / d;
		newY = prevY / d;
	}

//	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, newX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, newY );

	float speed = GetBasePlayer()->GetSequenceGroundSpeed( GetBasePlayer()->GetSequence() );

//	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveX, prevX );
	GetBasePlayer()->SetPoseParameter( pStudioHdr, m_PoseParameterData.m_iMoveY, prevY );

	return speed;
}

// -----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::AnimStateLog(const char* pMsg, ...)
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start(marker, pMsg);
	Q_vsnprintf(str, sizeof(str), pMsg, marker);
	va_end(marker);

	// Log it?	
	if (showanimstate_log.GetInt() == 1 || showanimstate_log.GetInt() == 3)
	{
		Msg("%s", str);
	}

	if (showanimstate_log.GetInt() > 1)
	{
#ifdef CLIENT_DLL
		const char* fname = "AnimStateClient.log";
#else
		const char* fname = "AnimStateServer.log";
#endif
		static FileHandle_t hFile = filesystem->Open(fname, "wt");
		filesystem->FPrintf(hFile, "%s", str);
		filesystem->Flush(hFile);
	}
}

// -----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::AnimStatePrintf(int iLine, const char* pMsg, ...)
{
	// Format the string.
	char str[4096];
	va_list marker;
	va_start(marker, pMsg);
	Q_vsnprintf(str, sizeof(str), pMsg, marker);
	va_end(marker);

	// Show it with Con_NPrintf.
	engine->Con_NPrintf(iLine, "%s", str);

	// Log it.
	AnimStateLog("%s\n", str);
}


// -----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::DebugShowAnimState(int iStartLine)
{
	Vector vOuterVel;
	GetOuterAbsVelocity(vOuterVel);

	int iLine = iStartLine;
	AnimStatePrintf(iLine++, "main: %s(%d), cycle: %.2f cyclerate: %.2f playbackrate: %.2f\n",
		GetSequenceName(m_pHL2MPPlayer->GetModelPtr(), m_pHL2MPPlayer->GetSequence()),
		m_pHL2MPPlayer->GetSequence(),
		m_pHL2MPPlayer->GetCycle(),
		m_pHL2MPPlayer->GetSequenceCycleRate(m_pHL2MPPlayer->GetModelPtr(), m_pHL2MPPlayer->GetSequence()),
		m_pHL2MPPlayer->GetPlaybackRate()
	);

	for (int i = 0; i < m_pHL2MPPlayer->GetNumAnimOverlays() - 1; i++)
	{
		CAnimationLayer* pLayer = m_pHL2MPPlayer->GetAnimOverlay(AIMSEQUENCE_LAYER + i);
#ifdef CLIENT_DLL
		AnimStatePrintf(iLine++, "%s(%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)",
			!pLayer->IsActive() ? "-- " : (pLayer->m_nSequence == 0 ? "-- " : GetSequenceName(m_pHL2MPPlayer->GetModelPtr(), pLayer->m_nSequence)),
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nSequence,
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flWeight,
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flCycle,
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nOrder,
			i
		);
#else
		AnimStatePrintf(iLine++, "%s(%d), flags (%d), weight: %.2f, cycle: %.2f, order (%d), aim (%d)",
			!pLayer->IsActive() ? "-- " : (pLayer->m_nSequence == 0 ? "-- " : GetSequenceName(m_pHL2MPPlayer->GetModelPtr(), pLayer->m_nSequence)),
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nSequence,
			!pLayer->IsActive() ? 0 : (int)pLayer->m_fFlags,// Doesn't exist on client
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flWeight,
			!pLayer->IsActive() ? 0 : (float)pLayer->m_flCycle,
			!pLayer->IsActive() ? 0 : (int)pLayer->m_nOrder,
			i
		);
#endif
	}

	AnimStatePrintf(iLine++, "vel: %.2f, time: %.2f, max: %.2f, animspeed: %.2f",
		vOuterVel.Length2D(), gpGlobals->curtime, GetInterpolatedGroundSpeed(), m_pHL2MPPlayer->GetSequenceGroundSpeed(m_pHL2MPPlayer->GetSequence()));

	{
		AnimStatePrintf(iLine++, "ent yaw: %.2f, body_yaw: %.2f, body_pitch: %.2f, move_x: %.2f, move_y: %.2f",
			m_angRender[YAW], g_flLastBodyYaw, g_flLastBodyPitch, m_DebugAnimData.m_vecMoveYaw.x, m_DebugAnimData.m_vecMoveYaw.y);
	}

	// Draw a red triangle on the ground for the eye yaw.
	float flBaseSize = 10;
	float flHeight = 80;
	Vector vBasePos = m_pHL2MPPlayer->GetAbsOrigin() + Vector(0, 0, 3);
	QAngle angles(0, 0, 0);
	angles[YAW] = m_flEyeYaw;
	Vector vForward, vRight, vUp;
	AngleVectors(angles, &vForward, &vRight, &vUp);
	debugoverlay->AddTriangleOverlay(vBasePos + vRight * flBaseSize / 2, vBasePos - vRight * flBaseSize / 2, vBasePos + vForward * flHeight, 255, 0, 0, 255, false, 0.01);

	// Draw a blue triangle on the ground for the body yaw.
	angles[YAW] = m_angRender[YAW];
	AngleVectors(angles, &vForward, &vRight, &vUp);
	debugoverlay->AddTriangleOverlay(vBasePos + vRight * flBaseSize / 2, vBasePos - vRight * flBaseSize / 2, vBasePos + vForward * flHeight, 0, 0, 255, 255, false, 0.01);

}

// -----------------------------------------------------------------------------
void CHL2MPPlayerAnimState::DebugShowAnimStateFull(int iStartLine)
{
	AnimStateLog("----------------- frame %d -----------------\n", gpGlobals->framecount);

	DebugShowAnimState(iStartLine);

	AnimStateLog("--------------------------------------------\n\n");
}
