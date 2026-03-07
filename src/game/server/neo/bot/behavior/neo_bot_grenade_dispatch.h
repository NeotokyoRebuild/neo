#ifndef NEO_BOT_GRENADE_DISPATCH_H
#define NEO_BOT_GRENADE_DISPATCH_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_bot_behavior.h"

class CNEOBot;
class CKnownEntity;

extern ConVar sv_neo_bot_grenade_throw_cooldown;

class CNEOBotGrenadeDispatch
{
public:
	static Action< CNEOBot > *ChooseGrenadeThrowBehavior( const CNEOBot *me, const CKnownEntity *threat );
};

#endif // NEO_BOT_GRENADE_DISPATCH_H
