#include "cbase.h"
#include "neo_bsp_lighting.h"
#include "bspfile.h"
#include "filesystem.h"
#include "engine/IEngineTrace.h"
#include "nav.h"
#include "mapentities_shared.h"
#include "fgdlib/entitydefs.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// named, not anonymous: the unity build shares one translation unit with other server files
namespace NEOBspLightingInternal
{

// Version 1 worldlights insert a shadow cast offset vector after the normal, which moves every later field;
// dworldlight_t is the version 0 layout.
constexpr int WORLDLIGHT_V1_EXTRA = sizeof( Vector );
constexpr int WORLDLIGHT_V1_EXTRA_AT = offsetof( dworldlight_t, normal ) + sizeof( Vector );

// A zero-count ambient index points at another leaf; follow it this many times at most.
constexpr int MAX_AMBIENT_LEAF_HOPS = 4;

// The torso of a standing player, raised off the floor, lowered under a low ceiling.
constexpr float TORSO_HEIGHT = 36.0f;
constexpr float FLOOR_CLEARANCE = 2.0f;
constexpr float CEILING_PROBE = 72.0f;

constexpr float SKY_RAY_LENGTH = 32768.0f;

// A light whose unoccluded contribution is below this is not worth a trace.
constexpr float MIN_LIGHT_CONTRIBUTION = 1e-5f;

// Exposure at which the normalized value is one half. Puts the P10..P90 of most ntre_ maps
// near 0.2..0.8 and keeps sun-lit maps uniformly bright.
constexpr float EXPOSURE_HALF_POINT = 0.1f;

float Luminance( const Vector &color )
{
	return 0.2126f * color.x + 0.7152f * color.y + 0.0722f * color.z;
}

// Decodes as ColorRGBExp32ToVector does, the unit VRAD writes ambient cubes and worldlights in.
float LuminanceRGBExp32( const ColorRGBExp32 &color )
{
	const Vector linear( ldexpf( color.r, color.exponent ), ldexpf( color.g, color.exponent ), ldexpf( color.b, color.exponent ) );
	return Luminance( linear );
}

// The six-direction mean of a Lambert cube lit from dir, so direct light adds to the ambient
// cube mean in the same units.
float CubeMeanLambert( const Vector &dir )
{
	return ( fabsf( dir.x ) + fabsf( dir.y ) + fabsf( dir.z ) ) / 6.0f;
}

bool ReadLump( FileHandle_t file, const lump_t &lump, CUtlVector<byte> *out )
{
	out->RemoveAll();

	// a nonzero fourCC is the uncompressed size of an LZMA lump
	if ( lump.uncompressedSize != 0 )
	{
		return false;
	}

	if ( lump.filelen <= 0 )
	{
		return true;
	}

	out->SetCount( lump.filelen );
	filesystem->Seek( file, lump.fileofs, FILESYSTEM_SEEK_HEAD );
	return filesystem->Read( out->Base(), lump.filelen, file ) == lump.filelen;
}

template <typename T>
T ReadAt( const CUtlVector<byte> &data, int offset )
{
	T value;
	V_memcpy( &value, data.Base() + offset, sizeof( T ) );
	return value;
}

// Reads one worldlight field, given by its dworldlight_t offset, from a light of the lump's version.
template <typename T>
T WorldLightField( const CUtlVector<byte> &data, int base, int version, size_t fieldOffset )
{
	const int shift = ( version == 1 && static_cast<int>( fieldOffset ) >= WORLDLIGHT_V1_EXTRA_AT ) ? WORLDLIGHT_V1_EXTRA : 0;
	return ReadAt<T>( data, base + static_cast<int>( fieldOffset ) + shift );
}

// Lights and their shadows come from opaque brushes and props; players and NPCs cast none.
// Lights with a style from this one up are named lights the map can switch; the light entity says whether one starts off.
constexpr int FIRST_SWITCHABLE_STYLE = 32;

// Whether an entity I/O output value ("target,input,param,delay,times") sends input to target.
bool OutputSends( const char *output, const char *target, const char *input )
{
	// Hammer separates the fields with commas, or with VMF_IOPARAM_STRING_DELIMITER so a parameter can hold commas.
	const int targetLen = V_strlen( target );
	const char delimiter = output[targetLen];
	return !V_strnicmp( output, target, targetLen ) && ( delimiter == ',' || delimiter == VMF_IOPARAM_STRING_DELIMITER )
		&& !V_strnicmp( output + targetLen + 1, input, V_strlen( input ) );
}

// Marks the switchable light styles that are dark in play: their light entity starts off, as CLight::Spawn sets it,
// unless the map only ever turns it on (a light switched on for good at round start lights the whole match).
void ReadStartOffStyles( const CUtlVector<byte> &entities, bool startOff[MAX_LIGHTSTYLES] )
{
	CUtlVector<char> text;
	text.SetCount( entities.Count() + 1 );
	V_memcpy( text.Base(), entities.Base(), entities.Count() );
	text[entities.Count()] = '\0';

	struct StartOffLight
	{
		CUtlString name;
		int style;
	};
	CUtlVector<StartOffLight> lights;
	CUtlVector<CUtlString> outputs;

	const char *data = text.Base();
	char key[MAPKEY_MAXLENGTH];
	char value[MAPKEY_MAXLENGTH];
	while ( ( data = MapEntity_ParseToken( data, key ) ) != nullptr )
	{
		if ( key[0] != '{' )
		{
			continue;
		}

		bool isLight = false;
		int style = 0;
		int spawnFlags = 0;
		CUtlString name;
		while ( ( data = MapEntity_ParseToken( data, key ) ) != nullptr && key[0] != '}' )
		{
			data = MapEntity_ParseToken( data, value );
			if ( !data )
			{
				break;
			}

			if ( !V_stricmp( key, "classname" ) )
			{
				isLight = !V_strnicmp( value, "light", 5 );
			}
			else if ( !V_stricmp( key, "style" ) )
			{
				style = V_atoi( value );
			}
			else if ( !V_stricmp( key, "spawnflags" ) )
			{
				spawnFlags = V_atoi( value );
			}
			else if ( !V_stricmp( key, "targetname" ) )
			{
				name = value;
			}
			else if ( !V_strnicmp( key, "On", 2 ) )
			{
				outputs.AddToTail( CUtlString( value ) );
			}
		}

		if ( isLight && style >= FIRST_SWITCHABLE_STYLE && style < MAX_LIGHTSTYLES && ( spawnFlags & SF_LIGHT_START_OFF ) )
		{
			lights.AddToTail( StartOffLight{ name, style } );
		}

		if ( !data )
		{
			break;
		}
	}

	for ( const StartOffLight &light : lights )
	{
		bool turnedOn = false;
		bool turnedOff = false;
		for ( const CUtlString &output : outputs )
		{
			turnedOn |= OutputSends( output.Get(), light.name.Get(), "TurnOn" );
			turnedOff |= OutputSends( output.Get(), light.name.Get(), "TurnOff" ) || OutputSends( output.Get(), light.name.Get(), "Toggle" );
		}

		if ( light.name.IsEmpty() || !turnedOn || turnedOff )
		{
			startOff[light.style] = true;
		}
	}
}

bool IsBlocked( const Vector &from, const Vector &to, trace_t *tr )
{
	Ray_t ray;
	ray.Init( from, to );
	CTraceFilterWorldAndPropsOnly filter;
	enginetrace->TraceRay( ray, MASK_OPAQUE, &filter, tr );
	return tr->DidHit();
}

} // namespace NEOBspLightingInternal

