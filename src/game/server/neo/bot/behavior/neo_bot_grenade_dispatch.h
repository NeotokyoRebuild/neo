#ifndef NEO_BOT_GRENADE_DISPATCH_H
#define NEO_BOT_GRENADE_DISPATCH_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_behavior.h"

class CNEOBot;
class CKnownEntity;

class CNEOBotGrenadeDispatch
{
public:
	static Action< CNEOBot > *ChooseGrenadeThrowBehavior( CNEOBot *me, const CKnownEntity *threat );
};

#endif // NEO_BOT_GRENADE_DISPATCH_H
