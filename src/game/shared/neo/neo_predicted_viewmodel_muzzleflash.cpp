#include "cbase.h"
#include "neo_predicted_viewmodel_muzzleflash.h"
#include "model_types.h"

#ifdef GAME_DLL
#include "baseanimating.h"
#endif

#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"
#ifdef CLIENT_DLL
static void RecvProxy_ScaleChangeFlag(const CRecvProxyData* pData, void* pStruct, void* pOut)
{
	CNEOPredictedViewModelMuzzleFlash* pViewModel = ((CNEOPredictedViewModelMuzzleFlash*)pStruct);
	if (pData->m_Value.m_Int != pViewModel->m_bScaleChangeFlag)
	{ // Client side value for m_flModelScale will not be updated correctly, work out the correct scale clientside
		pViewModel->UpdateMuzzleFlashProperties(GetActiveWeapon());
	}
	// Chain through to the default recieve proxy ...
	RecvProxy_IntToEHandle(pData, pStruct, pOut);
}
#endif // CLIENT_DLL

IMPLEMENT_NETWORKCLASS_ALIASED(NEOPredictedViewModelMuzzleFlash, DT_NEOPredictedViewModelMuzzleFlash)

BEGIN_NETWORK_TABLE(CNEOPredictedViewModelMuzzleFlash, DT_NEOPredictedViewModelMuzzleFlash)
#ifndef CLIENT_DLL
SendPropEHandle(SENDINFO_NAME(m_hMoveParent, moveparent)),
SendPropBool(SENDINFO(m_bActive)),
SendPropInt(SENDINFO(m_iAngleZ)),
SendPropInt(SENDINFO(m_iAngleZIncrement)),
SendPropBool(SENDINFO(m_bScaleChangeFlag))
#else
RecvPropInt(RECVINFO_NAME(m_hNetworkMoveParent, moveparent), 0, RecvProxy_IntToMoveParent),
RecvPropBool(RECVINFO(m_bActive)),
RecvPropInt(RECVINFO(m_iAngleZ)),
RecvPropInt(RECVINFO(m_iAngleZIncrement)),
RecvPropEHandle(RECVINFO(m_bScaleChangeFlag), RecvProxy_ScaleChangeFlag)
#endif
END_NETWORK_TABLE()

#ifdef CLIENT_DLL
BEGIN_PREDICTION_DATA(CNEOPredictedViewModelMuzzleFlash)
DEFINE_PRED_FIELD(m_hNetworkMoveParent, FIELD_EHANDLE, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bActive, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iAngleZ, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_iAngleZIncrement, FIELD_INTEGER, FTYPEDESC_INSENDTABLE),
DEFINE_PRED_FIELD(m_bScaleChangeFlag, FIELD_BOOLEAN, FTYPEDESC_INSENDTABLE),
END_PREDICTION_DATA()
#endif

LINK_ENTITY_TO_CLASS(neo_predicted_viewmodel_muzzleflash, CNEOPredictedViewModelMuzzleFlash);

constexpr const char* MUZZLE_FLASH_ENTITY_MODEL = "models/effect/fpmf/fpmf01.mdl";

CNEOPredictedViewModelMuzzleFlash::CNEOPredictedViewModelMuzzleFlash()
{
	m_bActive = true;
	m_iAngleZ = 0;
	m_iAngleZIncrement = -5;
	m_flTimeSwitchOffMuzzleFlash = gpGlobals->curtime;
	m_bScaleChangeFlag = false;
}

CNEOPredictedViewModelMuzzleFlash::~CNEOPredictedViewModelMuzzleFlash()
{
}

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
	AddEffects(EF_NOSHADOW | EF_NORECEIVESHADOW);
	RemoveEffects(EF_NODRAW | EF_BONEMERGE);
}

