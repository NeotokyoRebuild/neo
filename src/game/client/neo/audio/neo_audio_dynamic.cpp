#include "cbase.h"
#include "neo_audio_dynamic.h"

#include "cliententitylist.h"
#include "model_types.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{

constexpr float kDynamicMetresPerUnit = 0.0254f; // as neo_audio_geometry.cpp

// A model smaller than this every way barely occludes anything. The smallest movers in the maps are
// invisible trains carrying butterflies and fish around.
constexpr float kDynamicMinSizeMetres = 0.5f;

const char *const kDynamicMoverClassnames[] = { "func_door", "func_door_rotating", "func_movelinear", "func_tracktrain" };

// Props have no classname on the client, only their network class: prop_door_rotating's is
// CBasePropDoor, and the props the All scope adds are these.
const char kDynamicPropDoorNetworkClass[] = "CBasePropDoor";
const char *const kDynamicPropNetworkClasses[] = { "CDynamicProp", "CPhysicsProp", "CPhysicsPropMultiplayer", "CBreakableProp" };

bool DynamicIsNetworkClass(C_BaseEntity *pEntity, const char *pszName)
{
	const ClientClass *pClass = pEntity->GetClientClass();
	return pClass && pClass->m_pNetworkName && V_strcmp(pClass->m_pNetworkName, pszName) == 0;
}

bool DynamicLargeEnough(const CNeoAudioGeometry::Mesh &mesh)
{
	return !mesh.materialIndices.IsEmpty()
		&& Max(Max(mesh.maxs.x - mesh.mins.x, mesh.maxs.y - mesh.mins.y), mesh.maxs.z - mesh.mins.z) >= kDynamicMinSizeMetres;
}

NeoSpatial::Transform DynamicToTransform(const matrix3x4_t &matrix)
{
	NeoSpatial::Transform transform;
	for (int i = 0; i < 3; ++i)
	{
		for (int j = 0; j < 3; ++j)
		{
			transform.rotation[i][j] = matrix[i][j];
		}
	}
	transform.translation = { matrix[0][3] * kDynamicMetresPerUnit, matrix[1][3] * kDynamicMetresPerUnit,
							  matrix[2][3] * kDynamicMetresPerUnit };
	return transform;
}

} // namespace

CNeoAudioDynamicGeometry::CNeoAudioDynamicGeometry()
	: m_props(DefLessFunc(const model_t *))
	, m_models(DefLessFunc(const CNeoAudioGeometry::Mesh *))
	, m_instances(DefLessFunc(unsigned long))
{
}

void CNeoAudioDynamicGeometry::Init(CNeoAudioGeometry &geometry, const CNeoAudioGeometry::Services &services)
{
	Clear();
	m_services = services;
	geometry.TakeBrushModels(m_brushModels);
	m_brushes.SetCount(m_brushModels.Count());
	for (int m = 0; m < m_brushModels.Count(); ++m)
	{
		m_brushes[m].bUsable = DynamicLargeEnough(m_brushModels[m]->mesh);
		for (const char *pszClassname : kDynamicMoverClassnames)
		{
			m_brushes[m].bMover |= V_stricmp(m_brushModels[m]->classname.Get(), pszClassname) == 0;
		}
	}
}

void CNeoAudioDynamicGeometry::Clear()
{
	m_brushModels.PurgeAndDeleteElements();
	m_brushes.Purge();
	m_props.Purge();
	m_propMeshes.PurgeAndDeleteElements();
	m_models.Purge();
	m_instances.Purge();
}

void CNeoAudioDynamicGeometry::Gather(Scope scope, CUtlVector<Placement> &placementsOut)
{
	placementsOut.RemoveAll();
	if (scope == Scope::Off)
	{
		return;
	}

	for (C_BaseEntity *pEntity = ClientEntityList().FirstBaseEntity(); pEntity; pEntity = ClientEntityList().NextBaseEntity(pEntity))
	{
		const CNeoAudioGeometry::Mesh *pMesh = MeshFor(pEntity, scope);
		if (pMesh)
		{
			// Dormant entities keep their last known place: the server stops sending them, but a door
			// that went out of view is still there.
			placementsOut.AddToTail({ static_cast<unsigned long>(pEntity->GetRefEHandle().ToInt()), HierarchyGroup(pEntity), pMesh,
									  pEntity->EntityToWorldTransform() });
		}
	}
}

