// NEO HRTF: dynamic geometry in the acoustic scene. Doors, trains and whatever rides on them are
// left out of the static scene (and so out of the bake), and kept in the spatializer's scene instead
// as instances that follow their entities, found among the client's entities every frame. A closed
// door then occludes the sounds behind it; the baked paths through its doorway are left as they are.
//
// Entities are read on the game thread (Gather) and the scene is changed on the simulation thread
// (Apply), so the two halves only share the placements Gather writes and the meshes they point to.
#pragma once

#include "mathlib/mathlib.h"
#include "utlmap.h"
#include "utlvector.h"

#include "neo_audio_geometry.h"
#include "neo_spatializer.h"

class C_BaseEntity;
struct model_t;

class CNeoAudioDynamicGeometry
{
public:
	enum class Scope
	{
		Off,
		// Brush entities that move (func_door, func_door_rotating, func_movelinear, func_tracktrain),
		// prop_door_rotating, and the brush entities and props parented to any of them.
		Movers,
		// Also every other solid brush entity, and every solid dynamic, physics or breakable prop.
		All,
	};

	// One entity's geometry as it is now.
	struct Placement
	{
		unsigned long key; // the entity's handle (EHANDLE::ToInt)
		uint32 group; // HierarchyGroup
		const CNeoAudioGeometry::Mesh *pMesh; // owned by this object, unchanged until Clear
		matrix3x4_t transform;
	};

	CNeoAudioDynamicGeometry();

	// The map's brush models, taken from the geometry the static scene was built from. Nothing goes
	// to the backend until an entity using a model turns up.
	void Init(CNeoAudioGeometry &geometry, const CNeoAudioGeometry::Services &services);

	// Forgets every model and instance without touching the backend: for when the backend has
	// dropped them itself (a new scene, or the spatializer gone). Neither Gather nor Apply may run.
	void Clear();

	// Game thread: the geometry of every entity in scope, replacing placementsOut.
	void Gather(Scope scope, CUtlVector<Placement> &placementsOut);

	// Simulation thread: brings the backend's instances in line with placements from Gather.
	void Apply(NeoSpatial::ISpatializer *pSpatializer, const CUtlVector<Placement> &placements);

	// Simulation thread.
	int NumInstances() const { return m_instances.Count(); }

	// The group of dynamic geometry a sound played by entity `entIndex` belongs to, and so is not
	// occluded by: its whole parent hierarchy, named by the topmost entity. A door's own sounds,
	// and those of anything riding a train, ignore the door or the train and everything on it.
	// NO_DYNAMIC_GROUP for a sound with no entity. Game thread.
	static uint32 SoundGroup(int entIndex);

private:
	static uint32 HierarchyGroup(C_BaseEntity *pEntity);

	// Game thread.
	struct BrushEntry
	{
		bool bUsable = false; // has geometry large enough to matter
		bool bMover = false; // the map's entity using it is one of the movers
	};

	// The mesh an entity is to be placed with, or null if it does not qualify for the scope, or its
	// model is too small to matter or has no geometry.
	const CNeoAudioGeometry::Mesh *MeshFor(C_BaseEntity *pEntity, Scope scope);
	bool IsMover(C_BaseEntity *pEntity) const;
	int BrushModelIndex(C_BaseEntity *pEntity) const; // -1 if it has no brush model of this map

	CNeoAudioGeometry::Services m_services = {};
	CUtlVectorAutoPurge<CNeoAudioGeometry::BrushModel *> m_brushModels; // by model number
	CUtlVector<BrushEntry> m_brushes; // by model number
	CUtlMap<const model_t *, CNeoAudioGeometry::Mesh *> m_props; // null = no usable collision
	CUtlVectorAutoPurge<CNeoAudioGeometry::Mesh *> m_propMeshes; // what m_props points to

	// Simulation thread.
	struct Instance
	{
		NeoSpatial::DynamicInstance handle;
		const CNeoAudioGeometry::Mesh *pMesh;
		uint32 group; // HierarchyGroup when created
		matrix3x4_t transform; // as last sent to the backend
		bool bSeen; // still wanted this update
	};

	CUtlMap<const CNeoAudioGeometry::Mesh *, NeoSpatial::DynamicModel> m_models; // INVALID = the backend refused it
	CUtlMap<unsigned long, Instance> m_instances; // by entity handle
};
