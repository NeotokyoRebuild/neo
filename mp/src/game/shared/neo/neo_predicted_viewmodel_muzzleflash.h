#ifndef NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#define NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H
#ifdef _WIN32
#pragma once
#endif

#include "predicted_viewmodel.h"

#ifdef CLIENT_DLL
#define CNEOPredictedViewModelMuzzleFlash C_NEOPredictedViewModelMuzzleFlash
#endif

#define MUZZLE_FLASH_VIEW_MODEL_INDEX 1

class CNEOPredictedViewModelMuzzleFlash : public CPredictedViewModel
{
	DECLARE_CLASS(CNEOPredictedViewModelMuzzleFlash, CPredictedViewModel);
public:
	DECLARE_NETWORKCLASS();
	DECLARE_PREDICTABLE();
	CNEOPredictedViewModelMuzzleFlash()
	{
		m_bActive = true;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -5;
		m_flTimeSwitchOffMuzzleFlash = gpGlobals->curtime;
	}

#ifdef CLIENT_DLL
	virtual ShadowType_t	ShadowCastType() final override { return SHADOWS_NONE; };
	virtual RenderGroup_t	GetRenderGroup() final override { return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT; };
	virtual int				DrawModel(int flags) final override;
	virtual void			ProcessMuzzleFlashEvent() final override
	{
		if (!m_bActive)
			return;
		m_flTimeSwitchOffMuzzleFlash = gpGlobals->curtime + 0.01f;
		BaseClass::ProcessMuzzleFlashEvent();
	}
#endif

	virtual void Spawn(void) override;
	virtual void Precache(void) override;
	bool	m_bActive;
	int		m_iAngleZ;
	int		m_iAngleZIncrement;
	float	m_flTimeSwitchOffMuzzleFlash; // If the server can fire a user's weapon (maybe some kind of server triggered weapon cook off or something), this will need to be networked too.
};

#endif // NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H