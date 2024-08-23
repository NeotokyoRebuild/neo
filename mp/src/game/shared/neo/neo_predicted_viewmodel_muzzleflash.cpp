#include "cbase.h"
#include "model_types.h"

#ifdef GAME_DLL
#include "baseanimating.h"
#endif

#include "neo_predicted_viewmodel_muzzleflash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel_muzzleflash, CNEOPredictedViewModelMuzzleFlash);

// Start of our data description for the class
BEGIN_DATADESC(CNEOPredictedViewModelMuzzleFlash)

// Save/restore our active state
DEFINE_FIELD(m_bActive, FIELD_BOOLEAN),
DEFINE_FIELD(m_flNextChangeTime, FIELD_TIME),

END_DATADESC()

#define MUZZLE_FLASH_ENTITY_MODEL "models/effect/fpmf/fpmf01.mdl"

//-----------------------------------------------------------------------------
// Purpose: Precache assets used by the entity
//-----------------------------------------------------------------------------
void CNEOPredictedViewModelMuzzleFlash::Precache(void)
{
	PrecacheModel(MUZZLE_FLASH_ENTITY_MODEL);

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Sets up the entity's initial state
//-----------------------------------------------------------------------------
void CNEOPredictedViewModelMuzzleFlash::Spawn(void)
{
	Precache();
	SetModel(MUZZLE_FLASH_ENTITY_MODEL);
	SetSolid(SOLID_NONE);
	SetMoveType(MOVETYPE_NONE);
	AddEffects(EF_NOSHADOW);
	AddEffects(EF_NORECEIVESHADOW);
}

#ifdef CLIENT_DLL
int CNEOPredictedViewModelMuzzleFlash::DrawModel(int flags)
{
	if (!ShouldMuzzleFlash() || !m_bActive)
	{
		flash[selectedFlash]->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, true);
		return 0;
	}
	DisableMuzzleFlash();
	flash[selectedFlash]->SetMaterialVarFlag(MATERIAL_VAR_NO_DRAW, false);
	int ret = BaseClass::DrawModel(flags);
	return ret;
}
#endif