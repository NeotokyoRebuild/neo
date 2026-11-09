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
#include "utlvector.h"

// A reload is a linear sequence of phases; a reload sequence finishes each phase at a point of its animation
enum ReloadPhase_t
{
	RELOAD_PHASE_NONE = 0,	// no reload in progress, or none pending
	RELOAD_PHASE_UNLOADED,	// 1_unload: the old magazine is out
	RELOAD_PHASE_LOADED,	// 2_load: the fresh magazine is in
	RELOAD_PHASE_CYCLED,	// 3_cycle: the bolt or charging handle chambered a round

	RELOAD_PHASE_COUNT
};

// bits to network a phase
static constexpr int RELOAD_PHASE_BITS = 2;
static_assert((1 << RELOAD_PHASE_BITS) >= RELOAD_PHASE_COUNT, "RELOAD_PHASE_BITS cannot hold every phase");
static constexpr int RELOAD_PHASE_TABLES_MAX = 12;
// bits to network a table index, signed so that -1 (no table) fits
static constexpr int RELOAD_PHASE_TABLE_BITS = 5;
static_assert((1 << (RELOAD_PHASE_TABLE_BITS - 1)) >= RELOAD_PHASE_TABLES_MAX, "RELOAD_PHASE_TABLE_BITS cannot hold every table index");
static constexpr int RELOAD_PHASE_ROWS_MAX = RELOAD_PHASE_COUNT - 1;
static constexpr int RELOAD_SOUND_NAME_MAX = 64;
static constexpr int RELOAD_SEQUENCE_NAME_MAX = 32;
// a phase may need more than one sound piece, such as loading a magazine and the tap after it
static constexpr int RELOAD_SOUNDS_PER_ROW = 3;

// A sound piece of a phase
struct ReloadSoundPiece_t
{
	float flAt;	// when the piece's first transient plays, seconds from the sequence start
	char szName[RELOAD_SOUND_NAME_MAX];
};

// One phase of a reload sequence: when its ammo counts, and the sound pieces that go with it
struct ReloadPhaseRow_t
{
	ReloadPhase_t phase;
	float flAmmo;	// seconds from the sequence start
	ReloadSoundPiece_t sounds[RELOAD_SOUNDS_PER_ROW];
	char szResume[RELOAD_SEQUENCE_NAME_MAX];	// the sequence that resumes a reload interrupted after this phase, or empty
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
	// A closed-bolt weapon retains a chambered round when reloading with rounds left.
	bool	m_bChamberedRound;
	// Empty: the weapon reloads without phases. Allocated only for scripts with a ReloadPhases block.
	CUtlVector<ReloadPhaseTable_t> m_reloadTables;
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

	int FindReloadTable(const char *pszSequence) const;

private:
	void ParseReloadPhases(::KeyValues *pBlock);
};


#endif // NEO_WEAPON_PARSE_H