using namespace NEOBspLightingInternal;

bool CNEOBspLighting::Load()
{
	const char *mapName = STRING( gpGlobals->mapname );
	if ( !m_mapName.IsEmpty() && m_mapName == mapName )
	{
		return true;
	}

	// a map without usable lighting fails the same way for every nav area, so fail once
	if ( !m_failedMapName.IsEmpty() && m_failedMapName == mapName )
	{
		return false;
	}

	Release();
	m_failedMapName = mapName;

	char path[MAX_PATH];
	V_snprintf( path, sizeof( path ), "maps/%s.bsp", mapName );
	FileHandle_t file = filesystem->Open( path, "rb", "GAME" );
	if ( !file )
	{
		Warning( "BSP lighting: cannot open %s\n", path );
		return false;
	}

	dheader_t header;
	const bool headerOk = filesystem->Read( &header, sizeof( header ), file ) == sizeof( header ) && header.ident == IDBSPHEADER;

	CUtlVector<byte> leaves, ambientIndex, ambientLighting, worldLights, entities;
	bool hdr = false;
	int lightsVersion = 0;
	bool lumpsOk = headerOk;
	if ( lumpsOk )
	{
		hdr = header.lumps[LUMP_LEAF_AMBIENT_INDEX_HDR].filelen > 0;
		const lump_t &lightsLump = header.lumps[hdr && header.lumps[LUMP_WORLDLIGHTS_HDR].filelen > 0 ? LUMP_WORLDLIGHTS_HDR : LUMP_WORLDLIGHTS];
		lightsVersion = lightsLump.version;
		lumpsOk = ReadLump( file, header.lumps[LUMP_LEAFS], &leaves )
			&& ReadLump( file, header.lumps[hdr ? LUMP_LEAF_AMBIENT_INDEX_HDR : LUMP_LEAF_AMBIENT_INDEX], &ambientIndex )
			&& ReadLump( file, header.lumps[hdr ? LUMP_LEAF_AMBIENT_LIGHTING_HDR : LUMP_LEAF_AMBIENT_LIGHTING], &ambientLighting )
			&& ReadLump( file, lightsLump, &worldLights )
			&& ReadLump( file, header.lumps[LUMP_ENTITIES], &entities );
	}

	// Version 0 leaves end in an ambient cube that version 1 moved to its own lump; the fields before it match.
	const int leafStride = ( headerOk && header.lumps[LUMP_LEAFS].version == 0 ) ? sizeof( dleaf_version_0_t ) : sizeof( dleaf_t );
	filesystem->Close( file );

	if ( !lumpsOk || ambientIndex.IsEmpty() )
	{
		Warning( "BSP lighting: %s has no readable ambient lighting\n", path );
		return false;
	}

	const int numLeaves = leaves.Count() / leafStride;
	m_leafMins.SetCount( numLeaves );
	m_leafMaxs.SetCount( numLeaves );
	for ( int i = 0; i < numLeaves; ++i )
	{
		const dleaf_t leaf = ReadAt<dleaf_t>( leaves, i * leafStride );
		for ( int k = 0; k < 3; ++k )
		{
			m_leafMins[i][k] = leaf.mins[k];
			m_leafMaxs[i][k] = leaf.maxs[k];
		}
	}

	const int numIndex = ambientIndex.Count() / sizeof( dleafambientindex_t );
	m_leafAmbient.SetCount( numIndex );
	for ( int i = 0; i < numIndex; ++i )
	{
		const dleafambientindex_t index = ReadAt<dleafambientindex_t>( ambientIndex, i * sizeof( dleafambientindex_t ) );
		m_leafAmbient[i].count = index.ambientSampleCount;
		m_leafAmbient[i].first = index.firstAmbientSample;
	}

	const int numSamples = ambientLighting.Count() / sizeof( dleafambientlighting_t );
	m_ambientSamples.SetCount( numSamples );
	for ( int i = 0; i < numSamples; ++i )
	{
		const dleafambientlighting_t sample = ReadAt<dleafambientlighting_t>( ambientLighting, i * sizeof( dleafambientlighting_t ) );
		float sum = 0.0f;
		for ( int side = 0; side < 6; ++side )
		{
			sum += LuminanceRGBExp32( sample.cube.m_Color[side] );
		}

		m_ambientSamples[i].luminance = sum / 6.0f;
		m_ambientSamples[i].pos[0] = sample.x;
		m_ambientSamples[i].pos[1] = sample.y;
		m_ambientSamples[i].pos[2] = sample.z;
	}

	bool startOff[MAX_LIGHTSTYLES] = {};
	ReadStartOffStyles( entities, startOff );
	int lightsOff = 0;

	const int lightStride = sizeof( dworldlight_t ) + ( ( lightsVersion == 1 ) ? WORLDLIGHT_V1_EXTRA : 0 );
	for ( int base = 0; base + lightStride <= worldLights.Count(); base += lightStride )
	{
		const int type = WorldLightField<int>( worldLights, base, lightsVersion, offsetof( dworldlight_t, type ) );
		const int flags = WorldLightField<int>( worldLights, base, lightsVersion, offsetof( dworldlight_t, flags ) );
		const int style = WorldLightField<int>( worldLights, base, lightsVersion, offsetof( dworldlight_t, style ) );

		// the bake describes the map as it starts, so a light switched off at the start adds nothing
		if ( style >= 0 && style < MAX_LIGHTSTYLES && startOff[style] )
		{
			++lightsOff;
			continue;
		}

		// sky ambient and lights already in the ambient cubes would count twice
		if ( type == emit_skyambient || ( flags & DWL_FLAGS_INAMBIENTCUBE ) )
		{
			continue;
		}

		if ( type != emit_surface && type != emit_point && type != emit_spotlight && type != emit_skylight )
		{
			continue;
		}

		WorldLight &light = m_lights[m_lights.AddToTail()];
		light.type = type;
		light.origin = WorldLightField<Vector>( worldLights, base, lightsVersion, offsetof( dworldlight_t, origin ) );
		light.luminance = Luminance( WorldLightField<Vector>( worldLights, base, lightsVersion, offsetof( dworldlight_t, intensity ) ) );
		light.normal = WorldLightField<Vector>( worldLights, base, lightsVersion, offsetof( dworldlight_t, normal ) );
		light.stopDot = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, stopdot ) );
		light.stopDot2 = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, stopdot2 ) );
		light.exponent = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, exponent ) );
		light.radius = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, radius ) );
		light.constantAttn = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, constant_attn ) );
		light.linearAttn = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, linear_attn ) );
		light.quadraticAttn = WorldLightField<float>( worldLights, base, lightsVersion, offsetof( dworldlight_t, quadratic_attn ) );
	}

	m_mapName = mapName;
	m_failedMapName.Clear();
	Msg( "BSP lighting: %s %s, %d leaves, %d ambient samples, %d direct lights, %d off at start\n",
		mapName, hdr ? "HDR" : "LDR", numLeaves, numSamples, m_lights.Count(), lightsOff );
	return true;
}

