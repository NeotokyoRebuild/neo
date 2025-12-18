#ifndef NEO_BOT_SEEK_WEAPON_H
#define NEO_BOT_SEEK_WEAPON_H

#include "bot/neo_bot.h"

// General utility that can be used to see if it makes sense to transition into this behavior
CBaseEntity *FindNearestPrimaryWeapon( const Vector &searchOrigin, bool bShortCircuit = false );

//--------------------------------------------------------------------------------------------------------
class CNEOBotSeekWeapon : public Action< CNEOBot >
{
public:
	CNEOBotSeekWeapon( void );

	virtual ActionResult< CNEOBot > OnStart( CNEOBot *me, Action< CNEOBot > *priorAction ) override;
	virtual ActionResult< CNEOBot > Update( CNEOBot *me, float interval ) override;
	virtual ActionResult< CNEOBot > OnResume( CNEOBot *me, Action< CNEOBot > *interruptingAction ) override;

	virtual EventDesiredResult< CNEOBot > OnStuck( CNEOBot *me ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToSuccess( CNEOBot *me, const Path *path ) override;
	virtual EventDesiredResult< CNEOBot > OnMoveToFailure( CNEOBot *me, const Path *path, MoveToFailureType reason ) override;

	virtual const char *GetName( void ) const override { return "SeekWeapon"; }

private:
	PathFollower m_path;
	CHandle<CBaseEntity> m_hTargetWeapon;

	CBaseEntity *FindAndPathToWeapon( CNEOBot *me );
};

#endif // NEO_BOT_SEEK_WEAPON_H
