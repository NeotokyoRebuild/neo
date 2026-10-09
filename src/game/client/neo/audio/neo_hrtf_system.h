// NEO HRTF: client-side re-rendering of the engine's positional sounds through an
// NeoSpatial::ISpatializer backend. The engine mixer is closed source, so instead of
// hooking it this system polls the engine's channel list once per frame, mutes the
// engine's copy of each positional sound and plays its own spatialised copy on a
// separate miniaudio output device.
#pragma once

#include "igamesystem.h"
#include "mathlib/vector.h"
#include "engine/SndInfo.h"
#include "soundflags.h"
#include "tier0/threadtools.h"
#include "utldict.h"
#include "utlstring.h"
#include "utlvector.h"

#include "neo_audio_probes.h"
#include "neo_spatializer.h"

class CNeoAudioGeometry;
struct ma_device;

class CNeoHrtfSystem : public CAutoGameSystemPerFrame
{
public:
	static constexpr int kSampleRate = 48000;
	static constexpr int kFrameSize = 512;
	static constexpr int kMaxVoices = 32;
	static constexpr int kMaxErrorLen = 256;

	CNeoHrtfSystem() : CAutoGameSystemPerFrame("CNeoHrtfSystem") {}

	void Shutdown() override;
	void LevelInitPreEntity() override;
	void LevelInitPostEntity() override;
	void LevelShutdownPreEntity() override;
	void LevelShutdownPostEntity() override;
	void Update(float frametime) override;

	// Audio thread entry point (miniaudio data callback).
	void Render(float *pOutInterleaved, int frameCount);

	// Debug: writes the acoustic scene as <game dir>/hrtf_scene_<map>.obj.
	void SaveSceneObj() const;

	// (Re)bakes the current map's probes in the background, replacing any cached bake.
	void StartBake();
	void CancelBake();

private:
	// Decoded once per file, immutable afterwards, so the audio thread may read it
	// without locking for as long as a voice references it.
	struct CachedSound
	{
		CUtlString m_name;
		CUtlVector<float> m_samples; // mono, kSampleRate; empty = unplayable, left to the engine
		int m_loopStart = -1; // sample index to loop back to, -1 for one-shots
		soundlevel_t m_scriptLevel = SNDLVL_NORM; // loudest scripted level of the file, the voice default
	};

	// Written by the game thread under m_mutex, read by the audio thread under m_mutex.
	struct VoiceParams
	{
		NeoSpatial::Vec3 m_origin; // metres
		float m_gain;
		float m_rate; // playback rate relative to kSampleRate, from the engine pitch
		float m_distanceGain; // the straight-line distance part of m_gain
		float m_falloffPerMetre; // the engine's distance law for this sound, for paths
	};

	struct Voice
	{
		// Game thread only.
		bool m_bInUse = false;
		int m_guid = 0;
		float m_sourceVolume = 0.0f; // engine channel volume before we muted it, restored on release
		float m_engineVolume = -1.0f; // what the engine copy was last set to, -1 = not yet
		float m_engineGain = 0.0f; // the engine's distance and pan gain for the channel, 0 = unknown
		bool m_bEngineVolumeJustSet = false; // set last poll, so the spatialized report may predate it
		bool m_bSeenThisPoll = false;
		float m_lastReportedVolume = 0.0f; // the engine's last reports for the channel, for the debug overlay
		float m_lastSpatializedVolume = 0.0f;
		// What the engine attenuates the channel by: per voice, since one file can be played at
		// different levels (e.g. ambient_generics with different radii).
		soundlevel_t m_soundLevel = SNDLVL_NORM;
		float m_distMult = 0.0f; // engine dist_mult for m_soundLevel, 0 = no falloff
		bool m_bLevelFromEmitter = false; // from a networked ambient_generic rather than the scripts

		// Set by the game thread under m_mutex while the voice is created or released.
		const CachedSound *m_pSound = nullptr;
		NeoSpatial::VoiceHandle m_hSpatial = NeoSpatial::INVALID_VOICE;
		VoiceParams m_params = {};
		// Published after each simulation; a new voice is not rendered until its first path
		// arrives, so a sound behind a wall never starts with one unoccluded block.
		NeoSpatial::DirectPath m_path;
		NeoSpatial::PathParams m_paths; // around obstacles, through baked probes
		bool m_bHasPath = false;

