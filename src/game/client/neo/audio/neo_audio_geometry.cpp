// No cbase.h: the compile-time baker builds this file too. In the client the PCH still comes first.
#include "neo_audio_geometry.h"

#include "bspfile.h"
#include "checksum_crc.h"
#include "const.h"
#include "decals.h"
#include "filesystem.h"
#include "gamebspfile.h"
#include "KeyValues.h"
#include "mathlib/mathlib.h"
#include "phyfile.h"
#include "studio.h"
#include "tier0/dbg.h"
#include "tier1/lzmaDecoder.h"
#include "tier1/strtools.h"
#include "tier1/utlbuffer.h"
#include "vcollide.h"
#include "vcollide_parse.h"
#include "vphysics_interface.h"

#include "neo_audio_probe_lump.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

namespace
{

// What LoadFromBsp reads through, set for its duration (the helpers below are free functions).
IFileSystem *s_pFileSystem = nullptr;
IPhysicsSurfaceProps *s_pSurfaceProps = nullptr;
IPhysicsCollision *s_pCollision = nullptr;

// GetShapeCrc's resolution: coarse enough that two builds' float rounding cannot differ by a step.
constexpr float kGeometryShapeStepsPerMetre = 1000.0f;

constexpr float kGeometryMetresPerUnit = 0.0254f; // 1 Source unit = 1 inch

// Faces that are not solid surfaces: sky (sound escapes there, so leaving it open is the right
// acoustic answer too), tool brushes and anything never drawn.
constexpr int kGeometryIgnoredSurfFlags = SURF_SKY2D | SURF_SKY | SURF_NODRAW | SURF_TRIGGER | SURF_HINT | SURF_SKIP;

// A lump only ever holds a few megabytes; this bounds what a corrupt header can make us allocate.
constexpr int kGeometryMaxLumpBytes = 256 * 1024 * 1024;
constexpr int kGeometryMinDispPower = 2;
constexpr int kGeometryMaxDispPower = 4;
constexpr int kGeometryMaxPatchDepth = 4; // patch materials including patch materials
constexpr int kGeometryWorldModel = 0;

// The structs are read straight from the file, so their sizes must match the on-disk records.
COMPILE_TIME_ASSERT(sizeof(dmodel_t) == 48);
COMPILE_TIME_ASSERT(sizeof(dface_t) == 56);
COMPILE_TIME_ASSERT(sizeof(dedge_t) == 4);
COMPILE_TIME_ASSERT(sizeof(dvertex_t) == 12);
COMPILE_TIME_ASSERT(sizeof(texinfo_t) == 72);
COMPILE_TIME_ASSERT(sizeof(dtexdata_t) == 32);
COMPILE_TIME_ASSERT(sizeof(ddispinfo_t) == 176);
COMPILE_TIME_ASSERT(sizeof(CDispVert) == 20);
COMPILE_TIME_ASSERT(sizeof(dplane_t) == 20);
COMPILE_TIME_ASSERT(sizeof(dnode_t) == 32);
COMPILE_TIME_ASSERT(sizeof(dleaf_t) == 32);
COMPILE_TIME_ASSERT(sizeof(dleaf_version_0_t) == 56);

COMPILE_TIME_ASSERT(sizeof(dgamelumpheader_t) == 4);
COMPILE_TIME_ASSERT(sizeof(dgamelump_t) == 16);
COMPILE_TIME_ASSERT(sizeof(StaticPropDictLump_t) == STATIC_PROP_NAME_LENGTH);
COMPILE_TIME_ASSERT(sizeof(StaticPropLeafLump_t) == 2);
COMPILE_TIME_ASSERT(sizeof(StaticPropLumpV4_t) == 56);
COMPILE_TIME_ASSERT(sizeof(phyheader_t) == 16);

// Every static prop lump version starts with the version 4 record's fields up to m_Solid, which is
// all that is read here; later versions append (or rearrange after it). Version 11 appends a
// uniform scale as the record's last field.
constexpr int kGeometryMinStaticPropVersion = 4;
constexpr int kGeometryStaticPropScaleVersion = 11;
constexpr int kGeometryStudioId = ('T' << 24) + ('S' << 16) + ('D' << 8) + 'I'; // "IDST", as studiomdl writes it

constexpr char kGeometrySkyCameraClass[] = "sky_camera";
constexpr int kGeometryMaxEntityBlock = 4096;
constexpr int kGeometryMaxEntityValue = 256;

// Steam Audio's reference materials (phonon.h, IPLMaterial), indexed by AcousticPreset.
enum AcousticPreset
{
	ACOUSTIC_GENERIC,
	ACOUSTIC_CONCRETE,
	ACOUSTIC_CERAMIC,
	ACOUSTIC_GRAVEL,
	ACOUSTIC_CARPET,
	ACOUSTIC_GLASS,
	ACOUSTIC_PLASTER,
	ACOUSTIC_WOOD,
	ACOUSTIC_METAL,
	ACOUSTIC_GRATE,

