#include "cbase.h"
#include "weapon_knife.h"

#ifndef CLIENT_DLL
#include "ilagcompensationmanager.h"
#include "effect_dispatch_data.h"
#include "te_effect_dispatch.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define KNIFE_VM_ATTACK_ACT ACT_VM_PRIMARYATTACK

#define KNIFE_HULL_DIM 16.0f
static const Vector g_bludgeonMins(-KNIFE_HULL_DIM, -KNIFE_HULL_DIM, -KNIFE_HULL_DIM);
static const Vector g_bludgeonMaxs(KNIFE_HULL_DIM, KNIFE_HULL_DIM, KNIFE_HULL_DIM);

IMPLEMENT_NETWORKCLASS_ALIASED (WeaponKnife, DT_WeaponKnife)

BEGIN_NETWORK_TABLE(CWeaponKnife, DT_WeaponKnife)
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CWeaponKnife)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(weapon_knife, CWeaponKnife);

#ifdef GAME_DLL
BEGIN_DATADESC(CWeaponKnife)
END_DATADESC()
#endif

PRECACHE_WEAPON_REGISTER(weapon_knife);

#ifdef GAME_DLL // NEO FIXME (Rain): fix these values
acttable_t	CWeaponKnife::m_acttable[] =
{
	{ ACT_MP_STAND_IDLE, ACT_IDLE_MELEE, false },
	{ ACT_MP_RUN, ACT_MP_RUN_MELEE, false },
	{ ACT_MP_WALK, ACT_WALK, false },
	{ ACT_MP_JUMP, ACT_HOP, false },

	{ ACT_MP_AIRWALK, ACT_HOVER, false },
	{ ACT_MP_SWIM, ACT_SWIM, false },

	{ ACT_LEAP, ACT_LEAP, false },
	{ ACT_DIESIMPLE, ACT_DIESIMPLE, false },

	{ ACT_MP_CROUCH_IDLE, ACT_CROUCHIDLE, false },
	{ ACT_MP_CROUCHWALK, ACT_WALK_CROUCH, false },

	{ ACT_MP_RELOAD_STAND, ACT_RELOAD, false },

	{ ACT_CROUCHIDLE, ACT_HL2MP_IDLE_CROUCH_MELEE, false },
	{ ACT_WALK_CROUCH, ACT_HL2MP_WALK_CROUCH_MELEE, false }
};
IMPLEMENT_ACTTABLE(CWeaponKnife);
#endif

namespace {
constexpr float KNIFE_DAMAGE = 25.0f;
}

CWeaponKnife::CWeaponKnife()
{
	m_bFiresUnderwater = true;
}

void CWeaponKnife::Spawn()
{
	m_fMinRange1 = 0;
	m_fMinRange2 = 0;
	m_fMaxRange1 = 64;
	m_fMaxRange2 = 64;
	BaseClass::Spawn();
}

void CWeaponKnife::ItemPostFrame()
{
	ProcessAnimationEvents();
	BaseClass::ItemPostFrame();
}

void CWeaponKnife::PrimaryAttack()
{
	auto owner = static_cast<CNEO_Player*>(GetOwner());
	if (owner && owner->GetNeoFlags() & NEO_FL_FREEZETIME)
	{
		return;
	}

#ifndef CLIENT_DLL
	CHL2MP_Player *pPlayer = ToHL2MPPlayer(GetPlayerOwner());
	// Move other players back to history positions based on local player's lag
	lagcompensation->StartLagCompensation(pPlayer, pPlayer->GetCurrentCommand());
#endif
	Swing();
#ifndef CLIENT_DLL
	// Move other players back to history positions based on local player's lag
	lagcompensation->FinishLagCompensation(pPlayer);
#endif
}

bool CWeaponKnife::CanBePickedUpByClass(int classId)
{
	return (classId != NEO_CLASS_SUPPORT) && (classId != NEO_CLASS_JUGGERNAUT);
}