void CNEOBspLighting::Release()
{
	m_mapName.Clear();
	m_failedMapName.Clear();
	m_leafMins.Purge();
	m_leafMaxs.Purge();
	m_leafAmbient.Purge();
	m_ambientSamples.Purge();
	m_lights.Purge();
}

bool CNEOBspLighting::ExposureAt( const Vector &floorPos, float *ambient, float *direct ) const
{
	Vector torso;
	if ( !TorsoPoint( floorPos, &torso ) )
	{
		return false;
	}

	if ( !AmbientAt( torso, ambient ) )
	{
		return false;
	}

	*direct = DirectAt( torso );
	return true;
}

float CNEOBspLighting::Normalize( float exposure )
{
	exposure = Max( exposure, 0.0f );
	return exposure / ( exposure + EXPOSURE_HALF_POINT );
}

bool CNEOBspLighting::TorsoPoint( const Vector &navPos, Vector *torso ) const
{
	// the nav surface can lie a little below the real floor, such as on top of a prop
	trace_t tr;
	Vector floorPos = navPos;
	if ( enginetrace->GetPointContents( floorPos + Vector( 0, 0, FLOOR_CLEARANCE ) ) & CONTENTS_SOLID )
	{
		const Vector above = navPos + Vector( 0, 0, StepHeight );
		if ( enginetrace->GetPointContents( above ) & CONTENTS_SOLID )
		{
			return false;
		}

		Ray_t ray;
		ray.Init( above, navPos );
		CTraceFilterWorldAndPropsOnly filter;
		enginetrace->TraceRay( ray, MASK_SOLID, &filter, &tr );
		floorPos = tr.endpos;
	}

	const Vector base = floorPos + Vector( 0, 0, FLOOR_CLEARANCE );
	float height = TORSO_HEIGHT;
	if ( IsBlocked( base, floorPos + Vector( 0, 0, CEILING_PROBE ), &tr ) )
	{
		const float headroom = FLOOR_CLEARANCE + tr.fraction * ( CEILING_PROBE - FLOOR_CLEARANCE );
		height = Min( TORSO_HEIGHT, headroom * 0.5f );
	}

	*torso = floorPos + Vector( 0, 0, height );
	return true;
}