	ACOUSTIC_PRESET_COUNT
};

const NeoSpatial::AcousticMaterial kAcousticPresets[ACOUSTIC_PRESET_COUNT] = {
	{ { 0.10f, 0.20f, 0.30f }, 0.05f, { 0.100f, 0.050f, 0.030f } }, // generic
	{ { 0.05f, 0.07f, 0.08f }, 0.05f, { 0.015f, 0.002f, 0.001f } }, // concrete
	{ { 0.01f, 0.02f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } }, // ceramic
	{ { 0.60f, 0.70f, 0.80f }, 0.05f, { 0.031f, 0.012f, 0.008f } }, // gravel
	{ { 0.24f, 0.69f, 0.73f }, 0.05f, { 0.020f, 0.005f, 0.003f } }, // carpet
	{ { 0.06f, 0.03f, 0.02f }, 0.05f, { 0.060f, 0.044f, 0.011f } }, // glass
	{ { 0.12f, 0.06f, 0.04f }, 0.05f, { 0.056f, 0.056f, 0.004f } }, // plaster
	{ { 0.11f, 0.07f, 0.06f }, 0.05f, { 0.070f, 0.014f, 0.005f } }, // wood
	{ { 0.20f, 0.07f, 0.06f }, 0.05f, { 0.200f, 0.025f, 0.010f } }, // metal
	{ { 0.00f, 0.00f, 0.00f }, 0.00f, { 1.000f, 1.000f, 1.000f } }, // grate
};

// The surfaceprop's game material (CHAR_TEX_*) is the coarse class the game itself uses for
// impact sounds and decals, which is about the resolution these presets have anyway.
AcousticPreset GeometryPresetForGameMaterial(int gameMaterial)
{
	switch (gameMaterial)
	{
	case CHAR_TEX_CONCRETE:
		return ACOUSTIC_CONCRETE;
	case CHAR_TEX_METAL:
	case CHAR_TEX_VENT:
	case CHAR_TEX_COMPUTER:
		return ACOUSTIC_METAL;
	case CHAR_TEX_GRATE:
		return ACOUSTIC_GRATE;
	case CHAR_TEX_WOOD:
		return ACOUSTIC_WOOD;
	case CHAR_TEX_GLASS:
		return ACOUSTIC_GLASS;
	case CHAR_TEX_TILE:
		return ACOUSTIC_CERAMIC;
	case CHAR_TEX_DIRT:
	case CHAR_TEX_SAND:
		return ACOUSTIC_GRAVEL;
	case CHAR_TEX_FOLIAGE:
		return ACOUSTIC_CARPET;
	case CHAR_TEX_PLASTIC:
		return ACOUSTIC_PLASTER;
	default:
		return ACOUSTIC_GENERIC;
	}
}

// Reads a material's $surfaceprop, following patch materials (map-specific cubemap and water
// patches) to the material they include. Leaves pszOut empty if no surfaceprop is named.
void GeometryFindSurfaceProp(const char *pszVmtPath, char *pszOut, int outSize, int depth = 0)
{
	pszOut[0] = '\0';
	char path[MAX_PATH];
	V_strncpy(path, pszVmtPath, sizeof(path));
	V_FixSlashes(path, '/');
	V_strlower(path);

	KeyValues::AutoDelete pVmt("vmt");
	if (!pVmt->LoadFromFile(s_pFileSystem, path, "GAME"))
	{
		return;
	}

	if (V_stricmp(pVmt->GetName(), "patch") != 0)
	{
		V_strncpy(pszOut, pVmt->GetString("$surfaceprop"), outSize);
		return;
	}

	static const char *const s_pszPatchBlocks[] = { "replace", "insert" };
	for (const char *pszBlock : s_pszPatchBlocks)
	{
		KeyValues *pBlock = pVmt->FindKey(pszBlock);
		if (pBlock && *pBlock->GetString("$surfaceprop"))
		{
			V_strncpy(pszOut, pBlock->GetString("$surfaceprop"), outSize);
			return;
		}
	}

	const char *pszInclude = pVmt->GetString("include");
	if (*pszInclude && depth < kGeometryMaxPatchDepth)
	{
		GeometryFindSurfaceProp(pszInclude, pszOut, outSize, depth + 1);
	}
}

// An empty or unknown surfaceprop is "default", as it is to the game.
AcousticPreset GeometryPresetForSurfaceProp(const char *pszSurfaceProp)
{
	Assert(s_pSurfaceProps);
	int surfaceIndex = s_pSurfaceProps->GetSurfaceIndex(pszSurfaceProp[0] ? pszSurfaceProp : "default");
	if (surfaceIndex < 0)
	{
		surfaceIndex = s_pSurfaceProps->GetSurfaceIndex("default");
	}
	const surfacedata_t *pSurface = (surfaceIndex >= 0) ? s_pSurfaceProps->GetSurfaceData(surfaceIndex) : nullptr;
	return pSurface ? GeometryPresetForGameMaterial(pSurface->game.material) : ACOUSTIC_GENERIC;
}

AcousticPreset GeometryPresetForTexture(const char *pszTexture)
{
	char vmtPath[MAX_PATH];
	V_snprintf(vmtPath, sizeof(vmtPath), "materials/%s.vmt", pszTexture);
	char surfaceProp[128];
	GeometryFindSurfaceProp(vmtPath, surfaceProp, sizeof(surfaceProp));
	return GeometryPresetForSurfaceProp(surfaceProp);
}

// Reads `size` bytes stored at [offset, offset + diskLen) into pOut, LZMA-decoding them if
// bCompressed. False if the block lies outside the file or does not come out exactly `size` long.
bool GeometryReadBlock(FileHandle_t file, int fileSize, int offset, int diskLen, bool bCompressed, int size, void *pOut)
{
	if (offset < 0 || diskLen < 0 || offset > fileSize - diskLen)
	{
		return false;
	}

	s_pFileSystem->Seek(file, offset, FILESYSTEM_SEEK_HEAD);
	if (!bCompressed)
	{
		return diskLen == size && s_pFileSystem->Read(pOut, size, file) == size;
	}

	CUtlVector<uint8> compressed;
	compressed.SetCount(diskLen);
	if (s_pFileSystem->Read(compressed.Base(), diskLen, file) != diskLen
		|| diskLen < static_cast<int>(sizeof(lzma_header_t)) || !CLZMA::IsCompressed(compressed.Base()))
	{
		return false;
	}
	// The decoder trusts the header, so check it describes this block before decoding into pOut.
	const lzma_header_t *pLzma = reinterpret_cast<const lzma_header_t *>(compressed.Base());
	if (pLzma->actualSize != static_cast<unsigned int>(size)
		|| pLzma->lzmaSize > static_cast<unsigned int>(diskLen) - sizeof(lzma_header_t))
	{
		return false;
	}
	return CLZMA::Uncompress(compressed.Base(), static_cast<unsigned char *>(pOut)) == static_cast<unsigned int>(size);
}

// Reads one lump as an array of T, decompressing it if needed. False if the lump is malformed;
// an absent lump is valid and empty.
template <typename T>
bool GeometryReadLump(FileHandle_t file, int fileSize, const dheader_t &header, int lumpIndex, CUtlVector<T> &out)
{
	out.RemoveAll();
	const lump_t &lump = header.lumps[lumpIndex];
	if (lump.filelen == 0)
	{
		return true;
	}

	// A non-zero uncompressedSize marks an LZMA-compressed lump.
	const bool bCompressed = lump.uncompressedSize != 0;
	const int size = bCompressed ? lump.uncompressedSize : lump.filelen;
	if (size <= 0 || size > kGeometryMaxLumpBytes || size % sizeof(T) != 0)
	{
		return false;
	}
	out.SetCount(size / sizeof(T));
	return GeometryReadBlock(file, fileSize, lump.fileofs, lump.filelen, bCompressed, size, out.Base());
}

// Reads one game lump out of LUMP_GAME_LUMP, whose directory holds absolute file offsets. Game lumps
// are compressed one by one: a compressed one keeps its uncompressed filelen and ends where the next
// directory entry starts (compression appends a terminal entry for the last one). False if the
// directory or the game lump is malformed; an absent game lump is valid and empty.
bool GeometryReadGameLump(FileHandle_t file, int fileSize, const dheader_t &header, GameLumpId_t id,
						  CUtlVector<uint8> &out, int &version)
{
	out.RemoveAll();
	version = 0;
	const lump_t &lump = header.lumps[LUMP_GAME_LUMP];
	if (lump.filelen == 0)
	{
		return true;
	}

	// The directory itself is never compressed as a block.
	dgamelumpheader_t directory;
	if (lump.uncompressedSize != 0 || lump.filelen < static_cast<int>(sizeof(directory))
		|| !GeometryReadBlock(file, fileSize, lump.fileofs, sizeof(directory), false, sizeof(directory), &directory))
	{
		return false;
	}
	const int maxEntries = (lump.filelen - static_cast<int>(sizeof(directory))) / static_cast<int>(sizeof(dgamelump_t));
	if (directory.lumpCount < 0 || directory.lumpCount > maxEntries)
	{
		return false;
	}
	CUtlVector<dgamelump_t> entries;
	entries.SetCount(directory.lumpCount);
	const int entriesSize = entries.Count() * sizeof(dgamelump_t);
	if (entriesSize > 0
		&& !GeometryReadBlock(file, fileSize, lump.fileofs + sizeof(directory), entriesSize, false, entriesSize, entries.Base()))
	{
		return false;
	}

	for (int i = 0; i < entries.Count(); ++i)
	{
		const dgamelump_t &entry = entries[i];
		if (entry.id != id)
		{
			continue;
		}
		version = entry.version;
		if (entry.filelen == 0)
		{
			return true;
		}
		if (entry.filelen < 0 || entry.filelen > kGeometryMaxLumpBytes)
		{
			return false;
		}

		const bool bCompressed = (entry.flags & GAMELUMPFLAG_COMPRESSED) != 0;
		const int diskLen = !bCompressed ? entry.filelen
			: (i + 1 < entries.Count()) ? entries[i + 1].fileofs - entry.fileofs : -1;
		out.SetCount(entry.filelen);
		if (!GeometryReadBlock(file, fileSize, entry.fileofs, diskLen, bCompressed, entry.filelen, out.Base()))
		{
			out.RemoveAll();
			return false;
		}
		return true;
	}
	return true;
}

NeoSpatial::Vec3 GeometryToMetres(const Vector &v)
{
	return { v.x * kGeometryMetresPerUnit, v.y * kGeometryMetresPerUnit, v.z * kGeometryMetresPerUnit };
}

// The quoted value after "key" in one entity block, e.g. "origin" "1 2 3".
bool GeometryEntityValue(const char *pszBlock, const char *pszKey, char *pszOut, int outSize)
{
	char quotedKey[64];
	V_snprintf(quotedKey, sizeof(quotedKey), "\"%s\"", pszKey);
	const char *pszFound = V_strstr(pszBlock, quotedKey);
	const char *pszValue = pszFound ? strchr(pszFound + V_strlen(quotedKey), '"') : nullptr;
	const char *pszValueEnd = pszValue ? strchr(pszValue + 1, '"') : nullptr;
	if (!pszValueEnd)
	{
		return false;
	}
	V_strncpy(pszOut, pszValue + 1, Min(outSize, static_cast<int>(pszValueEnd - pszValue)));
	return true;
}

// The origin of the sky_camera entity, which sits inside the 3D skybox, if the map has one.
bool GeometryFindSkyCamera(const CUtlVector<char> &entities, Vector &origin)
{
	CUtlVector<char> text;
	text.CopyArray(entities.Base(), entities.Count());
	text.AddToTail('\0');

	char block[kGeometryMaxEntityBlock];
	char value[kGeometryMaxEntityValue];
	for (const char *pszOpen = strchr(text.Base(), '{'); pszOpen; pszOpen = strchr(pszOpen + 1, '{'))
	{
		const char *pszClose = strchr(pszOpen, '}');
		if (!pszClose)
		{
			break;
		}
		V_strncpy(block, pszOpen, Min(static_cast<int>(sizeof(block)), static_cast<int>(pszClose - pszOpen) + 1));
		if (GeometryEntityValue(block, "classname", value, sizeof(value)) && V_stricmp(value, kGeometrySkyCameraClass) == 0
			&& GeometryEntityValue(block, "origin", value, sizeof(value))
			&& sscanf(value, "%f %f %f", &origin.x, &origin.y, &origin.z) == 3)
		{
			return true;
		}
	}
	return false;
}

// The lumps the static world is built from, read whole and validated as they are indexed.
struct BspLumps
{
	CUtlVector<dmodel_t> models;
	CUtlVector<dface_t> faces;
	CUtlVector<int> surfEdges;
	CUtlVector<dedge_t> edges;
	CUtlVector<dvertex_t> vertices;
	CUtlVector<texinfo_t> texInfos;
	CUtlVector<dtexdata_t> texData;
	CUtlVector<int> texDataStringTable;
	CUtlVector<char> texDataStringData;
	CUtlVector<ddispinfo_t> dispInfos;
	CUtlVector<CDispVert> dispVerts;
	CUtlVector<dplane_t> planes;
	CUtlVector<dnode_t> nodes;
	CUtlVector<uint8> leafBytes; // dleaf_t or dleaf_version_0_t, by the lump's version
	CUtlVector<char> entities;
	CUtlVector<uint8> staticProps; // GAMELUMP_STATIC_PROPS
	int staticPropsVersion = 0;
};

// One static prop model's collision in model space, as an unindexed triangle list wound
// counter-clockwise seen from outside, with an AcousticPreset per triangle. Empty if the model has
// no collision of that kind.
struct PropCollisionMesh
{
	bool bLoaded = false; // whether loading was tried, not whether it found anything
	CUtlVector<Vector> vertices;
	CUtlVector<int32> presets;
};

// A model is loaded once however many props use it; a prop's solid type picks which mesh.
struct PropModel
{
	PropCollisionMesh vphysics; // SOLID_VPHYSICS: the .phy collision model
	PropCollisionMesh bbox;		// SOLID_BBOX: the .mdl's hull box
};

// The solid's other keys (mass, damping...) are of no use here.
class CGeometryIgnoreKeys : public IVPhysicsKeyHandler
{
public:
	void ParseKeyValue(void *, const char *, const char *) override {}
	void SetDefaults(void *) override {}
};

// Adds the triangles of one convex hull, orienting each away from the hull's centroid: the
// collision query does not document its winding, but on a convex hull outward is always that way.
void GeometryAddConvexTriangles(const Vector *pVertices, int numTriangles, int32 preset, PropCollisionMesh &mesh)
{
	if (numTriangles <= 0)
	{
		return;
	}
	Vector centroid(0.0f, 0.0f, 0.0f);
	for (int v = 0; v < numTriangles * 3; ++v)
	{
		centroid += pVertices[v];
	}
	centroid /= static_cast<float>(numTriangles * 3);

	for (int t = 0; t < numTriangles; ++t)
	{
		const Vector &a = pVertices[t * 3];
		const Vector *pB = &pVertices[t * 3 + 1];
		const Vector *pC = &pVertices[t * 3 + 2];
		const Vector normal = CrossProduct(*pB - a, *pC - a);
		if (normal.LengthSqr() <= 0.0f)
		{
			continue;
		}
		// Counter-clockwise seen from outside puts the right-handed normal outwards.
		if (DotProduct(normal, a - centroid) < 0.0f)
		{
			V_swap(pB, pC);
		}
		mesh.vertices.AddToTail(a);
		mesh.vertices.AddToTail(*pB);
		mesh.vertices.AddToTail(*pC);
		mesh.presets.AddToTail(preset);
	}
}

void GeometryModelPath(const char *pszModel, const char *pszExtension, char *pszOut, int outSize)
{
	V_strncpy(pszOut, pszModel, outSize);
	V_SetExtension(pszOut, pszExtension, outSize);
	V_FixSlashes(pszOut, '/');
	V_strlower(pszOut);
}

// The .phy next to the .mdl, which the engine collides SOLID_VPHYSICS static props with. Only
// solid 0 is used, as the engine does for static props: $staticprop models compile to one solid.
void GeometryLoadPropVPhysics(const char *pszModel, PropCollisionMesh &mesh)
{
	char path[MAX_PATH];
	GeometryModelPath(pszModel, ".phy", path, sizeof(path));
	CUtlBuffer file;
	phyheader_t header;
	if (!s_pFileSystem->ReadFile(path, "GAME", file) || file.TellPut() < static_cast<int>(sizeof(header)))
	{
		return;
	}
	V_memcpy(&header, file.Base(), sizeof(header));
	if (header.size != sizeof(header) || header.solidCount <= 0 || header.solidCount > MAXSTUDIOBONES)
	{
		return;
	}

	Assert(s_pCollision);
	vcollide_t collide;
	V_memset(&collide, 0, sizeof(collide));
	s_pCollision->VCollideLoad(&collide, header.solidCount, static_cast<const char *>(file.Base()) + header.size,
								file.TellPut() - header.size);
	if (collide.solidCount <= 0 || !collide.solids || !collide.solids[0])
	{
		s_pCollision->VCollideUnload(&collide);
		return;
	}

	char surfaceProp[sizeof(solid_t::surfaceprop)] = "";
	if (collide.pKeyValues)
	{
		CGeometryIgnoreKeys ignoreKeys;
		IVPhysicsKeyParser *pParse = s_pCollision->VPhysicsKeyParserCreate(collide.pKeyValues);
		while (!pParse->Finished())
		{
			if (V_stricmp(pParse->GetCurrentBlockName(), "solid") != 0)
			{
				pParse->SkipBlock();
				continue;
			}
			solid_t solid;
			ZeroSolid(solid);
			pParse->ParseSolid(&solid, &ignoreKeys);
			if (solid.index == 0)
			{
				V_strncpy(surfaceProp, solid.surfaceprop, sizeof(surfaceProp));
				break;
			}
		}
		s_pCollision->VPhysicsKeyParserDestroy(pParse);
	}
	const int32 preset = GeometryPresetForSurfaceProp(surfaceProp);

	CUtlVector<Vector> convexVertices;
	ICollisionQuery *pQuery = s_pCollision->CreateQueryModel(collide.solids[0]);
	for (int c = 0; c < pQuery->ConvexCount(); ++c)
	{
		const int numTriangles = Max(pQuery->TriangleCount(c), 0);
		convexVertices.SetCount(numTriangles * 3);
		for (int t = 0; t < numTriangles; ++t)
		{
			pQuery->GetTriangleVerts(c, t, &convexVertices[t * 3]);
		}
		GeometryAddConvexTriangles(convexVertices.Base(), numTriangles, preset, mesh);
	}
	s_pCollision->DestroyQueryModel(pQuery);
	s_pCollision->VCollideUnload(&collide);
}

// The .mdl's hull box, which the engine collides SOLID_BBOX static props with, turned with the prop.
void GeometryLoadPropBBox(const char *pszModel, PropCollisionMesh &mesh)
{
	char path[MAX_PATH];
	GeometryModelPath(pszModel, ".mdl", path, sizeof(path));
	CUtlBuffer file;
	if (!s_pFileSystem->ReadFile(path, "GAME", file) || file.TellPut() < static_cast<int>(sizeof(studiohdr_t)))
	{
		return;
	}
	const studiohdr_t *pStudio = static_cast<const studiohdr_t *>(file.Base());
	const Vector &mins = pStudio->hull_min;
	const Vector &maxs = pStudio->hull_max;
	if (pStudio->id != kGeometryStudioId ||!mins.IsValid() || !maxs.IsValid()
		|| mins.x > maxs.x || mins.y > maxs.y || mins.z > maxs.z)
	{
		return;
	}

	const int surfacePropIndex = pStudio->surfacepropindex;
	const bool bSurfaceProp = surfacePropIndex > 0 && surfacePropIndex < file.TellPut()
		&& memchr(pStudio->pszSurfaceProp(), '\0', file.TellPut() - surfacePropIndex);
	const int32 preset = GeometryPresetForSurfaceProp(bSurfaceProp ? pStudio->pszSurfaceProp() : "");

	// Corner bit 0 picks max x, bit 1 max y, bit 2 max z; each face lists its corners in order.
	static const int s_faceCorners[6][4] = {
		{ 0, 2, 6, 4 }, { 1, 3, 7, 5 }, { 0, 1, 5, 4 }, { 2, 3, 7, 6 }, { 0, 1, 3, 2 }, { 4, 5, 7, 6 },
	};
	Vector triangles[6 * 2 * 3];
	for (int f = 0; f < 6; ++f)
	{
		Vector corners[4];
		for (int c = 0; c < 4; ++c)
		{
			const int corner = s_faceCorners[f][c];
			corners[c].Init((corner & 1) ? maxs.x : mins.x, (corner & 2) ? maxs.y : mins.y, (corner & 4) ? maxs.z : mins.z);
		}
		Vector *pFace = &triangles[f * 6];
		pFace[0] = corners[0];
		pFace[1] = corners[1];
		pFace[2] = corners[2];
		pFace[3] = corners[0];
		pFace[4] = corners[2];
		pFace[5] = corners[3];
	}
	GeometryAddConvexTriangles(triangles, 6 * 2, preset, mesh);
}

} // namespace

