//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IN_MAIN_H
#define IN_MAIN_H
#ifdef _WIN32
#pragma once
#endif


#include "kbutton.h"


extern kbutton_t in_commandermousemove;
extern kbutton_t in_ducktoggle;

#ifdef NEO
void IN_LeanLeft();
void IN_LeanRight();
void IN_LeanReset();
void IN_SpeedReset();
#endif // NEO

#endif // IN_MAIN_H