		// Audio thread only (reset under m_mutex when the voice is created).
		double m_cursor = 0.0;
		bool m_bFinished = false;
	};

	struct PendingVoice
	{
		int m_soundIndex; // into m_activeSounds
		const CachedSound *m_pSound;
		int m_voiceIndex; // slot it was given, -1 if the backend refused a voice
	};

	void StartDevice();
	void StopDevice();
	void ReleaseAllVoices();
	void PollEngineSounds();
	void SimulatePaths();
	void BuildScene();
	void ReleaseScene();
	void SetupProbes(const CNeoAudioGeometry &geometry);
	bool LoadProbeCache();
	void UpdateBake();
	void SaveProbeCache();
	void DrawProbes() const;

	const CachedSound *FindOrLoadSound(const SndInfo_t &info);
	void LoadSound(CachedSound &sound);
	soundlevel_t LookupScriptLevel(const char *pszNormalisedName);
	void ResolveSoundLevel(Voice &voice, const SndInfo_t &info, const CachedSound &sound) const;

	VoiceParams ComputeParams(const SndInfo_t &info, float sourceVolume, float distMult, float outputScale) const;
	int FindVoice(int guid) const;
	void MuteEngineCopy(Voice &voice, const SndInfo_t &info, float spatializedTarget);
	void PrintDebug() const;

	void RenderBlock();
	bool ReadVoiceSamples(Voice &voice);

	// Game thread state.
	ma_device *m_pDevice = nullptr;
	NeoSpatial::ISpatializer *m_pSpatializer = nullptr;
	char m_szStartError[kMaxErrorLen] = ""; // why the last start failed; non-empty blocks retries until re-enabled
	bool m_bSceneStale = false; // the spatializer has no scene for the current map yet
	int m_sceneTriangles = 0; // 0 = no scene
	NeoSpatial::ProbeSet m_probes; // the current map's probes, also what the backend's batches hold
	uint32 m_probeCacheKey = 0; // geometry + probe layout + format; a cached bake must match it
	char m_szProbeCachePath[MAX_PATH] = "";
	NeoSpatial::BakeState m_lastBakeState = NeoSpatial::BakeState::Idle;
	CUtlVector<SndInfo_t> m_activeSounds;
	CUtlVector<int> m_ignoredGuids; // sounds deliberately left to the engine, rechecked by guid only
	CUtlVector<int> m_ignoredGuidsNext;
	CUtlDict<CachedSound *, int> m_cache;
	CUtlDict<soundlevel_t, int> m_soundLevels; // normalised wave name → loudest scripted level
	VoiceParams m_stagedParams[kMaxVoices];
	PendingVoice m_pending[kMaxVoices];
	NeoSpatial::VoiceHandle m_simHandles[kMaxVoices];
	NeoSpatial::Vec3 m_simOrigins[kMaxVoices];
	NeoSpatial::DirectPath m_simPaths[kMaxVoices];
	NeoSpatial::PathingVoice m_simPathingVoices[kMaxVoices];
	NeoSpatial::PathParams m_simPathing[kMaxVoices];
	NeoSpatial::BspTree m_bspTree; // the current map's, to find the listener's area (its probe batch)
	NeoSpatial::ProbeCoverage m_probeCoverage; // where Steam Audio's paths are actually recomputed
	int m_simVoiceIndices[kMaxVoices];
	float m_simMilliseconds = 0.0f; // last SimulatePaths, for the debug overlay
	Vector m_listenerOrigin;
	Vector m_listenerForward;
	Vector m_listenerRight;
	Vector m_listenerUp;

	// Shared between threads, guarded by m_mutex.
	mutable CThreadMutex m_mutex;
	Voice m_voices[kMaxVoices];
	NeoSpatial::Listener m_listener = {};
	NeoSpatial::ReverbParams m_reverbParams; // the room around the listener, from baked probes
	float m_reverbGain = 0.0f; // wet level on top of each voice's own gain
	bool m_bReverbDecorrelate = true; // false: both ears get the same tail, inside the head

	// Audio thread only.
	float m_carry[kFrameSize * 2]; // interleaved stereo block not yet handed to miniaudio
	int m_carryRead = 0;
	int m_carryAvailable = 0;
	float m_scratchMono[kFrameSize];
	float m_scratchLeft[kFrameSize];
	float m_scratchRight[kFrameSize];
};
