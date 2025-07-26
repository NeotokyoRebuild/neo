#pragma once

#include "NextBot/Player/NextBotPlayerBody.h"

//----------------------------------------------------------------------------
class CNEOBotBody : public PlayerBody
{
public:
	CNEOBotBody( INextBot* bot ) : PlayerBody( bot )
	{
	}

	virtual ~CNEOBotBody() { }

	virtual float GetHeadAimTrackingInterval( void ) const;			// return how often we should sample our target's position and velocity to update our aim tracking, to allow realistic slop in tracking
	virtual float GetCloakPower( void ) const;  		    		// return available thermoptic charge
	virtual bool IsCloakEnabled( void ) const;           			// return if thermoptic is enabled
};

