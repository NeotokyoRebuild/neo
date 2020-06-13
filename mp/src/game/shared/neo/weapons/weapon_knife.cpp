#include "cbase.h"
#include "weapon_knife.h"

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

CWeaponKnife::CWeaponKnife(void)
{
}

void CWeaponKnife::PrimaryAttack(void)
{
	BaseClass::PrimaryAttack();
}

// NEO TODO (Rain): inherit from CNEOBaseCombatWeapon or related,
// so we can do this in the base class Deploy method together
// with other NEO weps.
bool CWeaponKnife::Deploy(void)
{
	const bool ret = BaseClass::Deploy();

	if (ret)
	{
#ifdef DEBUG
		CNEO_Player* pOwner = NULL;
		if (GetOwner())
		{
			pOwner = dynamic_cast<CNEO_Player*>(GetOwner());
			Assert(pOwner);
		}
#else
		auto pOwner = static_cast<CNEO_Player*>(GetOwner());
#endif
		if (pOwner)
		{
			// This wep incurs no speed penalty, so we return class max speed as-is.

			if (pOwner->GetFlags() & FL_DUCKING)
			{
				pOwner->SetMaxSpeed(pOwner->GetCrouchSpeed());
			}
			else if (pOwner->IsWalking())
			{
				pOwner->SetMaxSpeed(pOwner->GetWalkSpeed());
			}
			else if (pOwner->IsSprinting())
			{
				pOwner->SetMaxSpeed(pOwner->GetSprintSpeed());
			}
			else
			{
				pOwner->SetMaxSpeed(pOwner->GetNormSpeed());
			}
		}
	}

	return ret;
}

void CWeaponKnife::Swing(int bIsSecondary)
{
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (!pOwner)
		return;

	// Try a ray
	trace_t traceHit;

	Vector swingStart = pOwner->Weapon_ShootPosition();
	Vector forward;

	pOwner->EyeVectors(&forward, NULL, NULL);

	Vector swingEnd = swingStart + forward * GetRange();
	UTIL_TraceLine(swingStart, swingEnd, MASK_SHOT_HULL, pOwner, COLLISION_GROUP_NONE, &traceHit);
	const Activity nHitActivity = KNIFE_VM_ATTACK_ACT;

#ifndef CLIENT_DLL
	// Like bullets, bludgeon traces have to trace against triggers.
	CTakeDamageInfo triggerInfo(GetOwner(), GetOwner(), GetDamageForActivity(nHitActivity), DMG_SLASH);
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
		Vector testEnd = swingStart + forward * GetRange();

		// See if we happened to hit water
		ImpactWater(swingStart, testEnd);
	}
	else
	{
		Hit(traceHit, nHitActivity);
	}

	// Send the anim
	SendWeaponAnim(nHitActivity);

	pOwner->SetAnimation(PLAYER_ATTACK1);

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

void CWeaponKnife::Hit(trace_t& traceHit, Activity nHitActivity)
{
	Assert(nHitActivity == KNIFE_VM_ATTACK_ACT);

	CBasePlayer* pPlayer = ToBasePlayer(GetOwner());

	//Do view kick
//	AddViewKick();

	CBaseEntity* pHitEntity = traceHit.m_pEnt;

	//Apply damage to a hit target
	if (pHitEntity != NULL)
	{
		// Shouldn't be able to hit self
		Assert(pPlayer != pHitEntity);

		Vector hitDirection;
		pPlayer->EyeVectors(&hitDirection, NULL, NULL);
		VectorNormalize(hitDirection);

#ifndef CLIENT_DLL
		CTakeDamageInfo info(GetOwner(), GetOwner(), GetDamageForActivity(nHitActivity), DMG_SLASH);

		CalculateMeleeDamageForce(&info, hitDirection, traceHit.endpos);

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