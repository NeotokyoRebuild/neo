// NEO HRTF: acoustic scene geometry read straight from a map's .bsp, for occlusion and
// propagation. The static world is model 0's brush faces and displacements, each with an acoustic
// material derived from its texture's $surfaceprop, and the collision models of solid static props,
// with the material of their collision model's surfaceprop. Brush entities' models (doors, trains,
// func_brush...) are extracted alongside, each on its own, for dynamic geometry.
//
// Audio blockers (func_audio_blocker) are brush entities that only block sound: their brushes are
// turned into faces of their own (their nodraw faces never reach the BSP), with a material that
// passes and reflects nothing, and added to the static scene after everything else.
//
// Shared by the client and the compile-time baker (utils/neo_soundbake), so it depends on no game
// DLL globals: the file system and physics interfaces it reads through are passed in.
#pragma once

#include "utlstring.h"
#include "utlvector.h"

#include "neo_audio_probe_lump.h"
#include "neo_audio_probes.h"
#include "neo_spatializer.h"

struct dplane_t;
struct dnode_t;
class IFileSystem;
class IPhysicsSurfaceProps;
class IPhysicsCollision;

class CNeoAudioGeometry
{
public:
	struct Services
	{
		IFileSystem *pFileSystem; // the map, materials (GAME path) and models
		IPhysicsSurfaceProps *pSurfaceProps; // surfaceprop name -> game material
		IPhysicsCollision *pCollision; // static prop collision models
	};

	// A mesh of its own, in metres in its own frame, with AcousticPreset material indices.
	struct Mesh
	{
		CUtlVector<NeoSpatial::Vec3> vertices;
		CUtlVector<int32> triangles; // 3 vertex indices per triangle, counter-clockwise from the front
		CUtlVector<int32> materialIndices;
		NeoSpatial::Vec3 mins = { 0.0f, 0.0f, 0.0f }; // bounds of the vertices
		NeoSpatial::Vec3 maxs = { 0.0f, 0.0f, 0.0f };

		// Valid until the mesh changes; the materials are the shared AcousticPreset table.
		NeoSpatial::SceneGeometry Geometry() const;
	};

	// A brush entity's model (*1, *2...), in the entity's own frame: brush entity vertices are stored
	// relative to the entity's origin, so its transform places them. Its own entity's classname
	// (from the entity lump) says what it is, e.g. a func_door.
	struct BrushModel
	{
		CUtlString classname; // empty if no entity in the map uses the model
		Mesh mesh;
	};

	// pszBspPath is relative to the GAME search path, e.g. "maps/nt_rise_ctg.bsp", or absolute. On
	// failure the geometry is empty and pszErrorOut says why.
	bool LoadFromBsp(const Services &services, const char *pszBspPath, char *pszErrorOut, int errorLen);

	// The collision model of a studio model (its .phy, solid 0), in model space: what a dynamic prop
	// collides with, as static props do. False (and an empty mesh) if it has none.
	static bool LoadModelCollision(const Services &services, const char *pszModel, Mesh &out);

	// Valid until the next LoadFromBsp or destruction.
	NeoSpatial::SceneGeometry GetSceneGeometry() const;
	// The scene without the audio blockers, which are not surfaces anyone stands on or under: what
	// probes are placed on. Probes are also kept out of the blockers' volumes.
	NeoSpatial::SceneGeometry GetProbeGeometry() const;
	const std::vector<NeoSpatial::ConvexVolume> &GetBlockerVolumes() const { return m_blockerVolumes; }
	int NumTriangles() const { return m_materialIndices.Count(); }
	int NumMaterials() const { return m_materials.Count(); }
	int NumAudioBlockerBrushes() const { return static_cast<int>(m_blockerVolumes.size()); }

	// Indexed by model number; model 0 (the world) is in the static scene instead, so it is empty here.
	int NumBrushModels() const { return m_brushModels.Count(); }
	const BrushModel &GetBrushModel(int model) const { return *m_brushModels[model]; }
	// Hands the brush models over, leaving none here, for dynamic geometry that outlives this object.
	void TakeBrushModels(CUtlVector<BrushModel *> &out) { out.Swap(m_brushModels); }

	// The map's BSP tree. Leaves in the 3D skybox's area are not open: nobody listens from there.
	const NeoSpatial::BspTree &GetBspTree() const { return m_tree; }

	// Changes whenever the geometry would, so baked data can be matched to the map it came from.
	uint32 GetGeometryCrc() const;

	// The shape alone: positions (to the millimetre), triangles and BSP leaves, but not materials.
	// What a bake made at compile time is checked against, on a machine that may classify materials
	// differently or round the last bit of a float differently.
	uint32 GetShapeCrc() const;

	// The probes baked into the map at compile time (NeoSpatial::PROBE_GAME_LUMP_ID), decoded:
	// decompressed, parsed and checked against this geometry. False if the map has none
	// (pszErrorOut empty) or they cannot be used (pszErrorOut says why). batches points into storage.
	bool ReadBakedProbes(NeoSpatial::ProbeLumpHeader &header, NeoSpatial::ProbeSet &probes, CUtlVector<uint8> &storage,
						 const uint8 *&batches, char *pszErrorOut, int errorLen) const;

private:
	void Clear();
	void LoadStaticProps(const char *pszBspPath, const CUtlVector<uint8> &lump, int version);
	bool LoadBspTree(const char *pszBspPath, const CUtlVector<dplane_t> &planes, const CUtlVector<dnode_t> &nodes,
					 const CUtlVector<uint8> &leafBytes, int leafVersion, const CUtlVector<char> &entities);

	NeoSpatial::BspTree m_tree;

	CUtlVector<NeoSpatial::Vec3> m_vertices;
	CUtlVector<int32> m_triangles;
	CUtlVector<int32> m_materialIndices;
	CUtlVector<NeoSpatial::AcousticMaterial> m_materials;
	int m_numProbeTriangles = 0; // the triangles before the audio blockers'
	std::vector<NeoSpatial::ConvexVolume> m_blockerVolumes; // in metres

	CUtlVectorAutoPurge<BrushModel *> m_brushModels;

	CUtlVector<uint8> m_probeLump;
	int m_probeLumpVersion = 0;
};
