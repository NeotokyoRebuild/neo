#include "cbase.h"
#include "neo_matproxy_thermoptic.h"

#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include <KeyValues.h>
#include "mathlib/vmatrix.h"
#include "functionproxy.h"
#include "toolframework_client.h"

#include "c_neo_player.h"

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
	char const *pResultVarName = pKeyValues->GetString("resultvar");

	if (!pResultVarName)
	{
		return false;
	}

	bool foundVar;

	m_pResultVar = pMaterial->FindVar(pResultVarName, &foundVar, true);

	return foundVar;
}

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

	auto renderable = static_cast<IClientRenderable*>(pC_BaseEntity);


	void *entity = dynamic_cast<C_NEO_Player*>(renderable);

	// This entity is a valid C_NEO_Player
	if (entity)
	{
		auto player = reinterpret_cast<C_NEO_Player*>(entity);

		const bool isCloaked = player->IsCloaked();

		m_pResultVar->SetIntValue(isCloaked ? 1 : 0);

		DevMsg("Camo for weapon is %s\n", isCloaked ? "enabled" : "disabled");
	}
	// It wasn't a player, see if it's a weapon.
	else
	{
		entity = dynamic_cast<C_BaseCombatWeapon*>(renderable);

		// It's neither a weapon nor a player.
		// We don't know what it is, but abort! This shouldn't happen.
		if (!entity)
		{
			Assert(false);
			return;
		}

		//auto weapon = reinterpret_cast<C_BaseCombatWeapon*>(entity);
	}
}

IMaterial *CNEOTocMaterialProxy::GetMaterial()
{
	return NULL; // TODO
}

EXPOSE_INTERFACE(CNEOTocMaterialProxy, IMaterialProxy, NEO_TOC_PROXY IMATERIAL_PROXY_INTERFACE_VERSION);
