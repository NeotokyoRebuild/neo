#pragma once

#include "NextBot/Player/NextBotPlayerLocomotion.h"
#include "neo_player_shared.h"
#include "../neo_player.h"

//----------------------------------------------------------------------------
class CNEOBotLocomotion : public PlayerLocomotion
{
public:
	DECLARE_CLASS( CNEOBotLocomotion, PlayerLocomotion );

	CNEOBotLocomotion( INextBot *bot ) : PlayerLocomotion( bot )
	{
	}

	virtual ~CNEOBotLocomotion() { }

	virtual void Update( void );								// (EXTEND) update internal state

	virtual void Approach( const Vector &pos, float goalWeight = 1.0f );	// move directly towards the given position

	virtual float GetMaxJumpHeight( void ) const;				// return maximum height of a jump
	virtual float GetDeathDropHeight( void ) const;			// distance at which we will die if we fall

	virtual float GetRunSpeed( void ) const override;			// get maximum running speed
	virtual float GetWalkSpeed( void ) const override;			// get maximum walking speed
	virtual bool IsRunning( void ) const override;

	virtual bool IsAreaTraversable( const CNavArea *baseArea ) const;	// return true if given area can be used for navigation
	virtual bool IsEntityTraversable( CBaseEntity *obstacle, TraverseWhenType when = EVENTUALLY ) const;

	mutable bool m_bBreakBreakableInPath = false;

protected:
	virtual void AdjustPosture( const Vector &moveGoal ) { }	// never crouch to navigate
};

inline float CNEOBotLocomotion::GetMaxJumpHeight( void ) const
{
	// NEO JANK: Assumes [MD]'s g_bMovementOptimizations = true, where we assume sv_gravity is 800 for navigation.
	// Changing that setting can potentially break bot navigation.
	extern ConVar sv_gravity;
	Assert( sv_gravity.GetFloat() == 800.0f );

	auto me = (CNEO_Player*)GetBot()->GetEntity();
	float theoreticalJumpHeight = 0.0f;

	switch (me->GetClass())
	{
		case NEO_CLASS_RECON:
			theoreticalJumpHeight = NEO_RECON_CROUCH_JUMP_HEIGHT;
			break;
		case NEO_CLASS_JUGGERNAUT:
			theoreticalJumpHeight = NEO_JUGGERNAUT_CROUCH_JUMP_HEIGHT;
			break;
		case NEO_CLASS_SUPPORT:
			theoreticalJumpHeight = NEO_SUPPORT_CROUCH_JUMP_HEIGHT;
			break;
		case NEO_CLASS_VIP:
			theoreticalJumpHeight = NEO_ASSAULT_CROUCH_JUMP_HEIGHT; // vip same as assault
			break;
		case NEO_CLASS_ASSAULT:
			theoreticalJumpHeight = NEO_ASSAULT_CROUCH_JUMP_HEIGHT;
			break;
		default:
			Assert(false);
			return 0.f;
	}

	return theoreticalJumpHeight - NEO_BOT_JUMP_HEIGHT_BUFFER;
}
