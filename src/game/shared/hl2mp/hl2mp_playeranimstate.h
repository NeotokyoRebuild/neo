//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef HL2MP_PLAYERANIMSTATE_H
#define HL2MP_PLAYERANIMSTATE_H
#ifdef _WIN32
#pragma once
#endif


#include "convar.h"
#include "multiplayer_animstate.h"

#if defined( CLIENT_DLL )
class C_HL2MP_Player;
#define CHL2MP_Player C_HL2MP_Player
#else
class CHL2MP_Player;
#endif

#ifdef NEO
// The part of the third-person reload gesture that a shorter first-person reload shows
enum ReloadGestureFit_t
{
	RELOAD_GESTURE_KEEP_START,	// a reload from the start: the gesture ends early
	RELOAD_GESTURE_KEEP_END,	// a resumed reload: the gesture starts late
};
#endif

// ------------------------------------------------------------------------------------------------ //
// CPlayerAnimState declaration.
// ------------------------------------------------------------------------------------------------ //
class CHL2MPPlayerAnimState : public CMultiPlayerAnimState
{
public:
	
	DECLARE_CLASS( CHL2MPPlayerAnimState, CMultiPlayerAnimState );

	CHL2MPPlayerAnimState();
	CHL2MPPlayerAnimState( CBasePlayer *pPlayer, MultiPlayerMovementData_t &movementData );
	~CHL2MPPlayerAnimState();

	void InitHL2MPAnimState( CHL2MP_Player *pPlayer );
	CHL2MP_Player *GetHL2MPPlayer( void )							{ return m_pHL2MPPlayer; }

	virtual void ClearAnimationState();
	int CalcSequenceIndex(const char* pBaseName, ...);
	virtual Activity TranslateActivity( Activity actDesired );
	virtual void Update( float eyeYaw, float eyePitch );

	void	DoAnimationEvent( PlayerAnimEvent_t event, int nData = 0 );

	bool	HandleMoving( Activity &idealActivity );
	bool	HandleJumping( Activity &idealActivity );
	bool	HandleDucking( Activity &idealActivity );
	bool	HandleSwimming( Activity &idealActivity );

	virtual float GetCurrentMaxGroundSpeed();
#ifdef NEO
	virtual void ComputeSequences(CStudioHdr* pStudioHdr);

	// A reload event's data: how long the first-person reload lasts, and which part of a longer gesture it shows
	static int PackReloadGestureData(ReloadGestureFit_t fit, float flDuration);
#endif

	void DebugShowAnimStateFull(int iStartLine);
	virtual void DebugShowAnimState(int iStartLine);
	void AnimStatePrintf(int iLine, PRINTF_FORMAT_STRING const char* pMsg, ...);
	void AnimStateLog(PRINTF_FORMAT_STRING const char* pMsg, ...);

private:

	bool						SetupPoseParameters( CStudioHdr *pStudioHdr );
	virtual void				EstimateYaw( void );
	virtual void				ComputePoseParam_MoveYaw( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimPitch( CStudioHdr *pStudioHdr );
	virtual void				ComputePoseParam_AimYaw( CStudioHdr *pStudioHdr );
	
#ifdef NEO
	bool	IsLeaning(CStudioHdr *pStudioHdr);
	void	SetGestureSlotCycle(int iGestureSlot, float flCycle);
	void	StartReloadGesture(int nData);
	void	UpdateReloadGesture();
#endif // NEO
	
	CHL2MP_Player   *m_pHL2MPPlayer;
#ifdef NEO
	// The playing reload gesture's sequence (-1: none) and the cycles it blends in from and out to
	int		m_iReloadGestureSequence;
	float	m_flReloadGestureStartCycle;
	float	m_flReloadGestureEndCycle;
#endif
#ifndef NEO
	bool		m_bInAirWalk;
	float		m_flHoldDeployedPoseUntilTime;
#endif
};

CHL2MPPlayerAnimState *CreateHL2MPPlayerAnimState( CHL2MP_Player *pPlayer );



#endif // HL2MP_PLAYERANIMSTATE_H
