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

class CNEOShotManipulator : public CShotManipulator {
public:
	CNEOShotManipulator(CNEO_Player *player, int numBullet, const Vector& vecForward) : CShotManipulator(vecForward)
	{
		m_iNumBullet = numBullet;
		m_pPlayer = player;
	}

	const Vector& ApplySpread(const Vector& vecSpread, float bias = 1.0);

private:
	int m_iNumBullet;

	CNEO_Player* m_pPlayer;
};

ConVar sv_neo_recoil_capbullets("sv_neo_recoil_capbullets", "10", FCVAR_CHEAT | FCVAR_REPLICATED, "Max Z recoil", true, 0.0f, false, 0.0f);
ConVar sv_neo_recoil_capscale("sv_neo_recoil_capscale", "1", FCVAR_CHEAT | FCVAR_REPLICATED, "Max Z recoil", true, 0.0f, false, 0.0f);

#ifndef GAME_DLL
AngularImpulse WorldToLocalRotation(const VMatrix& localToWorld, const Vector& worldAxis, float rotation)
{
	// fix axes of rotation to match axes of vector
	Vector rot = worldAxis * rotation;
	// since the matrix maps local to world, do a transpose rotation to get world to local
	AngularImpulse ang = localToWorld.VMul3x3Transpose(rot);

	return ang;
}
#endif

inline const Vector &CNEOShotManipulator::ApplySpread(const Vector& vecSpread, float bias)
{
	const float recoilAmount = sv_neo_recoil_capscale.GetFloat() * Min(m_iNumBullet, sv_neo_recoil_capbullets.GetInt()) / sv_neo_recoil_capbullets.GetFloat();
	
	QAngle myangles;
	VectorAngles(m_vecShotDirection, myangles);

	QAngle worldangles = TransformAnglesToWorldSpace(myangles, m_pPlayer->EntityToWorldTransform());

	matrix3x4_t attachedToWorld;
	AngleMatrix(worldangles, attachedToWorld);

	VMatrix vm = attachedToWorld;
	MatrixBuildRotationAboutAxis(vm, m_vecRight, recoilAmount);

	m_vecShotDirection = vm.ApplyRotation(m_vecShotDirection);

	CShotManipulator::ApplySpread(vecSpread, bias);

	return GetResult();
}

#endif