bool CNeoAudioGeometry::LoadFromBsp(const Services &services, const char *pszBspPath, char *pszErrorOut, int errorLen)
{
	Clear();
	pszErrorOut[0] = '\0';
	s_pFileSystem = services.pFileSystem;
	s_pSurfaceProps = services.pSurfaceProps;
	s_pCollision = services.pCollision;
	Assert(s_pFileSystem && s_pSurfaceProps && s_pCollision);

	FileHandle_t file = s_pFileSystem->Open(pszBspPath, "rb", "GAME");
	if (!file)
	{
		V_snprintf(pszErrorOut, errorLen, "cannot open %s", pszBspPath);
		return false;
	}

	BspLumps lumps;
	dheader_t header;
	const int fileSize = static_cast<int>(s_pFileSystem->Size(file));
	bool bRead = s_pFileSystem->Read(&header, sizeof(header), file) == sizeof(header);
	if (!bRead || header.ident != IDBSPHEADER || header.version < MINBSPVERSION || header.version > BSPVERSION)
	{
		s_pFileSystem->Close(file);
		V_snprintf(pszErrorOut, errorLen, "%s is not a version %d-%d BSP", pszBspPath, MINBSPVERSION, BSPVERSION);
		return false;
	}

	// HDR-only compiles may leave the LDR face lump empty; the geometry in both is identical.
	const int faceLump = (header.lumps[LUMP_FACES].filelen > 0) ? LUMP_FACES : LUMP_FACES_HDR;
	bRead = GeometryReadLump(file, fileSize, header, LUMP_MODELS, lumps.models)
		&& GeometryReadLump(file, fileSize, header, faceLump, lumps.faces)
		&& GeometryReadLump(file, fileSize, header, LUMP_SURFEDGES, lumps.surfEdges)
		&& GeometryReadLump(file, fileSize, header, LUMP_EDGES, lumps.edges)
		&& GeometryReadLump(file, fileSize, header, LUMP_VERTEXES, lumps.vertices)
		&& GeometryReadLump(file, fileSize, header, LUMP_TEXINFO, lumps.texInfos)
		&& GeometryReadLump(file, fileSize, header, LUMP_TEXDATA, lumps.texData)
		&& GeometryReadLump(file, fileSize, header, LUMP_TEXDATA_STRING_TABLE, lumps.texDataStringTable)
		&& GeometryReadLump(file, fileSize, header, LUMP_TEXDATA_STRING_DATA, lumps.texDataStringData)
		&& GeometryReadLump(file, fileSize, header, LUMP_DISPINFO, lumps.dispInfos)
		&& GeometryReadLump(file, fileSize, header, LUMP_DISP_VERTS, lumps.dispVerts)
		&& GeometryReadLump(file, fileSize, header, LUMP_PLANES, lumps.planes)
		&& GeometryReadLump(file, fileSize, header, LUMP_NODES, lumps.nodes)
		&& GeometryReadLump(file, fileSize, header, LUMP_LEAFS, lumps.leafBytes)
		&& GeometryReadLump(file, fileSize, header, LUMP_ENTITIES, lumps.entities);
	// Static props are an extra: a malformed game lump only leaves them out.
	const bool bPropsRead = GeometryReadGameLump(file, fileSize, header, GAMELUMP_STATIC_PROPS, lumps.staticProps,
												 lumps.staticPropsVersion);
	// So are the probes baked at compile time: without them the game bakes its own.
	if (!GeometryReadGameLump(file, fileSize, header, NeoSpatial::PROBE_GAME_LUMP_ID, m_probeLump, m_probeLumpVersion))
	{
		m_probeLump.RemoveAll();
		DevWarning("NEO HRTF: %s has a malformed baked probe lump, ignoring it\n", pszBspPath);
	}
	s_pFileSystem->Close(file);
	if (!bRead || lumps.models.Count() <= kGeometryWorldModel)
	{
		V_snprintf(pszErrorOut, errorLen, "%s has a malformed lump", pszBspPath);
		return false;
	}

	// Brush entities (doors, func_brush...) are models 1+ and can move, so only the world goes
	// into the static mesh.
	const dmodel_t &world = lumps.models[kGeometryWorldModel];
	if (world.firstface < 0 || world.numfaces < 0 || world.firstface > lumps.faces.Count() - world.numfaces)
	{
		V_snprintf(pszErrorOut, errorLen, "%s has a malformed world model", pszBspPath);
		return false;
	}

	// Material indices are AcousticPreset values; unused presets cost nothing.
	m_materials.AddMultipleToTail(ACOUSTIC_PRESET_COUNT, kAcousticPresets);

	// Brush faces share the vertex lump; only the vertices they use are kept.
	CUtlVector<int> vertexRemap;
	vertexRemap.SetCount(lumps.vertices.Count());
	vertexRemap.FillWithValue(-1);
	CUtlVector<int> texDataPreset; // resolved lazily, -1 = not yet
	texDataPreset.SetCount(lumps.texData.Count());
	texDataPreset.FillWithValue(-1);
	CUtlVector<int> faceVertices;
	int skippedFaces = 0;

	for (int f = world.firstface; f < world.firstface + world.numfaces; ++f)
	{
		const dface_t &face = lumps.faces[f];
		if (face.texinfo < 0 || face.texinfo >= lumps.texInfos.Count() || face.numedges < 3
			|| face.firstedge < 0 || face.firstedge > lumps.surfEdges.Count() - face.numedges)
		{
			++skippedFaces;
			continue;
		}
		const texinfo_t &texInfo = lumps.texInfos[face.texinfo];
		if (texInfo.flags & kGeometryIgnoredSurfFlags)
		{
			continue;
		}

		int preset = ACOUSTIC_GENERIC;
		if (texInfo.texdata >= 0 && texInfo.texdata < lumps.texData.Count())
		{
			int &cachedPreset = texDataPreset[texInfo.texdata];
			if (cachedPreset < 0)
			{
				cachedPreset = ACOUSTIC_GENERIC;
				const int stringIndex = lumps.texData[texInfo.texdata].nameStringTableID;
				if (stringIndex >= 0 && stringIndex < lumps.texDataStringTable.Count())
				{
					const int offset = lumps.texDataStringTable[stringIndex];
					const int maxLen = lumps.texDataStringData.Count() - offset;
					if (offset >= 0 && maxLen > 0 && memchr(&lumps.texDataStringData[offset], '\0', maxLen))
					{
						cachedPreset = GeometryPresetForTexture(&lumps.texDataStringData[offset]);
					}
				}
			}
			preset = cachedPreset;
		}

		// A surfedge's sign picks which end of the edge comes first in the face's winding.
		faceVertices.RemoveAll();
		bool bValid = true;
		for (int e = 0; e < face.numedges && bValid; ++e)
		{
			const int surfEdge = lumps.surfEdges[face.firstedge + e];
			bValid = surfEdge > -lumps.edges.Count() && surfEdge < lumps.edges.Count();
			if (bValid)
			{
				const int vertex = lumps.edges[abs(surfEdge)].v[(surfEdge >= 0) ? 0 : 1];
				bValid = vertex < lumps.vertices.Count();
				faceVertices.AddToTail(vertex);
			}
		}
		if (!bValid)
		{
			++skippedFaces;
			continue;
		}

		if (face.dispinfo < 0)
		{
			for (int &vertex : faceVertices)
			{
				int &remapped = vertexRemap[vertex];
				if (remapped < 0)
				{
					remapped = m_vertices.AddToTail(GeometryToMetres(lumps.vertices[vertex].point));
				}
				vertex = remapped;
			}

			// Brush faces are convex and wound clockwise seen from the front (the Quake/Source
			// convention), so each fan triangle is emitted reversed to come out counter-clockwise.
			for (int i = 1; i + 1 < faceVertices.Count(); ++i)
			{
				const int32 triangle[3] = { faceVertices[0], faceVertices[i + 1], faceVertices[i] };
				if (triangle[0] != triangle[1] && triangle[1] != triangle[2] && triangle[0] != triangle[2])
				{
					m_triangles.AddMultipleToTail(3, triangle);
					m_materialIndices.AddToTail(preset);
				}
			}
			continue;
		}

		// A displacement replaces its 4-sided base face with a (2^power + 1)^2 vertex grid.
		const ddispinfo_t *pDisp = (face.dispinfo < lumps.dispInfos.Count()) ? &lumps.dispInfos[face.dispinfo] : nullptr;
		if (!pDisp || faceVertices.Count() != 4 || pDisp->power < kGeometryMinDispPower || pDisp->power > kGeometryMaxDispPower
			|| pDisp->m_iDispVertStart < 0 || pDisp->m_iDispVertStart > lumps.dispVerts.Count() - pDisp->NumVerts())
		{
			++skippedFaces;
			continue;
		}

		// The grid starts at the base corner nearest startPosition and keeps the face's winding,
		// as the engine builds it (CCoreDispInfo::GenerateDispSurf).
		int startCorner = 0;
		float bestDistSqr = FLT_MAX;
		for (int c = 0; c < 4; ++c)
		{
			const float distSqr = lumps.vertices[faceVertices[c]].point.DistToSqr(pDisp->startPosition);
			if (distSqr < bestDistSqr)
			{
				bestDistSqr = distSqr;
				startCorner = c;
			}
		}
		Vector corners[4];
		for (int c = 0; c < 4; ++c)
		{
			corners[c] = lumps.vertices[faceVertices[(startCorner + c) % 4]].point;
		}

		// Rows run from corner 0 towards corner 1, columns from the 0-1 edge towards the 3-2 edge.
		const int size = (1 << pDisp->power) + 1;
		const float step = 1.0f / (size - 1);
		const int firstVertex = m_vertices.Count();
		const CDispVert *pDispVerts = &lumps.dispVerts[pDisp->m_iDispVertStart];
		for (int row = 0; row < size; ++row)
		{
			const Vector rowStart = Lerp(row * step, corners[0], corners[1]);
			const Vector rowEnd = Lerp(row * step, corners[3], corners[2]);
			for (int col = 0; col < size; ++col)
			{
				const CDispVert &dispVert = pDispVerts[row * size + col];
				const Vector position = Lerp(col * step, rowStart, rowEnd) + dispVert.m_vVector * dispVert.m_flDist;
				m_vertices.AddToTail(GeometryToMetres(position));
			}
		}

		// (row, col) -> (row + 1, col) -> (row + 1, col + 1) follows the base face's clockwise
		// winding, so each cell's two triangles are emitted reversed, as for brush faces.
		for (int row = 0; row + 1 < size; ++row)
		{
			for (int col = 0; col + 1 < size; ++col)
			{
				const int32 v00 = firstVertex + row * size + col;
				const int32 v10 = v00 + size;
				const int32 v11 = v10 + 1;
				const int32 v01 = v00 + 1;
				const int32 triangles[6] = { v00, v11, v10, v00, v01, v11 };
				m_triangles.AddMultipleToTail(6, triangles);
				m_materialIndices.AddToTail(preset);
				m_materialIndices.AddToTail(preset);
			}
		}
	}

	if (skippedFaces > 0)
	{
		DevWarning("NEO HRTF: skipped %d malformed faces in %s\n", skippedFaces, pszBspPath);
	}
	if (m_materialIndices.IsEmpty())
	{
		Clear();
		V_snprintf(pszErrorOut, errorLen, "%s has no world geometry", pszBspPath);
		return false;
	}

	if (bPropsRead)
	{
		LoadStaticProps(pszBspPath, lumps.staticProps, lumps.staticPropsVersion);
	}
	else
	{
		DevWarning("NEO HRTF: %s has a malformed static prop lump, so no static props\n", pszBspPath);
	}

	// Without a usable tree there are just no probes; the scene itself is still good.
	if (!LoadBspTree(pszBspPath, lumps.planes, lumps.nodes, lumps.leafBytes, header.lumps[LUMP_LEAFS].version, lumps.entities))
	{
		m_tree = NeoSpatial::BspTree();
	}
	return true;
}