// Inverse squared distance over the leaf's samples, as Mod_LeafAmbientColorAtPos.
bool CNEOBspLighting::AmbientAt( const Vector &pos, float *ambient ) const
{
	int leaf = enginetrace->GetLeafContainingPoint( pos );
	for ( int hop = 0; hop < MAX_AMBIENT_LEAF_HOPS; ++hop )
	{
		if ( !m_leafAmbient.IsValidIndex( leaf ) || !m_leafMins.IsValidIndex( leaf ) )
		{
			return false;
		}

		const LeafAmbient &index = m_leafAmbient[leaf];
		if ( index.count == 0 )
		{
			leaf = index.first;
			continue;
		}

		if ( index.first + index.count > m_ambientSamples.Count() )
		{
			return false;
		}

		const Vector &mins = m_leafMins[leaf];
		const Vector size = m_leafMaxs[leaf] - mins;
		float totalWeight = 0.0f;
		float sum = 0.0f;
		for ( int i = index.first; i < index.first + index.count; ++i )
		{
			const AmbientSample &sample = m_ambientSamples[i];
			const Vector samplePos( mins.x + size.x * sample.pos[0] / 255.0f,
				mins.y + size.y * sample.pos[1] / 255.0f,
				mins.z + size.z * sample.pos[2] / 255.0f );
			const float weight = 1.0f / ( samplePos.DistToSqr( pos ) + 1.0f );
			totalWeight += weight;
			sum += weight * sample.luminance;
		}

		*ambient = sum / totalWeight;
		return true;
	}

	return false;
}

