#include "weapon_neobaseprojectile.h"
#include "weapon_smokegrenade.h"

constexpr float PROJECTILE_RADIUS = 4;

#ifdef GAME_DLL
#include "neo_detpack.h"
static_assert(PROJECTILE_RADIUS == GRENADE_RADIUS);
static_assert(PROJECTILE_RADIUS == NEO_DEPLOYED_DETPACK_RADIUS);
#endif
#if defined(DBGFLAG_ASSERT)
#include "neo_gamerules.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

void CNEOBaseProjectile::GetThrowPos(const Vector& throwFwd, const Vector& pos, Vector& outPos) const
{
	constexpr float playerMaxsY = 16; // Cached for perf
#ifdef DBGFLAG_ASSERT
	Assert(NEORules()->GetViewVectors()->m_vHullMax.y == playerMaxsY);
	AssertFloatEquals(throwFwd.LengthSqr(), 1.f, 0.01f); // Expecting unit vector
#endif

	// The projectile hull... and a bit of magical leeway, because for slanted
	// surfaces, we might get stuck if doing a hull trace with the exact radius.
	constexpr float fuzzFactor = 2;
	static const Vector projectileMaxs{
		PROJECTILE_RADIUS + fuzzFactor,
		PROJECTILE_RADIUS + fuzzFactor,
		PROJECTILE_RADIUS + fuzzFactor };
	static const Vector projectileMins = -projectileMaxs;

	// Projectile spawning in front of the player's fwd hull maxs needs a
	// clearance of at least the size of the projectile's collider radius.
	constexpr float clearanceRequired = PROJECTILE_RADIUS + playerMaxsY;

	CTraceFilterNoNPCsOrPlayer filter(nullptr, COLLISION_GROUP_NONE);
	trace_t tr;
	UTIL_TraceHull(pos,
		pos + throwFwd * clearanceRequired,
		projectileMins, projectileMaxs,
		CONTENTS_SOLID, &filter, &tr);
	outPos = pos + throwFwd * (playerMaxsY * tr.fraction);
}