void CNeoAudioGeometry::LoadStaticProps(const char *pszBspPath, const CUtlVector<uint8> &lump, int version)
{
	if (lump.IsEmpty())
	{
		return;
	}
	if (version < kGeometryMinStaticPropVersion)
	{
		DevWarning("NEO HRTF: %s has static prop lump version %d, so no static props\n", pszBspPath, version);
		return;
	}

	// The lump is three count-prefixed arrays: model names, leaves (not needed here), then props.
	int offset = 0;
	auto readCount = [&](int elementSize, int &count) -> bool
	{
		if (offset > lump.Count() - static_cast<int>(sizeof(int)))
		{
			return false;
		}
		V_memcpy(&count, lump.Base() + offset, sizeof(int));
		offset += sizeof(int);
		return count >= 0 && count <= (lump.Count() - offset) / elementSize;
	};
	int numModels = 0;
	int numLeaves = 0;
	int numProps = 0;
	bool bValid = readCount(sizeof(StaticPropDictLump_t), numModels);
	const char *pModelNames = reinterpret_cast<const char *>(lump.Base() + offset);
	offset += numModels * sizeof(StaticPropDictLump_t);
	bValid = bValid && readCount(sizeof(StaticPropLeafLump_t), numLeaves);
	offset += numLeaves * sizeof(StaticPropLeafLump_t);
	bValid = bValid && readCount(1, numProps);

	// The record size differs between versions, and between branches sharing a version number, so
	// it is taken from the lump itself.
	const int propBytes = lump.Count() - offset;
	const int stride = (numProps > 0) ? propBytes / numProps : 0;
	if (!bValid || (numProps > 0 && (propBytes % numProps != 0 || stride < static_cast<int>(sizeof(StaticPropLumpV4_t)))))
	{
		DevWarning("NEO HRTF: %s has a malformed static prop lump, so no static props\n", pszBspPath);
		return;
	}

	CUtlVector<PropModel> models;
	models.SetCount(numModels);
	const int firstTriangle = m_materialIndices.Count();
	int numAdded = 0;
	int numNoCollision = 0;
	int numMalformed = 0;
	for (int i = 0; i < numProps; ++i)
	{
		const uint8 *pRecord = lump.Base() + offset + i * stride;
		StaticPropLumpV4_t prop;
		V_memcpy(&prop, pRecord, sizeof(prop));

		// Props the game lets everything pass through are left out too.
		if (prop.m_Solid != SOLID_VPHYSICS && prop.m_Solid != SOLID_BBOX)
		{
			continue;
		}
		if (prop.m_PropType >= numModels || !prop.m_Origin.IsValid() || !prop.m_Angles.IsValid())
		{
			++numMalformed;
			continue;
		}

		float scale = 1.0f;
		if (version >= kGeometryStaticPropScaleVersion)
		{
			V_memcpy(&scale, pRecord + stride - sizeof(float), sizeof(float));
			if (!IsFinite(scale) || scale <= 0.0f)
			{
				scale = 1.0f;
			}
		}

		PropModel &model = models[prop.m_PropType];
		PropCollisionMesh &mesh = (prop.m_Solid == SOLID_BBOX) ? model.bbox : model.vphysics;
		if (!mesh.bLoaded)
		{
			mesh.bLoaded = true;
			char name[STATIC_PROP_NAME_LENGTH + 1];
			V_strncpy(name, pModelNames + prop.m_PropType * sizeof(StaticPropDictLump_t), sizeof(name));
			if (prop.m_Solid == SOLID_BBOX)
			{
				GeometryLoadPropBBox(name, mesh);
			}
			else
			{
				GeometryLoadPropVPhysics(name, mesh);
			}
		}
		if (mesh.presets.IsEmpty())
		{
			++numNoCollision;
			continue;
		}

		// A rotation and a positive uniform scale both keep the winding.
		matrix3x4_t toWorld;
		AngleMatrix(prop.m_Angles, prop.m_Origin, toWorld);
		for (const Vector &vertex : mesh.vertices)
		{
			Vector world;
			VectorTransform(vertex * scale, toWorld, world);
			m_triangles.AddToTail(m_vertices.AddToTail(GeometryToMetres(world)));
		}
		m_materialIndices.AddMultipleToTail(mesh.presets.Count(), mesh.presets.Base());
		++numAdded;
	}

	if (numMalformed > 0)
	{
		DevWarning("NEO HRTF: skipped %d malformed static props in %s\n", numMalformed, pszBspPath);
	}
	if (numNoCollision > 0)
	{
		DevMsg("NEO HRTF: %d solid static props in %s have no collision model\n", numNoCollision, pszBspPath);
	}
	DevMsg("NEO HRTF: %d static props in %s, %d triangles\n", numAdded, pszBspPath,
		   m_materialIndices.Count() - firstTriangle);
}

