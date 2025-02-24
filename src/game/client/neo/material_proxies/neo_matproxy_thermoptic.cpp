#include "cbase.h"
#include "neo_matproxy_thermoptic.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include <KeyValues.h>
#include "mathlib/vmatrix.h"
#include "functionproxy.h"
#include "toolframework_client.h"

#include "c_neo_player.h"
#include "weapon_neobasecombatweapon.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// This is the NT thermoptic proxy name defined in models/player/toc.vmt
#define NEO_TOC_PROXY "PlayerTO"

CNEOTocMaterialProxy::CNEOTocMaterialProxy()
{
	m_pResultVar = NULL;
}

CNEOTocMaterialProxy::~CNEOTocMaterialProxy()
{
}

bool CNEOTocMaterialProxy::Init(IMaterial *pMaterial, KeyValues *pKeyValues)
{
	m_pMaterial = pMaterial;

	char const *pResultVarName = pKeyValues->GetString("resultvar");

	if (!pResultVarName)
	{
		return false;
	}

	bool foundVar;

	m_pResultVar = pMaterial->FindVar(pResultVarName, &foundVar, true);

	return foundVar;
}

ConVar mat_neo_toc_test("mat_neo_toc_test", "0.1", FCVAR_CHEAT);

void CNEOTocMaterialProxy::OnBind(void *pC_BaseEntity)
{
	if (!m_pResultVar)
	{
		return;
	}

	if (!pC_BaseEntity)
	{
		return;
	}

	m_pResultVar->SetFloatValue(mat_neo_toc_test.GetFloat());
	return;

	auto renderable = static_cast<IClientRenderable*>(pC_BaseEntity);
	Assert(renderable);

	// This entity is a valid C_NEO_Player?
	auto pNeoPlayer = dynamic_cast<C_NEO_Player*>(renderable);
	if (pNeoPlayer)
	{
		m_pResultVar->SetIntValue(pNeoPlayer->IsCloaked() ? 1 : 0);

		//DevMsg("Camo for weapon is %s\n", isCloaked ? "enabled" : "disabled");
		return;
	}

	// This entity is a viewmodel?
	auto pVM = dynamic_cast<C_NEOPredictedViewModel*>(renderable);
	if (pVM)
	{
		auto pOwner = static_cast<C_NEO_Player*>(pVM->GetOwner());
		Assert(pOwner);
		m_pResultVar->SetIntValue(pOwner->IsCloaked() ? 1 : 0);
		return;
	}

	// This entity is a weapon?
	auto pWep = dynamic_cast<C_BaseHL2MPCombatWeapon*>(renderable);
	if (pWep)
	{
		auto pOwner = static_cast<C_NEO_Player*>(pWep->GetOwner());
		if (pOwner)
		{
			m_pResultVar->SetIntValue(pOwner->IsCloaked() ? 1 : 0);
		}
		else
		{
			m_pResultVar->SetIntValue(0);
		}
		return;
	}

	// All checks above failed; this is not a player/vm/wep.
	// Probably a mistake if we reach here. If not, add your use case above.
	Assert(false);

	m_pResultVar->SetIntValue(0);
}

IMaterial *CNEOTocMaterialProxy::GetMaterial()
{
	return m_pMaterial;
}

EXPOSE_INTERFACE(CNEOTocMaterialProxy, IMaterialProxy, NEO_TOC_PROXY IMATERIAL_PROXY_INTERFACE_VERSION);
