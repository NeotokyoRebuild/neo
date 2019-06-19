#ifndef NEO_PLAYER_H
#define NEO_PLAYER_H
#ifdef _WIN32
#pragma once
#endif

class CNEO_Player;

#include "basemultiplayerplayer.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "utldict.h"
#include "hl2mp_player.h"

class CNEO_Player : public CHL2MP_Player
{
public:
	DECLARE_CLASS(CNEO_Player, CHL2MP_Player);

	CNEO_Player();
	virtual ~CNEO_Player(void);

	static CNEO_Player *CreatePlayer(const char *className, edict_t *ed)
	{
		CNEO_Player::s_PlayerEdict = ed;
		return (CNEO_Player*)CreateEntityByName(className);
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void PostThink( void );
	virtual void PreThink( void );
	virtual void PlayerDeathThink( void );
	virtual void SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool HandleCommand_JoinTeam( int team );
	virtual bool ClientCommand( const CCommand &args );
	virtual void CreateViewModel( int viewmodelindex = 0 );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );
	virtual bool WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;
	virtual void FireBullets ( const FireBulletsInfo_t &info );
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void ChangeTeam( int iTeam );
	virtual void PickupObject ( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void UpdateOnRemove( void );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual CBaseEntity* EntSelectSpawnPoint( void );
	virtual void EquipSuit(bool bPlayEffects = true);
	virtual void RemoveSuit(void);

	void SetPlayerTeamModel( void );
	virtual void PickDefaultSpawnTeam( void );

	virtual bool StartObserverMode( int mode );
	virtual void StopObserverMode( void );

	virtual bool	CanHearAndReadChatFrom( CBasePlayer *pPlayer );

	inline bool IsAllowedToDrop(CBaseCombatWeapon *pWep);
	inline bool IsAllowedToZoom(CBaseCombatWeapon *pWep);

	void Weapon_AimToggle( CBaseCombatWeapon *pWep );

	void DoThirdPersonLean(void);
	void SoftSuicide(void);
	void GiveAllItems(void);
	inline bool ProcessTeamSwitchRequest(int iTeam);

	inline void Weapon_SetZoom( bool bZoomIn );

	IMPLEMENT_NETWORK_VAR_FOR_DERIVED(m_EyeAngleOffset);

public:
	CNetworkVar(int, m_nNeoSkin);
	CNetworkVar(int, m_nCyborgClass);

private:
	bool m_bInLeanLeft, m_bInLeanRight;
	Vector m_leanPosTargetOffset;
};

inline CNEO_Player *ToNEOPlayer(CBaseEntity *pEntity)
{
	if (!pEntity || !pEntity->IsPlayer())
	{
		return NULL;
	}

	return dynamic_cast<CNEO_Player*>(pEntity);
}

#endif // NEO_PLAYER_H