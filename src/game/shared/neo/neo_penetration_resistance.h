#pragma once

#include "mathlib/vector.h"

class ITraceFilter;
class CGameTrace;
typedef CGameTrace trace_t;

#define MAX_PENETRATION_DEPTH 12.0f
#define MATERIALS_NUM 26
static constexpr const float PENETRATION_RESISTANCE[MATERIALS_NUM] =
{
	1.0,						// CHAR_TEX_ANTLION
	1.0,						// CHAR_TEX_BLOODYFLESH	
	0.3,						// CHAR_TEX_CONCRETE		
	0.2,						// CHAR_TEX_DIRT			
	1.0,						// CHAR_TEX_EGGSHELL		
	0.6,						// CHAR_TEX_FLESH			
	0.75,						// CHAR_TEX_GRATE			
	0.6,						// CHAR_TEX_ALIENFLESH		
	1.0,						// CHAR_TEX_CLIP			
	1.0,						// CHAR_TEX_BLOCKBULLETS		
	1.0,						// CHAR_TEX_UNUSED		
	0.8,						// CHAR_TEX_PLASTIC		
	0.5,						// CHAR_TEX_METAL			
	0.2,						// CHAR_TEX_SAND			
	0.8,						// CHAR_TEX_FOLIAGE		
	0.5,						// CHAR_TEX_COMPUTER		
	1.0,						// CHAR_TEX_UNUSED		
	1.0,						// CHAR_TEX_UNUSED		
	1.0,						// CHAR_TEX_SLOSH			
	0.5,						// CHAR_TEX_TILE			
	1.0,						// CHAR_TEX_UNUSED		
	0.75,						// CHAR_TEX_VENT			
	0.75,						// CHAR_TEX_WOOD			
	1.0,						// CHAR_TEX_UNUSED		
	0.9,						// CHAR_TEX_GLASS			
	1.0,						// CHAR_TEX_WARPSHIELD
};

void TestPenetrationTrace(trace_t &penetrationTrace,
		const trace_t &tr, const Vector &vecDir, ITraceFilter *pTraceFilter);

