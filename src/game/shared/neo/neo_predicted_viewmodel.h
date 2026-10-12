#ifndef NEO_PREDICTED_VIEWMODEL_H
#define NEO_PREDICTED_VIEWMODEL_H
#ifdef _WIN32
#pragma once
#endif

#include "predicted_viewmodel.h"
#include "neo_ads.h"

#ifdef CLIENT_DLL
//#include "clienteffectprecachesystem.h"
//#include <engine/IClientLeafSystem.h>
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
	DECLARE_PREDICTABLE();

	CNEOPredictedViewModel(void);
	virtual ~CNEOPredictedViewModel( void );

	virtual void CalcViewModelView(CBasePlayer *pOwner,
		const Vector& eyePosition, const QAngle& eyeAngles);

	float freeRoomForLean(float leanAmount, CNEO_Player *player);

	// Returns the amount of pelvis rotation expected of player bone controller.
	float lean(CNEO_Player *player);

#ifdef CLIENT_DLL
	virtual void PostDataUpdate(DataUpdateType_t updateType) override;
	virtual void ClientThink() override;
	virtual void StandardBlendingRules(CStudioHdr *hdr, Vector pos[], Quaternion q[], float currentTime, int boneMask) override;

	virtual int DrawModel(int flags);
	int DrawGun(int flags);
	virtual void ProcessMuzzleFlashEvent() final override;

	virtual RenderGroup_t GetRenderGroup() override;
	virtual bool UsesPowerOfTwoFrameBufferTexture() override;
#endif

#ifdef CLIENT_DLL
	float GetLeanInterp()
	{
		return GetInterpolationAmount(m_LagAnglesHistory.GetType());
	}
#endif

#ifdef CLIENT_DLL
	void DrawRenderToTextureDebugInfo(IClientRenderable* pRenderable,
		const Vector& mins, const Vector& maxs, const Vector &rgbColor,
		const char *message = "", const Vector &vecOrigin = vec3_origin);
#endif

#ifdef GAME_DLL
	CNetworkVar(float, m_flLeanRatio);
#else
	float m_flLeanRatio;
	CInterpolatedVar<float> m_iv_flLeanRatio;
#endif
	float m_flGunPush = 0.f;
	float m_flGunPushLastChangeTime = 0.f;

	// 0 at the hip, 1 fully on the sights (eased); stays 0 while ADS is off or the weapon has not opted in.
	float GetAdsBlend() const { return m_flAdsBlend; }

private:
	float m_flAdsBlend = 0.f;
#ifdef CLIENT_DLL
	NeoAdsRestPose m_adsRest;		// idle first frame
	NeoAdsRestPose m_adsSettled;	// current fire animation's last frame
#endif
	float m_flStartAimingChange;
	bool m_bViewAim;
	Vector m_vOffset;
	QAngle m_angOffset;
};

#endif // NEO_PREDICTED_VIEWMODEL_H
