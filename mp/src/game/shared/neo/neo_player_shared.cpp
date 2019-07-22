#include "cbase.h"
#include "neo_player_shared.h"

#include "in_buttons.h"
#include "neo_gamerules.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#ifndef CNEO_Player
#define CNEO_Player C_NEO_Player
#endif
#else
#include "neo_player.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar neo_cl_cyborgclass("neo_cl_cyborgclass", "-1", FCVAR_USERINFO,
	"Chosen class number.", true, -1, true, NEO_CLASS_SUPPORT);
ConVar neo_cl_skin("neo_cl_skin", "-1", FCVAR_USERINFO,
	"Chosen skin number.", true, -1, true, NEO_SKIN_THIRD);