#ifdef CLIENT_DLL
int CNEOPredictedViewModelMuzzleFlash::DrawModel(int flags)
{
	if (m_flTimeSwitchOffMuzzleFlash > gpGlobals->curtime && m_bActive)
	{
		CBasePlayer* pOwner = ToBasePlayer(GetOwner());
		if (pOwner == NULL) { return -1; }
		CBaseViewModel* vm = pOwner->GetViewModel(0, false);
		if (vm == NULL || !vm->IsVisible()) { return -1; }

		int iAttachment = vm->LookupAttachment("muzzle");
		if (iAttachment < 0) { return -1; }

		Vector localOrigin;
		QAngle localAngle;
		vm->GetAttachment(iAttachment, localOrigin, localAngle);
		UncorrectViewModelAttachment(localOrigin); // Need position of muzzle without fov modifications & viewmodel offset
		m_iAngleZ = (m_iAngleZ + m_iAngleZIncrement) % 360; // NEOTODO (Adam) ? Speed of rotation depends on how often DrawModel() is called. Should this be tied to global time?
		localAngle.z = m_iAngleZ;
		SetAbsOrigin(localOrigin);
		SetAbsAngles(localAngle);

		return BaseClass::DrawModel(flags);
	}
	return -1;
}

void CNEOPredictedViewModelMuzzleFlash::ClientThink()
{
	// Client side entities can only have one think function. This think function will only run when the muzzle flash properties needs to be updated. If more functionality is
	// inserted here consider adding a variable to CNEOPredictedviewModelMuzzleFlash that can be checked here so this is done only when necessary, and remove this comment
	CBasePlayer* pOwner = ToBasePlayer(GetOwner());
	if (pOwner)
	{
		UpdateMuzzleFlashProperties(pOwner->GetActiveWeapon(), false);
	}
}

#endif //CLIENT_DLL

void CNEOPredictedViewModelMuzzleFlash::UpdateMuzzleFlashProperties(CBaseCombatWeapon* pWeapon, bool repeat)
{
	auto neoWep = static_cast<CNEOBaseCombatWeapon*>(pWeapon);
	if (!neoWep)
	{
#ifdef CLIENT_DLL
		// Server likely switched us to a weapon that doesn't yet exist, try updating the muzzle flash properties again next tick, but not recursively
		if (repeat)
		{
			SetNextClientThink(gpGlobals->curtime + gpGlobals->interval_per_tick);
		}
#endif //CLIENT_DLL
		return;
	}

	const auto neoWepBits = neoWep->GetNeoWepBits();
	if (neoWepBits & (NEO_WEP_THROWABLE | NEO_WEP_GHOST | NEO_WEP_KNIFE | NEO_WEP_SUPPRESSED))
	{
		m_bActive = false;
	}
	else if (neoWepBits & (NEO_WEP_PZ | NEO_WEP_TACHI | NEO_WEP_KYLA))
	{
		m_bActive = true;
		m_nSkin = 1;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -100;
		SetModelScale(0.75);
		m_bScaleChangeFlag = !m_bScaleChangeFlag;
	}
	else if (neoWepBits & NEO_WEP_SUPA7)
	{
		m_bActive = true;
		m_nSkin = 1;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -90;
		SetModelScale(2);
		m_bScaleChangeFlag = !m_bScaleChangeFlag;
	}
	else if (neoWepBits & (NEO_WEP_SRM | NEO_WEP_JITTE))
	{
		m_bActive = true;
		m_nSkin = 0;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -90;
		SetModelScale(0.75);
		m_bScaleChangeFlag = !m_bScaleChangeFlag;
	}
	else if (neoWepBits & (NEO_WEP_MX | NEO_WEP_AA13))
	{
		m_bActive = true;
		m_nSkin = 0;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -100;
		SetModelScale(0.6);
		m_bScaleChangeFlag = !m_bScaleChangeFlag;
	}
	else
	{
		m_bActive = true;
		m_nSkin = 0;
		m_iAngleZ = 0;
		m_iAngleZIncrement = -90;
		SetModelScale(0.75);
		m_bScaleChangeFlag = !m_bScaleChangeFlag;
	}
}