bool CNeoAudioGeometry::LoadBspTree(const char *pszBspPath, const CUtlVector<dplane_t> &planes, const CUtlVector<dnode_t> &nodes,
									const CUtlVector<uint8> &leafBytes, int leafVersion, const CUtlVector<char> &entities)
{
	const int leafStride = (leafVersion == 0) ? sizeof(dleaf_version_0_t) : sizeof(dleaf_t);
	if (planes.IsEmpty() || nodes.IsEmpty() || leafBytes.Count() % leafStride != 0)
	{
		DevWarning("NEO HRTF: %s has no usable BSP tree, so no probes\n", pszBspPath);
		return false;
	}

	m_tree.planes.reserve(planes.Count());
	for (const dplane_t &plane : planes)
	{
		m_tree.planes.push_back({ { plane.normal.x, plane.normal.y, plane.normal.z }, plane.dist * kGeometryMetresPerUnit });
	}
	m_tree.nodes.reserve(nodes.Count());
	for (const dnode_t &node : nodes)
	{
		m_tree.nodes.push_back({ node.planenum, { node.children[0], node.children[1] } });
	}

	// Both leaf versions start with the fields used here; version 0 only appends lighting.
	const int numLeaves = leafBytes.Count() / leafStride;
	m_tree.leaves.reserve(numLeaves);
	for (int i = 0; i < numLeaves; ++i)
	{
		dleaf_t leaf;
		V_memcpy(&leaf, leafBytes.Base() + i * leafStride, sizeof(dleaf_t));
		const Vector mins(leaf.mins[0], leaf.mins[1], leaf.mins[2]);
		const Vector maxs(leaf.maxs[0], leaf.maxs[1], leaf.maxs[2]);
		// Solid leaves and leaves outside the map (no vis cluster) can never hold a listener.
		const bool bOpen = !(leaf.contents & CONTENTS_SOLID) && leaf.cluster >= 0;
		m_tree.leaves.push_back({ GeometryToMetres(mins), GeometryToMetres(maxs), leaf.area, bOpen });
	}

	// The 3D skybox is a sealed region of its own, so its leaves share an area no player reaches.
	Vector skyCamera;
	if (GeometryFindSkyCamera(entities, skyCamera))
	{
		const int skyLeaf = m_tree.FindLeaf(GeometryToMetres(skyCamera));
		const int skyArea = (skyLeaf >= 0) ? m_tree.leaves[skyLeaf].area : -1;
		for (NeoSpatial::BspTree::Leaf &leaf : m_tree.leaves)
		{
			if (skyArea > 0 && leaf.area == skyArea)
			{
				leaf.open = false;
			}
		}
	}
	return true;
}

