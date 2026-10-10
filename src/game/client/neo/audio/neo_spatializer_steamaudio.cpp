// NEO HRTF: Steam Audio backend for NeoSpatial::ISpatializer.
//
// phonon is loaded at runtime rather than linked so that a missing or incompatible library
// only disables HRTF, and so the client does not depend on the dynamic loader finding the mod's
// bin directory. Only the functions listed in NEO_PHONON_FUNCTIONS are resolved.
#include "neo_spatializer.h"

#include <phonon.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace NeoSpatial
{

namespace
{

#define NEO_PHONON_FUNCTIONS(X) \
	X(iplContextCreate) \
	X(iplContextRelease) \
	X(iplHRTFCreate) \
	X(iplHRTFRelease) \
	X(iplBinauralEffectCreate) \
	X(iplBinauralEffectRelease) \
	X(iplBinauralEffectReset) \
	X(iplBinauralEffectApply) \
	X(iplSceneCreate) \
	X(iplSceneRelease) \
	X(iplSceneCommit) \
	X(iplSceneSaveOBJ) \
	X(iplStaticMeshCreate) \
	X(iplStaticMeshRelease) \
	X(iplStaticMeshAdd) \
	X(iplSimulatorCreate) \
	X(iplSimulatorRelease) \
	X(iplSimulatorSetScene) \
	X(iplSimulatorSetSharedInputs) \
	X(iplSimulatorCommit) \
	X(iplSimulatorRunDirect) \
	X(iplSourceCreate) \
	X(iplSourceRelease) \
	X(iplSourceAdd) \
	X(iplSourceRemove) \
	X(iplSourceSetInputs) \
	X(iplSourceGetOutputs) \
	X(iplDirectEffectCreate) \
	X(iplDirectEffectRelease) \
	X(iplDirectEffectReset) \
	X(iplDirectEffectApply) \
	X(iplProbeArrayCreate) \
	X(iplProbeArrayRelease) \
	X(iplProbeArrayGenerateProbes) \
	X(iplProbeArrayGetNumProbes) \
	X(iplProbeArrayGetProbe) \
	X(iplProbeBatchCreate) \
	X(iplProbeBatchRelease) \
	X(iplProbeBatchAddProbe) \
	X(iplProbeBatchCommit) \
	X(iplProbeBatchGetNumProbes) \
	X(iplProbeBatchGetDataSize) \
	X(iplProbeBatchSave) \
	X(iplProbeBatchLoad) \
	X(iplSerializedObjectCreate) \
	X(iplSerializedObjectRelease) \
	X(iplSerializedObjectGetSize) \
	X(iplSerializedObjectGetData) \
	X(iplReflectionsBakerBake) \
	X(iplReflectionsBakerCancelBake) \
	X(iplSimulatorAddProbeBatch) \
	X(iplSimulatorRemoveProbeBatch) \
	X(iplSimulatorRunReflections) \
	X(iplReflectionEffectCreate) \
	X(iplReflectionEffectRelease) \
	X(iplReflectionEffectReset) \
	X(iplReflectionEffectApply) \
	X(iplPathBakerBake) \
	X(iplPathBakerCancelBake) \
	X(iplSimulatorRunPathing) \
	X(iplPathEffectCreate) \
	X(iplPathEffectRelease) \
	X(iplPathEffectReset) \
	X(iplPathEffectApply)

struct PhononApi
{
#define NEO_PHONON_DECLARE(name) decltype(&::name) name = nullptr;
	NEO_PHONON_FUNCTIONS(NEO_PHONON_DECLARE)
#undef NEO_PHONON_DECLARE
};

#ifdef _WIN32
typedef HMODULE LibraryHandle;
// LOAD_WITH_ALTERED_SEARCH_PATH resolves phonon.dll's own dependencies from its directory.
LibraryHandle OpenLibrary(const char *path) { return LoadLibraryExA(path, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH); }
void *FindSymbol(LibraryHandle library, const char *name) { return reinterpret_cast<void *>(GetProcAddress(library, name)); }
void CloseLibrary(LibraryHandle library) { FreeLibrary(library); }
void FormatLoaderError(char *out, int len) { snprintf(out, len, "Win32 error %lu", GetLastError()); }
#else
typedef void *LibraryHandle;
// RTLD_LOCAL keeps phonon's bundled symbols from interposing on the engine's.
LibraryHandle OpenLibrary(const char *path) { return dlopen(path, RTLD_NOW | RTLD_LOCAL); }
void *FindSymbol(LibraryHandle library, const char *name) { return dlsym(library, name); }
void CloseLibrary(LibraryHandle library) { dlclose(library); }
void FormatLoaderError(char *out, int len) { const char *error = dlerror(); snprintf(out, len, "%s", error ? error : "unknown dlerror"); }
#endif

constexpr int MAX_VOICES = 64;
constexpr int LOADER_ERROR_LEN = 256;

// Below this the source is effectively inside the listener's head and has no direction.
constexpr float MIN_SOURCE_DISTANCE_SQR = 1e-6f;

// Steam Audio's frame: +x right, +y up, -z ahead.
constexpr IPLVector3 STEAMAUDIO_AHEAD = { 0.0f, 0.0f, -1.0f };

// Volumetric occlusion treats a source as a sphere and reports the fraction of points sampled in it
// that the listener can see, so occlusion fades in as a source rounds a corner instead of
// switching. The radius is roughly half a player's width.
constexpr float OCCLUSION_RADIUS_METRES = 0.3f;
constexpr int OCCLUSION_SAMPLES = 16;
// Occluders beyond the first few between source and listener do not change what is heard.
constexpr int TRANSMISSION_RAYS = 3;

// Probe batches are baked with listener-centric parametric reverb: three decay times per probe,
// a few bytes each, where convolution IRs would cost hundreds of KB per probe across a whole map.
// The duration has to cover the decay of the largest rooms for the fit to be meaningful.
constexpr int BAKE_RAYS = 8192;
constexpr int BAKE_DIFFUSE_SAMPLES = 32;
constexpr int BAKE_BOUNCES = 32;
constexpr float BAKE_DURATION_SECONDS = 1.0f;
constexpr int BAKE_AMBISONIC_ORDER = 0; // parametric reverb has no directionality
constexpr float BAKE_IRRADIANCE_MIN_DISTANCE = 1.0f;

// Baked reverb is a lookup at the listener (no rays are traced for baked sources), but the
// simulator still sizes its reflection state from these, so they mirror the bake.
constexpr int REVERB_LOOKUP_RAYS = 1024;
constexpr int REVERB_LOOKUP_BOUNCES = 1;

// Decay times are fitted to BAKE_DURATION_SECONDS of simulated energy, so much longer ones are
// extrapolations; on NT;RE maps 90% of probes sit under 1.4 s, with rare fits up to 10 s in open,
// barely-reflecting spots.
constexpr float MAX_REVERB_SECONDS = 3.0f;

// The parametric reverb is a single feedback delay network with one output (Steam Audio fills only
// channel 0), so both ears would hear the same tail, inside the head. Each ear instead gets the
// tail through its own chain of Schroeder all-pass filters: flat in magnitude, different in phase,
// so the two come out decorrelated and the reverb surrounds the listener. Delays are primes
// (samples at 48 kHz, scaled for other rates), 5-25 ms, distinct between the ears.
constexpr int DECORRELATOR_STAGES = 3;
constexpr int DECORRELATOR_DELAYS[2][DECORRELATOR_STAGES] = { { 241, 557, 1031 }, { 307, 677, 1201 } };
constexpr int DECORRELATOR_REFERENCE_RATE = 48000;
constexpr float DECORRELATOR_GAIN = 0.5f;

class CAllpassChain
{
public:
	void Init(const int (&delays)[DECORRELATOR_STAGES], int sampleRate)
	{
		for (int s = 0; s < DECORRELATOR_STAGES; ++s)
		{
			const int delay = std::max(1, delays[s] * sampleRate / DECORRELATOR_REFERENCE_RATE);
			m_stages[s].buffer.assign(delay, 0.0f);
			m_stages[s].pos = 0;
		}
	}

	void Reset()
	{
		for (Stage &stage : m_stages)
		{
			std::fill(stage.buffer.begin(), stage.buffer.end(), 0.0f);
		}
	}

	// y[n] = -g x[n] + v[n - D], with v[n] = x[n] + g v[n - D] the delay line's input.
	void Process(const float *in, float *out, int frames)
	{
		std::copy_n(in, frames, out);
		for (Stage &stage : m_stages)
		{
			const int size = static_cast<int>(stage.buffer.size());
			for (int f = 0; f < frames; ++f)
			{
				const float delayed = stage.buffer[stage.pos];
				const float v = out[f] + DECORRELATOR_GAIN * delayed;
				out[f] = delayed - DECORRELATOR_GAIN * v;
				stage.buffer[stage.pos] = v;
				stage.pos = (stage.pos + 1 == size) ? 0 : stage.pos + 1;
			}
		}
	}

private:
	struct Stage
	{
		std::vector<float> buffer;
		int pos = 0;
	};
	Stage m_stages[DECORRELATOR_STAGES];
};

constexpr uint32_t PROBE_BLOB_MAGIC = 0x3142504eu; // "NPB1"

IPLBakedDataIdentifier ReverbIdentifier()
{
	IPLBakedDataIdentifier identifier = {};
	identifier.type = IPL_BAKEDDATATYPE_REFLECTIONS;
	identifier.variation = IPL_BAKEDDATAVARIATION_REVERB;
	return identifier;
}

// Pathing bakes, for every pair of probes in a batch that can see each other, which probes sound
// travels through. Two probes are mutually visible when more than PATH_VIS_THRESHOLD of the
// PATH_VIS_SAMPLES^2 rays between points in spheres of PATH_VIS_RADIUS get through. Pairs further
// apart than PATH_VIS_RANGE are never linked directly (paths chain through nearer probes), and
// paths longer than PATH_RANGE are dropped: by then even a gunshot is close to inaudible.
constexpr int PATH_VIS_SAMPLES = 4;
constexpr float PATH_VIS_RADIUS_METRES = 0.5f;
constexpr float PATH_VIS_THRESHOLD = 0.1f;
constexpr float PATH_VIS_RANGE_METRES = 20.0f;
constexpr float PATH_RANGE_METRES = 100.0f;
constexpr int PATHING_ORDER = 1; // PATHING_AMBISONIC_COEFFS = (order + 1)^2
// A path's omnidirectional coefficient for a source at unit gain: the order-0 spherical harmonic,
// 1 / (2 sqrt(pi)).
constexpr float PATHING_OMNI_UNIT_GAIN = 0.28209479f;

static_assert((PATHING_ORDER + 1) * (PATHING_ORDER + 1) == PATHING_AMBISONIC_COEFFS, "PathParams::sh holds one order");

IPLBakedDataIdentifier PathingIdentifier()
{
	IPLBakedDataIdentifier identifier = {};
	identifier.type = IPL_BAKEDDATATYPE_PATHING;
	identifier.variation = IPL_BAKEDDATAVARIATION_DYNAMIC;
	return identifier;
}

// The caller's distance law, evaluated by the pathing simulation along each path.
struct DistanceLaw
{
	float falloffPerMetre = 0.0f;
	float minGain = 0.0f;
};

float IPLCALL EvaluateDistanceLaw(IPLfloat32 distance, void *userData)
{
	const DistanceLaw &law = *static_cast<const DistanceLaw *>(userData);
	const float scaled = distance * law.falloffPerMetre;
	const float gain = (scaled > 1.0f) ? 1.0f / scaled : 1.0f;
	return (gain < law.minGain) ? 0.0f : gain;
}

static_assert(sizeof(IPLTriangle) == 3 * sizeof(int32_t), "SceneGeometry::triangles is passed as IPLTriangle");
static_assert(NUM_ACOUSTIC_BANDS == IPL_NUM_BANDS, "AcousticMaterial bands must match Steam Audio's");

float Dot(const Vec3 &a, const Vec3 &b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// Source (+x forward, +y left, +z up) to Steam Audio world space (+y up). Both are right-handed
// and this is a rotation, so triangle winding keeps its meaning.
IPLVector3 ToSteamAudioPosition(const Vec3 &v)
{
	return { v.x, v.z, -v.y };
}

Vec3 FromSteamAudioPosition(const IPLVector3 &v)
{
	return { v.x, -v.z, v.y };
}

// Directions rotate the same way; Source's forward/right/up become ahead/right/up, which Steam
// Audio expects as right x up = -ahead, and the rotation preserves that.
IPLCoordinateSpace3 ToSteamAudioListener(const Listener &listener)
{
	IPLCoordinateSpace3 space = {};
	space.right = ToSteamAudioPosition(listener.right);
	space.up = ToSteamAudioPosition(listener.up);
	space.ahead = ToSteamAudioPosition(listener.forward);
	space.origin = ToSteamAudioPosition(listener.origin);
	return space;
}

IPLMaterial ToSteamAudioMaterial(const AcousticMaterial &material)
{
	IPLMaterial out = {};
	for (int band = 0; band < IPL_NUM_BANDS; ++band)
	{
		out.absorption[band] = material.absorption[band];
		out.transmission[band] = material.transmission[band];
	}
	out.scattering = material.scattering;
	return out;
}

IPLVector3 ToSteamAudioDirection(const Listener &listener, const Vec3 &origin)
{
	const Vec3 delta = { origin.x - listener.origin.x, origin.y - listener.origin.y, origin.z - listener.origin.z };
	const float distanceSqr = Dot(delta, delta);
	if (distanceSqr < MIN_SOURCE_DISTANCE_SQR)
	{
		return STEAMAUDIO_AHEAD;
	}

	const float invDistance = 1.0f / sqrtf(distanceSqr);
	const Vec3 dir = { delta.x * invDistance, delta.y * invDistance, delta.z * invDistance };
	return { Dot(dir, listener.right), Dot(dir, listener.up), -Dot(dir, listener.forward) };
}

class CSteamAudioSpatializer final : public ISpatializer
{
public:
	CSteamAudioSpatializer(LibraryHandle library, const PhononApi &api)
		: m_library(library)
		, m_api(api)
	{
	}

	~CSteamAudioSpatializer() override
	{
		ReleaseScene();
		for (Voice &voice : m_voices)
		{
			if (voice.effect)
			{
				m_api.iplBinauralEffectRelease(&voice.effect);
			}
			if (voice.directEffect)
			{
				m_api.iplDirectEffectRelease(&voice.directEffect);
			}
			if (voice.pathEffect)
			{
				m_api.iplPathEffectRelease(&voice.pathEffect);
			}
		}
		if (m_reflectionEffect)
		{
			m_api.iplReflectionEffectRelease(&m_reflectionEffect);
		}
		if (m_hrtf)
		{
			m_api.iplHRTFRelease(&m_hrtf);
		}
		if (m_context)
		{
			m_api.iplContextRelease(&m_context);
		}
		CloseLibrary(m_library);
	}

	bool Init(int sampleRate, int frameSize, char *errorOut, int errorLen)
	{
		IPLContextSettings contextSettings = {};
		contextSettings.version = STEAMAUDIO_VERSION;
		// The default cap is SSE2; AVX512 is avoided because it can throttle the CPU clock.
		contextSettings.simdLevel = IPL_SIMDLEVEL_AVX2;
		IPLerror status = m_api.iplContextCreate(&contextSettings, &m_context);
		if (status != IPL_STATUS_SUCCESS)
		{
			snprintf(errorOut, errorLen, "iplContextCreate failed (status %d, built against Steam Audio %d.%d.%d)",
					 static_cast<int>(status), STEAMAUDIO_VERSION_MAJOR, STEAMAUDIO_VERSION_MINOR, STEAMAUDIO_VERSION_PATCH);
			return false;
		}

		m_audioSettings.samplingRate = sampleRate;
		m_audioSettings.frameSize = frameSize;
		m_directOut.resize(frameSize);

		IPLHRTFSettings hrtfSettings = {};
		hrtfSettings.type = IPL_HRTFTYPE_DEFAULT;
		hrtfSettings.volume = 1.0f;
		hrtfSettings.normType = IPL_HRTFNORMTYPE_NONE;
		status = m_api.iplHRTFCreate(m_context, &m_audioSettings, &hrtfSettings, &m_hrtf);
		if (status != IPL_STATUS_SUCCESS)
		{
			snprintf(errorOut, errorLen, "iplHRTFCreate failed (status %d)", static_cast<int>(status));
			return false;
		}

		// One reverb for everything: the baked data describes the space around the listener, so
		// every voice feeds the same effect (as Steam Audio's own listener reverb does).
		IPLReflectionEffectSettings reflectionSettings = {};
		reflectionSettings.type = IPL_REFLECTIONEFFECTTYPE_PARAMETRIC;
		reflectionSettings.irSize = frameSize; // unused by parametric reverb
		reflectionSettings.numChannels = 1;
		status = m_api.iplReflectionEffectCreate(m_context, &m_audioSettings, &reflectionSettings, &m_reflectionEffect);
		if (status != IPL_STATUS_SUCCESS)
		{
			m_reflectionEffect = nullptr;
			snprintf(errorOut, errorLen, "iplReflectionEffectCreate failed (status %d)", static_cast<int>(status));
			return false;
		}
		m_reverbIn.assign(frameSize, 0.0f);
		m_reverbOut.assign(frameSize, 0.0f);
		m_pathIn.assign(frameSize, 0.0f);
		m_pathOutLeft.assign(frameSize, 0.0f);
		m_pathOutRight.assign(frameSize, 0.0f);
		for (int ear = 0; ear < 2; ++ear)
		{
			m_decorrelators[ear].Init(DECORRELATOR_DELAYS[ear], sampleRate);
		}
		return true;
	}

	void SetListener(const Listener &listener) override
	{
		m_listener = listener;
	}

	VoiceHandle CreateVoice() override
	{
		for (int i = 0; i < MAX_VOICES; ++i)
		{
			Voice &voice = m_voices[i];
			if (voice.inUse)
			{
				continue;
			}

			// Effects are kept after ReleaseVoice so a busy scene reuses them instead of
			// reallocating filter state for every new sound.
			if (!PrepareEffects(voice))
			{
				return INVALID_VOICE;
			}

			// A voice without a source just stays unoccluded; the sound itself still plays.
			voice.inUse = true;
			CreateSource(voice);
			return static_cast<VoiceHandle>(i + 1);
		}
		return INVALID_VOICE;
	}

	void ReleaseVoice(VoiceHandle handle) override
	{
		if (Voice *const voice = FindVoice(handle))
		{
			ReleaseSource(*voice);
			voice->inUse = false;
		}
	}

	void Process(VoiceHandle handle, const Vec3 &origin, const VoiceRender &render, const float *monoIn,
				 float *outLeft, float *outRight) override
	{
		const int frames = m_audioSettings.frameSize;
		Voice *const voice = FindVoice(handle);
		if (!voice)
		{
			std::fill_n(outLeft, frames, 0.0f);
			std::fill_n(outRight, frames, 0.0f);
			return;
		}

		// Always applied, even on an open path, so the filter state glides rather than jumps
		// when a source moves behind cover.
		const DirectPath &path = render.direct;
		IPLDirectEffectParams directParams = {};
		directParams.flags = static_cast<IPLDirectEffectFlags>(IPL_DIRECTEFFECTFLAGS_APPLYOCCLUSION | IPL_DIRECTEFFECTFLAGS_APPLYTRANSMISSION);
		directParams.transmissionType = IPL_TRANSMISSIONTYPE_FREQDEPENDENT;
		directParams.occlusion = path.occlusion;
		std::copy_n(path.transmission, IPL_NUM_BANDS, directParams.transmission);

		// The C API takes non-const channel pointers but never writes to an input buffer.
		float *monoChannels[] = { const_cast<float *>(monoIn) };
		float *directChannels[] = { m_directOut.data() };
		IPLAudioBuffer monoBuffer = { 1, frames, monoChannels };
		IPLAudioBuffer directBuffer = { 1, frames, directChannels };
		m_api.iplDirectEffectApply(voice->directEffect, &directParams, &monoBuffer, &directBuffer);

		// The send is taken after occlusion, so a source behind a wall excites the room only as
		// much as it is heard in it: through the wall here, and around it in ProcessPaths.
		if (render.reverbSend > 0.0f)
		{
			for (int f = 0; f < frames; ++f)
			{
				m_reverbIn[f] += render.reverbSend * m_directOut[f];
			}
		}

		IPLBinauralEffectParams binauralParams = {};
		binauralParams.direction = ToSteamAudioDirection(m_listener, origin);
		binauralParams.interpolation = IPL_HRTFINTERPOLATION_BILINEAR;
		binauralParams.spatialBlend = 1.0f;
		binauralParams.hrtf = m_hrtf;

		float *outChannels[] = { outLeft, outRight };
		IPLAudioBuffer outBuffer = { 2, frames, outChannels };
		m_api.iplBinauralEffectApply(voice->effect, &binauralParams, &directBuffer, &outBuffer);

		ProcessPaths(*voice, render, monoIn, outLeft, outRight);
	}

	void ProcessReverb(const ReverbParams &params, bool bDecorrelate, float *outLeft, float *outRight)
	{
		const int frames = m_audioSettings.frameSize;
		if (!params.valid)
		{
			// Nothing baked here (yet): drop the input, and start clean once there is a room.
			if (m_bReverbActive)
			{
				m_api.iplReflectionEffectReset(m_reflectionEffect);
				m_decorrelators[0].Reset();
				m_decorrelators[1].Reset();
				m_bReverbActive = false;
			}
			std::fill(m_reverbIn.begin(), m_reverbIn.end(), 0.0f);
			std::fill_n(outLeft, frames, 0.0f);
			std::fill_n(outRight, frames, 0.0f);
			return;
		}
		m_bReverbActive = true;

		IPLReflectionEffectParams reflectionParams = {};
		reflectionParams.type = IPL_REFLECTIONEFFECTTYPE_PARAMETRIC;
		std::copy_n(params.reverbTimes, IPL_NUM_BANDS, reflectionParams.reverbTimes);
		reflectionParams.numChannels = 1;
		reflectionParams.irSize = frames;

		float *inChannels[] = { m_reverbIn.data() };
		float *outChannels[] = { m_reverbOut.data() };
		IPLAudioBuffer inBuffer = { 1, frames, inChannels };
		IPLAudioBuffer outBuffer = { 1, frames, outChannels };
		m_api.iplReflectionEffectApply(m_reflectionEffect, &reflectionParams, &inBuffer, &outBuffer, nullptr);
		std::fill(m_reverbIn.begin(), m_reverbIn.end(), 0.0f);

		if (!bDecorrelate)
		{
			// The all-passes are flat in magnitude, so the tail is as loud either way. Their
			// history is dropped so turning them back on starts clean.
			if (m_bDecorrelatorsActive)
			{
				m_decorrelators[0].Reset();
				m_decorrelators[1].Reset();
				m_bDecorrelatorsActive = false;
			}
			std::copy_n(m_reverbOut.data(), frames, outLeft);
			std::copy_n(m_reverbOut.data(), frames, outRight);
			return;
		}
		m_bDecorrelatorsActive = true;
		m_decorrelators[0].Process(m_reverbOut.data(), outLeft, frames);
		m_decorrelators[1].Process(m_reverbOut.data(), outRight, frames);
	}

	bool SetSceneGeometry(const SceneGeometry &geometry, char *errorOut, int errorLen) override
	{
		ReleaseScene();
		if (geometry.numTriangles <= 0)
		{
			return true;
		}

		// The default scene type is Steam Audio's own ray tracer, which needs no extra runtime
		// library (Embree or Radeon Rays) shipped next to phonon.
		IPLSceneSettings sceneSettings = {};
		sceneSettings.type = IPL_SCENETYPE_DEFAULT;
		IPLerror status = m_api.iplSceneCreate(m_context, &sceneSettings, &m_scene);
		if (status != IPL_STATUS_SUCCESS)
		{
			m_scene = nullptr;
			snprintf(errorOut, errorLen, "iplSceneCreate failed (status %d)", static_cast<int>(status));
			return false;
		}

		// iplStaticMeshCreate copies everything, so these only live for this call.
		std::vector<IPLVector3> vertices(geometry.numVertices);
		std::transform(geometry.vertices, geometry.vertices + geometry.numVertices, vertices.begin(), ToSteamAudioPosition);
		std::vector<IPLMaterial> materials(geometry.numMaterials);
		std::transform(geometry.materials, geometry.materials + geometry.numMaterials, materials.begin(), ToSteamAudioMaterial);

		// The C API takes non-const arrays but only reads them.
		IPLStaticMeshSettings meshSettings = {};
		meshSettings.numVertices = geometry.numVertices;
		meshSettings.numTriangles = geometry.numTriangles;
		meshSettings.numMaterials = geometry.numMaterials;
		meshSettings.vertices = vertices.data();
		meshSettings.triangles = reinterpret_cast<IPLTriangle *>(const_cast<int32_t *>(geometry.triangles));
		meshSettings.materialIndices = const_cast<IPLint32 *>(geometry.materialIndices);
		meshSettings.materials = materials.data();
		status = m_api.iplStaticMeshCreate(m_scene, &meshSettings, &m_staticMesh);
		if (status != IPL_STATUS_SUCCESS)
		{
			m_staticMesh = nullptr;
			ReleaseScene();
			snprintf(errorOut, errorLen, "iplStaticMeshCreate failed (status %d)", static_cast<int>(status));
			return false;
		}

		m_api.iplStaticMeshAdd(m_staticMesh, m_scene);
		m_api.iplSceneCommit(m_scene);

		// The simulator lives and dies with the scene, so releasing the scene at level shutdown
		// really frees it instead of leaving it referenced by an idle simulator.
		IPLSimulationSettings simulationSettings = {};
		simulationSettings.flags = static_cast<IPLSimulationFlags>(IPL_SIMULATIONFLAGS_DIRECT | /*IPL_SIMULATIONFLAGS_REFLECTIONS |*/ IPL_SIMULATIONFLAGS_PATHING);
		simulationSettings.numVisSamples = PATH_VIS_SAMPLES;
		simulationSettings.sceneType = IPL_SCENETYPE_DEFAULT;
		simulationSettings.reflectionType = IPL_REFLECTIONEFFECTTYPE_PARAMETRIC;
		simulationSettings.maxNumOcclusionSamples = OCCLUSION_SAMPLES;
		simulationSettings.maxNumRays = REVERB_LOOKUP_RAYS;
		simulationSettings.numDiffuseSamples = BAKE_DIFFUSE_SAMPLES;
		simulationSettings.maxDuration = BAKE_DURATION_SECONDS;
		// Also caps the order pathing outputs; order 0 would leave paths with a level but no direction.
		simulationSettings.maxOrder = std::max(BAKE_AMBISONIC_ORDER, PATHING_ORDER);
		simulationSettings.maxNumSources = MAX_VOICES + 1; // the voices and the listener reverb
		simulationSettings.numThreads = 1;
		simulationSettings.samplingRate = m_audioSettings.samplingRate;
		simulationSettings.frameSize = m_audioSettings.frameSize;
		status = m_api.iplSimulatorCreate(m_context, &simulationSettings, &m_simulator);
		if (status != IPL_STATUS_SUCCESS)
		{
			m_simulator = nullptr;
			ReleaseScene();
			snprintf(errorOut, errorLen, "iplSimulatorCreate failed (status %d)", static_cast<int>(status));
			return false;
		}
		m_api.iplSimulatorSetScene(m_simulator, m_scene);

		// Listener-centric baked reverb is looked up at the listener whatever the source position,
		// so a single source stands for the whole reverb bus.
		IPLSourceSettings reverbSourceSettings = {};
		reverbSourceSettings.flags = IPL_SIMULATIONFLAGS_REFLECTIONS;
		if (m_api.iplSourceCreate(m_simulator, &reverbSourceSettings, &m_reverbSource) == IPL_STATUS_SUCCESS)
		{
			m_api.iplSourceAdd(m_reverbSource, m_simulator);
		}
		else
		{
			m_reverbSource = nullptr;
		}

		// Voices already playing join the new simulator.
		for (Voice &voice : m_voices)
		{
			if (voice.inUse)
			{
				CreateSource(voice);
			}
		}
		m_api.iplSimulatorCommit(m_simulator);
		m_bSimulatorDirty = false;
		return true;
	}

	void SimulateDirect(const Listener &listener, const VoiceHandle *voices, const Vec3 *origins,
						DirectPath *pathsOut, int count) override
	{
		std::fill_n(pathsOut, count, DirectPath());
		if (!m_simulator || count <= 0)
		{
			return;
		}

		// Sources added or removed since the last run only take part after a commit.
		if (m_bSimulatorDirty)
		{
			m_api.iplSimulatorCommit(m_simulator);
			m_bSimulatorDirty = false;
		}

		// Distance attenuation, air absorption and directivity stay with the caller's model, so only
		// what the geometry does is simulated.
		IPLSimulationInputs inputs = {};
		inputs.flags = IPL_SIMULATIONFLAGS_DIRECT;
		inputs.directFlags = static_cast<IPLDirectSimulationFlags>(IPL_DIRECTSIMULATIONFLAGS_OCCLUSION | IPL_DIRECTSIMULATIONFLAGS_TRANSMISSION);
		inputs.source.right = { 1.0f, 0.0f, 0.0f };
		inputs.source.up = { 0.0f, 1.0f, 0.0f };
		inputs.source.ahead = STEAMAUDIO_AHEAD;
		inputs.occlusionType = IPL_OCCLUSIONTYPE_VOLUMETRIC;
		inputs.occlusionRadius = OCCLUSION_RADIUS_METRES;
		inputs.numOcclusionSamples = OCCLUSION_SAMPLES;
		inputs.numTransmissionRays = TRANSMISSION_RAYS;

		bool bAnySource = false;
		for (int i = 0; i < count; ++i)
		{
			const Voice *const voice = FindVoice(voices[i]);
			if (voice && voice->source)
			{
				inputs.source.origin = ToSteamAudioPosition(origins[i]);
				m_api.iplSourceSetInputs(voice->source, IPL_SIMULATIONFLAGS_DIRECT, &inputs);
				bAnySource = true;
			}
		}
		if (!bAnySource)
		{
			return;
		}

		IPLSimulationSharedInputs sharedInputs = {};
		sharedInputs.listener = ToSteamAudioListener(listener);
		m_api.iplSimulatorSetSharedInputs(m_simulator, IPL_SIMULATIONFLAGS_DIRECT, &sharedInputs);
		m_api.iplSimulatorRunDirect(m_simulator);

		for (int i = 0; i < count; ++i)
		{
			const Voice *const voice = FindVoice(voices[i]);
			if (voice && voice->source)
			{
				IPLSimulationOutputs outputs = {};
				m_api.iplSourceGetOutputs(voice->source, IPL_SIMULATIONFLAGS_DIRECT, &outputs);
				pathsOut[i].occlusion = outputs.direct.occlusion;
				std::copy_n(outputs.direct.transmission, IPL_NUM_BANDS, pathsOut[i].transmission);
			}
		}
	}

	void SimulateReverb(const Listener &listener, ReverbParams *out) override
	{
		*out = ReverbParams();
		if (!m_simulator || !m_reverbSource)
		{
			return;
		}

		// Batches join the simulator only once fully baked, so it never reads one the bake thread
		// is still writing; whatever changes them again detaches them first.
		if (!m_bBatchesAttached && m_bakeState == BakeState::Done && !m_batches.empty())
		{
			AttachBatches();
		}
		if (!m_bBatchesAttached)
		{
			return;
		}
		if (m_bSimulatorDirty)
		{
			m_api.iplSimulatorCommit(m_simulator);
			m_bSimulatorDirty = false;
		}

		IPLSimulationInputs inputs = {};
		inputs.flags = IPL_SIMULATIONFLAGS_REFLECTIONS;
		inputs.source = ToSteamAudioListener(listener);
		inputs.baked = IPL_TRUE;
		inputs.bakedDataIdentifier = ReverbIdentifier();
		std::fill_n(inputs.reverbScale, IPL_NUM_BANDS, 1.0f);
		m_api.iplSourceSetInputs(m_reverbSource, IPL_SIMULATIONFLAGS_REFLECTIONS, &inputs);

		IPLSimulationSharedInputs sharedInputs = {};
		sharedInputs.listener = ToSteamAudioListener(listener);
		sharedInputs.numRays = REVERB_LOOKUP_RAYS;
		sharedInputs.numBounces = REVERB_LOOKUP_BOUNCES;
		sharedInputs.duration = BAKE_DURATION_SECONDS;
		sharedInputs.order = BAKE_AMBISONIC_ORDER;
		sharedInputs.irradianceMinDistance = BAKE_IRRADIANCE_MIN_DISTANCE;
		m_api.iplSimulatorSetSharedInputs(m_simulator, IPL_SIMULATIONFLAGS_REFLECTIONS, &sharedInputs);
		m_api.iplSimulatorRunReflections(m_simulator);

		IPLSimulationOutputs outputs = {};
		m_api.iplSourceGetOutputs(m_reverbSource, IPL_SIMULATIONFLAGS_REFLECTIONS, &outputs);

		// Away from every probe the lookup has nothing to blend and reports no decay; the reverb
		// effect would turn that into its 0.1 s minimum, a small fake room. Keep the last real one.
		bool bFound = false;
		bool bFinite = true;
		for (int band = 0; band < IPL_NUM_BANDS; ++band)
		{
			const float time = outputs.reflections.reverbTimes[band];
			bFinite = bFinite && std::isfinite(time) && time >= 0.0f;
			bFound = bFound || time > 0.0f;
		}
		if (bFound && bFinite)
		{
			m_lastReverb.valid = true;
			for (int band = 0; band < IPL_NUM_BANDS; ++band)
			{
				m_lastReverb.reverbTimes[band] = std::min(outputs.reflections.reverbTimes[band], MAX_REVERB_SECONDS);
			}
		}
		*out = m_lastReverb;
	}

	void SimulatePathing(const Listener &listener, int32_t batchId, float minGain, const PathingVoice *voices,
						 PathParams *out, int count) override
	{
		std::fill_n(out, count, PathParams());
		if (!m_simulator || count <= 0)
		{
			return;
		}
		if (!m_bBatchesAttached && m_bakeState == BakeState::Done && !m_batches.empty())
		{
			AttachBatches();
		}
		const Batch *pBatch = nullptr;
		for (const Batch &batch : m_batches)
		{
			pBatch = (batch.id == batchId) ? &batch : pBatch;
		}
		if (!m_bBatchesAttached || !pBatch)
		{
			return;
		}
		if (m_bSimulatorDirty)
		{
			m_api.iplSimulatorCommit(m_simulator);
			m_bSimulatorDirty = false;
		}

		// The world is static and the paths were baked against it, so they need no re-validation;
		// that would cost real-time visibility rays every frame.
		static IPLDeviationModel s_deviationModel = { IPL_DEVIATIONTYPE_DEFAULT, nullptr, nullptr };
		IPLSimulationInputs inputs = {};
		inputs.flags = IPL_SIMULATIONFLAGS_PATHING;
		inputs.source.right = { 1.0f, 0.0f, 0.0f };
		inputs.source.up = { 0.0f, 1.0f, 0.0f };
		inputs.source.ahead = STEAMAUDIO_AHEAD;
		inputs.distanceAttenuationModel.type = IPL_DISTANCEATTENUATIONTYPE_CALLBACK;
		inputs.distanceAttenuationModel.callback = EvaluateDistanceLaw;
		inputs.pathingProbes = pBatch->batch;
		inputs.visRadius = PATH_VIS_RADIUS_METRES;
		inputs.visThreshold = PATH_VIS_THRESHOLD;
		inputs.visRange = PATH_VIS_RANGE_METRES;
		inputs.pathingOrder = PATHING_ORDER;
		inputs.enableValidation = IPL_FALSE;
		inputs.findAlternatePaths = IPL_FALSE;
		inputs.deviationModel = &s_deviationModel;

		bool bAnySource = false;
		for (int i = 0; i < count; ++i)
		{
			Voice *const voice = FindVoice(voices[i].voice);
			if (voice && voice->source)
			{
				voice->distanceLaw = { voices[i].falloffPerMetre, minGain };
				inputs.source.origin = ToSteamAudioPosition(voices[i].origin);
				inputs.distanceAttenuationModel.userData = &voice->distanceLaw;
				m_api.iplSourceSetInputs(voice->source, IPL_SIMULATIONFLAGS_PATHING, &inputs);
				bAnySource = true;
			}
		}
		if (!bAnySource)
		{
			return;
		}

		IPLSimulationSharedInputs sharedInputs = {};
		sharedInputs.listener = ToSteamAudioListener(listener);
		m_api.iplSimulatorSetSharedInputs(m_simulator, IPL_SIMULATIONFLAGS_PATHING, &sharedInputs);
		m_api.iplSimulatorRunPathing(m_simulator);

		for (int i = 0; i < count; ++i)
		{
			const Voice *const voice = FindVoice(voices[i].voice);
			if (!voice || !voice->source)
			{
				continue;
			}
			IPLSimulationOutputs outputs = {};
			m_api.iplSourceGetOutputs(voice->source, IPL_SIMULATIONFLAGS_PATHING, &outputs);
			if (!outputs.pathing.shCoeffs)
			{
				continue;
			}
			bool bFinite = true;
			for (int c = 0; c < PATHING_AMBISONIC_COEFFS; ++c)
			{
				bFinite = bFinite && std::isfinite(outputs.pathing.shCoeffs[c]);
			}
			for (int band = 0; band < IPL_NUM_BANDS; ++band)
			{
				bFinite = bFinite && std::isfinite(outputs.pathing.eqCoeffs[band]);
			}
			if (bFinite)
			{
				out[i].valid = true;
				std::copy_n(outputs.pathing.eqCoeffs, IPL_NUM_BANDS, out[i].eq);
				std::copy_n(outputs.pathing.shCoeffs, PATHING_AMBISONIC_COEFFS, out[i].sh);
			}
		}
	}

	bool SaveSceneObj(const char *fileBaseName) override
	{
		if (!m_scene)
		{
			return false;
		}
		m_api.iplSceneSaveOBJ(m_scene, fileBaseName);
		return true;
	}

	int GenerateFloorProbes(const Vec3 &boxMins, const Vec3 &boxMaxs, float spacing, float height,
							Vec3 *centresOut, int maxOut) override
	{
		if (!m_scene)
		{
			return 0;
		}

		// The generator maps a unit cube centred on the origin through this transform (its source
		// works in [-0.5, 0.5], whatever the header says) and casts down local -y, so the box is
		// converted to Steam Audio's y-up world as scale + translation.
		const IPLVector3 lo = ToSteamAudioPosition(boxMins);
		const IPLVector3 hi = ToSteamAudioPosition(boxMaxs);
		IPLProbeGenerationParams params = {};
		params.type = IPL_PROBEGENERATIONTYPE_UNIFORMFLOOR;
		params.spacing = spacing;
		params.height = height;
		params.transform.elements[0][0] = fabsf(hi.x - lo.x);
		params.transform.elements[1][1] = fabsf(hi.y - lo.y);
		params.transform.elements[2][2] = fabsf(hi.z - lo.z);
		params.transform.elements[0][3] = (lo.x + hi.x) * 0.5f;
		params.transform.elements[1][3] = (lo.y + hi.y) * 0.5f;
		params.transform.elements[2][3] = (lo.z + hi.z) * 0.5f;
		params.transform.elements[3][3] = 1.0f;

		IPLProbeArray probeArray = nullptr;
		if (m_api.iplProbeArrayCreate(m_context, &probeArray) != IPL_STATUS_SUCCESS)
		{
			return 0;
		}
		m_api.iplProbeArrayGenerateProbes(probeArray, m_scene, &params);
		const int count = m_api.iplProbeArrayGetNumProbes(probeArray);
		for (int i = 0; i < count && i < maxOut; ++i)
		{
			centresOut[i] = FromSteamAudioPosition(m_api.iplProbeArrayGetProbe(probeArray, i).center);
		}
		m_api.iplProbeArrayRelease(&probeArray);
		return count;
	}

	bool SetProbeBatches(const ProbeLayout &layout, char *errorOut, int errorLen) override
	{
		StopBake();
		ReleaseProbeBatches();
		for (int b = 0; b < layout.numBatches; ++b)
		{
			Batch batch;
			batch.id = layout.batchIds[b];
			if (m_api.iplProbeBatchCreate(m_context, &batch.batch) != IPL_STATUS_SUCCESS)
			{
				ReleaseProbeBatches();
				snprintf(errorOut, errorLen, "iplProbeBatchCreate failed");
				return false;
			}
			for (int p = layout.batchStarts[b]; p < layout.batchStarts[b + 1]; ++p)
			{
				IPLSphere probe = {};
				probe.center = ToSteamAudioPosition(layout.centres[p]);
				probe.radius = layout.radius;
				m_api.iplProbeBatchAddProbe(batch.batch, probe);
			}
			m_api.iplProbeBatchCommit(batch.batch);
			m_batches.push_back(batch);
		}
		m_numProbes = layout.numProbes;
		return true;
	}

	bool StartBake(int numThreads) override
	{
		StopBake();
		if (m_batches.empty() || !m_scene)
		{
			return false;
		}
		// The bake writes into the batches, so the simulator must not be reading them.
		DetachBatches();
		m_bakeCancel = false;
		m_bakeProgress = 0.0f;
		m_bakeState = BakeState::Running;
		m_bakeThread = std::thread(&CSteamAudioSpatializer::BakeThreadMain, this, std::max(numThreads, 1));
		return true;
	}

	void CancelBake() override
	{
		StopBake();
	}

	BakeState GetBakeState(float *progressOut) override
	{
		if (progressOut)
		{
			*progressOut = m_bakeProgress;
		}
		return m_bakeState;
	}

	const uint8_t *SerializeProbeBatches(int64_t *sizeOut) override
	{
		*sizeOut = 0;
		if (m_bakeState == BakeState::Running || m_batches.empty())
		{
			return nullptr;
		}

		// "NPB1", batch count, then per batch: id, byte count, Steam Audio's own serialisation.
		m_serialized.clear();
		AppendPod(PROBE_BLOB_MAGIC);
		AppendPod(static_cast<int32_t>(m_batches.size()));
		for (const Batch &batch : m_batches)
		{
			IPLSerializedObjectSettings settings = {};
			IPLSerializedObject object = nullptr;
			if (m_api.iplSerializedObjectCreate(m_context, &settings, &object) != IPL_STATUS_SUCCESS)
			{
				m_serialized.clear();
				return nullptr;
			}
			m_api.iplProbeBatchSave(batch.batch, object);
			const int64_t size = static_cast<int64_t>(m_api.iplSerializedObjectGetSize(object));
			const uint8_t *data = m_api.iplSerializedObjectGetData(object);
			AppendPod(batch.id);
			AppendPod(size);
			m_serialized.insert(m_serialized.end(), data, data + size);
			m_api.iplSerializedObjectRelease(&object);
		}
		*sizeOut = static_cast<int64_t>(m_serialized.size());
		return m_serialized.data();
	}

	bool LoadProbeBatches(const uint8_t *data, int64_t size, char *errorOut, int errorLen) override
	{
		StopBake();
		ReleaseProbeBatches();

		int64_t offset = 0;
		uint32_t magic = 0;
		int32_t count = 0;
		if (!ReadPod(data, size, offset, magic) || magic != PROBE_BLOB_MAGIC || !ReadPod(data, size, offset, count) || count < 0)
		{
			snprintf(errorOut, errorLen, "not a probe batch blob");
			return false;
		}

		bool bAllBaked = true;
		IPLBakedDataIdentifier identifier = ReverbIdentifier();
		IPLBakedDataIdentifier pathingIdentifier = PathingIdentifier();
		for (int32_t b = 0; b < count; ++b)
		{
			Batch batch;
			int64_t batchSize = 0;
			if (!ReadPod(data, size, offset, batch.id) || !ReadPod(data, size, offset, batchSize)
				|| batchSize <= 0 || batchSize > size - offset)
			{
				ReleaseProbeBatches();
				snprintf(errorOut, errorLen, "probe batch %d is truncated", b);
				return false;
			}

			// The serialised object only reads from the buffer while loading.
			IPLSerializedObjectSettings settings = {};
			settings.data = const_cast<IPLbyte *>(data + offset);
			settings.size = static_cast<IPLsize>(batchSize);
			IPLSerializedObject object = nullptr;
			IPLerror status = m_api.iplSerializedObjectCreate(m_context, &settings, &object);
			if (status == IPL_STATUS_SUCCESS)
			{
				status = m_api.iplProbeBatchLoad(m_context, object, &batch.batch);
				m_api.iplSerializedObjectRelease(&object);
			}
			if (status != IPL_STATUS_SUCCESS)
			{
				ReleaseProbeBatches();
				snprintf(errorOut, errorLen, "probe batch %d does not load (status %d)", b, static_cast<int>(status));
				return false;
			}
			offset += batchSize;

			m_api.iplProbeBatchCommit(batch.batch);
			bAllBaked = bAllBaked && m_api.iplProbeBatchGetDataSize(batch.batch, &identifier) > 0
				&& m_api.iplProbeBatchGetDataSize(batch.batch, &pathingIdentifier) > 0;
			m_numProbes += m_api.iplProbeBatchGetNumProbes(batch.batch);
			m_batches.push_back(batch);
		}
		m_bakeState = (bAllBaked && !m_batches.empty()) ? BakeState::Done : BakeState::Idle;
		m_bakeProgress = (m_bakeState == BakeState::Done) ? 1.0f : 0.0f;
		return true;
	}

	int NumProbeBatches() const override
	{
		return static_cast<int>(m_batches.size());
	}

	int NumProbes() const override
	{
		return m_numProbes;
	}

	uint32_t GetLibraryVersion() const override
	{
		return STEAMAUDIO_VERSION;
	}

private:
	struct Batch
	{
		IPLProbeBatch batch = nullptr;
		int32_t id = 0;
	};

	// Bake thread. Batches are baked one after another, reverb then pathing for each (Steam Audio
	// allows one bake at a time), each spread over numThreads.
	void BakeThreadMain(int numThreads)
	{
		IPLReflectionsBakeParams reverbParams = {};
		reverbParams.scene = m_scene;
		reverbParams.sceneType = IPL_SCENETYPE_DEFAULT;
		reverbParams.identifier = ReverbIdentifier();
		reverbParams.bakeFlags = IPL_REFLECTIONSBAKEFLAGS_BAKEPARAMETRIC;
		reverbParams.numRays = BAKE_RAYS;
		reverbParams.numDiffuseSamples = BAKE_DIFFUSE_SAMPLES;
		reverbParams.numBounces = BAKE_BOUNCES;
		reverbParams.simulatedDuration = BAKE_DURATION_SECONDS;
		reverbParams.savedDuration = BAKE_DURATION_SECONDS;
		reverbParams.order = BAKE_AMBISONIC_ORDER;
		reverbParams.numThreads = numThreads;
		reverbParams.irradianceMinDistance = BAKE_IRRADIANCE_MIN_DISTANCE;
		reverbParams.bakeBatchSize = 1;

		IPLPathBakeParams pathParams = {};
		pathParams.scene = m_scene;
		pathParams.identifier = PathingIdentifier();
		pathParams.numSamples = PATH_VIS_SAMPLES;
		pathParams.radius = PATH_VIS_RADIUS_METRES;
		pathParams.threshold = PATH_VIS_THRESHOLD;
		pathParams.visRange = PATH_VIS_RANGE_METRES;
		pathParams.pathRange = PATH_RANGE_METRES;
		pathParams.numThreads = numThreads;

		const int count = static_cast<int>(m_batches.size());
		for (int b = 0; b < count && !m_bakeCancel; ++b)
		{
			m_bakeStep = 2 * b;
			reverbParams.probeBatch = m_batches[b].batch;
			m_api.iplReflectionsBakerBake(m_context, &reverbParams, &CSteamAudioSpatializer::BakeProgress, this);
			if (m_bakeCancel)
			{
				break;
			}
			m_bakeStep = 2 * b + 1;
			pathParams.probeBatch = m_batches[b].batch;
			m_api.iplPathBakerBake(m_context, &pathParams, &CSteamAudioSpatializer::BakeProgress, this);
		}
		m_bakeProgress = m_bakeCancel ? m_bakeProgress.load() : 1.0f;
		m_bakeState = m_bakeCancel ? BakeState::Failed : BakeState::Done;
	}

	static void IPLCALL BakeProgress(IPLfloat32 progress, void *userData)
	{
		CSteamAudioSpatializer *const self = static_cast<CSteamAudioSpatializer *>(userData);
		self->m_bakeProgress = (self->m_bakeStep + progress) / static_cast<float>(2 * self->m_batches.size());
	}

	void StopBake()
	{
		if (m_bakeThread.joinable())
		{
			// The flag stops the loop between batches; the cancel call cuts short the one running.
			m_bakeCancel = true;
			m_api.iplReflectionsBakerCancelBake(m_context);
			m_api.iplPathBakerCancelBake(m_context);
			m_bakeThread.join();
		}
	}

	void AttachBatches()
	{
		for (const Batch &batch : m_batches)
		{
			m_api.iplSimulatorAddProbeBatch(m_simulator, batch.batch);
		}
		m_api.iplSimulatorCommit(m_simulator);
		m_bSimulatorDirty = false;
		m_bBatchesAttached = true;
	}

	// Committed at once, so the simulator lets go of the batches before anyone changes them.
	void DetachBatches()
	{
		if (m_bBatchesAttached && m_simulator)
		{
			for (const Batch &batch : m_batches)
			{
				m_api.iplSimulatorRemoveProbeBatch(m_simulator, batch.batch);
			}
			m_api.iplSimulatorCommit(m_simulator);
			m_bSimulatorDirty = false;
		}
		m_bBatchesAttached = false;
		m_lastReverb = ReverbParams();
	}

	void ReleaseProbeBatches()
	{
		DetachBatches();
		for (Batch &batch : m_batches)
		{
			m_api.iplProbeBatchRelease(&batch.batch);
		}
		m_batches.clear();
		m_serialized.clear();
		m_numProbes = 0;
		m_bakeState = BakeState::Idle;
		m_bakeProgress = 0.0f;
	}

	template <typename T>
	void AppendPod(const T &value)
	{
		const uint8_t *bytes = reinterpret_cast<const uint8_t *>(&value);
		m_serialized.insert(m_serialized.end(), bytes, bytes + sizeof(T));
	}

	template <typename T>
	static bool ReadPod(const uint8_t *data, int64_t size, int64_t &offset, T &out)
	{
		if (size - offset < static_cast<int64_t>(sizeof(T)))
		{
			return false;
		}
		memcpy(&out, data + offset, sizeof(T));
		offset += sizeof(T);
		return true;
	}

	struct Voice
	{
		IPLBinauralEffect effect = nullptr;
		IPLDirectEffect directEffect = nullptr;
		IPLPathEffect pathEffect = nullptr;
		IPLSource source = nullptr; // only while there is a simulator
		bool inUse = false;
		DistanceLaw distanceLaw; // game thread: read by the pathing simulation through the source
		float pathMix = 0.0f; // audio thread: the occluded share at the end of the last block
		bool bPathsActive = false; // audio thread
	};

	// Audio thread: adds the occluded share of the block, rendered along the voice's paths, to the
	// outputs. The share is ramped across the block from the last one, so a source stepping behind
	// cover crossfades from direct to pathed instead of clicking.
	void ProcessPaths(Voice &voice, const VoiceRender &render, const float *monoIn, float *outLeft, float *outRight)
	{
		const int frames = m_audioSettings.frameSize;
		if (!render.paths.valid || render.pathGain <= 0.0f)
		{
			if (voice.bPathsActive)
			{
				m_api.iplPathEffectReset(voice.pathEffect);
				voice.bPathsActive = false;
			}
			voice.pathMix = 0.0f;
			return;
		}
		voice.bPathsActive = true;

		const float targetMix = std::clamp(1.0f - render.direct.occlusion, 0.0f, 1.0f);
		const float step = (targetMix - voice.pathMix) / frames;
		float mix = voice.pathMix;
		for (int f = 0; f < frames; ++f)
		{
			mix += step;
			m_pathIn[f] = monoIn[f] * mix * render.pathGain;
		}
		voice.pathMix = targetMix;

		float sh[PATHING_AMBISONIC_COEFFS];
		std::copy_n(render.paths.sh, PATHING_AMBISONIC_COEFFS, sh);
		IPLPathEffectParams pathParams = {};
		std::copy_n(render.paths.eq, IPL_NUM_BANDS, pathParams.eqCoeffs);
		pathParams.shCoeffs = sh;
		pathParams.order = PATHING_ORDER;
		pathParams.binaural = IPL_TRUE;
		pathParams.hrtf = m_hrtf;
		pathParams.listener = ToSteamAudioListener(m_listener);
		// Not normalised: the deviation EQ is what makes bending around an obstacle cost level. On open
		// maps the way around is barely longer than the straight line, so normalised paths came through
		// at nearly unoccluded loudness and undid occlusion map-wide (measured on ntre_oilstain_ctg).
		pathParams.normalizeEQ = IPL_FALSE;

		float *inChannels[] = { m_pathIn.data() };
		float *outChannels[] = { m_pathOutLeft.data(), m_pathOutRight.data() };
		IPLAudioBuffer inBuffer = { 1, frames, inChannels };
		IPLAudioBuffer outBuffer = { 2, frames, outChannels };
		m_api.iplPathEffectApply(voice.pathEffect, &pathParams, &inBuffer, &outBuffer);
		for (int f = 0; f < frames; ++f)
		{
			outLeft[f] += m_pathOutLeft[f];
			outRight[f] += m_pathOutRight[f];
		}

		// What arrives around the obstacle excites the listener's room too; with the send taken only
		// after occlusion, a source out of sight left the room silent. Its level is the paths' omni
		// coefficient (the distance law along them) times their mean spectral loss.
		float eqMean = 0.0f;
		for (const float band : render.paths.eq)
		{
			eqMean += band;
		}
		eqMean /= NUM_ACOUSTIC_BANDS;
		const float pathSend = render.reverbSend * std::max(render.paths.sh[0], 0.0f) / PATHING_OMNI_UNIT_GAIN * eqMean;
		if (pathSend > 0.0f)
		{
			for (int f = 0; f < frames; ++f)
			{
				m_reverbIn[f] += pathSend * m_pathIn[f];
			}
		}
	}

	bool PrepareEffects(Voice &voice)
	{
		if (voice.effect)
		{
			m_api.iplBinauralEffectReset(voice.effect);
		}
		else
		{
			IPLBinauralEffectSettings effectSettings = {};
			effectSettings.hrtf = m_hrtf;
			if (m_api.iplBinauralEffectCreate(m_context, &m_audioSettings, &effectSettings, &voice.effect) != IPL_STATUS_SUCCESS)
			{
				voice.effect = nullptr;
				return false;
			}
		}

		if (voice.directEffect)
		{
			m_api.iplDirectEffectReset(voice.directEffect);
		}
		else
		{
			IPLDirectEffectSettings effectSettings = {};
			effectSettings.numChannels = 1;
			if (m_api.iplDirectEffectCreate(m_context, &m_audioSettings, &effectSettings, &voice.directEffect) != IPL_STATUS_SUCCESS)
			{
				voice.directEffect = nullptr;
				return false;
			}
		}

		if (voice.pathEffect)
		{
			m_api.iplPathEffectReset(voice.pathEffect);
		}
		else
		{
			// Rotation to the listener and binaural rendering happen inside the effect.
			IPLPathEffectSettings effectSettings = {};
			effectSettings.maxOrder = PATHING_ORDER;
			effectSettings.spatialize = IPL_TRUE;
			effectSettings.speakerLayout.type = IPL_SPEAKERLAYOUTTYPE_STEREO;
			effectSettings.hrtf = m_hrtf;
			if (m_api.iplPathEffectCreate(m_context, &m_audioSettings, &effectSettings, &voice.pathEffect) != IPL_STATUS_SUCCESS)
			{
				voice.pathEffect = nullptr;
				return false;
			}
		}
		voice.pathMix = 0.0f;
		voice.bPathsActive = false;
		return true;
	}

	void CreateSource(Voice &voice)
	{
		if (!m_simulator || voice.source)
		{
			return;
		}
		IPLSourceSettings sourceSettings = {};
		sourceSettings.flags = static_cast<IPLSimulationFlags>(IPL_SIMULATIONFLAGS_DIRECT | IPL_SIMULATIONFLAGS_REFLECTIONS | IPL_SIMULATIONFLAGS_PATHING);
		if (m_api.iplSourceCreate(m_simulator, &sourceSettings, &voice.source) != IPL_STATUS_SUCCESS)
		{
			voice.source = nullptr;
			return;
		}
		m_api.iplSourceAdd(voice.source, m_simulator);
		m_bSimulatorDirty = true;
	}

	void ReleaseSource(Voice &voice)
	{
		if (voice.source)
		{
			m_api.iplSourceRemove(voice.source, m_simulator);
			m_api.iplSourceRelease(&voice.source);
			m_bSimulatorDirty = true;
		}
	}

	void ReleaseScene()
	{
		// Probes and their baked data describe this scene, so they go with it.
		StopBake();
		ReleaseProbeBatches();
		for (Voice &voice : m_voices)
		{
			ReleaseSource(voice);
		}
		if (m_reverbSource)
		{
			m_api.iplSourceRemove(m_reverbSource, m_simulator);
			m_api.iplSourceRelease(&m_reverbSource);
		}
		if (m_simulator)
		{
			m_api.iplSimulatorRelease(&m_simulator);
		}
		m_bSimulatorDirty = false;
		if (m_staticMesh)
		{
			m_api.iplStaticMeshRelease(&m_staticMesh);
		}
		if (m_scene)
		{
			m_api.iplSceneRelease(&m_scene);
		}
	}

	Voice *FindVoice(VoiceHandle handle)
	{
		if (handle == INVALID_VOICE || handle > static_cast<VoiceHandle>(MAX_VOICES))
		{
			return nullptr;
		}
		Voice &voice = m_voices[handle - 1];
		return voice.inUse ? &voice : nullptr;
	}

	const LibraryHandle m_library;
	const PhononApi m_api;

	IPLContext m_context = nullptr;
	IPLHRTF m_hrtf = nullptr;
	IPLScene m_scene = nullptr;
	IPLStaticMesh m_staticMesh = nullptr;
	IPLSimulator m_simulator = nullptr;
	IPLSource m_reverbSource = nullptr; // baked listener reverb, only while there is a simulator
	bool m_bBatchesAttached = false; // probe batches are in the simulator (only while fully baked)
	ReverbParams m_lastReverb; // held while the listener is away from probes

	// Audio thread.
	IPLReflectionEffect m_reflectionEffect = nullptr;
	std::vector<float> m_reverbIn; // this block's sends, summed
	std::vector<float> m_reverbOut;
	std::vector<float> m_pathIn; // one voice's occluded share
	std::vector<float> m_pathOutLeft;
	std::vector<float> m_pathOutRight;
	CAllpassChain m_decorrelators[2];
	bool m_bReverbActive = false;
	bool m_bDecorrelatorsActive = false; // whether the all-passes hold history from the last block
	bool m_bSimulatorDirty = false; // sources added or removed since the last commit
	IPLAudioSettings m_audioSettings = {};
	Listener m_listener = {};
	Voice m_voices[MAX_VOICES];
	std::vector<float> m_directOut; // audio thread: one voice's block after the direct effect

	// Game thread, except that the bake thread reads m_batches and m_scene while it runs; every
	// game-thread change to either stops the bake first.
	std::vector<Batch> m_batches;
	int m_numProbes = 0;
	std::vector<uint8_t> m_serialized;
	std::thread m_bakeThread;
	std::atomic<BakeState> m_bakeState{ BakeState::Idle };
	std::atomic<float> m_bakeProgress{ 0.0f };
	std::atomic<bool> m_bakeCancel{ false };
	std::atomic<int> m_bakeStep{ 0 }; // two per batch: reverb, then pathing
};

} // namespace

ISpatializer *CreateSteamAudioSpatializer(const char *phononLibraryPath, int sampleRate, int frameSize,
										  char *errorOut, int errorLen)
{
	const LibraryHandle library = OpenLibrary(phononLibraryPath);
	if (!library)
	{
		char loaderError[LOADER_ERROR_LEN];
		FormatLoaderError(loaderError, LOADER_ERROR_LEN);
		snprintf(errorOut, errorLen, "failed to load %s: %s", phononLibraryPath, loaderError);
		return nullptr;
	}

	PhononApi api;
#define NEO_PHONON_RESOLVE(name) \
	api.name = reinterpret_cast<decltype(api.name)>(FindSymbol(library, #name)); \
	if (!api.name) \
	{ \
		snprintf(errorOut, errorLen, "%s does not export " #name, phononLibraryPath); \
		CloseLibrary(library); \
		return nullptr; \
	}
	NEO_PHONON_FUNCTIONS(NEO_PHONON_RESOLVE)
#undef NEO_PHONON_RESOLVE

	// From here the spatializer owns the library and releases whatever Init created.
	CSteamAudioSpatializer *const spatializer = new CSteamAudioSpatializer(library, api);
	if (!spatializer->Init(sampleRate, frameSize, errorOut, errorLen))
	{
		delete spatializer;
		return nullptr;
	}
	return spatializer;
}

} // namespace NeoSpatial
