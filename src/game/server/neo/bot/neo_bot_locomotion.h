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

protected:
	virtual void AdjustPosture( const Vector &moveGoal );	// duck when only a crouched hull fits ahead

private:
	bool IsOnlyCrouchClearAhead( const Vector &moveGoal ) const;
};

inline float CNEOBotLocomotion::GetMaxJumpHeight( void ) const
{
	// NEO JANK: Assumes [MD]'s g_bMovementOptimizations = true, where we assume sv_gravity is 800 for navigation.
	// Changing that setting can potentially break bot navigation.
	extern ConVar sv_gravity;
	Assert( sv_gravity.GetFloat() == 800.0f );

	auto me = (CNEO_Player*)GetBot()->GetEntity();
	float crouchJumpHeight = 0.0f;
	float buffer = NEO_BOT_JUMP_HEIGHT_BUFFER;

	switch (me->GetClass())
	{
		case NEO_CLASS_RECON:
			crouchJumpHeight = NEO_RECON_CROUCH_JUMP_HEIGHT;
			buffer = NEO_BOT_RECON_JUMP_HEIGHT_BUFFER;
			break;
		case NEO_CLASS_JUGGERNAUT:
			crouchJumpHeight = NEO_JUGGERNAUT_CROUCH_JUMP_HEIGHT;
			break;
		case NEO_CLASS_SUPPORT:
			crouchJumpHeight = NEO_SUPPORT_CROUCH_JUMP_HEIGHT;
			buffer = NEO_BOT_SUPPORT_JUMP_HEIGHT_BUFFER;
			break;
		case NEO_CLASS_VIP: // vip moves like assault
		case NEO_CLASS_ASSAULT:
			crouchJumpHeight = NEO_ASSAULT_CROUCH_JUMP_HEIGHT;
			buffer = NEO_BOT_ASSAULT_JUMP_HEIGHT_BUFFER;
			break;
		default:
			Assert(false);
			return 0.f;
	}

	return crouchJumpHeight - buffer;
}
