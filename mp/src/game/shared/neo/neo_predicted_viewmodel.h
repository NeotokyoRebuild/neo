#ifndef NEO_PREDICTED_VIEWMODEL_H
#define NEO_PREDICTED_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predicted_viewmodel.h"
#ifdef CLIENT_DLL
#include "clienteffectprecachesystem.h"
#endif

#ifdef CLIENT_DLL
#define CNEOPredictedViewModel C_NEOPredictedViewModel
#define CNEO_Player C_NEO_Player
#endif

class CNEO_Player;

class CNEOPredictedViewModel : public CPredictedViewModel
{
	DECLARE_CLASS(CNEOPredictedViewModel, CPredictedViewModel);
public:
	DECLARE_NETWORKCLASS();

	CNEOPredictedViewModel(void);
	virtual ~CNEOPredictedViewModel( void );

	virtual void CalcViewModelView(CBasePlayer *pOwner,
		const Vector& eyePosition, const QAngle& eyeAngles);

	virtual void CalcViewModelLag(Vector& origin, QAngle& angles,
		QAngle& original_angles);

	float freeRoomForLean(float leanAmount, CNEO_Player *player);
	float calculateLeanAngle(float freeRoom, CNEO_Player *player);
	void lean(CNEO_Player *player);

	virtual void SetWeaponModel(const char* pszModelname,
		CBaseCombatWeapon* weapon);

#ifdef CLIENT_DLL
	float GetLeanInterp()
	{
		return GetInterpolationAmount(m_LagAnglesHistory.GetType());
	}
#endif

#ifdef CLIENT_DLL
	inline void DrawRenderToTextureDebugInfo(IClientRenderable* pRenderable,
		const Vector& mins, const Vector& maxs, const Vector &rgbColor,
		const char *message = "", const Vector &vecOrigin = vec3_origin);
#endif

private:
	float Yprevious;
};

#endif // NEO_PREDICTED_VIEWMODEL_H