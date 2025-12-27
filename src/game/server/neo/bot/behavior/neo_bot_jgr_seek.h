#ifndef NEO_BOT_JGR_SEEK_H
#define NEO_BOT_JGR_SEEK_H
#pragma once

#include "bot/behavior/neo_bot_seek_and_destroy.h"

//
// JGR game mode behavior dispatcher
//
class CNEOBotJgrSeek : public CNEOBotSeekAndDestroy
{
public:
	CNEOBotJgrSeek( float duration = -1.0f ) : CNEOBotSeekAndDestroy( duration ) { }

	virtual ActionResult< CNEOBot >	Update( CNEOBot *me, float interval );
	virtual const char *GetName( void ) const	{ return "jgrSeek"; };

protected:
	virtual void RecomputeSeekPath( CNEOBot *me );
};
#endif // NEO_BOT_JGR_SEEK_H
