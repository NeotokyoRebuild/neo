#ifndef NEO_BSP_LIGHTING_H
#define NEO_BSP_LIGHTING_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "utlstring.h"

// Static light exposure of a standing body, read from the map's compiled lighting: the leaf
// ambient cubes plus the worldlights the cubes leave out, each traced for occlusion.
// The server engine exposes no lighting query, so the BSP lumps are read directly.
class CNEOBspLighting
{
public:
	// Loads the current map's lighting lumps, once per map. False when the map has none, until Release().
	bool Load();
	void Release();

	// Exposure at the torso of a body standing on floorPos, in ambient cube units.
	// False when floorPos is inside solid or no ambient sample covers it.
	bool ExposureAt( const Vector &floorPos, float *ambient, float *direct ) const;

	// Maps a raw exposure to 0..1 on a fixed curve, so values compare across maps.
	static float Normalize( float exposure );

private:
	struct AmbientSample
	{
		float luminance;
		byte pos[3];
	};

	struct LeafAmbient
	{
		unsigned short count;
		unsigned short first;
	};

	struct WorldLight
	{
		int type;
		Vector origin;
		Vector normal;
		float luminance;
		float stopDot;
		float stopDot2;
		float exponent;
		float radius;
		float constantAttn;
		float linearAttn;
		float quadraticAttn;
	};

	bool TorsoPoint( const Vector &navPos, Vector *torso ) const;
	bool AmbientAt( const Vector &pos, float *ambient ) const;
	float DirectAt( const Vector &pos ) const;
	float LightFalloff( const WorldLight &light, const Vector &toLight, float dist ) const;

	CUtlString m_mapName;
	CUtlString m_failedMapName;
	CUtlVector<Vector> m_leafMins;
	CUtlVector<Vector> m_leafMaxs;
	CUtlVector<LeafAmbient> m_leafAmbient;
	CUtlVector<AmbientSample> m_ambientSamples;
	CUtlVector<WorldLight> m_lights;
};

CNEOBspLighting &NEOBspLighting();

#endif // NEO_BSP_LIGHTING_H