uint32 CNeoAudioGeometry::GetGeometryCrc() const
{
	// Field by field, so struct padding never feeds the checksum.
	CRC32_t crc;
	CRC32_Init(&crc);
	CRC32_ProcessBuffer(&crc, m_vertices.Base(), m_vertices.Count() * sizeof(NeoSpatial::Vec3));
	CRC32_ProcessBuffer(&crc, m_triangles.Base(), m_triangles.Count() * sizeof(int32));
	CRC32_ProcessBuffer(&crc, m_materialIndices.Base(), m_materialIndices.Count() * sizeof(int32));
	CRC32_ProcessBuffer(&crc, m_materials.Base(), m_materials.Count() * sizeof(NeoSpatial::AcousticMaterial));
	for (const NeoSpatial::BspTree::Leaf &leaf : m_tree.leaves)
	{
		const uint8 open = leaf.open ? 1 : 0;
		CRC32_ProcessBuffer(&crc, &leaf.mins, sizeof(leaf.mins));
		CRC32_ProcessBuffer(&crc, &leaf.maxs, sizeof(leaf.maxs));
		CRC32_ProcessBuffer(&crc, &leaf.area, sizeof(leaf.area));
		CRC32_ProcessBuffer(&crc, &open, sizeof(open));
	}
	CRC32_Final(&crc);
	return crc;
}

