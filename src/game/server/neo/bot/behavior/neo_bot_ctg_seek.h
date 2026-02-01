#pragma once

#include "bot/behavior/neo_bot_seek_and_destroy.h"

//
// CTG game mode behavior dispatcher
//
class CNEOBotCtgSeek : public CNEOBotSeekAndDestroy
{
public:
	CNEOBotCtgSeek( float duration = -1.0f ) : CNEOBotSeekAndDestroy( duration ) { }

	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval ) override;
	virtual const char *GetName( void ) const override { return "ctgSeek"; };

protected:
	virtual void RecomputeSeekPath( CNEOBot *me ) override;
};
