//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef NEO_WEAPON_PARSE_H
#define NEO_WEAPON_PARSE_H
#ifdef _WIN32
#pragma once
#endif

#include "hl2mp_weapon_parse.h"

// A reload is a linear sequence of phases; a reload sequence credits each phase at a point of its animation
enum ReloadPhase_t
{
	RELOAD_PHASE_NONE = 0,	// no reload in progress, or none pending
	RELOAD_PHASE_DROPPED,	// the old magazine is out
	RELOAD_PHASE_SEATED,	// the fresh magazine is in
	RELOAD_PHASE_SLAPPED,	// the magazine was tapped or slapped home
	RELOAD_PHASE_BOLTED,	// the bolt or charging handle chambered a round

	RELOAD_PHASE_COUNT
};

static constexpr int RELOAD_PHASE_TABLES_MAX = 8;
static constexpr int RELOAD_PHASE_ROWS_MAX = RELOAD_PHASE_COUNT - 1;
static constexpr int RELOAD_SOUND_NAME_MAX = 64;
static constexpr int RELOAD_SEQUENCE_NAME_MAX = 32;

// One phase of a reload sequence: when it is credited, and the sound piece that goes with it
struct ReloadPhaseRow_t
{
	ReloadPhase_t phase;
	float flCredit;	// seconds from the sequence start
	float flSoundAt;	// when the sound piece's first transient plays, seconds from the sequence start
	char szSound[RELOAD_SOUND_NAME_MAX];
};

// The phases one reload sequence carries, in order
struct ReloadPhaseTable_t
{
	char szSequence[RELOAD_SEQUENCE_NAME_MAX];
	int iNumRows;
	ReloadPhaseRow_t rows[RELOAD_PHASE_ROWS_MAX];
};

//--------------------------------------------------------------------------------------------------------
class CNEOWeaponInfo : public CHL2MPSWeaponInfo
{
public:
	DECLARE_CLASS_GAMEROOT( CNEOWeaponInfo, CHL2MPSWeaponInfo );

	CNEOWeaponInfo();

	virtual void Parse( ::KeyValues *pKeyValuesData, const char *szWeaponName );

	int		m_iBullets;
	float	m_flCycleTime;
	float	m_flReloadReadyBeforeEnd;
	bool	m_bChamberedRound;	// a partial reload keeps the round in the chamber
	int		m_iNumReloadTables;	// 0: the weapon reloads without phases
	ReloadPhaseTable_t m_reloadTables[RELOAD_PHASE_TABLES_MAX];

	int FindReloadTable(const char *pszSequence) const;
	char	szViewModel2[MAX_WEAPON_STRING];		// Team 2 (NSF) wep vm
	char	szBulletCharacter[MAX_BULLET_CHARACTER];// character used to display ammunition in current clip
	char	szDeathIcon[MAX_BULLET_CHARACTER];
	float	m_flPenetration;
	bool	m_bDropOnDeath;
	int		iAimFOV;

	float	m_flVMFov;
	Vector	m_vecVMPosOffset;
	QAngle	m_angVMAngOffset;

	float	m_flVMAimFov;
	Vector	m_vecVMAimPosOffset;
	QAngle	m_angVMAimAngOffset;

private:
	void ParseReloadPhases(::KeyValues *pBlock);
};


#endif // NEO_WEAPON_PARSE_H
