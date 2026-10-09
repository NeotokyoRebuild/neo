// NEO HRTF: acoustic scene geometry read straight from a map's .bsp, for occlusion and
// propagation. Only the static world is extracted: model 0's brush faces and displacements, each
// with an acoustic material derived from its texture's $surfaceprop, and the collision models of
// solid static props, with the material of their collision model's surfaceprop.
#pragma once

#include "utlvector.h"

#include "neo_audio_probes.h"
#include "neo_spatializer.h"

struct dplane_t;
struct dnode_t;

class CNeoAudioGeometry
{
public:
	// pszBspPath is relative to the GAME search path, e.g. "maps/nt_rise_ctg.bsp". On failure the
	// geometry is empty and pszErrorOut says why.
	bool LoadFromBsp(const char *pszBspPath, char *pszErrorOut, int errorLen);

	// Valid until the next LoadFromBsp or destruction.
	NeoSpatial::SceneGeometry GetSceneGeometry() const;
	int NumTriangles() const { return m_materialIndices.Count(); }
	int NumMaterials() const { return m_materials.Count(); }

	// The map's BSP tree. Leaves in the 3D skybox's area are not open: nobody listens from there.
	const NeoSpatial::BspTree &GetBspTree() const { return m_tree; }

	// Changes whenever the geometry would, so baked data can be matched to the map it came from.
	uint32 GetGeometryCrc() const;

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
};
