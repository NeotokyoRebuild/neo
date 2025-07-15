#include "cbase.h"

#include "neo_bot.h"
#include "neo_bot_body.h"


// 
// Return how often we should sample our target's position and 
// velocity to update our aim tracking, to allow realistic slop in tracking
//
float CNEOBotBody::GetHeadAimTrackingInterval( void ) const
{
	CNEOBot *me = (CNEOBot *)GetBot();

	switch( me->GetDifficulty() )
	{
	case CNEOBot::EXPERT:
		return 0.05f;

	case CNEOBot::HARD:
		return 0.1f;

	case CNEOBot::NORMAL:
		return 0.25f;

	case CNEOBot::EASY:
		return 1.0f;
	}

	return 0.0f;
}

float CNEOBotBody::GetCloakPower( void ) const
{
	auto me = static_cast<CNEO_Player*>(GetBot()->GetEntity());
	return me ? me->CloakPower_Get() : 0.0f;
}

bool CNEOBotBody::IsCloakEnabled( void ) const
{
	auto me = static_cast<CNEO_Player*>(GetBot()->GetEntity());
	return me ? me->GetCloakState() : false;
}