bool CNeoAudioGeometry::ReadBakedProbes(NeoSpatial::ProbeLumpHeader &header, NeoSpatial::ProbeSet &probes,
										CUtlVector<uint8> &storage, const uint8 *&batches, char *pszErrorOut, int errorLen) const
{
	pszErrorOut[0] = '\0';
	if (m_probeLump.IsEmpty())
	{
		return false;
	}
	if (m_probeLumpVersion != NeoSpatial::PROBE_GAME_LUMP_VERSION)
	{
		V_snprintf(pszErrorOut, errorLen, "the baked probes are game lump version %d, this build reads %d", m_probeLumpVersion,
				   NeoSpatial::PROBE_GAME_LUMP_VERSION);
		return false;
	}

	// The baker stores the payload LZMA-compressed; the decoder trusts the header, so check it first.
	const uint8 *pPayload = m_probeLump.Base();
	int64 payloadSize = m_probeLump.Count();
	unsigned char *pStored = const_cast<unsigned char *>(m_probeLump.Base());
	if (m_probeLump.Count() >= static_cast<int>(sizeof(lzma_header_t)) && CLZMA::IsCompressed(pStored))
	{
		const lzma_header_t *pLzma = reinterpret_cast<const lzma_header_t *>(pStored);
		if (pLzma->actualSize == 0 || pLzma->actualSize > static_cast<unsigned int>(kGeometryMaxLumpBytes)
			|| pLzma->lzmaSize > static_cast<unsigned int>(m_probeLump.Count()) - sizeof(lzma_header_t))
		{
			V_snprintf(pszErrorOut, errorLen, "the baked probe lump is malformed");
			return false;
		}
		storage.SetCount(static_cast<int>(pLzma->actualSize));
		if (CLZMA::Uncompress(pStored, storage.Base()) != pLzma->actualSize)
		{
			V_snprintf(pszErrorOut, errorLen, "the baked probe lump does not decompress");
			return false;
		}
		pPayload = storage.Base();
		payloadSize = storage.Count();
	}
	else
	{
		storage.CopyArray(m_probeLump.Base(), m_probeLump.Count());
		pPayload = storage.Base();
	}

	if (!NeoSpatial::ReadProbeLump(pPayload, payloadSize, header, probes, batches, pszErrorOut, errorLen))
	{
		return false;
	}

	// Baked against other geometry: the map was recompiled without running the baker again.
	if (header.shapeCrc != GetShapeCrc())
	{
		V_snprintf(pszErrorOut, errorLen, "the baked probes are out of date (map recompiled without neo_soundbake?)");
		return false;
	}
	return true;
}

