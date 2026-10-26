#ifndef NEO_PLAYERANIMSTATE_H
#define NEO_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif

#include "base_playeranimstate.h"

#ifdef CLIENT_DLL
class C_HL2MP_Player;
#define CHL2MP_Player C_HL2MP_Player
#else
class CHL2MP_Player;
#endif

enum PlayerAnimEvent_t
{
	PLAYERANIMEVENT_ATTACK_PRIMARY,
	PLAYERANIMEVENT_RELOAD,
	PLAYERANIMEVENT_JUMP,
	PLAYERANIMEVENT_SPAWN,

	PLAYERANIMEVENT_COUNT
};

class CNEOPlayerAnimState : public CBasePlayerAnimState
{
public:
	DECLARE_CLASS( CNEOPlayerAnimState, CBasePlayerAnimState );

	CNEOPlayerAnimState( CHL2MP_Player *pPlayer );

	void DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	virtual void ClearAnimationState() override;
	virtual void ClearAnimationLayers() override;
	virtual void ComputeSequences( CStudioHdr *pStudioHdr ) override;
	virtual Activity CalcMainActivity() override;
	virtual int CalcAimLayerSequence( float *flCycle, float *flAimSequenceWeight, bool bForceIdle ) override;
	virtual float GetCurrentMaxGroundSpeed() override;

protected:
	virtual void ComputePoseParam_BodyYaw() override;

private:
	const char *GetAimSuffix() const;
	int CalcFireSequence();
	int CalcReloadSequence();
	void StartGesture( int iSequence );
	void ComputeGestureSequence( CStudioHdr *pStudioHdr );

	bool HandleJumping();
	bool IsLeaning() const;

	CHL2MP_Player *m_pPlayer;

	// the jump pose plays from the jump event until the player lands
	bool m_bJumping;
	bool m_bFirstJumpFrame;
	float m_flJumpStartTime;

	// fire, reload and throw sequences share one layer above the aim layers, and a new one restarts it
	int m_iGestureSequence;
	float m_flGestureCycle;
	EHANDLE m_hGestureWeapon;
};

CNEOPlayerAnimState *CreateNEOPlayerAnimState( CHL2MP_Player *pPlayer );

#endif // NEO_PLAYERANIMSTATE_H
