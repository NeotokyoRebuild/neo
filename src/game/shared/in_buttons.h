//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef IN_BUTTONS_H
#define IN_BUTTONS_H
#ifdef _WIN32
#pragma once
#endif

#define IN_ATTACK		(1 << 0)
#define IN_JUMP			(1 << 1)
#define IN_DUCK			(1 << 2)
#define IN_FORWARD		(1 << 3)
#define IN_BACK			(1 << 4)
#define IN_USE			(1 << 5)
#define IN_CANCEL		(1 << 6)
#define IN_LEFT			(1 << 7)
#define IN_RIGHT		(1 << 8)
#define IN_MOVELEFT		(1 << 9)
#define IN_MOVERIGHT	(1 << 10)
#define IN_ATTACK2		(1 << 11)
#define IN_RUN			(1 << 12)
#define IN_RELOAD		(1 << 13)
#define IN_ALT1			(1 << 14)
#define IN_ALT2			(1 << 15)
#define IN_SCORE		(1 << 16)   // Used by client.dll for when scoreboard is held down
#define IN_SPEED		(1 << 17)	// Player is holding the speed key
#define IN_WALK			(1 << 18)	// Player holding walk key
#define IN_ZOOM			(1 << 19)	// Zoom key for HUD zoom
#define IN_WEAPON1		(1 << 20)	// weapon defines these bits
#define IN_WEAPON2		(1 << 21)	// weapon defines these bits
#define IN_BULLRUSH		(1 << 22)
#define IN_GRENADE1		(1 << 23)	// grenade 1
#define IN_GRENADE2		(1 << 24)	// grenade 2
#define	IN_ATTACK3		(1 << 25)

#ifdef NEO
#define IN_DROP         (1 << 26) // Weapon drop
#define IN_AIM          (1 << 27) // Weapon aim
#define IN_LEAN_LEFT    (1 << 28)
#define IN_LEAN_RIGHT   (1 << 29)
#define IN_THERMOPTIC	(1 << 30) // Thermoptic cloaking
#define IN_VISION		(1 << 31) // Vision modes
#endif

#endif // IN_BUTTONS_H
