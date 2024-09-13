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

#define MUZZLE_FLASH_VIEW_MODEL_INDEX 1

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
		m_iAngleZ = 0;
		m_iAngleZIncrement = -5;
		m_flTimeSwitchOffMuzzleFlash = gpGlobals->curtime;
		m_iModelScale = 1;
	}

#ifdef CLIENT_DLL
	ShadowType_t	ShadowCastType() override { return SHADOWS_NONE; };
	RenderGroup_t	GetRenderGroup() override { return RENDER_GROUP_VIEW_MODEL_TRANSLUCENT; };
	int				DrawModel(int flags) override;
	void			ProcessMuzzleFlashEvent() override
	{
		if (!m_bActive)
			return;
		m_flTimeSwitchOffMuzzleFlash = gpGlobals->curtime + 0.01f;
		BaseClass::ProcessMuzzleFlashEvent();
	}
#endif

	void Spawn(void);
	void Precache(void);
	bool	m_bActive;
	int		m_iAngleZ;
	int		m_iAngleZIncrement;
	float	m_iModelScale;
	float	m_flTimeSwitchOffMuzzleFlash; // If the server can fire a user's weapon (maybe some kind of server triggered weapon cook off or something), this will need to be networked too.
};

#endif // NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H