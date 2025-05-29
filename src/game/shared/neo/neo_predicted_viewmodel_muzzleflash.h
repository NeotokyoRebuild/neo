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
	CNEOPredictedViewModelMuzzleFlash();
	virtual ~CNEOPredictedViewModelMuzzleFlash();
	void UpdateMuzzleFlashProperties(CBaseCombatWeapon* pWeapon, bool repeat = true);
	virtual void Spawn(void) override;
	virtual void Precache(void) override;
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
	virtual void			ClientThink() override;
#endif

#ifdef CLIENT_DLL
	bool	m_bActive;
	int		m_iAngleZ;
	int		m_iAngleZIncrement;
	bool	m_bScaleChangeFlag;
#else
	CNetworkVar(bool, m_bActive);
	CNetworkVar(int, m_iAngleZ);
	CNetworkVar(int, m_iAngleZIncrement);
	CNetworkVar(bool, m_bScaleChangeFlag);
#endif // CLIENT_DLL
	float	m_flTimeSwitchOffMuzzleFlash;
};

#endif // NEO_PREDICTED_VIEWMODEL_MUZZLEFLASH_H