#ifdef CLIENT_DLL
bool CWeaponKnife::ShouldDraw()
{
	auto owner = static_cast<CNEO_Player*>(GetOwner());
	return (owner && owner->IsAlive() && owner->GetActiveWeapon() == this);
}
#else
bool CWeaponKnife::IsViewable()
{
	auto owner = static_cast<CNEO_Player*>(GetOwner());
	return (owner && owner->IsAlive() && owner->GetActiveWeapon() == this);
}
#endif

void CWeaponKnife::Swing()
{
	auto pOwner = static_cast<CNEO_Player*>(GetOwner());
	if (!pOwner)
		return;

	// Try a ray
	trace_t traceHit;

	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;

	pOwner->EyeVectors(&forward, NULL, NULL);

	Vector swingEnd = swingStart + forward * NEO_WEP_KNIFE_RANGE;
	UTIL_TraceLine(swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
	const Activity nHitActivity = KNIFE_VM_ATTACK_ACT;

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo(GetOwner(), GetOwner(), KNIFE_DAMAGE, DMG_SLASH);
	TraceAttackToTriggers(triggerInfo, traceHit.startpos, traceHit.endpos, vec3_origin);
#endif

	if (traceHit.fraction == 1.0)
	{
		float bludgeonHullRadius = 1.732f * KNIFE_HULL_DIM;  // use cuberoot of 2 to determine how big the hull is from center to the corner point

		// Back off by hull "radius"
		swingEnd -= forward * bludgeonHullRadius;

		UTIL_TraceHull(swingStart, swingEnd, g_bludgeonMins, g_bludgeonMaxs, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
		if (traceHit.fraction < 1.0 && traceHit.m_pEnt)
		{
			Vector vecToTarget = traceHit.m_pEnt->GetAbsOrigin() - swingStart;
			VectorNormalize(vecToTarget);

			float dot = vecToTarget.Dot(forward);

			// YWB:  Make sure they are sort of facing the guy at least...
			if (dot < 0.70721f)
			{
				// Force amiss
				traceHit.fraction = 1.0f;
			}
			else
			{
				ChooseIntersectionPointAndActivity(traceHit, g_bludgeonMins, g_bludgeonMaxs, pOwner);
			}
		}
	}

	WeaponSound(SINGLE);

	// -------------------------
	//	Miss
	// -------------------------
	if (traceHit.fraction == 1.0f)
	{
		// We want to test the first swing again
		Vector testEnd = swingStart + forward * NEO_WEP_KNIFE_RANGE;

		// See if we happened to hit water
		ImpactWater(swingStart, testEnd);
	}
	else
	{
		Hit(traceHit, nHitActivity);
	}

	// Send the anim
	SendWeaponAnim(nHitActivity);

	pOwner->DoAnimationEvent(PLAYERANIMEVENT_ATTACK_PRIMARY);

	//Setup our next attack times
	m_flNextPrimaryAttack = gpGlobals->curtime + GetFireRate();
	m_flNextSecondaryAttack = FLT_MAX;
}

Activity CWeaponKnife::ChooseIntersectionPointAndActivity(trace_t& hitTrace, const Vector& mins,
	const Vector& maxs, CBasePlayer* pOwner)
{
	int			i, j, k;
	float		distance;
	const float* minmaxs[2] = { mins.Base(), maxs.Base() };
	trace_t		tmpTrace;
	Vector		vecHullEnd = hitTrace.endpos;
	Vector		vecEnd;

	distance = 1e6f;
	Vector vecSrc = hitTrace.startpos;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine(vecSrc, vecHullEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace);
	if (tmpTrace.fraction == 1.0)
	{
		for (i = 0; i < 2; i++)
		{
			for (j = 0; j < 2; j++)
			{
				for (k = 0; k < 2; k++)
				{
					vecEnd.x = vecHullEnd.x + minmaxs[i][0];
					vecEnd.y = vecHullEnd.y + minmaxs[j][1];
					vecEnd.z = vecHullEnd.z + minmaxs[k][2];

					UTIL_TraceLine(vecSrc, vecEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &tmpTrace);
					if (tmpTrace.fraction < 1.0)
					{
						float thisDistance = (tmpTrace.endpos - vecSrc).Length();
						if (thisDistance < distance)
						{
							hitTrace = tmpTrace;
							distance = thisDistance;
						}
					}
				}
			}
		}
	}
	else
	{
		hitTrace = tmpTrace;
	}

	return KNIFE_VM_ATTACK_ACT;
}

void CWeaponKnife::Hit(trace_t& traceHit, [[maybe_unused]] Activity nHitActivity)
{
	Assert(nHitActivity == KNIFE_VM_ATTACK_ACT);

	CBasePlayer *pPlayer = ToBasePlayer(GetOwner());

	// Do view kick
	//AddViewKick();

	CBaseEntity *pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if (pHitEntity != NULL)
	{
		// Shouldn't be able to hit self
		Assert(pPlayer != pHitEntity);

		Vector hitDirection;
		pPlayer->EyeVectors(&hitDirection, NULL, NULL);
		VectorNormalize(hitDirection);

#ifndef CLIENT_DLL

		Vector forward;
		AngleVectors(pHitEntity->GetAbsAngles(), &forward);

		Vector2D forward2D = Vector2D(forward.x, forward.y);
		forward2D.NormalizeInPlace();

		Vector attackerToTarget = pHitEntity->GetAbsOrigin() - pPlayer->GetAbsOrigin();
		Vector2D attackerToTarget2D = Vector2D(attackerToTarget.x, attackerToTarget.y);
		attackerToTarget2D.NormalizeInPlace();

		const float currentAngle = acos(DotProduct2D(forward2D, attackerToTarget2D));

		static constexpr float maxBackStabAngle = 0.6435011; // ~ asin(0.6);

		static constexpr int damageToOneShotSupport = (100 * (1 / NEO_SUPPORT_DAMAGE_MODIFIER)) + 1;

		CTakeDamageInfo info(GetOwner(), GetOwner(), KNIFE_DAMAGE, DMG_SLASH);

		CalculateMeleeDamageForce(&info, hitDirection, traceHit.endpos, 0.05f);

		if (currentAngle <= maxBackStabAngle)
		{	// increase damage if backstabbing only after melee damage force has been calculated, so objects cannot be "backstabbed" to launch them further
			info.SetDamage(damageToOneShotSupport);
		}

		pHitEntity->DispatchTraceAttack(info, hitDirection, &traceHit);
		ApplyMultiDamage();

		// Now hit all triggers along the ray that...
		TraceAttackToTriggers(info, traceHit.startpos, traceHit.endpos, hitDirection);
#endif
		WeaponSound(MELEE_HIT);
	}

	// Apply an impact effect
	ImpactEffect(traceHit);
}

void CWeaponKnife::ImpactEffect(trace_t &traceHit)
{
	// See if we hit water (we don't do the other impact effects in this case)
	if (ImpactWater(traceHit.startpos, traceHit.endpos))
		return;

	UTIL_ImpactTrace(&traceHit, DMG_SLASH);
}

bool CWeaponKnife::ImpactWater(const Vector &start, const Vector &end)
{
	//FIXME: This doesn't handle the case of trying to splash while being underwater, but that's not going to look good
	//		 right now anyway...

	// We must start outside the water
	if (UTIL_PointContents(start) & (CONTENTS_WATER | CONTENTS_SLIME))
		return false;

	// We must end inside of water
	if (!(UTIL_PointContents(end) & (CONTENTS_WATER | CONTENTS_SLIME)))
		return false;

	trace_t	waterTrace;

	UTIL_TraceLine(start, end, (CONTENTS_WATER | CONTENTS_SLIME), GetOwner(), COLLISION_GROUP_NONE, &waterTrace);

	if (waterTrace.fraction < 1.0f)
	{
#ifndef CLIENT_DLL
		CEffectData	data;

		data.m_fFlags = 0;
		data.m_vOrigin = waterTrace.endpos;
		data.m_vNormal = waterTrace.plane.normal;
		data.m_flScale = 8.0f;

		// See if we hit slime
		if (waterTrace.contents & CONTENTS_SLIME)
		{
			data.m_fFlags |= FX_WATER_IN_SLIME;
		}

		DispatchEffect("watersplash", data);
#endif
	}

	return true;
}
