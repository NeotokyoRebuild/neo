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
	}

	const Vector& ApplySpread(const Vector& vecSpread, float bias = 1.0);

private:
	float GetVerticalRecoil() const;

private:
	int m_iNumBullet;

	CNEO_Player* m_pPlayer;

	CNEOBaseCombatWeapon* m_pWeapon;
};

extern ConVar sv_neo_recoil_capbullets;
extern ConVar sv_neo_recoil_capscale;
extern ConVar sv_neo_recoil_viewfollow_scale;

inline const Vector &CNEOShotManipulator::ApplySpread(const Vector& vecSpread, float bias)
{
	QAngle myangles;
	VectorAngles(m_vecShotDirection, myangles);

	QAngle worldangles = TransformAnglesToWorldSpace(myangles, m_pPlayer->EntityToWorldTransform());

	matrix3x4_t attachedToWorld;
	AngleMatrix(worldangles, attachedToWorld);

	VMatrix vm = attachedToWorld;
	MatrixBuildRotationAboutAxis(vm, m_vecRight, GetVerticalRecoil());

	m_vecShotDirection = vm.ApplyRotation(m_vecShotDirection);

	CShotManipulator::ApplySpread(vecSpread, bias);

	return GetResult();
}
#endif