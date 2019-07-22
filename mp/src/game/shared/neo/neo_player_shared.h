#ifndef NEO_PLAYER_SHARED_H
#define NEO_PLAYER_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "neo_predicted_viewmodel.h"

enum NeoSkin {
	NEO_SKIN_FIRST = 0,
	NEO_SKIN_SECOND,
	NEO_SKIN_THIRD,

	NEO_SKIN_ENUM_COUNT
};

enum NeoClass {
	NEO_CLASS_RECON = 0,
	NEO_CLASS_ASSAULT,
	NEO_CLASS_SUPPORT,

	// NOTENOTE: VIP *must* be last, because we are
	// using array offsets for recon/assault/support
	NEO_CLASS_VIP,

	NEO_CLASS_ENUM_COUNT
};

extern ConVar neo_cl_cyborgclass;
extern ConVar neo_cl_skin;

class CNEO_Player;

bool IsThereRoomForLeanSlide(CNEO_Player *player,
	const Vector &targetViewOffset, bool &outStartInSolid);

#endif // NEO_PLAYER_SHARED_H