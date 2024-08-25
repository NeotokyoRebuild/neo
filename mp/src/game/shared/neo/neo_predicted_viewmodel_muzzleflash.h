#ifndef NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#define NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#ifdef _WIN32
#pragma once
#endif

#include "predicted_viewmodel.h"

#ifdef CLIENT_DLL
#define CNEOPredictedViewModelMuzzleFlash C_NEOPredictedViewModelMuzzleFlash
#define CNEO_Player C_NEO_Player
#endif

class CNEO_Player;
class CNEOPredictedViewModelMuzzleFlash : public CPredictedViewModel
{
	DECLARE_CLASS(CNEOPredictedViewModelMuzzleFlash, CPredictedViewModel);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	CNEOPredictedViewModelMuzzleFlash()
	{
		m_bActive = true;
	}

#ifdef CLIENT_DLL
	ShadowType_t	ShadowCastType() override { return SHADOWS_NONE; };
	RenderGroup_t	GetRenderGroup() override { return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT; };
	int				DrawModel(int flags) override;
	void			ProcessMuzzleFlashEvent() override	{ RemoveEffects(EF_NODRAW);	}
#endif

	void Spawn(void);
	void Precache(void);

	bool	m_bActive;
};

#endif // NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H