uint32 CNeoAudioGeometry::GetShapeCrc() const
{
	CRC32_t crc;
	CRC32_Init(&crc);
	auto processQuantised = [&crc](const NeoSpatial::Vec3 &v)
	{
		const int32 steps[3] = { static_cast<int32>(roundf(v.x * kGeometryShapeStepsPerMetre)),
								 static_cast<int32>(roundf(v.y * kGeometryShapeStepsPerMetre)),
								 static_cast<int32>(roundf(v.z * kGeometryShapeStepsPerMetre)) };
		CRC32_ProcessBuffer(&crc, steps, sizeof(steps));
	};
	for (const NeoSpatial::Vec3 &vertex : m_vertices)
	{
		processQuantised(vertex);
	}
	CRC32_ProcessBuffer(&crc, m_triangles.Base(), m_triangles.Count() * sizeof(int32));
	for (const NeoSpatial::BspTree::Leaf &leaf : m_tree.leaves)
	{
		const int32 areaAndOpen[2] = { leaf.area, leaf.open ? 1 : 0 };
		processQuantised(leaf.mins);
		processQuantised(leaf.maxs);
		CRC32_ProcessBuffer(&crc, areaAndOpen, sizeof(areaAndOpen));
	}
	CRC32_Final(&crc);
	return crc;
}

NeoSpatial::SceneGeometry CNeoAudioGeometry::GetSceneGeometry() const
{
	NeoSpatial::SceneGeometry geometry;
	geometry.vertices = m_vertices.Base();
	geometry.numVertices = m_vertices.Count();
	geometry.triangles = m_triangles.Base();
	geometry.materialIndices = m_materialIndices.Base();
	geometry.numTriangles = m_materialIndices.Count();
	geometry.materials = m_materials.Base();
	geometry.numMaterials = m_materials.Count();
	return geometry;
}

void CNeoAudioGeometry::Clear()
{
	m_vertices.Purge();
	m_triangles.Purge();
	m_materialIndices.Purge();
	m_materials.Purge();
	m_tree = NeoSpatial::BspTree();
	m_probeLump.Purge();
	m_probeLumpVersion = 0;
}
