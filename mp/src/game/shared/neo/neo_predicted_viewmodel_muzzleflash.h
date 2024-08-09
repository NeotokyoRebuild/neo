#ifndef NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#define NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#ifdef _WIN32
#pragma once
#endif

#ifdef GAME_DLL
#include "baseanimating.h"
#include "baseviewmodel_shared.h"
#else
#include "c_baseviewmodel.h"
#endif

class CNEO_Player;
class CNEOPredictedViewModelMuzzleFlash : public CBaseAnimating
{
public:
	DECLARE_CLASS(CNEOPredictedViewModelMuzzleFlash, CBaseAnimating);
	DECLARE_DATADESC();
	CNEOPredictedViewModelMuzzleFlash()
	{
		m_bActive = false;
		m_flNextChangeTime = 0.f;
#ifdef CLIENT_DLL
		m_hRender = INVALID_CLIENT_RENDER_HANDLE;
		AddToLeafSystem();
#endif
	}

#ifdef CLIENT_DLL
	ShadowType_t	ShadowCastType() override { return SHADOWS_NONE; };
	bool			IsViewModel() const override { return true; };
	RenderGroup_t	GetRenderGroup() override { return RENDER_GROUP_VIEW_MODEL_OPAQUE; };
	bool			ShouldDraw() override { return true; };
	int				DrawModel(int flags) override;
#endif

	void Spawn(void);
	void Precache(void);

	IMaterial* selectedFlash;
	IMaterial* starFlash;
	IMaterial* circleFlash;

private:
	bool	m_bActive;
	float	m_flNextChangeTime;
};

#endif // NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H