void CNeoAudioDynamicGeometry::Apply(NeoSpatial::ISpatializer *pSpatializer, const CUtlVector<Placement> &placements)
{
	FOR_EACH_MAP_FAST(m_instances, i)
	{
		m_instances[i].bSeen = false;
	}

	for (const Placement &placement : placements)
	{
		unsigned short modelIndex = m_models.Find(placement.pMesh);
		if (modelIndex == m_models.InvalidIndex())
		{
			modelIndex = m_models.Insert(placement.pMesh, pSpatializer->CreateDynamicModel(placement.pMesh->Geometry()));
		}
		const NeoSpatial::DynamicModel model = m_models[modelIndex];
		if (model == NeoSpatial::INVALID_DYNAMIC_MODEL)
		{
			continue;
		}

		unsigned short index = m_instances.Find(placement.key);
		if (index != m_instances.InvalidIndex()
			&& (m_instances[index].pMesh != placement.pMesh || m_instances[index].group != placement.group))
		{
			// The entity changed model, or was parented to something else.
			pSpatializer->ReleaseDynamicInstance(m_instances[index].handle);
			m_instances.RemoveAt(index);
			index = m_instances.InvalidIndex();
		}
		if (index == m_instances.InvalidIndex())
		{
			const NeoSpatial::DynamicInstance handle =
				pSpatializer->CreateDynamicInstance(model, DynamicToTransform(placement.transform), placement.group);
			if (handle != NeoSpatial::INVALID_DYNAMIC_INSTANCE)
			{
				m_instances.Insert(placement.key, { handle, placement.pMesh, placement.group, placement.transform, true });
			}
			continue;
		}

		Instance &instance = m_instances[index];
		instance.bSeen = true;
		if (V_memcmp(&instance.transform, &placement.transform, sizeof(placement.transform)) != 0)
		{
			pSpatializer->MoveDynamicInstance(instance.handle, DynamicToTransform(placement.transform));
			instance.transform = placement.transform;
		}
	}

	// Entities gone, or no longer wanted.
	for (unsigned short i = m_instances.FirstInorder(); i != m_instances.InvalidIndex();)
	{
		const unsigned short next = m_instances.NextInorder(i);
		if (!m_instances[i].bSeen)
		{
			pSpatializer->ReleaseDynamicInstance(m_instances[i].handle);
			m_instances.RemoveAt(i);
		}
		i = next;
	}
}

const CNeoAudioGeometry::Mesh *CNeoAudioDynamicGeometry::MeshFor(C_BaseEntity *pEntity, Scope scope)
{
	const model_t *pModel = pEntity->GetModel();
	if (!pModel || pEntity->IsPlayer() || pEntity->IsBaseCombatWeapon()
		|| (pEntity->IsEffectActive(EF_NODRAW) && !pEntity->IsSolid()))
	{
		return nullptr;
	}

	// Movers count whether solid or not (a door can be set passable, and still blocks sound), and so
	// does everything riding on them.
	bool bWanted = IsMover(pEntity);
	for (C_BaseEntity *pParent = pEntity->GetMoveParent(); pParent && !bWanted; pParent = pParent->GetMoveParent())
	{
		bWanted = IsMover(pParent);
	}
	const int brush = BrushModelIndex(pEntity);
	if (!bWanted && scope == Scope::All && pEntity->IsSolid())
	{
		bWanted = brush >= 0;
		for (const char *pszName : kDynamicPropNetworkClasses)
		{
			bWanted = bWanted || DynamicIsNetworkClass(pEntity, pszName);
		}
	}
	if (!bWanted)
	{
		return nullptr;
	}

	if (brush >= 0)
	{
		return m_brushes[brush].bUsable ? &m_brushModels[brush]->mesh : nullptr;
	}

	if (modelinfo->GetModelType(pModel) != mod_studio)
	{
		return nullptr;
	}
	unsigned short index = m_props.Find(pModel);
	if (index == m_props.InvalidIndex())
	{
		CNeoAudioGeometry::Mesh *pMesh = new CNeoAudioGeometry::Mesh;
		if (CNeoAudioGeometry::LoadModelCollision(m_services, modelinfo->GetModelName(pModel), *pMesh) && DynamicLargeEnough(*pMesh))
		{
			m_propMeshes.AddToTail(pMesh);
		}
		else
		{
			delete pMesh;
			pMesh = nullptr;
		}
		index = m_props.Insert(pModel, pMesh);
	}
	return m_props[index];
}

uint32 CNeoAudioDynamicGeometry::SoundGroup(int entIndex)
{
	C_BaseEntity *pEntity = (entIndex > 0) ? ClientEntityList().GetBaseEntity(entIndex) : nullptr;
	return pEntity ? HierarchyGroup(pEntity) : NeoSpatial::NO_DYNAMIC_GROUP;
}

uint32 CNeoAudioDynamicGeometry::HierarchyGroup(C_BaseEntity *pEntity)
{
	C_BaseEntity *pRoot = pEntity;
	while (C_BaseEntity *pParent = pRoot->GetMoveParent())
	{
		pRoot = pParent;
	}
	// The handle holds the entity index in its low bits, so only the world's can be 0 (no group).
	return static_cast<uint32>(pRoot->GetRefEHandle().ToInt());
}

bool CNeoAudioDynamicGeometry::IsMover(C_BaseEntity *pEntity) const
{
	const int brush = BrushModelIndex(pEntity);
	return (brush >= 0) ? m_brushes[brush].bMover : DynamicIsNetworkClass(pEntity, kDynamicPropDoorNetworkClass);
}

int CNeoAudioDynamicGeometry::BrushModelIndex(C_BaseEntity *pEntity) const
{
	const model_t *pModel = pEntity->GetModel();
	if (!pModel || modelinfo->GetModelType(pModel) != mod_brush)
	{
		return -1;
	}
	// Brush entity models are named after their number in the map, "*1", "*2"...
	int index = -1;
	const char *pszName = modelinfo->GetModelName(pModel);
	return (pszName && sscanf(pszName, "*%d", &index) == 1 && index > 0 && index < m_brushes.Count()) ? index : -1;
}
