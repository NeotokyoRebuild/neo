
#include "cbase.h"
#include "neo_detpack_hitbox.h"
#include "neo_detpack.h"
#include "util_shared.h"
#include "collisionutils.h"
#include "debugoverlay_shared.h" // Add this include

// NOTE: This has to be the last file included!
#include "tier0/memdbgon.h"

extern ConVar sv_neo_detpack_debug_draw_hitbox; // Extern declaration for ConVar

LINK_ENTITY_TO_CLASS(neo_detpack_hitbox, CNEODetpackHitbox);

BEGIN_DATADESC(CNEODetpackHitbox)
	DEFINE_FIELD(m_hParentDetpack, FIELD_EHANDLE),
END_DATADESC()

void CNEODetpackHitbox::Spawn()
{
	BaseClass::Spawn();
	SetSolid(SOLID_BBOX);
	//SetSolidFlags(FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST | FSOLID_NOT_SOLID);
	SetSolidFlags(FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST);
	UTIL_SetSize(this, Vector(-10, -10, -2), Vector(10, 10, 10));
	//SetCollisionGroup(COLLISION_GROUP_NONE);

	if (sv_neo_detpack_debug_draw_hitbox.GetBool())
	{
		NDebugOverlay::Box(GetAbsOrigin(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), 255, 0, 0, 100, 10.0f);
	}
}

void CNEODetpackHitbox::SetParentDetpack(CNEODeployedDetpack* pParent)
{
	m_hParentDetpack = pParent;
	if (pParent)
	{
		SetOwnerEntity(pParent->GetOwnerEntity());
	}

	if (sv_neo_detpack_debug_draw_hitbox.GetBool())
	{
		NDebugOverlay::Box(GetAbsOrigin(), CollisionProp()->OBBMins(), CollisionProp()->OBBMaxs(), 0, 255, 0, 100, 10.0f);
	}
}

int CNEODetpackHitbox::OnTakeDamage(const CTakeDamageInfo& inputInfo)
{
	if (m_hParentDetpack)
	{
		// Forward the damage to the parent detpack.
		// The damage info is already in world space, so we can just pass it along.
		m_hParentDetpack->TakeDamage(inputInfo);
	}
	return 0; // The hitbox itself takes no damage.
}

bool CNEODetpackHitbox::TestCollision(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr)
{
	// We only want to collide with bullets.
	if ((fContentsMask & MASK_SHOT) == 0)
		return false;

	// Since we use FSOLID_CUSTOMRAYTEST, we have to do the intersection test ourselves.
	Vector worldMins, worldMaxs;
	CollisionProp()->WorldSpaceAABB(&worldMins, &worldMaxs); // Get world-space AABB
	return IntersectRayWithBox(ray, worldMins, worldMaxs, 0.0f, &tr); // Use worldMins and worldMaxs
}

bool CNEODetpackHitbox::TestHitboxes(const Ray_t& ray, unsigned int fContentsMask, trace_t& tr)
{
	// We don't have hitboxes, so we don't need to do anything here.
	return true;
}
