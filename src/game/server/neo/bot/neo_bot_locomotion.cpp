#include "cbase.h"

#include "neo_bot.h"
#include "neo_bot_locomotion.h"
#include "particle_parse.h"

extern ConVar falldamage;

//-----------------------------------------------------------------------------------------
void CNEOBotLocomotion::Update( void )
{
	CNEOBot* me = ToNEOBot( GetBot()->GetEntity() );
	if ( !me || me->GetNeoFlags() & NEO_FL_FREEZETIME)
	{
		return;
	}

	BaseClass::Update();

	// always 'crouch jump'
	if ( IsOnGround() )
	{
		// NEO JANK resetting of crouch timer moved to NextBotPlayer::PressJumpButton
		// so far crouch jump seems to still be working, but watch out for a regression
		/* disabled:
		me->ReleaseCrouchButton();
		*/
	}
	else
	{
		me->PressCrouchButton( 0.3f );
	}
}


//-----------------------------------------------------------------------------------------
// Move directly towards the given position
void CNEOBotLocomotion::Approach( const Vector& pos, float goalWeight )
{
	BaseClass::Approach( pos, goalWeight );
}


//-----------------------------------------------------------------------------------------
// Distance at which we will die if we fall
float CNEOBotLocomotion::GetDeathDropHeight( void ) const
{
	CNEOBot* me = ( CNEOBot* )GetBot()->GetEntity();

	// misyl: Fall damage only deals 10 health otherwise. 
	if ( falldamage.GetInt() != 1 )
	{
		if ( me->GetHealth() > 10.0f )
			return MAX_COORD_FLOAT;

		// #define PLAYER_MAX_SAFE_FALL_SPEED	526.5f // approx 20 feet sqrt( 2 * gravity * 20 * 12 )
		return 240.0f;
	}
	else
	{
		return 1000.0f;
	}
}


//-----------------------------------------------------------------------------------------
// Get maximum running speed
float CNEOBotLocomotion::GetRunSpeed( void ) const
{
	return hl2_normspeed.GetFloat();
	// TODO(misyl): Teach bots to sprint.
	//return hl2_sprintspeed.GetFloat();
}


//-----------------------------------------------------------------------------------------
// Return true if given area can be used for navigation
bool CNEOBotLocomotion::IsAreaTraversable( const CNavArea* area ) const
{
	CNEOBot* me = ( CNEOBot* )GetBot()->GetEntity();

	if ( area->IsBlocked( me->GetTeamNumber() ) )
	{
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------------------
bool CNEOBotLocomotion::IsEntityTraversable( CBaseEntity* obstacle, TraverseWhenType when ) const
{
	// assume all players are "traversable" in that they will move or can be killed
	if ( obstacle && obstacle->IsPlayer() )
	{
		return true;
	}

	if ( obstacle )
	{
		// misyl:
		// override the base brush logic here to work better for hl2mp (w/ solid flags)
		// changing this for TF would be scary.
		//
		// if we hit a clip brush, ignore it if it is not BRUSHSOLID_ALWAYS
		if ( FClassnameIs( obstacle, "func_brush" ) )
		{
			CFuncBrush* brush = ( CFuncBrush* )obstacle;

			switch ( brush->m_iSolidity )
			{
			case CFuncBrush::BRUSHSOLID_ALWAYS:
				return false;
			case CFuncBrush::BRUSHSOLID_NEVER:
				return true;
			case CFuncBrush::BRUSHSOLID_TOGGLE:
				return brush->GetSolidFlags() & FSOLID_NOT_SOLID;
			}
		}
	}

	return PlayerLocomotion::IsEntityTraversable( obstacle, when );
}
