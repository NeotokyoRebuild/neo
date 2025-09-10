#pragma once

#include "NextBot/Player/NextBotPlayerBody.h"

class CNEO_Player;

//----------------------------------------------------------------------------
class CNEOBotBody : public PlayerBody
{
public:
	CNEOBotBody( INextBot* bot ) : PlayerBody( bot )
	{
	}

	virtual ~CNEOBotBody() { }

	CNEO_Player *NEOPlayerEnt() const;

	float GetHeadAimTrackingInterval() const override;			// return how often we should sample our target's position and velocity to update our aim tracking, to allow realistic slop in tracking
	float GetCloakPower() const;          		    		// return available thermoptic charge
	bool IsCloakEnabled() const;                  			// return if thermoptic is enabled

	// NEO NOTE (nullsystem): These are override to use the NT;RE's VEC_... macros
	float GetHullWidth() const override;
	float GetStandHullHeight() const override;
	float GetCrouchHullHeight() const override;
	const Vector &GetHullMins() const override;
	const Vector &GetHullMaxs() const override;
};

