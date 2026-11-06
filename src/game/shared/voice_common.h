//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VOICE_COMMON_H
#define VOICE_COMMON_H
#ifdef _WIN32
#pragma once
#endif


#include "bitvec.h"
#include "const.h"


#define VOICE_MAX_PLAYERS		MAX_PLAYERS
#define VOICE_MAX_PLAYERS_DW	((VOICE_MAX_PLAYERS / 32) + !!(VOICE_MAX_PLAYERS & 31))

typedef CBitVec<VOICE_MAX_PLAYERS> CPlayerBitVec;

#define VOICE_DEFAULT_PROXIMITY_RANGE 1200 //100 feet

#ifdef NEO
enum NeoVoiceTransmitType
{
	NEO_VOICE_TRANSMIT_NONE = 0,
	NEO_VOICE_TRANSMIT_GLOBALTEAM,
	NEO_VOICE_TRANSMIT_LOCAL,

	NEO_VOICE_TRANSMIT__TOTAL = NEO_VOICE_TRANSMIT_LOCAL,
	NEO_VOICE_TRANSMIT__BITS = 2
};

COMPILE_TIME_ASSERT(1 << NEO_VOICE_TRANSMIT__BITS > NEO_VOICE_TRANSMIT__TOTAL);
#endif // NEO

#endif // VOICE_COMMON_H
