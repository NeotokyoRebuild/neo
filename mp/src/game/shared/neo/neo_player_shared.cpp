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
