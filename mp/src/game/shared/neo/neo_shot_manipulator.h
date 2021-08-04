#ifndef NEO_SHOT_MANIPULATOR_H
#define NEO_SHOT_MANIPULATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "shot_manipulator.h"

#ifdef CLIENT_DLL
#include "c_neo_player.h"
#else
#include "neo_player.h"
#endif

class CNEOBaseCombatWeapon;

class CNEOShotManipulator : public CShotManipulator {
public:
	CNEOShotManipulator(int numBullet, const Vector& vecForward, CNEO_Player* player, CNEOBaseCombatWeapon* neoWep = NULL)
		: CShotManipulator(vecForward)
	{
		Assert(player);
		m_pPlayer = player;

		m_pWeapon = neoWep; // we're ok with a nullptr here (ie. not an NT gun); just handle as a non-recoiled weapon then.

		m_iNumBullet = numBullet;

		m_vecRecoilDirection = vec3_invalid;
	}

	const Vector& ApplySpread(const Vector& vecSpread, float bias = 1.0);
	const Vector& ApplyRecoil(const Vector& vecSpread, float bias = 1.0);
	
	const Vector& GetRecoilDirection() const { return m_vecRecoilDirection; }

	void GetSpreadAndRecoilDirections(Vector& outSpreadDir, Vector& outRecoilDir) const
	{
		outSpreadDir = GetShotDirection();
		outRecoilDir = GetRecoilDirection();
	}

private:
	float GetVerticalRecoil() const;

private:
	int m_iNumBullet;

	Vector m_vecRecoilDirection;

	CNEO_Player* m_pPlayer;

	CNEOBaseCombatWeapon* m_pWeapon;
};

extern ConVar sv_neo_recoil_capbullets;
extern ConVar sv_neo_recoil_capscale;
extern ConVar sv_neo_recoil_viewfollow_scale;

inline const Vector &CNEOShotManipulator::ApplySpread(const Vector& vecSpread, float bias)
{
	VectorAdd(ApplyRecoil(vecSpread, bias), CShotManipulator::ApplySpread(vecSpread, bias), m_vecShotDirection);
	return GetShotDirection();
}

inline const Vector& CNEOShotManipulator::ApplyRecoil(const Vector& vecSpread, float bias)
{
	QAngle myangles;
	VectorAngles(m_vecShotDirection, myangles);

	QAngle worldangles = TransformAnglesToWorldSpace(myangles, m_pPlayer->EntityToWorldTransform());

	matrix3x4_t attachedToWorld;
	AngleMatrix(worldangles, attachedToWorld);

	VMatrix vm = attachedToWorld;
	MatrixBuildRotationAboutAxis(vm, m_vecRight, GetVerticalRecoil());

	m_vecRecoilDirection = vm.ApplyRotation(m_vecShotDirection) * bias;
	return m_vecRecoilDirection;
}
#endif