// Falloff and cone as VRAD lights a point; zero when out of the cone.
float CNEOBspLighting::LightFalloff( const WorldLight &light, const Vector &toLight, float dist ) const
{
	const float facing = -DotProduct( toLight, light.normal );
	const float attenuation = light.constantAttn + light.linearAttn * dist + light.quadraticAttn * dist * dist;
	switch ( light.type )
	{
	case emit_point:
		return 1.0f / attenuation;

	case emit_surface:
		return Max( facing, 0.0f ) / ( dist * dist );

	case emit_spotlight:
	{
		if ( facing <= light.stopDot2 )
		{
			return 0.0f;
		}

		float falloff = facing / attenuation;
		if ( facing <= light.stopDot )
		{
			float penumbra = clamp( ( facing - light.stopDot2 ) / ( light.stopDot - light.stopDot2 ), 0.0f, 1.0f );
			if ( light.exponent != 0.0f && light.exponent != 1.0f )
			{
				penumbra = powf( penumbra, light.exponent );
			}

			falloff *= penumbra;
		}

		return falloff;
	}

	default:
		return 0.0f;
	}
}

float CNEOBspLighting::DirectAt( const Vector &pos ) const
{
	float sum = 0.0f;
	trace_t tr;
	FOR_EACH_VEC( m_lights, i )
	{
		const WorldLight &light = m_lights[i];
		if ( light.type == emit_skylight )
		{
			// the sun counts only where its ray ends on a sky face
			const Vector toSun = -light.normal;
			if ( IsBlocked( pos, pos + toSun * SKY_RAY_LENGTH, &tr ) && ( tr.surface.flags & ( SURF_SKY | SURF_SKY2D ) ) )
			{
				sum += light.luminance * CubeMeanLambert( toSun );
			}

			continue;
		}

		Vector toLight = light.origin - pos;
		const float length = toLight.NormalizeInPlace();
		const float dist = Max( length, 1.0f );
		if ( light.radius > 0.0f && dist > light.radius )
		{
			continue;
		}

		const float contribution = light.luminance * LightFalloff( light, toLight, dist ) * CubeMeanLambert( toLight );
		if ( !( contribution > MIN_LIGHT_CONTRIBUTION ) )
		{
			continue;
		}

		// stop short of the light, and off a surface light's own face
		Vector end = light.origin - toLight;
		if ( light.type == emit_surface )
		{
			end += light.normal * FLOOR_CLEARANCE;
		}

		if ( !IsBlocked( pos, end, &tr ) )
		{
			sum += contribution;
		}
	}

	return sum;
}

CNEOBspLighting &NEOBspLighting()
{
	static CNEOBspLighting s_lighting;
	return s_lighting;
}
