#include "neo_penetration_resistance.h"

#include "cbase.h"
#include "gametrace.h"

void TestPenetrationTrace(trace_t &penetrationTrace,
		const trace_t &tr, const Vector &vecDir, ITraceFilter *pTraceFilter)
{
	// Find the furthest point along the bullets trajectory
	Vector testPos = tr.endpos + (vecDir * MAX_PENETRATION_DEPTH);

	// Instead of tracing backwards from the furthest point the bullet could penetrate given a thick enough object initially penetrated, step into the object originally hit and find the next object behind this object,
	// if any, and trace backwards from that next object. There must be empty space between the next object and the object originally hit for the next object to be detected. This is a limitation of traceline and is why
	// placing toolsbulletblock inside of an object doesn't stop bullets penetrating straight through the bigger object.
	trace_t	nextObjectTrace;
	UTIL_TraceLine(tr.endpos + (vecDir * 0.1), testPos, MASK_SHOT, pTraceFilter, &nextObjectTrace);

	// Re-trace backwards to find the bullet ext
	// tracelines started inside of props get stuck unlike those started inside of brushes, revert to the original behaviour in those cases
	Vector penetrationTraceStart = nextObjectTrace.fraction == 0.0f ? tr.endpos + (vecDir * MAX_PENETRATION_DEPTH) : nextObjectTrace.endpos - (vecDir * 0.1);
	UTIL_TraceLine(penetrationTraceStart, tr.endpos, MASK_SHOT, nullptr, &penetrationTrace);
}

