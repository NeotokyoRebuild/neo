//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef SMOKE_FOG_OVERLAY_H
#define SMOKE_FOG_OVERLAY_H


#include "basetypes.h"
#include "mathlib/vector.h"
#include "smoke_fog_overlay_shared.h"

#ifdef NEO
#define ROTATION_SPEED				0.6
#define TRADE_DURATION_MIN			5
#define TRADE_DURATION_MAX			10
#define SMOKEGRENADE_PARTICLERADIUS	55

#define SMOKESPHERE_EXPAND_TIME		5.5		// Take N seconds to expand to SMOKESPHERE_MAX_RADIUS.

#define NUM_PARTICLES_PER_DIMENSION	6
#define SMOKEPARTICLE_OVERLAP		0

#define SMOKEPARTICLE_SIZE			55
#define NUM_MATERIAL_HANDLES		1
#else
#define ROTATION_SPEED				0.1
#define TRADE_DURATION_MIN			10
#define TRADE_DURATION_MAX			20
#define SMOKEGRENADE_PARTICLERADIUS	80

#define SMOKESPHERE_EXPAND_TIME		1		// Take N seconds to expand to SMOKESPHERE_MAX_RADIUS.

#define NUM_PARTICLES_PER_DIMENSION 4
#define SMOKEPARTICLE_OVERLAP		20

#define SMOKEPARTICLE_SIZE			80
#define NUM_MATERIAL_HANDLES		1
#endif // NEO

void InitSmokeFogOverlay();
void TermSmokeFogOverlay();
void DrawSmokeFogOverlay();
#ifdef NEO
void UpdateThermalOverride();
#endif

// Set these before calling DrawSmokeFogOverlay.
extern float	g_SmokeFogOverlayAlpha;
extern Vector	g_SmokeFogOverlayColor;
#ifdef NEO
extern bool		g_SmokeFogOverlayThermalOverride;
#endif

#endif


