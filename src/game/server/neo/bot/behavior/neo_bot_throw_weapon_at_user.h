#ifndef NEO_BOT_THROW_WEAPON_AT_USER_H
#define NEO_BOT_THROW_WEAPON_AT_USER_H
#pragma once

#include "bot/behavior/neo_bot_throw_weapon_at_player.h"
#include "bot/neo_bot.h"
#include "neo_player.h"

//-----------------------------------------------------------------------------------------
// If the bot command system is disabled, bots simply drop their weapon for the user
class CNEOBotThrowWeaponAtUser : public CNEOBotThrowWeaponAtPlayer
{
public:
	CNEOBotThrowWeaponAtUser(CNEO_Player* pTargetPlayer) : CNEOBotThrowWeaponAtPlayer(pTargetPlayer) {}

	virtual void OnEnd(CNEOBot* me, Action< CNEOBot >* nextAction) override
	{
		me->m_hCommandingPlayer = nullptr;
		CNEOBotThrowWeaponAtPlayer::OnEnd(me, nextAction);
	}
	
	virtual const char* GetName(void) const override { return "ThrowWeaponAtUser"; };

	static bool ReadyAimToThrowWeapon(CNEOBot* me, CNEO_Player* pCommander)
	{
		me->GetBodyInterface()->AimHeadTowards(pCommander->GetAbsOrigin(), IBody::MANDATORY, 0.2f, nullptr, "Aiming at user's feet to throw weapon");

		Vector vecToUserFeet = pCommander->GetAbsOrigin() - me->EyePosition();
		vecToUserFeet.NormalizeInPlace();

		Vector vecBotFacing;
		me->EyeVectors(&vecBotFacing);

		return (vecBotFacing.Dot(vecToUserFeet) > 0.95f);
	}
};

#endif // NEO_BOT_THROW_WEAPON_AT_USER_H
