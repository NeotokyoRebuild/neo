#include "cbase.h"
#include "model_types.h"

#ifdef GAME_DLL
#include "baseanimating.h"
#endif

#include "neo_predicted_viewmodel_muzzleflash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

IMPLEMENT_NETWORKCLASS_ALIASED(NEOPredictedViewModelMuzzleFlash, DT_NEOPredictedViewModelMuzzleFlash)

BEGIN_NETWORK_TABLE(CNEOPredictedViewModelMuzzleFlash, DT_NEOPredictedViewModelMuzzleFlash)
#ifndef CLIENT_DLL
SendPropEHandle(SENDINFO_NAME(m_hMoveParent, moveparent)),
SendPropBool(SENDINFO(m_bActive)),
SendPropInt(SENDINFO(m_iAngleZ)),
SendPropInt(SENDINFO(m_iAngleZIncrement)),
SendPropFloat(SENDINFO(m_iModelScale)),
#else
RecvPropInt(RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent),
RecvPropBool(RECVINFO(m_bActive)),
RecvPropInt(RECVINFO(m_iAngleZ)),
RecvPropInt(RECVINFO(m_iAngleZIncrement)),
RecvPropFloat(RECVINFO(m_iModelScale)),
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOPredictedViewModelMuzzleFlash)
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel_muzzleflash, CNEOPredictedViewModelMuzzleFlash);

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
	AddEffects(EF_NOSHADOW);
	AddEffects(EF_NORECEIVESHADOW);
	RemoveEffects(EF_NODRAW);
	RemoveEffects(EF_BONEMERGE);
}

#ifdef CLIENT_DLL
int CNEOPredictedViewModelMuzzleFlash::DrawModel(int flags)
{
	if (m_flTimeSwitchOffMuzzleFlash > gpGlobals->curtime && m_bActive)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());
		if (pOwner == NULL)
			return -1;
		CBaseViewModel* vm = pOwner->GetViewModel(0, false);
		if (vm == NULL)
			return -1;

		if (m_iModelScale != GetModelScale())
			SetModelScale(m_iModelScale); // NEOTODO (Adam) model scale is a networked property in cbaseanimating, but for some reason its not being networked when set on the server side when initially spawning/respawning the player, so sometimes muzzle flash size can be incorrect if using SetModelScale in Weapon_Equip()

		int iAttachment = vm->LookupAttachment("muzzle");
		if (iAttachment < 0)
			return -1;

		Vector localOrigin;
		QAngle localAngle;
		vm->GetAttachment(iAttachment, localOrigin, localAngle);
		UncorrectViewModelAttachment(localOrigin); // Need position of muzzle without fov modifications & viewmodel offset
		m_iAngleZ = (m_iAngleZ + m_iAngleZIncrement) % 360; // NEOTODO (Adam) ? Speed of rotation depends on how often DrawModel() is called. Should this be tied to global time?
		localAngle.z = m_iAngleZ;
		SetAbsOrigin(localOrigin);
		SetAbsAngles(localAngle);

		int ret = BaseClass::DrawModel(flags);
		return ret;
	}
	return -1;
}
#endif